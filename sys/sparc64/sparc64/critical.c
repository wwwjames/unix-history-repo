/*-
 * Copyright (c) 2001 Matthew Dillon.  This code is distributed under
 * the BSD copyright, /usr/src/COPYRIGHT.
 *
 * $FreeBSD$
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/pcpu.h>
#include <sys/eventhandler.h>	/* XX */
#include <sys/ktr.h>		/* XX */
#include <sys/signalvar.h>
#include <sys/sysproto.h>	/* XX */
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/sysctl.h>
#include <sys/ucontext.h>

void
cpu_critical_enter(void)
{
	struct thread *td;
	critical_t pil;

	td = curthread;
	pil = rdpr(pil);
	wrpr(pil, 0, 14);
	td->td_md.md_savecrit = pil;
} 
 
void
cpu_critical_exit(void)
{
	struct thread *td;

	td = curthread;
	wrpr(pil, td->td_md.md_savecrit, 0); 
}

/*
 * cpu_critical_fork_exit() - cleanup after fork
 */
void
cpu_critical_fork_exit(void)
{
	struct thread *td;

	td = curthread;
	td->td_critnest = 1;
	td->td_md.md_savecrit = 0;
}

/*
 * cpu_thread_link() - thread linkup, initialize machine-dependant fields
 */
void
cpu_thread_link(struct thread *td)
{

	td->td_md.md_savecrit = 0;
}
