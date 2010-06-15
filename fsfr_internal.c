/*
 * fsfr_internal.c
 *
 * Copyright (c) 2010, Tyler Larson <devel@tlarson.com>
 *
 * This software is licensed under the terms of the MIT License.
 * See the included file "LICENSE" for more information.
 *
 */

#include "fsfr.h"

#include <dlfcn.h>
#include <stdlib.h>

/****************************************************************
 *  Internal helper functions
 ****************************************************************/

// fetch the "real" syscall for the provided function
void * fsfr_dlnext(const char *sym) {
	//fprintf(stderr,"LOADING %s\n",sym);
	void * rtn = dlsym(RTLD_NEXT,sym);
	//fprintf(stderr,"SYM %s: %llu\n",sym,(long long unsigned int)rtn);
	if (NULL==rtn) {
		const char *err = dlerror();
		if (err) {
			fprintf(stderr,"Failed loading %s: %s\n",sym,dlerror());
		} else {
			//retry, but with "default" symbol instead
			rtn = dlsym(RTLD_DEFAULT,sym);
			if (NULL==rtn) fprintf(stderr,"Failed loading %s: (unknown error)\n",sym);
		}
		// not sure what would be a good generic error, since dlsym doesn't set errno
		if (NULL==rtn) {
			//fprintf(stderr,"Failed to load %s.",sym);
			errno = EINVAL;
		}
	}
	return rtn;
}


int fsfr_proxygetxattr_int(int64_t inode, const char *name)
{
	char *proxy_dir = getenv("FSFR_PROXY_DIR");
	if (!proxy_dir) return -1;
	char fpath[PATH_MAX];
	snprintf(fpath,PATH_MAX,"%s/%lli.fsfr",proxy_dir,(long long int) inode);
	return fsfr_getxattr_int(fpath,name);
}
int fsfr_proxysetxattr_int(int64_t inode, const char *name, int64_t val)
{
	char *proxy_dir = getenv("FSFR_PROXY_DIR");
	if (!proxy_dir) return 0;
	// fails silently if FSFR_PROXY_DIR is unset, as this extension
	// is optional. This is a judgement call; if you don't like it, change
	// 0 to -1 above.
	char fpath[PATH_MAX];
	snprintf(fpath,PATH_MAX,"%s/%lli.fsfr",proxy_dir,(long long int) inode);
	struct stat st;
	if (fsfr_base_stat(fpath,&st)) {
		fsfr_base_open(fpath,O_WRONLY|O_CREAT|O_TRUNC,0644);
	}
	return fsfr_setxattr_int(fpath,name,val);
}
int fsfr_proxyunsetxattr_int(int64_t inode, const char *name)
{
	char *proxy_dir = getenv("FSFR_PROXY_DIR");
	if (!proxy_dir) return 0;
	// Also fails silently. See note in fsfr_proxysetxattr_int
	char fpath[PATH_MAX];
	snprintf(fpath,PATH_MAX,"%s/%lli.fsfr",proxy_dir,(long long int) inode);
	return fsfr_unset_attr(fpath,name);
}

// fsfr_Xgetxattr_int: Gets an integer stored in xattrs
// stored explicitly as int64 for cross-architecture safety and future-proofing
int fsfr_getxattr_int(const char *fpath, const char *name)
{
	int64_t rtn = -1;
	if (getxattr(fpath,name,&rtn,sizeof(rtn))>0) return (int) rtn;
	return -1;
}
int fsfr_lgetxattr_int(const char *fpath, const char *name)
{
	int64_t rtn = -1;
	if (lgetxattr(fpath,name,&rtn,sizeof(rtn))>0) return (int) rtn;
	return -1;
}
int fsfr_fgetxattr_int(int fd, const char *name)
{
	int64_t rtn = -1;
	if (fgetxattr(fd,name,&rtn,sizeof(rtn))>0) return (int) rtn;
	return -1;
}
int fsfr_setxattr_int(const char *fpath, const char* name, int64_t val)
{
	//fprintf(stderr,"%s: 0x%llx 0%llo\n",name,val,val);
	return setxattr(fpath,name,&val,sizeof(val),0)?-errno:0;
}
int fsfr_fsetxattr_int(int fd, const char* name, int64_t val)
{
	//fprintf(stderr,"%s: 0x%llx 0%llo\n",name,val,val);
	return fsetxattr(fd,name,&val,sizeof(val),0)?-errno:0;
}
int fsfr_lsetxattr_int(const char *fpath, const char* name, int64_t val)
{
	return lsetxattr(fpath,name,&val,sizeof(val),0)?-errno:0;
}
int fsfr_unset_attr(const char *fpath, const char* name)
{
	return removexattr(fpath,name)?-errno:0;
}
int fsfr_funset_attr(int fd, const char* name)
{
	return fremovexattr(fd,name)?-errno:0;
}
int fsfr_lunset_attr(const char *fpath, const char* name)
{
	return lremovexattr(fpath,name)?-errno:0;
}


int fsfr_getxattr_int_stat(const char *fpath, const char *name, const struct stat *st)
{
	if (S_ISLNK(st->st_mode)) return fsfr_proxygetxattr_int(st->st_ino,name);
	return fsfr_getxattr_int(fpath,name);
}
int fsfr_lgetxattr_int_stat(const char *fpath, const char *name, const struct stat *st)
{
	if (S_ISLNK(st->st_mode)) return fsfr_proxygetxattr_int(st->st_ino,name);
	return fsfr_lgetxattr_int(fpath,name);
}
int fsfr_fgetxattr_int_stat(int fd, const char *name, const struct stat *st)
{
	if (S_ISLNK(st->st_mode)) return fsfr_proxygetxattr_int(st->st_ino,name);
	return fsfr_fgetxattr_int(fd,name);
}
int fsfr_setxattr_int_stat(const char *fpath, const char* name, int64_t val, const struct stat *st)
{
	if (S_ISLNK(st->st_mode)) return fsfr_proxysetxattr_int(st->st_ino,name,val);
	return fsfr_setxattr_int(fpath,name,val);
}
int fsfr_fsetxattr_int_stat(int fd, const char* name, int64_t val, const struct stat *st)
{
	if (S_ISLNK(st->st_mode)) return fsfr_proxysetxattr_int(st->st_ino,name,val);
	return fsfr_fsetxattr_int(fd,name,val);
}
int fsfr_lsetxattr_int_stat(const char *fpath, const char* name, int64_t val, const struct stat *st)
{
	if (S_ISLNK(st->st_mode)) return fsfr_proxysetxattr_int(st->st_ino,name,val);
	return fsfr_lsetxattr_int(fpath,name,val);
}
int fsfr_unset_attr_stat(const char *fpath, const char* name, const struct stat *st)
{
	if (S_ISLNK(st->st_mode)) return fsfr_proxyunsetxattr_int(st->st_ino,name);
	return fsfr_unset_attr(fpath,name);
}
int fsfr_funset_attr_stat(int fd, const char* name, const struct stat *st)
{
	if (S_ISLNK(st->st_mode)) return fsfr_proxyunsetxattr_int(st->st_ino,name);
	return fsfr_funset_attr(fd,name);
}
int fsfr_lunset_attr_stat(const char *fpath, const char* name, const struct stat *st)
{
	if (S_ISLNK(st->st_mode)) return fsfr_proxyunsetxattr_int(st->st_ino,name);
	return fsfr_lunset_attr(fpath,name);
}
int fsfr_getxattr_int_stat64(const char *fpath, const char *name, const struct stat64 *st)
{
	if (S_ISLNK(st->st_mode)) return fsfr_proxygetxattr_int(st->st_ino,name);
	return fsfr_getxattr_int(fpath,name);
}
int fsfr_lgetxattr_int_stat64(const char *fpath, const char *name, const struct stat64 *st)
{
	if (S_ISLNK(st->st_mode)) return fsfr_proxygetxattr_int(st->st_ino,name);
	return fsfr_lgetxattr_int(fpath,name);
}
int fsfr_fgetxattr_int_stat64(int fd, const char *name, const struct stat64 *st)
{
	if (S_ISLNK(st->st_mode)) return fsfr_proxygetxattr_int(st->st_ino,name);
	return fsfr_fgetxattr_int(fd,name);
}
