/*
 * fsrakeroot.c
 *
 * Copyright (c) 2010, Tyler Larson <devel@tlarson.com>
 *
 * This software is licensed under the terms of the MIT License.
 * See the included file "LICENSE" for more information.
 *
 */

#define _ATFILE_SOURCE // for AT_FDCWD, et. al.

#include "fsfr.h"
#include <stdlib.h>
#include <string.h>


#undef __xstat
#undef __fxstat
#undef __lxstat
#undef __xstat64
#undef __fxstat64
#undef __lxstat64
#undef _FILE_OFFSET_BITS


/****************************************************************
 *  getuid() et. al.
 ****************************************************************/
// We're faking root; this does the most obvious part of that process
uid_t getuid(void)  { return 0; }
uid_t geteuid(void) { return 0; }
gid_t getgid(void)  { return 0; }
gid_t getegid(void) { return 0; }

/****************************************************************
 *  stat() variations
 *  	Replace returned statistics with those stored in xattrs
 *  	if the xattrs are set.
 ****************************************************************/
#define IMPLEMENT_STAT(NAME,FILETYPE,STATTYPE,GETATTR)						\
int NAME(int ver, FILETYPE file, STATTYPE buf)								\
{																			\
	static int(*fn_orig)(int,FILETYPE,STATTYPE) = NULL;						\
	if (NULL==fn_orig) fn_orig = fsfr_dlnext(#NAME);						\
	if (!fn_orig) { return -1; }											\
	int rtn = fn_orig(ver,file,buf);										\
	if (rtn || !buf) return rtn;											\
	int mask = GETATTR(file,XATTR_MODEMASK,buf);							\
	int uid = GETATTR(file,XATTR_UID,buf);									\
	int gid = GETATTR(file,XATTR_GID,buf);									\
	int rdev = GETATTR(file,XATTR_RDEV,buf);								\
	if (mask!=-1) {															\
		int mode = GETATTR(file,XATTR_MODE,buf);							\
		buf->st_mode = (buf->st_mode & ~mask) | (mode & mask);				\
	}																		\
	if (uid!=-1) buf->st_uid = uid;											\
	if (gid!=-1) buf->st_gid = gid;											\
	if (rdev!=-1) buf->st_rdev = rdev;										\
	return 0;																\
}
IMPLEMENT_STAT(__xstat,		const char*,	struct stat*, 	fsfr_getxattr_int_stat)
IMPLEMENT_STAT(__fxstat,	int,			struct stat*, 	fsfr_fgetxattr_int_stat)
IMPLEMENT_STAT(__lxstat,	const char*, 	struct stat*, 	fsfr_lgetxattr_int_stat)
IMPLEMENT_STAT(__xstat64,	const char*, 	struct stat64*,	fsfr_getxattr_int_stat64)
IMPLEMENT_STAT(__fxstat64,	int,			struct stat64*,	fsfr_fgetxattr_int_stat64)
IMPLEMENT_STAT(__lxstat64,	const char*, 	struct stat64*,	fsfr_lgetxattr_int_stat64)
#undef IMPLEMENT_STAT

/****************************************************************
 *  chown() variations
 *  	Set new UID/GID in the xattrs
 ****************************************************************/
// we don't even attempt to change REAL ownership -- Even as root,
// this operates on the "visible" owner, leaving the "real" owner intact

#define IMPLEMENT_CHOWN(NAME,FILETYPE,SETATTR,FSTAT,CHMOD)			\
int NAME(FILETYPE file, uid_t owner, gid_t group)					\
{																	\
	int rtn=0;														\
	struct stat st;													\
	FSTAT(file,&st);												\
	if ((st.st_mode&0600) != 0600)CHMOD(file,st.st_mode & 0777);	\
	if (owner!=-1) rtn = SETATTR(file,XATTR_UID,owner,&st) || rtn;	\
	if (group!=-1) rtn = SETATTR(file,XATTR_GID,group,&st) || rtn;	\
	return rtn?-1:0;												\
}
IMPLEMENT_CHOWN(chown,	const char*,fsfr_setxattr_int_stat,fsfr_base_stat,chmod)
IMPLEMENT_CHOWN(fchown,	int,		fsfr_fsetxattr_int_stat,fsfr_base_fstat,fchmod)
IMPLEMENT_CHOWN(lchown,	const char*,fsfr_lsetxattr_int_stat,fsfr_base_lstat,lchmod)
#undef IMPLEMENT_CHOWN

/****************************************************************
 *  chmod() variations
 *  	* Set new mode in the xattrs
 *  	* Enforce a minumum accessability standard, so that
 *  	  the user can always access the files (as root could)
 ****************************************************************/
// Root wouldn't be able to lock himself out of a directory or file,
// so we just make sure that u+rwX stays set; only for files and dirs
int chmod(const char *path, mode_t mode)
{
	//fprintf(stderr,"chmod %s: 0%3o\n",path,mode);
	struct stat st;
	fsfr_base_stat(path,&st);
	int oldmask = fsfr_getxattr_int_stat(path,XATTR_MODEMASK,&st);
	if (oldmask==-1) oldmask = 0;
	int reqmode = 00600;
	if (S_ISDIR(st.st_mode)) {
		reqmode = reqmode | 00100;	// dirs require u+x
	} else if (!S_ISREG(st.st_mode)) {
		return fsfr_base_chmod(path,mode);	// we don't mess with everything else
	}

	int filemode = mode | reqmode;
	int fakemode = mode;
	int newmask = filemode ^ fakemode;

	if (oldmask & ~0777) {
		int oldmode = fsfr_getxattr_int_stat(path,XATTR_MODE,&st);
		if (oldmode==-1) oldmode = 0;
		// fake non-file
		fakemode = fakemode | (oldmode & ~0777);
		newmask = newmask | (oldmask & ~0777);
	}

	if (fsfr_base_chmod(path,filemode)) {
		return -1;
	}

	if (filemode == fakemode) {	// clear mode xattr if no longer used
		fsfr_unset_attr_stat(path,XATTR_MODE,&st);
		fsfr_unset_attr_stat(path,XATTR_MODEMASK,&st);
	} else {
		fsfr_setxattr_int_stat(path,XATTR_MODE,fakemode,&st);
		fsfr_setxattr_int_stat(path,XATTR_MODEMASK,newmask,&st);
	}

	return 0;
}
// same as above, but w/ FDs
int fchmod(int fd, mode_t mode)
{
	//fprintf(stderr,"chmod %s: 0%3o\n",path,mode);
	struct stat st;
	fsfr_base_fstat(fd,&st);
	int oldmask = fsfr_fgetxattr_int_stat(fd,XATTR_MODEMASK,&st);
	if (oldmask==-1) oldmask = 0;
	int reqmode = 00600;
	if (S_ISDIR(st.st_mode)) {
		reqmode = reqmode | 00100;	// dirs require u+x
	} else if (!S_ISREG(st.st_mode)) {
		return fsfr_base_fchmod(fd,mode);	// we don't mess with everything else
	}

	int filemode = mode | reqmode;
	int fakemode = mode;
	int newmask = filemode ^ fakemode;

	if (oldmask & ~0777) {
		int oldmode = fsfr_fgetxattr_int_stat(fd,XATTR_MODE,&st);
		if (oldmode==-1) oldmode = 0;
		// fake non-file
		fakemode = fakemode | (oldmode & ~0777);
		newmask = newmask | (oldmask & ~0777);
	}

	if (fsfr_base_fchmod(fd,filemode)) {
		return -1;
	}

	if (filemode == fakemode) {	// clear mode xattr if no longer used
		fsfr_funset_attr_stat(fd,XATTR_MODE,&st);
		fsfr_funset_attr_stat(fd,XATTR_MODEMASK,&st);
	} else {
		fsfr_fsetxattr_int_stat(fd,XATTR_MODE,fakemode,&st);
		fsfr_fsetxattr_int_stat(fd,XATTR_MODEMASK,newmask,&st);
	}

	return 0;
}


/****************************************************************
 *  listxattr() variations
 *  	Filter out xattrs used internally by fsfr. They're 
 *  	tecnically not supposed to be there, right?
 ****************************************************************/
ssize_t fsfr_filter_xattr(char *list, size_t size)
{
	if (list==NULL) return size;
	char *write_p;
	char *read_p;
	char *end;
	read_p = write_p = list;
	end = list + size;
	int prefix_len = strlen(XATTR_PREFIX);
	while (read_p < end) {
		int len = strlen(read_p);
		if (strncmp(read_p,XATTR_PREFIX,prefix_len)) {
			if (read_p!=write_p) {
				memcpy(write_p,read_p,len);
				write_p[len] = (char)0;
			}
			write_p += len + 1;
		}
		read_p += len + 1;
	}
	if (write_p < end) memset(write_p,0,end-write_p);
	return write_p - list;
}

ssize_t listxattr(const char *path, char *list, size_t size) 
{
	int rtn = fsfr_base_listxattr(path,list,size);
	if (rtn > 0) return fsfr_filter_xattr(list,rtn);
	return rtn;
}
ssize_t llistxattr(const char *path, char *list, size_t size) 
{
	int rtn = fsfr_base_llistxattr(path,list,size);
	if (rtn > 0) return fsfr_filter_xattr(list,rtn);
	return rtn;
}
ssize_t flistxattr(int fd, char *list, size_t size) 
{
	int rtn = fsfr_base_flistxattr(fd,list,size);
	if (rtn > 0) return fsfr_filter_xattr(list,rtn);
	return rtn;
}

/****************************************************************
 *  mknod() variations
 *  	Create an empty plain file, and st the appropriate
 *  	xattrs to simulate a dev node.
 *
 *  	NOTE: don't write to these nodes! They don't work. You'll
 *  	just be writing to the underlying file.
 ****************************************************************/

void frfs_mknod_helper(int fd, mode_t mode, dev_t dev)
{
	int mask = 00777;
	fsfr_fsetxattr_int(fd,XATTR_UID,0);
	fsfr_fsetxattr_int(fd,XATTR_GID,0);
	fsfr_fsetxattr_int(fd,XATTR_RDEV,dev);
	fsfr_fsetxattr_int(fd,XATTR_MODE,mode & ~mask);
	fsfr_fsetxattr_int(fd,XATTR_MODEMASK,~mask);
}

int __xmknod(int ver, const char *pathname, mode_t mode, dev_t * dev)
{
	if (!dev) {
		errno=EINVAL;
		return  -1;
	}
	//fprintf(stderr, "0x%x 0%o 0x%x\n",mode,mode,*dev);
	int mask = 00777;
	int fd = open(pathname, O_WRONLY|O_CREAT|O_TRUNC, mode & mask);
	if (fd==-1) return -1;
	frfs_mknod_helper(fd,mode,*dev);
	close(fd);
	return 0;
}

int __xmknodat(int ver, int fd, const char *pathname, mode_t mode, dev_t * dev)
{
	if (!dev) {
		errno=EINVAL;
		return  -1;
	}
	int mask = 00777;
	int ffd = openat(fd, pathname, O_WRONLY|O_CREAT|O_TRUNC, mode & mask);
	if (ffd==-1) return -1;
	frfs_mknod_helper(ffd,mode,*dev);
	close(ffd);
	return 0;
}

/****************************************************************
 *  symlink()
 *  Change symlink ownership to root.root. This will fail silently
 *  if the FSFR_PROXY_DIR env var is not set. See the note attached
 *  to fsfr_proxygetxattr_int for details.
 ****************************************************************/
int symlink(const char *oldpath, const char *newpath)
{
	static int(*fn_orig)(const char*,const char*) = NULL;
	if (NULL==fn_orig) fn_orig = fsfr_dlnext("symlink");
	if (!fn_orig) { return -1; }
	int rtn = fn_orig(oldpath,newpath);
	if (!rtn) {
		lchown(newpath,0,0);
	}
	return rtn;
}


/****************************************************************
 *  mkdir()
 *  	* Enforce minimum accessiblity (u+rwx)
 *  	* Set ownership to root.root
 ****************************************************************/
int mkdir(const char *pathname, mode_t mode)
{
	int reqmode = 0700;
	int new_mode = mode | reqmode;
	static int(*fn_orig)(const char*,mode_t) = NULL;
	if (NULL==fn_orig) fn_orig = fsfr_dlnext("mkdir");
	if (!fn_orig) { return -1; }
	int rtn = fn_orig(pathname,new_mode);
	if (!rtn) {
		fsfr_setxattr_int(pathname,XATTR_UID,0);
		fsfr_setxattr_int(pathname,XATTR_GID,0);
		if (mode != new_mode) {
			fsfr_setxattr_int(pathname,XATTR_MODE,mode&reqmode);
			fsfr_setxattr_int(pathname,XATTR_MODEMASK,reqmode);
		}
	}
	return rtn;
}

int mkdirat(int fd, const char *pathname, mode_t mode)
{
	int reqmode = 0700;
	int new_mode = mode | reqmode;
	static int(*fn_orig)(int,const char*,mode_t) = NULL;
	if (NULL==fn_orig) fn_orig = fsfr_dlnext("mkdirat");
	if (!fn_orig) { return -1; }
	int rtn = fn_orig(fd,pathname,new_mode);
	if (!rtn) {
		int newfd = fsfr_base_openat(fd,pathname,O_DIRECTORY,0777);
		if (newfd!=-1) {
			fsfr_fsetxattr_int(newfd,XATTR_UID,0);
			fsfr_fsetxattr_int(newfd,XATTR_GID,0);
			if (mode != new_mode) {
				fsfr_fsetxattr_int(newfd,XATTR_MODE,mode&reqmode);
				fsfr_fsetxattr_int(newfd,XATTR_MODEMASK,reqmode);
			}
		}
		close(newfd);
	}
	return rtn;
}

/****************************************************************
 *  open()
 *  	* Enforce accessiblity (u+rw)
 *  	* Set ownership to root.root
 *
 *  So, I can't figure out which base syscall is being used for
 *  open(), so I'm skipping this one. It's not critical, but I'd
 *  like it to be included. If you know what fn to override, drop
 *  me a line at devel@tlarson.com
 ****************************************************************/
/*
int open(const char *pathname, int flags, mode_t mode)
{
	int mask = 0600;
	int _umask = umask(0);
	umask(_umask & ~mask);
	int fd = fsfr_base_open(pathname,flags,mode|mask);
	umask(_umask);
	if (fd != -1) {
		if (((mask&_umask)!=0)||((mask|mode)!=mode)) {
			fsfr_fsetxattr_int(fd,XATTR_MODE,mode & mask);
			fsfr_fsetxattr_int(fd,XATTR_MODEMASK,mask);
		}
		fsfr_fsetxattr_int(fd,XATTR_UID,0);
		fsfr_fsetxattr_int(fd,XATTR_GID,0);
	}
	return fd;
}

int openat(int fd, const char *pathname, int flags, mode_t mode)
{
	int mask = 0600;
	int _umask = umask(0);
	umask(_umask & ~mask);
	int fd = fsfr_base_openat(fd,pathname,flags,mode|mask);
	umask(_umask);
	if (fd != -1) {
		if (((mask&_umask)!=0)||((mask|mode)!=mode)) {
			fsfr_fsetxattr_int(fd,XATTR_MODE,mode & mask);
			fsfr_fsetxattr_int(fd,XATTR_MODEMASK,mask);
		}
		fsfr_fsetxattr_int(fd,XATTR_UID,0);
		fsfr_fsetxattr_int(fd,XATTR_GID,0);
	}
	return fd;
}
*/

/****************************************************************
 *  all the f...at() functions
 *  	Since there is no fgetxattrat function, we need to be
 *  	able to use traditional path semantics to find the file.
 *		First, we check for a proc directory, and if we can use
 *		it, we set the path relative to the corresponding fd
 *		entry. Failing at that, we open the dir by fd, chdir to
 *		to it, get the current working direcotory, then then
 *		chdir back to whence we came. This second option has
 *		the possiblity of causing a race condition in
 *		multithreaded apps, but if you've got a better idea,
 *		I'm all ears.
 *  	
 ****************************************************************/
#define IMPLEMENT_AT(OUTFN,LOUTFN,...) 							\
	if (pathname[0]=='/' || AT_FDCWD==fd) {						\
		const char *path = pathname;							\
		if ((flags & AT_SYMLINK_NOFOLLOW)==AT_SYMLINK_NOFOLLOW)	\
			return LOUTFN(__VA_ARGS__);							\
		else													\
			return OUTFN(__VA_ARGS__);							\
	} else														\
	{															\
		char *path = alloca(PATH_MAX+1);						\
		if (!access("/proc/self/fd",X_OK)) {					\
			sprintf(path,"/proc/self/fd/%i/",fd);				\
			strncat(path,pathname,PATH_MAX);					\
		} else {												\
			DIR* save = opendir(".");							\
			fchdir(fd);											\
			getcwd(path,PATH_MAX);								\
			fchdir(dirfd(save));								\
			closedir(save);										\
			strncat(path,"/",PATH_MAX);							\
		}														\
		strncat(path,pathname,PATH_MAX);						\
		if ((flags & AT_SYMLINK_NOFOLLOW)==AT_SYMLINK_NOFOLLOW)	\
			return LOUTFN(__VA_ARGS__);							\
		else													\
			return OUTFN(__VA_ARGS__);							\
	}

int __fxstatat(int ver, int fd, const char *pathname, struct stat *buf, int flags)
{
	IMPLEMENT_AT(__xstat,__lxstat,ver,path,buf)
}

int __fxstatat64(int ver, int fd, const char *pathname, struct stat64 *buf, int flags)
{
	IMPLEMENT_AT(__xstat64,__lxstat64,ver,path,buf)
}

int fchownat(int fd, const char*pathname, uid_t owner, gid_t group, int flags)
{
	IMPLEMENT_AT(chown,lchown,path,owner,group)
}

int fchmodat(int fd, const char*pathname, mode_t mode, int flags)
{
	IMPLEMENT_AT(chmod,lchmod,path,mode);
}

int symlinkat(const char *oldpath, int fd, const char *pathname)
{
	int flags=0;
	IMPLEMENT_AT(symlink,symlink,oldpath,path);
}

#undef IMPLEMENT_AT

