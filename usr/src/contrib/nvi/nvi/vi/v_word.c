/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char sccsid[] = "@(#)v_word.c	8.1 (Berkeley) 6/9/93";
#endif /* not lint */

#include <sys/types.h>

#include <ctype.h>

#include "vi.h"
#include "vcmd.h"

/*
 * There are two types of "words".  Bigwords are easy -- groups of anything
 * delimited by whitespace.  Normal words are trickier.  They are either a
 * group of characters, numbers and underscores, or a group of anything but,
 * delimited by whitespace.  When for a word, if you're in whitespace, it's
 * easy, just remove the whitespace and go to the beginning or end of the
 * word.  Otherwise, figure out if the next character is in a different group.
 * If it is, go to the beginning or end of that group, otherwise, go to the
 * beginning or end of the current group.  The historic version of vi didn't
 * get this right.  To get it right you have to resolve the cursor after each
 * search so that the look-ahead to figure out what type of "word" the cursor
 * is in will be correct.
 *
 * Empty lines count as a single word, and the beginning and end of the file
 * counts as an infinite number of words.
 */

#define	FW(test)	for (; len && (test); --len, ++p)
#define	BW(test)	for (; len && (test); --len, --p)

static int bword __P((SCR *, EXF *, VICMDARG *, MARK *, MARK *, int));
static int eword __P((SCR *, EXF *, VICMDARG *, MARK *, MARK *, int));
static int fword __P((SCR *, EXF *, VICMDARG *, MARK *, MARK *, int));

/*
 * v_wordw -- [count]w
 *	Move forward a word at a time.
 */
int
v_wordw(sp, ep, vp, fm, tm, rp)
	SCR *sp;
	EXF *ep;
	VICMDARG *vp;
	MARK *fm, *tm, *rp;
{
	return (fword(sp, ep, vp, fm, rp, 0));
}

/*
 * v_wordW -- [count]W
 *	Move forward a bigword at a time.
 */
int
v_wordW(sp, ep, vp, fm, tm, rp)
	SCR *sp;
	EXF *ep;
	VICMDARG *vp;
	MARK *fm, *tm, *rp;
{
	return (fword(sp, ep, vp, fm, rp, 1));
}

/*
 * fword --
 *	Move forward by words.
 */
static int
fword(sp, ep, vp, fm, rp, spaceonly)
	SCR *sp;
	EXF *ep;
	VICMDARG *vp;
	MARK *fm, *rp;
	int spaceonly;
{
	register char *p;
	recno_t lno;
	size_t len, llen;
	u_long cno, cnt;
	int empty;
	char *startp;

	lno = fm->lno;
	cno = fm->cno;

	if ((p = file_gline(sp, ep, lno, &llen)) == NULL) {
		if (file_lline(sp, ep, &lno))
			return (1);
		if (lno == 0)
			v_eof(sp, ep, NULL);
		else
			GETLINE_ERR(sp, lno);
		return (1);
	}

	cnt = F_ISSET(vp, VC_C1SET) ? vp->count : 1;

	/*
	 * Reset the length; the first character is the current cursor
	 * position.  If no more characters in this line, may already
	 * be at EOF.
	 *
	 * Note, there are several special cases.  Movements associated
	 * with commands are slightly different than movement commands.
	 * For example, in "abc def ghi", "cw" is from 'a' to 'c', while
	 * "w" is from 'a' to 'd'.  "Bill, it's another ugly tale from
	 * BSD's ugliest city."
	 */
	len = llen - cno;
	empty = llen == 0 || llen == cno + 1;
	for (startp = p += cno; cnt--; empty = 0) {
		if (len != 0)
			if (spaceonly) {
				FW(!isspace(*p));
				/* 'c' and 'y' don't skip more space. */
				if (cnt == 0 && F_ISSET(vp, VC_C | VC_Y))
					break;
				FW(isspace(*p));
			} else {
				if (inword(*p))
					FW(inword(*p));
				else
					FW(!isspace(*p) && !inword(*p));
				/* 'c' and 'y' don't skip more space. */
				if (cnt == 0 && F_ISSET(vp, VC_C | VC_Y))
					break;
				FW(isspace(*p));
			}
		/*
		 * 'c', 'd' and 'y' don't move into the next line unless
		 * this line was empty.
		 */
		if (cnt == 0 && llen != 0 && F_ISSET(vp, VC_C | VC_D | VC_Y))
			break;
		if (len == 0) {
			/* If we hit EOF, stay there (historic practice). */
			if ((p = file_gline(sp, ep, ++lno, &llen)) == NULL) {
				/*
				 * Complain if were already at eof, unless
				 * it's a change command.
				 */
				if (empty && !F_ISSET(vp, VC_C)) {
					v_eof(sp, ep, NULL);
					return (1);
				}
				if ((p =
				    file_gline(sp, ep, --lno, &llen)) == NULL) {
					GETLINE_ERR(sp, lno);
					return (1);
				}
				rp->lno = lno;

				/* 'c', 'd', and 'y' move 1 past EOF. */
				rp->cno = llen ?
				    F_ISSET(vp, VC_C | VC_D | VC_Y) ? 
				    llen : llen - 1 : 0;
				return (0);
			}
			len = llen;
			cno = 0;
			startp = p;

			/* Skip leading space to first word. */
			FW(isspace(*p));
		}
	}
	rp->lno = lno;
	rp->cno = cno + (p - startp);
	return (0);
}

/*
 * v_wordb -- [count]b
 *	Move backward a word at a time.
 */
int
v_wordb(sp, ep, vp, fm, tm, rp)
	SCR *sp;
	EXF *ep;
	VICMDARG *vp;
	MARK *fm, *tm, *rp;
{
	return (bword(sp, ep, vp, fm, rp, 0));
}

/*
 * v_WordB -- [count]B
 *	Move backward a bigword at a time.
 */
int
v_wordB(sp, ep, vp, fm, tm, rp)
	SCR *sp;
	EXF *ep;
	VICMDARG *vp;
	MARK *fm, *tm, *rp;
{
	return (bword(sp, ep, vp, fm, rp, 1));
}

/*
 * bword --
 *	Move backward by words.
 */
static int
bword(sp, ep, vp, fm, rp, spaceonly)
	SCR *sp;
	EXF *ep;
	VICMDARG *vp;
	MARK *fm, *rp;
	int spaceonly;
{
	register char *p;
	recno_t lno;
	size_t len;
	u_long cno, cnt;
	char *startp;

	lno = fm->lno;
	cno = fm->cno;

	/* Check for start of file. */
	if (lno == 1 && cno == 0) {
		v_sof(sp, NULL);
		return (1);
	}

	if ((p = file_gline(sp, ep, lno, &len)) == NULL) {
		if (file_lline(sp, ep, &lno))
			return (1);
		if (lno == 0)
			v_sof(sp, NULL);
		else
			GETLINE_ERR(sp, lno);
		return (1);
	}
		
	cnt = F_ISSET(vp, VC_C1SET) ? vp->count : 1;

	/*
	 * Reset the length to the number of characters in the line; the
	 * first character is the current cursor position.
	 */
	len = cno ? cno + 1 : 0;
	if (len == 0)
		goto line;
	for (startp = p, p += cno; cnt--;) {
		if (spaceonly) {
			if (!isspace(*p)) {
				if (len < 2)
					goto line;
				--p;
				--len;
			}
			BW(isspace(*p));
			if (len)
				BW(!isspace(*p));
			else
				goto line;
		} else {
			if (!isspace(*p)) {
				if (len < 2)
					goto line;
				--p;
				--len;
			}
			BW(isspace(*p));
			if (len)
				if (inword(*p))
					BW(inword(*p));
				else
					BW(!isspace(*p) && !inword(*p));
			else
				goto line;
		}

		if (cnt && len == 0) {
			/* If we hit SOF, stay there (historic practice). */
line:			if (lno == 1) {
				rp->lno = 1;
				rp->cno = 0;
				return (0);
			}

			/*
			 * Get the line.  If the line is empty, decrement
			 * count and get another one.
			 */
			if ((p = file_gline(sp, ep, --lno, &len)) == NULL) {
				GETLINE_ERR(sp, lno);
				return (1);
			}
			if (len == 0) {
				if (cnt == 0 || --cnt == 0) {
					rp->lno = lno;
					rp->cno = 0;
					return (0);
				}
				goto line;
			}

			/*
			 * Set the cursor to the end of the line.  If the word
			 * at the end of this line has only a single character,
			 * we've already skipped over it.
			 */
			startp = p;
			if (len) {
				p += len - 1;
				if (cnt && len > 1 && !isspace(p[0]))
					if (inword(p[0])) {
						if (!inword(p[-1]))
							--cnt;
					} else if (!isspace(p[-1]) &&
					    !inword(p[-1]))
							--cnt;
			}
		} else {
			++p;
			++len;
		}
	}
	rp->lno = lno;
	rp->cno = p - startp;
	return (0);
}

/*
 * v_worde -- [count]e
 *	Move forward to the end of the word.
 */
int
v_worde(sp, ep, vp, fm, tm, rp)
	SCR *sp;
	EXF *ep;
	VICMDARG *vp;
	MARK *fm, *tm, *rp;
{
	return (eword(sp, ep, vp, fm, rp, 0));
}

/*
 * v_wordE -- [count]E
 *	Move forward to the end of the bigword.
 */
int
v_wordE(sp, ep, vp, fm, tm, rp)
	SCR *sp;
	EXF *ep;
	VICMDARG *vp;
	MARK *fm, *tm, *rp;
{
	return (eword(sp, ep, vp, fm, rp, 1));
}

/*
 * eword --
 *	Move forward to the end of the word.
 */
static int
eword(sp, ep, vp, fm, rp, spaceonly)
	SCR *sp;
	EXF *ep;
	VICMDARG *vp;
	MARK *fm, *rp;
	int spaceonly;
{
	register char *p;
	recno_t lno;
	size_t len, llen;
	u_long cno, cnt;
	int empty;
	char *startp;

	lno = fm->lno;
	cno = fm->cno;

	if ((p = file_gline(sp, ep, lno, &llen)) == NULL) {
		if (file_lline(sp, ep, &lno))
			return (1);
		if (lno == 0)
			v_eof(sp, ep, NULL);
		else
			GETLINE_ERR(sp, lno);
		return (1);
	}

	cnt = F_ISSET(vp, VC_C1SET) ? vp->count : 1;

	/*
	 * Reset the length; the first character is the current cursor
	 * position.  If no more characters in this line, may already
	 * be at EOF.
	 */
	len = llen - cno;
	if (empty = llen == 0 || llen == cno + 1)
		goto line;

	for (startp = p += cno; cnt--; empty = 0) {
		if (spaceonly) {
			if (!isspace(*p)) {
				if (len < 2)
					goto line;
				++p;
				--len;
			}
			FW(isspace(*p));
			if (len)
				FW(!isspace(*p));
			else
				++cnt;
		} else {
			if (!isspace(*p)) {
				if (len < 2)
					goto line;
				++p;
				--len;
			}
			FW(isspace(*p));
			if (len)
				if (inword(*p))
					FW(inword(*p));
				else
					FW(!isspace(*p) && !inword(*p));
			else
				++cnt;
		}

		if (cnt && len == 0) {
			/* If we hit EOF, stay there (historic practice). */
line:			if ((p = file_gline(sp, ep, ++lno, &llen)) == NULL) {
				/*
				 * If already at eof, complain, unless it's
				 * a change command or a delete command and
				 * there's something to delete.
				 */
				if (empty) {
					if (F_ISSET(vp, VC_C) ||
					    F_ISSET(vp, VC_D) && llen != 0) {
						rp->lno = lno - 1;
						rp->cno = llen ? llen : 1;
						return (0);
					}
					v_eof(sp, ep, NULL);
					return (1);
				}
				if ((p =
				    file_gline(sp, ep, --lno, &llen)) == NULL) {
					GETLINE_ERR(sp, lno);
					return (1);
				}
				rp->lno = lno;
				rp->cno = llen ? llen - 1 : 0;
				/* The 'c', 'd' and 'y' need one more space. */
				if (F_ISSET(vp, VC_C | VC_D | VC_Y))
					++rp->cno;
				return (0);
			}
			len = llen;
			cno = 0;
			startp = p;
		} else {
			--p;
			++len;
		}
	}
	rp->lno = lno;
	rp->cno = cno + (p - startp);

	/* The 'c', 'd' and 'y' need one more space. */
	if (F_ISSET(vp, VC_C | VC_D | VC_Y))
		++rp->cno;
	return (0);
}
