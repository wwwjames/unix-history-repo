/*-
 * Copyright (c) 2003-2009 Tim Kientzle
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef ARCHIVE_READ_DISK_PRIVATE_H_INCLUDED
#define ARCHIVE_READ_DISK_PRIVATE_H_INCLUDED

struct archive_read_disk {
	struct archive	archive;

	/*
	 * Symlink mode is one of 'L'ogical, 'P'hysical, or 'H'ybrid,
	 * following an old BSD convention.  'L' follows all symlinks,
	 * 'P' follows none, 'H' follows symlinks only for the first
	 * item.
	 */
	char	symlink_mode;

	/*
	 * Since symlink interaction changes, we need to track whether
	 * we're following symlinks for the current item.  'L' mode above
	 * sets this true, 'P' sets it false, 'H' changes it as we traverse.
	 */
	char	follow_symlinks;  /* Either 'L' or 'P'. */

	const char * (*lookup_gname)(void *private, gid_t gid);
	void	(*cleanup_gname)(void *private);
	void	 *lookup_gname_data;
	const char * (*lookup_uname)(void *private, gid_t gid);
	void	(*cleanup_uname)(void *private);
	void	 *lookup_uname_data;
};

#endif
