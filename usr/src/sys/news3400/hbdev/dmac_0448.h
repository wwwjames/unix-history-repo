/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Sony Corp. and Kazumasa Utashiro of Software Research Associates, Inc.
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
 *
 * from: $Hdr: dmac_0448.h,v 4.300 91/06/09 06:21:36 root Rel41 $ SONY
 *
 *	@(#)dmac_0448.h	8.1 (Berkeley) 6/11/93
 */

/*
 * Copyright (c) 1989- by SONY Corporation.
 */
/*
 *	dmac_0448.h
 *		DMAC L7A0448
 */

/*	dmac register base address	*/
#define DMAC_BASE		0xbfe00000

/*	register definition	*/
#define DMAC_GSTAT		(DMAC_BASE + 0xf)
#define DMAC_GSEL		(DMAC_BASE + 0xe)

#define DMAC_CSTAT		(DMAC_BASE + 0x2)
#define DMAC_CCTL		(DMAC_BASE + 0x3)
#define DMAC_CTRCL		(DMAC_BASE + 0x4)
#define DMAC_CTRCM		(DMAC_BASE + 0x5)
#define DMAC_CTRCH		(DMAC_BASE + 0x6)
#define DMAC_CTAG		(DMAC_BASE + 0x7)
#define DMAC_CWID		(DMAC_BASE + 0x8)
#define DMAC_COFSL		(DMAC_BASE + 0x9)
#define DMAC_COFSH		(DMAC_BASE + 0xa)
#define DMAC_CMAP		(DMAC_BASE + 0xc)
#define DMAC_CMAPH		(DMAC_BASE + 0xc)
#define DMAC_CMAPL		(DMAC_BASE + 0xd)

#ifdef mips
#define	VOLATILE	volatile
#else
#define	VOLATILE
#endif

#ifndef U_CHAR
#define U_CHAR	unsigned VOLATILE char
#endif

#ifndef U_SHORT
#define U_SHORT	unsigned VOLATILE short
#endif

#define dmac_gstat		*(U_CHAR *)DMAC_GSTAT
#define dmac_gsel		*(U_CHAR *)DMAC_GSEL

#define dmac_cstat		*(U_CHAR *)DMAC_CSTAT
#define dmac_cctl		*(U_CHAR *)DMAC_CCTL
#define dmac_ctrcl		*(U_CHAR *)DMAC_CTRCL
#define dmac_ctrcm		*(U_CHAR *)DMAC_CTRCM
#define dmac_ctrch		*(U_CHAR *)DMAC_CTRCH
#define dmac_ctag		*(U_CHAR *)DMAC_CTAG
#define dmac_cwid		*(U_CHAR *)DMAC_CWID
#define dmac_cofsl		*(U_CHAR *)DMAC_COFSL
#define dmac_cofsh		*(U_CHAR *)DMAC_COFSH
#define dmac_cmap		*(U_SHORT *)DMAC_CMAP
#define dmac_cmaph		*(U_CHAR *)DMAC_CMAPH
#define dmac_cmapl		*(U_CHAR *)DMAC_CMAPL

/*	status/control bit definition	*/
#define	DM_TCZ			0x80
#define	DM_A28			0x40
#define	DM_AFIX			0x20
#define	DM_APAD			0x10
#define	DM_ZINTEN		0x8
#define	DM_RST			0x4
#define	DM_MODE			0x2
#define DM_ENABLE		1

/*	general status bit definition	*/
#define CH_INT(x)		(u_char)(1 << (2 * x))
#define CH0_INT			1
#define CH1_INT			4
#define CH2_INT			0x10
#define CH3_INT			0x40

#define CH_MRQ(x)		(u_char)(1 << (2 * x + 1))
#define CH0_MRQ			2
#define CH1_MRQ			8
#define CH2_MRQ			0x20
#define CH3_MRQ			0x80

/*	channel definition	*/
#define	CH_SCSI			0
#define	CH_FDC			1
#define	CH_AUDIO		2
#define	CH_VIDEO		3

/*	dma status		*/

	struct	dm_stat {
		unsigned int dm_gstat;
		unsigned int dm_cstat;
		unsigned int dm_cctl;
		unsigned int dm_tcnt;
		unsigned int dm_offset;
		unsigned int dm_tag;
		unsigned int dm_width;
	} ;

#define	DMAC_WAIT	nops(10)

#define PINTEN		0xbfc80001
# define	DMA_INTEN	0x10
#define PINTSTAT	0xbfc80003
