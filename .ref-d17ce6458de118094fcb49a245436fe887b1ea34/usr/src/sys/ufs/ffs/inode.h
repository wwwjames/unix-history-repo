/*
 * Copyright (c) 1982, 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 *
 *	@(#)inode.h	7.16 (Berkeley) %G%
 */

#ifdef KERNEL
#include "../ufs/dinode.h"
#else
#include <ufs/dinode.h>
#endif

/*
 * The I node is the focus of all file activity in UNIX.
 * There is a unique inode allocated for each active file,
 * each current directory, each mounted-on file, text file, and the root.
 * An inode is 'named' by its dev/inumber pair. (iget/iget.c)
 * Data in `struct dinode' is read in from permanent inode on volume.
 */

struct inode {
	struct	inode *i_chain[2]; /* hash chain, MUST be first */
	struct	vnode *i_vnode;	/* vnode associated with this inode */
	struct	vnode *i_devvp;	/* vnode for block I/O */
	u_long	i_flag;		/* see below */
	dev_t	i_dev;		/* device where inode resides */
	ino_t	i_number;	/* i number, 1-to-1 with device address */
	struct	fs *i_fs;	/* file sys associated with this inode */
	struct	dquot *i_dquot[MAXQUOTAS]; /* pointer to dquot structures */
	struct	lockf *i_lockf;	/* Head of byte-level lock list */
	long	i_diroff;	/* offset in dir, where we found last entry */
	off_t	i_endoff;	/* end of useful stuff in directory */
	long	i_spare0;
	long	i_spare1;
	struct	dinode i_din;	/* the on-disk inode */
};

#define	i_mode		i_din.di_mode
#define	i_nlink		i_din.di_nlink
#define	i_uid		i_din.di_uid
#define	i_gid		i_din.di_gid
#if BYTE_ORDER == LITTLE_ENDIAN || defined(tahoe) /* ugh! -- must be fixed */
#define	i_size		i_din.di_qsize.val[0]
#else /* BYTE_ORDER == BIG_ENDIAN */
#define	i_size		i_din.di_qsize.val[1]
#endif
#define	i_db		i_din.di_db
#define	i_ib		i_din.di_ib
#define	i_atime		i_din.di_atime
#define	i_mtime		i_din.di_mtime
#define	i_ctime		i_din.di_ctime
#define i_blocks	i_din.di_blocks
#define	i_rdev		i_din.di_db[0]
#define i_flags		i_din.di_flags
#define i_gen		i_din.di_gen
#define	i_forw		i_chain[0]
#define	i_back		i_chain[1]

/* flags */
#define	ILOCKED		0x0001		/* inode is locked */
#define	IWANT		0x0002		/* some process waiting on lock */
#define	IRENAME		0x0004		/* inode is being renamed */
#define	IUPD		0x0010		/* file has been modified */
#define	IACC		0x0020		/* inode access time to be updated */
#define	ICHG		0x0040		/* inode has been changed */
#define	IMOD		0x0080		/* inode has been modified */
#define	ISHLOCK		0x0100		/* file has shared lock */
#define	IEXLOCK		0x0200		/* file has exclusive lock */
#define	ILWAIT		0x0400		/* someone waiting on file lock */

#ifdef KERNEL
/*
 * Convert between inode pointers and vnode pointers
 */
#define VTOI(vp)	((struct inode *)(vp)->v_data)
#define ITOV(ip)	((ip)->i_vnode)

/*
 * Convert between vnode types and inode formats
 */
extern enum vtype	iftovt_tab[];
extern int		vttoif_tab[];
#define IFTOVT(mode)	(iftovt_tab[((mode) & IFMT) >> 12])
#define VTTOIF(indx)	(vttoif_tab[(int)(indx)])

#define MAKEIMODE(indx, mode)	(int)(VTTOIF(indx) | (mode))

u_long	nextgennumber;		/* next generation number to assign */

extern ino_t	dirpref();

/*
 * Lock and unlock inodes.
 */
#ifdef notdef
#define	ILOCK(ip) { \
	while ((ip)->i_flag & ILOCKED) { \
		(ip)->i_flag |= IWANT; \
		(void) sleep((caddr_t)(ip), PINOD); \
	} \
	(ip)->i_flag |= ILOCKED; \
}

#define	IUNLOCK(ip) { \
	(ip)->i_flag &= ~ILOCKED; \
	if ((ip)->i_flag&IWANT) { \
		(ip)->i_flag &= ~IWANT; \
		wakeup((caddr_t)(ip)); \
	} \
}
#else
#define ILOCK(ip)	ilock(ip)
#define IUNLOCK(ip)	iunlock(ip)
#endif

#define	IUPDAT(ip, t1, t2, waitfor) { \
	if (ip->i_flag&(IUPD|IACC|ICHG|IMOD)) \
		(void) iupdat(ip, t1, t2, waitfor); \
}

#define	ITIMES(ip, t1, t2) { \
	if ((ip)->i_flag&(IUPD|IACC|ICHG)) { \
		(ip)->i_flag |= IMOD; \
		if ((ip)->i_flag&IACC) \
			(ip)->i_atime = (t1)->tv_sec; \
		if ((ip)->i_flag&IUPD) \
			(ip)->i_mtime = (t2)->tv_sec; \
		if ((ip)->i_flag&ICHG) \
			(ip)->i_ctime = time.tv_sec; \
		(ip)->i_flag &= ~(IACC|IUPD|ICHG); \
	} \
}

/*
 * This overlays the fid sturcture (see mount.h)
 */
struct ufid {
	u_short	ufid_len;	/* length of structure */
	u_short	ufid_pad;	/* force long alignment */
	ino_t	ufid_ino;	/* file number (ino) */
	long	ufid_gen;	/* generation number */
};

/*
 * Prototypes for UFS vnode operations
 */
int	ufs_lookup __P((
		struct vnode *vp,
		struct nameidata *ndp,
		struct proc *p));
int	ufs_create __P((
		struct nameidata *ndp,
		struct vattr *vap,
		struct proc *p));
int	ufs_mknod __P((
		struct nameidata *ndp,
		struct vattr *vap,
		struct ucred *cred,
		struct proc *p));
int	ufs_open __P((
		struct vnode *vp,
		int mode,
		struct ucred *cred,
		struct proc *p));
int	ufs_close __P((
		struct vnode *vp,
		int fflag,
		struct ucred *cred,
		struct proc *p));
int	ufs_access __P((
		struct vnode *vp,
		int mode,
		struct ucred *cred,
		struct proc *p));
int	ufs_getattr __P((
		struct vnode *vp,
		struct vattr *vap,
		struct ucred *cred,
		struct proc *p));
int	ufs_setattr __P((
		struct vnode *vp,
		struct vattr *vap,
		struct ucred *cred,
		struct proc *p));
int	ufs_read __P((
		struct vnode *vp,
		struct uio *uio,
		int ioflag,
		struct ucred *cred));
int	ufs_write __P((
		struct vnode *vp,
		struct uio *uio,
		int ioflag,
		struct ucred *cred));
int	ufs_ioctl __P((
		struct vnode *vp,
		int command,
		caddr_t data,
		int fflag,
		struct ucred *cred,
		struct proc *p));
int	ufs_select __P((
		struct vnode *vp,
		int which,
		int fflags,
		struct ucred *cred,
		struct proc *p));
int	ufs_mmap __P((
		struct vnode *vp,
		int fflags,
		struct ucred *cred,
		struct proc *p));
int	ufs_fsync __P((
		struct vnode *vp,
		int fflags,
		struct ucred *cred,
		int waitfor,
		struct proc *p));
int	ufs_seek __P((
		struct vnode *vp,
		off_t oldoff,
		off_t newoff,
		struct ucred *cred));
int	ufs_remove __P((
		struct nameidata *ndp,
		struct proc *p));
int	ufs_link __P((
		struct vnode *vp,
		struct nameidata *ndp,
		struct proc *p));
int	ufs_rename __P((
		struct nameidata *fndp,
		struct nameidata *tdnp,
		struct proc *p));
int	ufs_mkdir __P((
		struct nameidata *ndp,
		struct vattr *vap,
		struct proc *p));
int	ufs_rmdir __P((
		struct nameidata *ndp,
		struct proc *p));
int	ufs_symlink __P((
		struct nameidata *ndp,
		struct vattr *vap,
		char *target,
		struct proc *p));
int	ufs_readdir __P((
		struct vnode *vp,
		struct uio *uio,
		struct ucred *cred,
		int *eofflagp));
int	ufs_readlink __P((
		struct vnode *vp,
		struct uio *uio,
		struct ucred *cred));
int	ufs_abortop __P((
		struct nameidata *ndp));
int	ufs_inactive __P((
		struct vnode *vp,
		struct proc *p));
int	ufs_reclaim __P((
		struct vnode *vp));
int	ufs_lock __P((
		struct vnode *vp));
int	ufs_unlock __P((
		struct vnode *vp));
int	ufs_bmap __P((
		struct vnode *vp,
		daddr_t bn,
		struct vnode **vpp,
		daddr_t *bnp));
int	ufs_strategy __P((
		struct buf *bp));
int	ufs_print __P((
		struct vnode *vp));
int	ufs_islocked __P((
		struct vnode *vp));
int	ufs_advlock __P((
		struct vnode *vp,
		caddr_t id,
		int op,
		struct flock *fl,
		int flags));
#endif /* KERNEL */
