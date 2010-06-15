/*
 * fsfr.h
 *
 * Copyright (c) 2010, Tyler Larson <devel@tlarson.com>
 *
 * This software is licensed under the terms of the MIT License.
 * See the included file "LICENSE" for more information.
 *
 */

#ifndef FSFR_H_
#define FSFR_H_

#define _GNU_SOURCE // for RTLD_NEXT
#define _ATFILE_SOURCE // for AT_*

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <sys/xattr.h>
#include <string.h>
#include <alloca.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


// xattr names used by this mechanism
#define XATTR_PREFIX "user.fsfr."
#define XATTR_MODE XATTR_PREFIX "mode"
#define XATTR_MODEMASK XATTR_PREFIX "modemask"
#define XATTR_UID XATTR_PREFIX "uid"
#define XATTR_GID XATTR_PREFIX "gid"
#define XATTR_RDEV XATTR_PREFIX "rdev"

void * fsfr_dlnext(const char *sym);

int fsfr_getxattr_int(const char *fpath, const char *name);
int fsfr_lgetxattr_int(const char *fpath, const char *name);
int fsfr_fgetxattr_int(int fd, const char *name);
int fsfr_setxattr_int(const char *fpath, const char* name, int64_t val);
int fsfr_fsetxattr_int(int fd, const char* name, int64_t val);
int fsfr_lsetxattr_int(const char *fpath, const char* name, int64_t val);
int fsfr_unset_attr(const char *fpath, const char* name);
int fsfr_funset_attr(int fd, const char* name);
int fsfr_lunset_attr(const char *fpath, const char* name);

int fsfr_proxygetxattr_int(int64_t inode, const char *name);
int fsfr_proxysetxattr_int(int64_t inode, const char *name, int64_t val);
int fsfr_proxyunsetxattr_int(int64_t inode, const char *name);

int fsfr_getxattr_int_stat(const char *fpath, const char *name, const struct stat *st);
int fsfr_lgetxattr_int_stat(const char *fpath, const char *name, const struct stat *st);
int fsfr_fgetxattr_int_stat(int fd, const char *name, const struct stat *st);
int fsfr_setxattr_int_stat(const char *fpath, const char* name, int64_t val, const struct stat *st);
int fsfr_fsetxattr_int_stat(int fd, const char* name, int64_t val, const struct stat *st);
int fsfr_lsetxattr_int_stat(const char *fpath, const char* name, int64_t val, const struct stat *st);
int fsfr_unset_attr_stat(const char *fpath, const char* name, const struct stat *st);
int fsfr_funset_attr_stat(int fd, const char* name, const struct stat *st);
int fsfr_lunset_attr_stat(const char *fpath, const char* name, const struct stat *st);
int fsfr_getxattr_int_stat64(const char *fpath, const char *name, const struct stat64 *st);
int fsfr_lgetxattr_int_stat64(const char *fpath, const char *name, const struct stat64 *st);
int fsfr_fgetxattr_int_stat64(int fd, const char *name, const struct stat64 *st);

int fsfr_base_stat(const char *path,struct stat* buf);
int fsfr_base_lstat(const char *path,struct stat* buf);
int fsfr_base_fstat(int fd,struct stat* buf);
int fsfr_base_fstatat(int dirfd, const char *path, struct stat *buf, int flags);
int fsfr_base_openat(int dirfd, const char *pathname, int flags, mode_t mode);
int fsfr_base_open(const char *pathname, int flags, mode_t mode);

int fsfr_base_chmod(const char *path, mode_t mode);
int fsfr_base_fchmod(int fd, mode_t mode);

ssize_t fsfr_base_listxattr(const char *path, char *list, size_t size);
ssize_t fsfr_base_llistxattr(const char *path, char *list, size_t size);
ssize_t fsfr_base_flistxattr(int filedes, char *list, size_t size);

extern __thread void *fsfr_statignore;

#endif /* FSFR_H_ */
