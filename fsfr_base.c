/*
 * fsfr_base.c
 *
 * Copyright (c) 2010, Tyler Larson <devel@tlarson.com>
 *
 * This software is licensed under the terms of the MIT License.
 * See the included file "LICENSE" for more information.
 *
 */

#include "fsfr.h"
#include <sys/types.h>
#include <sys/xattr.h>

/****************************************************************
 *  stat
 *  	Note that we've replaced __xstat() not stat(), which means
 *  	internal calls must use the *system* stat, which in turn calls
 *  	that function. We fsfr_statignore to tell us when to NOT apply
 *  	custom stat values
 ****************************************************************/
#define IMPLEMENT_STAT(NAME,FILETYPE,STATTYPE)                  \
int fsfr_base_##NAME(FILETYPE file, STATTYPE *buf)              \
{                                                               \
	fsfr_statignore = buf;                                      \
	int rtn = NAME(file,buf);                                   \
	fsfr_statignore = 0;                                        \
	return rtn;                                                 \
}                                                               
IMPLEMENT_STAT(stat,		const char*,	struct stat)
IMPLEMENT_STAT(fstat,		int,			struct stat)
IMPLEMENT_STAT(lstat,		const char*,	struct stat)
// These aren't currently used
//IMPLEMENT_STAT(stat64,		const char*,	struct stat64)
//IMPLEMENT_STAT(fstat64,		int,			struct stat64)
//IMPLEMENT_STAT(lstat64,		const char*,	struct stat64)
#undef IMPLEMENT_STAT

int fsfr_base_fstatat(int dirfd, const char *path, struct stat *buf, int flags)
{
	fsfr_statignore = buf;
	int rtn = fstatat(dirfd,path,buf,flags);
	fsfr_statignore = 0;
	return rtn;
}

ssize_t fsfr_base_listxattr(const char *path, char *list, size_t size) {
	static int(*fn_orig)(const char *, char *, size_t) = NULL;
	if (fn_orig==NULL) fn_orig = fsfr_dlnext("listxattr");
	if (!fn_orig) { return -1; }
	return fn_orig(path,list,size);
}
ssize_t fsfr_base_llistxattr(const char *path, char *list, size_t size) {
	static int(*fn_orig)(const char *, char *, size_t) = NULL;
	if (fn_orig==NULL) fn_orig = fsfr_dlnext("llistxattr");
	if (!fn_orig) { return -1; }
	return fn_orig(path,list,size);
}
ssize_t fsfr_base_flistxattr(int filedes, char *list, size_t size) {
	static int(*fn_orig)(int, char *, size_t) = NULL;
	if (fn_orig==NULL) fn_orig = fsfr_dlnext("flistxattr");
	if (!fn_orig) { return -1; }
	return fn_orig(filedes,list,size);
}

/****************************************************************
 *  open
 ****************************************************************/
int fsfr_base_openat(int dirfd, const char *pathname, int flags, mode_t mode)
{
	static int(*fn_orig)(int,const char *,int,mode_t) = NULL;
	if (fn_orig==NULL) fn_orig = fsfr_dlnext("openat");
	if (!fn_orig) { return -1; }
	return fn_orig(dirfd,pathname,flags,mode);
}

int fsfr_base_open(const char *pathname, int flags, mode_t mode)
{
	static int(*fn_orig)(const char *, int, mode_t) = NULL;
	if (fn_orig==NULL) fn_orig = fsfr_dlnext("open");
	if (!fn_orig) { return -1; }
	return fn_orig(pathname,flags,mode);
}


/****************************************************************
 *  chown
 ****************************************************************/
int fsfr_base_chmod(const char *path, mode_t mode)
{
	static int(*fn_orig)(const char *, mode_t) = NULL;
	if (fn_orig==NULL) fn_orig = fsfr_dlnext("chmod");
	if (!fn_orig) { return -1; }
	return fn_orig(path,mode);
}

int fsfr_base_fchmod(int fd, mode_t mode)
{
	static int(*fn_orig)(int, mode_t) = NULL;
	if (fn_orig==NULL) fn_orig = fsfr_dlnext("fchmod");
	if (!fn_orig) { return -1; }
	return fn_orig(fd,mode);
}

int fsfr_base_lchmod(const char *path, mode_t mode)
{
	static int(*fn_orig)(const char *, mode_t) = NULL;
	if (fn_orig==NULL) fn_orig = fsfr_dlnext("lchmod");
	if (!fn_orig) { return -1; }
	return fn_orig(path,mode);
}

