/*-
 * Copyright (c) 2000-2004 Mark R V Murray
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/filio.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/selinfo.h>
#include <sys/uio.h>
#include <sys/unistd.h>
#include <sys/vnode.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <dev/random/randomdev.h>

#define RANDOM_MINOR	0

static d_close_t random_close;
static d_read_t random_read;
static d_write_t random_write;
static d_ioctl_t random_ioctl;
static d_poll_t random_poll;

static struct cdevsw random_cdevsw = {
	.d_version = D_VERSION,
	.d_close = random_close,
	.d_read = random_read,
	.d_write = random_write,
	.d_ioctl = random_ioctl,
	.d_poll = random_poll,
	.d_name = "random",
};

static void *random_buf;

struct random_systat random_systat;

/* For use with make_dev(9)/destroy_dev(9). */
static dev_t random_dev;

/* Used to fake out unused random calls in random_systat */
void
random_null_func(void)
{
}

/* ARGSUSED */
static int
random_close(dev_t dev __unused, int flags, int fmt __unused,
    struct thread *td)
{
	if ((flags & FWRITE) && (suser(td) == 0)
	    && (securelevel_gt(td->td_ucred, 0) == 0)) {
		mtx_lock(&random_systat.lock);
		(*random_systat.reseed)();
		random_systat.seeded = 1;
		mtx_unlock(&random_systat.lock);
	}
	return (0);
}

/* ARGSUSED */
static int
random_read(dev_t dev __unused, struct uio *uio, int flag)
{
	int c, error = 0;

	mtx_lock(&random_systat.lock);

	/* Blocking logic */
	while (!random_systat.seeded && !error) {
		if (flag & IO_NDELAY)
			error = EWOULDBLOCK;
		else
			error = msleep(&random_systat, &random_systat.lock,
			    PUSER | PCATCH, "block", 0);
	}

	/* The actual read */
	if (!error) {
		while (uio->uio_resid > 0 && !error) {
			c = MIN(uio->uio_resid, PAGE_SIZE);
			c = (*random_systat.read)(random_buf, c);
			error = uiomove(random_buf, c, uio);
		}
	}

	mtx_unlock(&random_systat.lock);

	return (error);
}

/* ARGSUSED */
static int
random_write(dev_t dev __unused, struct uio *uio, int flag __unused)
{
	int c, error = 0;

	mtx_lock(&random_systat.lock);
	while (uio->uio_resid > 0) {
		c = MIN((int)uio->uio_resid, PAGE_SIZE);
		error = uiomove(random_buf, c, uio);
		if (error)
			break;
		(*random_systat.write)(random_buf, c);
	}
	mtx_unlock(&random_systat.lock);
	return (error);
}

/* ARGSUSED */
static int
random_ioctl(dev_t dev __unused, u_long cmd, caddr_t addr __unused,
    int flags __unused, struct thread *td __unused)
{
	int error = 0;

	switch (cmd) {
		/* Really handled in upper layer */
	case FIOASYNC:
	case FIONBIO:
		break;
	default:
		error = ENOTTY;
	}
	return (error);
}

/* ARGSUSED */
static int
random_poll(dev_t dev __unused, int events, struct thread *td)
{
	int revents = 0;

	if (events & (POLLIN | POLLRDNORM)) {
		if (random_systat.seeded)
			revents = events & (POLLIN | POLLRDNORM);
		else
			selrecord(td, &random_systat.rsel);
	}
	return (revents);
}

/* ARGSUSED */
static int
random_modevent(module_t mod __unused, int type, void *data __unused)
{
	int error = 0;

	switch (type) {
	case MOD_LOAD:
		random_buf = (void *)malloc(PAGE_SIZE, M_TEMP, M_WAITOK);
		random_ident_hardware(&random_systat);
		mtx_init(&random_systat.lock, "entropy device lock",
		    NULL, MTX_DEF);
		(*random_systat.init)();

		printf("random: <entropy source, %s>\n", random_systat.ident);

		random_dev = make_dev(&random_cdevsw, RANDOM_MINOR,
		    UID_ROOT, GID_WHEEL, 0666, "random");
		make_dev_alias(random_dev, "urandom");	/* XXX Deprecated */

		break;

	case MOD_UNLOAD:
		(*random_systat.deinit)();
		free(random_buf, M_TEMP);
		mtx_destroy(&random_systat.lock);

		destroy_dev(random_dev);

		break;

	case MOD_SHUTDOWN:
		break;

	}
	return (error);
}

DEV_MODULE(random, random_modevent, NULL);
