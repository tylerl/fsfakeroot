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
 ****************************************************************/
int fsfr_base_stat(const char *path,struct stat* buf)
{
	static int(*fn_orig)(int,const char*,struct stat*) = NULL;
	if (fn_orig==NULL) fn_orig = fsfr_dlnext("__xstat");
	if (!fn_orig) { return -1; }
	return fn_orig(3,path,buf);
}
int fsfr_base_lstat(const char *path,struct stat* buf)
{
	static int(*fn_orig)(int,const char*,struct stat*) = NULL;
	if (fn_orig==NULL) fn_orig = fsfr_dlnext("__lxstat");
	if (!fn_orig) { return -1; }
	return fn_orig(3,path,buf);
}
int fsfr_base_fstat(int fd,struct stat* buf)
{
	static int(*fn_orig)(int,int,struct stat*) = NULL;
	if (fn_orig==NULL) fn_orig = fsfr_dlnext("__fxstat");
	if (!fn_orig) { return -1; }
	return fn_orig(3,fd,buf);
}
int fsfr_base_fstatat(int dirfd, const char *path, struct stat *buf, int flags)
{
	static int(*fn_orig)(int,int,const char*,struct stat*,int flags) = NULL;
	if (fn_orig==NULL) fn_orig = fsfr_dlnext("__fxstatat");
	if (!fn_orig) { return -1; }
	return fn_orig(3,dirfd,path,buf,flags);
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

