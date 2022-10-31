#ifndef _LIBMINIARC_H_
#define _LIBMINIARC_H_

#ifdef __cplusplus
extern "C"{
#endif

#include "../compat.h"
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>

#define	ARCHIVE_EOF	  1	/* Found end of archive. */
#define	ARCHIVE_OK	  0	/* Operation was successful. */
#define	ARCHIVE_RETRY	(-10)	/* Retry might succeed. */
#define	ARCHIVE_WARN	(-20)	/* Partial success. */
/* For example, if write_header "fails", then you can't push data. */
#define	ARCHIVE_FAILED	(-25)	/* Current operation cannot complete. */
/* But if write_header is "fatal," then this archive is dead and useless. */
#define	ARCHIVE_FATAL	(-30)	/* No more operations are possible. */

/* Default: Do not try to set owner/group. */
#define	ARCHIVE_EXTRACT_OWNER			(0x0001)
/* Default: Do obey umask, do not restore SUID/SGID/SVTX bits. */
#define	ARCHIVE_EXTRACT_PERM			(0x0002)
/* Default: Do not restore mtime/atime. */
#define	ARCHIVE_EXTRACT_TIME			(0x0004)
/* Default: Replace existing files. */
#define	ARCHIVE_EXTRACT_NO_OVERWRITE 		(0x0008)
/* Default: Try create first, unlink only if create fails with EEXIST. */
#define	ARCHIVE_EXTRACT_UNLINK			(0x0010)
/* Default: Do not restore ACLs. */
#define	ARCHIVE_EXTRACT_ACL			(0x0020)
/* Default: Do not restore fflags. */
#define	ARCHIVE_EXTRACT_FFLAGS			(0x0040)
/* Default: Do not restore xattrs. */
#define	ARCHIVE_EXTRACT_XATTR 			(0x0080)
/* Default: Do not try to guard against extracts redirected by symlinks. */
/* Note: With ARCHIVE_EXTRACT_UNLINK, will remove any intermediate symlink. */
#define	ARCHIVE_EXTRACT_SECURE_SYMLINKS		(0x0100)
/* Default: Do not reject entries with '..' as path elements. */
#define	ARCHIVE_EXTRACT_SECURE_NODOTDOT		(0x0200)
/* Default: Create parent directories as needed. */
#define	ARCHIVE_EXTRACT_NO_AUTODIR		(0x0400)
/* Default: Overwrite files, even if one on disk is newer. */
#define	ARCHIVE_EXTRACT_NO_OVERWRITE_NEWER	(0x0800)
/* Detect blocks of 0 and write holes instead. */
#define	ARCHIVE_EXTRACT_SPARSE			(0x1000)
/* Default: Do not restore Mac extended metadata. */
/* This has no effect except on Mac OS. */
#define	ARCHIVE_EXTRACT_MAC_METADATA		(0x2000)
/* Default: Use HFS+ compression if it was compressed. */
/* This has no effect except on Mac OS v10.6 or later. */
#define	ARCHIVE_EXTRACT_NO_HFS_COMPRESSION	(0x4000)
/* Default: Do not use HFS+ compression if it was not compressed. */
/* This has no effect except on Mac OS v10.6 or later. */
#define	ARCHIVE_EXTRACT_HFS_COMPRESSION_FORCED	(0x8000)
/* Default: Do not reject entries with absolute paths */
#define ARCHIVE_EXTRACT_SECURE_NOABSOLUTEPATHS (0x10000)
/* Default: Do not clear no-change flags when unlinking object */
#define	ARCHIVE_EXTRACT_CLEAR_NOCHANGE_FFLAGS	(0x20000)
/* Default: Do not extract atomically (using rename) */
#define	ARCHIVE_EXTRACT_SAFE_WRITES		(0x40000)

typedef int (*func_i)(void);
typedef void* (*func_p)(void);
typedef int (*func_ip)(void* a);
typedef int (*func_ipi)(void* a,int b);
typedef int (*func_ipCi)(void* a,const char *b,int c);
typedef int (*func_ipP)(void* a,void **b);
typedef int (*func_ipp)(void* a,void *b);
typedef const char* (*func_Cp)(void* a);
typedef const char* (*func_Cpi)(void* a,int b);
typedef long long (*func_lp)(void* a);
typedef time_t (*func_tp)(void* a);
typedef int (*func_ipC)(void* a,const char *b);
typedef int (*func_ipPSL)(void* a,void **b,size_t *c,long long *d);
typedef int (*func_ippsl)(void* a,void *b,size_t c,long long d);
typedef int (*func_ipps)(void* a,void *b,size_t c);
typedef void (*func_vpT)(void* a,struct stat *b);
typedef int (*func_ippppp)(void* a,void *b,void *c,void *d,void *e);
typedef int (*func_ipCCC)(void* a,const char *b,const char *c,const char *d);
typedef void (*func_vp)(void* a);
typedef void (*func_vpC)(void* a,const char* b);
typedef void (*func_vpi)(void* a,int b);
typedef void (*func_vpl)(void* a,long long b);
typedef void (*func_vptl)(void* a,time_t b,long c);

#define EXTERN extern
EXTERN func_i parchive_version_number, parchive_check_magic;
EXTERN func_p parchive_read_new, parchive_write_new, parchive_write_disk_new, parchive_entry_new;
EXTERN func_ip parchive_format, parchive_compression;
EXTERN func_ip parchive_entry_filetype;
EXTERN func_ip parchive_read_support_format_all, parchive_read_support_filter_all, parchive_entry_clear, parchive_entry_free;
EXTERN func_ip parchive_read_close, parchive_read_free;
EXTERN func_ip parchive_write_close, parchive_write_free, parchive_write_finish_entry, parchive_write_disk_set_standard_lookup;
EXTERN func_ip parchive_write_set_compression_none;
EXTERN func_ip parchive_write_add_filter_none;
EXTERN func_ip parchive_entry_filetype, parchive_entry_perm, parchive_entry_mode;
EXTERN func_ipi parchive_filter_code;
EXTERN func_ipi parchive_write_disk_set_options, parchive_write_set_bytes_in_last_block;
EXTERN func_ipCi parchive_read_open_filename;
EXTERN func_ipP parchive_read_next_header;
EXTERN func_ipp parchive_write_header;
EXTERN func_Cp parchive_error_string, parchive_entry_pathname, parchive_entry_strmode;
EXTERN func_Cp parchive_format_name, parchive_compression_name;
EXTERN func_Cpi parchive_filter_name;
EXTERN func_lp parchive_entry_size;
EXTERN func_tp parchive_entry_mtime, parchive_entry_atime, parchive_entry_ctime, parchive_entry_birthtime;
EXTERN func_ipC parchive_read_add_passphrase;
EXTERN func_ipC parchive_write_set_passphrase, parchive_write_set_format_by_name, parchive_write_add_filter_by_name, parchive_write_open_filename;
EXTERN func_ipPSL parchive_read_data_block;
EXTERN func_ippsl parchive_write_data_block;
EXTERN func_ipps parchive_write_data;
EXTERN func_vpT parchive_entry_copy_stat;
EXTERN func_ippppp parchive_write_open, parchive_read_open;
EXTERN func_ipCCC parchive_write_set_format_option, parchive_write_set_filter_option;
EXTERN func_vp parchive_clear_error;
EXTERN func_vpC parchive_entry_set_pathname;
EXTERN func_vpi parchive_entry_set_filetype, parchive_entry_set_perm, parchive_entry_set_mode;
EXTERN func_vpl parchive_entry_set_size;
EXTERN func_vptl parchive_entry_set_mtime, parchive_entry_set_atime, parchive_entry_set_ctime;
#undef EXTERN

int openLibArchive();
bool aliveLibArchive();
int getLibArchiveFileName(char* path, int siz);
int closeLibArchive();
const char* myarchive_compression_name(void *a);
int myarchive_compression(void *a);
int myarchive_write_open_filename(void *a,const char *name);
int myarchive_read_open_filename(void *a,const char *name,int siz);

#ifdef __cplusplus
}
#endif

#endif
