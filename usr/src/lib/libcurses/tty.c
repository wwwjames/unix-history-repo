/*-
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "@(#)tty.c	5.16 (Berkeley) %G%";
#endif /* not lint */

#include <sys/ioctl.h>

#include <curses.h>
#include <termios.h>
#include <unistd.h>

/*
 * In general, curses should leave tty hardware settings alone (speed, parity,
 * word size).  This is most easily done in BSD by using TCSASOFT on all
 * tcsetattr calls.  On other systems, it would be better to get and restore
 * those attributes at each change, or at least when stopped and restarted.
 * See also the comments in getterm().
 */
#ifdef TCSASOFT
#define	TCACTION (TCSASOFT | TCSADRAIN)		/* ignore hardware settings */
#else
#define	TCACTION  TCSADRAIN
#endif

struct termios __orig_termios;
static struct termios baset, cbreakt, rawt, *curt;
static int useraw;

#ifndef	OXTABS
#ifdef	XTABS			/* SMI uses XTABS. */
#define	OXTABS	XTABS
#else
#define	OXTABS	0
#endif
#endif

/*
 * gettmode --
 *	Do terminal type initialization.
 */
int
gettmode()
{
	useraw = 0;
	
	if (tcgetattr(STDIN_FILENO, &__orig_termios))
		return (ERR);

	GT = (__orig_termios.c_oflag & OXTABS) == 0;
	NONL = (__orig_termios.c_oflag & ONLCR) == 0;

	baset = __orig_termios;
	baset.c_oflag &= ~OXTABS;

	/*
	 * XXX
	 * System V and SMI systems overload VMIN and VTIME, such that
	 * VMIN is the same as the VEOF element, and VTIME is the same
	 * as the VEOL element.  This means that, if VEOF was ^D, the
	 * default VMIN is 4.  Majorly stupid.
	 */
	cbreakt = baset;
	cbreakt.c_lflag &= ~ICANON;
	cbreakt.c_cc[VMIN] = 1;
	cbreakt.c_cc[VTIME] = 0;

	rawt = cbreakt;
	rawt.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|INLCR|IGNCR|ICRNL|IXON);
	rawt.c_oflag &= ~OPOST;
	rawt.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
#if 0
	/*
	 * In general, curses should leave hardware-related settings alone.
	 * This includes parity and word size.  Older versions set the tty
	 * to 8 bits, no parity in raw(), but this is considered to be an
	 * artifact of the old tty interface.  If it's desired to change
	 * parity and word size, the TCSASOFT bit would have to be removed
	 * from the calls that switch to/from "raw" mode.
	 */
	rawt.c_iflag &= ~ISTRIP;
	rawt.c_cflag &= ~(CSIZE|PARENB);
	rawt.c_cflag |= CS8;
#endif

	curt = &baset;
	return (tcsetattr(STDIN_FILENO, TCACTION, &baset) ? ERR : OK);
}

int
raw()
{
	useraw = __pfast = __rawmode = 1;
	curt = &rawt;
	return (tcsetattr(STDIN_FILENO, TCACTION, &rawt));
}

int
noraw()
{
	useraw = __pfast = __rawmode = 0;
	curt = &baset;
	return (tcsetattr(STDIN_FILENO, TCACTION, &baset));
}

int
cbreak()
{

	__rawmode = 1;
	curt = useraw ? &rawt : &cbreakt;
	return (tcsetattr(STDIN_FILENO, TCACTION, curt));
}

int
nocbreak()
{

	__rawmode = 0;
	curt = useraw ? &rawt : &baset;
	return (tcsetattr(STDIN_FILENO, TCACTION, curt));
}
	
int
echo()
{
	rawt.c_lflag |= ECHO;
	cbreakt.c_lflag |= ECHO;
	baset.c_lflag |= ECHO;
	
	__echoit = 1;
	return (tcsetattr(STDIN_FILENO, TCACTION, curt));
}

int
noecho()
{
	rawt.c_lflag &= ~ECHO;
	cbreakt.c_lflag &= ~ECHO;
	baset.c_lflag &= ~ECHO;
	
	__echoit = 0;
	return (tcsetattr(STDIN_FILENO, TCACTION, curt));
}

int
nl()
{
	rawt.c_iflag |= ICRNL;
	rawt.c_oflag |= ONLCR;
	cbreakt.c_iflag |= ICRNL;
	cbreakt.c_oflag |= ONLCR;
	baset.c_iflag |= ICRNL;
	baset.c_oflag |= ONLCR;

	__pfast = __rawmode;
	return (tcsetattr(STDIN_FILENO, TCACTION, curt));
}

int
nonl()
{
	rawt.c_iflag &= ~ICRNL;
	rawt.c_oflag &= ~ONLCR;
	cbreakt.c_iflag &= ~ICRNL;
	cbreakt.c_oflag &= ~ONLCR;
	baset.c_iflag &= ~ICRNL;
	baset.c_oflag &= ~ONLCR;

	__pfast = 1;
	return (tcsetattr(STDIN_FILENO, TCACTION, curt));
}

void
__startwin()
{
	(void)fflush(stdout);
	(void)setvbuf(stdout, NULL, _IOFBF, 0);

	tputs(TI, 0, __cputchar);
	tputs(VS, 0, __cputchar);
}

int
endwin()
{
	if (curscr != NULL) {
		if (curscr->flags & __WSTANDOUT) {
			tputs(SE, 0, __cputchar);
			curscr->flags &= ~__WSTANDOUT;
		}
		__mvcur(curscr->cury, curscr->cury, curscr->maxy - 1, 0, 0);
	}

	(void)tputs(VE, 0, __cputchar);
	(void)tputs(TE, 0, __cputchar);
	(void)fflush(stdout);
	(void)setvbuf(stdout, NULL, _IOLBF, 0);

	return (tcsetattr(STDIN_FILENO, TCACTION, &__orig_termios));
}

/*
 * The following routines, savetty and resetty are completely useless and
 * are left in only as stubs.  If people actually use them they will almost
 * certainly screw up the state of the world.
 */
static struct termios savedtty;
int
savetty()
{
	return (tcgetattr(STDIN_FILENO, &savedtty));
}

int
resetty()
{
	return (tcsetattr(STDIN_FILENO, TCACTION, &savedtty));
}
