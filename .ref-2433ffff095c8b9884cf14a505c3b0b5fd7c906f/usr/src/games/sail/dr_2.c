/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static char sccsid[] = "@(#)dr_2.c	5.3 (Berkeley) %G%";
#endif /* not lint */

#include "driver.h"

#define couldwin(f,t) (f->specs->crew2 > t->specs->crew2 * 1.5)

thinkofgrapples()
{
	register struct ship *sp, *sq;
	char friendly;

	foreachship(sp) {
		if (sp->file->captain[0] || sp->file->dir == 0)
			continue;
		foreachship(sq) {
			friendly = sp->nationality == capship(sq)->nationality;
			if (!friendly) {
				if (sp->file->struck || sp->file->captured != 0)
					continue;
				if (range(sp, sq) != 1)
					continue;
				if (grappled2(sp, sq))
					if (toughmelee(sp, sq, 0, 0))
						ungrap(sp, sq);
					else
						grap(sp, sq);
				else if (couldwin(sp, sq)) {
					grap(sp, sq);
					sp->file->loadwith = L_GRAPE;
				}
			} else
				ungrap(sp, sq);
		}
	}
}

checkup()
{
	register struct ship *sp, *sq;
	register char explode, sink;

	foreachship(sp) {
		if (sp->file->dir == 0)
			continue;
		explode = sp->file->explode;
		sink = sp->file->sink;
		if (explode != 1 && sink != 1)
			continue;
		if (die() < 5)
			continue;
		Write(sink == 1 ? W_SINK : W_EXPLODE, sp, 0, 2, 0, 0, 0);
		Write(W_DIR, sp, 0, 0, 0, 0, 0);
		if (snagged(sp))
			foreachship(sq)
				cleansnag(sp, sq, 1);
		if (sink != 1) {
			makesignal(sp, "exploding!", (struct ship *)0);
			foreachship(sq) {
				if (sp != sq && sq->file->dir && range(sp, sq) < 4)
					table(RIGGING, L_EXPLODE, sp->specs->guns/13, sq, sp, 6);
			}
		} else
			makesignal(sp, "sinking!", (struct ship *)0);
	}
}

prizecheck()
{
	register struct ship *sp;

	foreachship(sp) {
		if (sp->file->captured == 0)
			continue;
		if (sp->file->struck || sp->file->dir == 0)
			continue;
		if (sp->specs->crew1 + sp->specs->crew2 + sp->specs->crew3 > sp->file->pcrew * 6) {
			Write(W_SIGNAL, sp, 1,
				(int)"prize crew overthrown", 0, 0, 0);
			Write(W_POINTS, sp->file->captured, 0, sp->file->captured->file->points - 2 * sp->specs->pts, 0, 0, 0);
			Write(W_CAPTURED, sp, 0, -1, 0, 0, 0);
		}
	}
}

strend(str)
char *str;
{
	register char *p;

	for (p = str; *p; p++)
		;
	return p == str ? 0 : p[-1];
}

closeon(from, to, command, ta, ma, af)
register struct ship *from, *to;
char command[];
int ma, ta, af;
{
	int high;
	char temp[10];

	temp[0] = command[0] = '\0';
	high = -30000;
	try(command, temp, ma, ta, af, ma, from->file->dir, from, to, &high, 0);
}

int dtab[] = {0,1,1,2,3,4,4,5};		/* diagonal distances in x==y */

score(movement, ship, to, onlytemp)
char movement[];
register struct ship *ship, *to;
char onlytemp;
{
	char drift;
	int row, col, dir, total, ran;
	register struct File *fp = ship->file;

	if ((dir = fp->dir) == 0)
		return 0;
	row = fp->row;
	col = fp->col;
	drift = fp->drift;
	move(movement, ship, &fp->dir, &fp->row, &fp->col, &drift);
	if (!*movement)
		(void) strcpy(movement, "d");

	ran = range(ship, to);
	total = -50 * ran;
	if (ran < 4 && gunsbear(ship, to))
		total += 60;
	if ((ran = portside(ship, to, 1) - fp->dir) == 4 || ran == -4)
		total = -30000;

	if (!onlytemp) {
		fp->row = row;
		fp->col = col;
		fp->dir = dir;
	}
	return total;
}

move(p, ship, dir, row, col, drift)
register char *p;
register struct ship *ship;
register char *dir;
register short *row, *col;
register char *drift;
{
	int dist;
	char moved = 0;

	for (; *p; p++) {
		switch (*p) {
		case 'r':
			if (++*dir == 9)
				*dir = 1;
			break;
		case 'l':
			if (--*dir == 0)
				*dir = 8;
			break;
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7':
			moved++;
			if (*dir % 2 == 0)
				dist = dtab[*p - '0'];
			else
				dist = *p - '0';
			*row -= dr[*dir] * dist;
			*col -= dc[*dir] * dist;
			break;
		}
	}
	if (!moved) {
		if (windspeed != 0 && ++*drift > 2) {
			if (ship->specs->class >= 3 && !snagged(ship)
			    || (turn & 1) == 0) {
				*row -= dr[winddir];
				*col -= dc[winddir];
			}
		}
	} else
		*drift = 0;
}

try(command, temp, ma, ta, af, vma, dir, f, t, high, rakeme)
register struct ship *f, *t;
int ma, ta, af, *high, rakeme;
char command[], temp[];
{
	register int new, n;
	char st[4];
#define rakeyou (gunsbear(f, t) && !gunsbear(t, f))

	if ((n = strend(temp)) < '1' || n > '9')
		for (n = 1; vma - n >= 0; n++) {
			(void) sprintf(st, "%d", n);
			(void) strcat(temp, st);
			new = score(temp, f, t, rakeme);
			if (new > *high && (!rakeme || rakeyou)) {
				*high = new;
				(void) strcpy(command, temp);
			}
			try(command, temp, ma-n, ta, af, vma-n,
				dir, f, t, high, rakeme);
			rmend(temp);
		}
	if (ma > 0 && ta > 0 && (n = strend(temp)) != 'l' && n != 'r' || !strlen(temp)) {
		(void) strcat(temp, "r");
		new = score(temp, f, t, rakeme);
		if (new > *high && (!rakeme || gunsbear(f, t) && !gunsbear(t, f))) {
			*high = new;
			(void) strcpy(command, temp);
		}
		try(command, temp, ma-1, ta-1, af, min(ma-1, maxmove(f, (dir == 8 ? 1 : dir+1), 0)), (dir == 8 ? 1 : dir+1),f,t,high,rakeme);
		rmend(temp);
	}
	if ((ma > 0 && ta > 0 && (n = strend(temp)) != 'l' && n != 'r') || !strlen(temp)){
		(void) strcat(temp, "l");
		new = score(temp, f, t, rakeme);
		if (new > *high && (!rakeme || (gunsbear(f, t) && !gunsbear(t, f)))){
			*high = new;
			(void) strcpy(command, temp);
		}
		try(command, temp, ma-1, ta-1, af, (min(ma-1,maxmove(f, (dir-1 ? dir-1 : 8), 0))), (dir-1 ? dir -1 : 8), f, t, high, rakeme);
		rmend(temp);
	}
}

rmend(str)
char *str;
{
	register char *p;

	for (p = str; *p; p++)
		;
	if (p != str)
		*--p = 0;
}
