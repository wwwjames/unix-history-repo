/*
 * System call numbers.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * created from	@(#)syscalls.master	8.1 (Berkeley) 6/11/93
 */

#define	SYS_exit	1
#define	SYS_fork	2
#define	SYS_read	3
#define	SYS_write	4
#define	SYS_open	5
#define	SYS_close	6
#define	SYS_wait4	7
#define	SYS_ocreat	8
#define	SYS_link	9
#define	SYS_unlink	10
				/* 11 is obsolete execv */
#define	SYS_chdir	12
				/* 13 is old news_time */
#define	SYS_mknod	14
#define	SYS_chmod	15
#define	SYS_chown	16
#define	SYS_break	17
				/* 18 is obsolete stat */
#define	SYS_olseek	19
#define	SYS_getpid	20
#define	SYS_mount	21
				/* 22 is obsolete umount */
#define	SYS_setuid	23
#define	SYS_getuid	24
				/* 25 is obsolete stime */
#define	SYS_ptrace	26
				/* 27 is obsolete alarm */
				/* 28 is obsolete fstat */
				/* 29 is obsolete pause */
				/* 30 is obsolete utime */
#define	SYS_access	33
				/* 34 is obsolete access */
				/* 35 is obsolete ftime */
#define	SYS_sync	36
#define	SYS_kill	37
#define	SYS_ostat	38
				/* 39 is obsolete setpgrp */
#define	SYS_olstat	40
#define	SYS_dup	41
#define	SYS_pipe	42
				/* 43 is obsolete times */
#define	SYS_profil	44
				/* 46 is obsolete setgid */
#define	SYS_getgid	47
				/* 48 is obsolete ssig sig */
#define	SYS_acct	51
#define	SYS_ioctl	54
#define	SYS_reboot	55
#define	SYS_symlink	57
#define	SYS_readlink	58
#define	SYS_execve	59
#define	SYS_umask	60
#define	SYS_chroot	61
#define	SYS_ofstat	62
#define	SYS_ogetpagesize	64
#define	SYS_vfork	66
				/* 67 is obsolete vread */
				/* 68 is obsolete vwrite */
#define	SYS_sbrk	69
#define	SYS_sstk	70
#define	SYS_vadvise	72
#define	SYS_munmap	73
#define	SYS_mprotect	74
#define	SYS_madvise	75
				/* 76 is obsolete vhangup */
				/* 77 is obsolete vlimit */
#define	SYS_mincore	78
#define	SYS_getgroups	79
#define	SYS_setgroups	80
#define	SYS_getpgrp	81
#define	SYS_setpgid	82
#define	SYS_setitimer	83
#define	SYS_owait	84
#define	SYS_swapon	85
#define	SYS_getitimer	86
#define	SYS_ogethostname	87
#define	SYS_osethostname	88
#define	SYS_getdtablesize	89
#define	SYS_dup2	90
#define	SYS_fcntl	92
#define	SYS_select	93
#define	SYS_fsync	95
#define	SYS_setpriority	96
#define	SYS_socket	97
#define	SYS_connect	98
#define	SYS_oaccept	99
#define	SYS_getpriority	100
#define	SYS_osend	101
#define	SYS_orecv	102
#define	SYS_sigreturn	103
#define	SYS_bind	104
#define	SYS_setsockopt	105
#define	SYS_listen	106
				/* 107 is obsolete vtimes */
#define	SYS_osigvec	108
#define	SYS_osigblock	109
#define	SYS_osigsetmask	110
#define	SYS_sigsuspend	111
#define	SYS_osigstack	112
#define	SYS_orecvmsg	113
#define	SYS_osendmsg	114
#define	SYS_vtrace	115
				/* 115 is obsolete vtrace */
#define	SYS_gettimeofday	116
#define	SYS_getrusage	117
#define	SYS_getsockopt	118
#define	SYS_readv	120
#define	SYS_writev	121
#define	SYS_settimeofday	122
#define	SYS_fchown	123
#define	SYS_fchmod	124
#define	SYS_orecvfrom	125
#define	SYS_osetreuid	126
#define	SYS_osetregid	127
#define	SYS_rename	128
#define	SYS_otruncate	129
#define	SYS_oftruncate	130
#define	SYS_flock	131
#define	SYS_sendto	133
#define	SYS_shutdown	134
#define	SYS_socketpair	135
#define	SYS_mkdir	136
#define	SYS_rmdir	137
#define	SYS_utimes	138
				/* 139 is obsolete 4.2 sigreturn */
#define	SYS_adjtime	140
#define	SYS_ogetpeername	141
#define	SYS_ogethostid	142
#define	SYS_osethostid	143
#define	SYS_ogetrlimit	144
#define	SYS_osetrlimit	145
#define	SYS_okillpg	146
#define	SYS_quotactl	148
#define	SYS_oquota	149
#define	SYS_ogetsockname	150
#define	SYS_news_setenvp	151
#define	SYS_news_sysnews	152
#define	SYS_nfssvc	155
#define	SYS_ogetdirentries	156
#define	SYS_statfs	157
#define	SYS_fstatfs	158
#define	SYS_getfh	161
#define	SYS_sun_getdomainname	162
#define	SYS_sun_setdomainname	163
#define	SYS_shmsys	171
#define	SYS_sun_getdents	174
