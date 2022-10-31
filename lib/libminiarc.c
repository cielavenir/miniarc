#include "libminiarc.h"
#include <string.h>
#include <stdlib.h>

#define EXTERN
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

#if defined(_WIN32) || (!defined(__GNUC__) && !defined(__clang__))
#else
	typedef void* HMODULE;
#endif
static HMODULE hlibarc=NULL;
static bool _aliveLibArchive(HMODULE h);

const char* myarchive_compression_name(void *a){
	if(parchive_compression_name){
		return parchive_compression_name(a);
	}else if(parchive_filter_name){
		return parchive_filter_name(a,0);
	}else{
		return NULL;
	}
}

int myarchive_compression(void *a){
	if(parchive_compression){
		return parchive_compression(a);
	}else if(parchive_filter_code){
		return parchive_filter_code(a,0);
	}else{
		return -1;
	}
}

typedef struct{
	FILE *f;
	char pathname[1];
}mywriter;
static int wmyopen(void *a, void *c){
	mywriter *w = (mywriter*)c;
	w->f = fopen(w->pathname,"wb");
	if(!w->f)return ARCHIVE_FATAL;
	//return ARCHIVE_OK;
	return parchive_write_set_bytes_in_last_block(a, 1);
}
static size_t wmywrite(void *a, void *c, const void *buff, size_t length){
	mywriter *w = (mywriter*)c;
	return fwrite(buff,1,length,w->f);
}
static int wmyclose(void *a, void *c){
	mywriter *w = (mywriter*)c;
	if(w->f)fclose(w->f);
	free(w);
	return ARCHIVE_OK;
}
int myarchive_write_open_filename(void *a,const char *name){
	if(parchive_write_open_filename){
		return parchive_write_open_filename(a,name);
	}else if(parchive_write_open){
		mywriter *w = malloc(sizeof(mywriter)+strlen(name));
		strcpy(w->pathname, name);
		return parchive_write_open(a, w, wmyopen, wmywrite, wmyclose);
	}else{
		return ARCHIVE_FATAL;
	}
}

typedef struct{
	FILE *f;
	char buffer[65536];
	char pathname[1];
}myreader;
static int rmyopen(void *a, void *c){
	myreader *r = (myreader*)c;
	r->f = fopen(r->pathname,"rb");
	if(!r->f)return ARCHIVE_FATAL;
	return ARCHIVE_OK;
}
static size_t rmyread(void *a, void *c, const void **buff){
	myreader *r = (myreader*)c;
	*buff = r->buffer;
	return fread(r->buffer,1,sizeof(r->buffer),r->f);
}
static int rmyclose(void *a, void *c){
	myreader *r = (myreader*)c;
	if(r->f)fclose(r->f);
	free(r);
	return ARCHIVE_OK;
}
int myarchive_read_open_filename(void *a,const char *name,int siz){
	if(parchive_read_open_filename){
		return parchive_read_open_filename(a,name,siz);
	}else if(parchive_read_open){
		// todo myreader block size respect siz
		myreader *r = malloc(sizeof(myreader)+strlen(name));
		strcpy(r->pathname, name);
		return parchive_read_open(a, r, rmyopen, rmyread, rmyclose);
	}else{
		return ARCHIVE_FATAL;
	}
}

static void* load(HMODULE h,long long ident,const char* funcname){
	void *p = GetProcAddress(h,funcname);
	//fprintf(stderr,"%s %016llx\n",funcname,p);
	if(!p)return NULL;

#ifdef DLADDR_OK
	// if the address is not from expected module, it needs to be ignored.
	Dl_info dlinfo;
	dladdr(p, &dlinfo);
	struct stat st;
	stat(dlinfo.dli_fname, &st);
	if(ident != st.st_ino)return NULL;
#endif
	return p;
}

/// not thread-safe. ///
static HMODULE chk(const char *name){
#ifdef NODLOPEN
	return NULL;
#else
	HMODULE h=LoadLibraryA(name);
	if(!h)return NULL;

	parchive_version_number=(func_i)GetProcAddress(h,"archive_version_number");
	if(!parchive_version_number){
		FreeLibrary(h);
		return NULL;
	}

	char pathname[768];
#if 0
//defined(DL_ANDROID)
	GetModuleFileNameA(parchive_version_number,pathname,768);
#else
	GetModuleFileNameA(h,pathname,768);
#endif
	struct stat st;
	stat(pathname, &st);
	long long ident = st.st_ino;

	parchive_format=(func_ip)load(h,ident,"archive_format");
	parchive_format_name=(func_Cp)load(h,ident,"archive_format_name");
	parchive_filter_code=(func_ipi)load(h,ident,"archive_filter_code"); // optional
	parchive_filter_name=(func_Cpi)load(h,ident,"archive_filter_name"); // optional
	parchive_compression=(func_ip)load(h,ident,"archive_compression"); // optional
	parchive_compression_name=(func_Cp)load(h,ident,"archive_compression_name"); // optional
	parchive_error_string=(func_Cp)load(h,ident,"archive_error_string");
	parchive_clear_error=(func_vp)load(h,ident,"archive_clear_error");

	parchive_read_new=(func_p)load(h,ident,"archive_read_new");
	parchive_read_free=(func_ip)load(h,ident,parchive_version_number()>=3000000 ? "archive_read_free" : "archive_read_finish");
	parchive_read_close=(func_ip)load(h,ident,"archive_read_close");
	parchive_read_open=(func_ippppp)load(h,ident,"archive_read_open");
	parchive_read_support_format_all=(func_ip)load(h,ident,"archive_read_support_format_all");
	parchive_read_support_filter_all=(func_ip)load(h,ident,parchive_version_number()>=3000000 ? "archive_read_support_filter_all" : "archive_read_support_compression_all");
	parchive_read_open_filename=(func_ipCi)load(h,ident,"archive_read_open_filename");
	parchive_read_next_header=(func_ipP)load(h,ident,"archive_read_next_header");
	parchive_read_data_block=(func_ipPSL)load(h,ident,"archive_read_data_block");
	parchive_read_add_passphrase=(func_ipC)load(h,ident,"archive_read_add_passphrase"); // optional, cf https://github.com/libarchive/libarchive/commit/6c222e59f461bf61962c7de318f946147f58d29b

	parchive_write_new=(func_p)load(h,ident,"archive_write_new");
	parchive_write_free=(func_ip)load(h,ident,parchive_version_number()>=3000000 ? "archive_write_free" : "archive_write_finish");
	parchive_write_disk_new=(func_p)load(h,ident,"archive_write_disk_new");
	parchive_write_finish_entry=(func_ip)load(h,ident,"archive_write_finish_entry");
	parchive_write_close=(func_ip)load(h,ident,"archive_write_close");
	parchive_write_open=(func_ippppp)load(h,ident,"archive_write_open");
	parchive_write_open_filename=(func_ipC)load(h,ident,"archive_write_open_filename"); // optional cmake does not support
	parchive_write_disk_set_standard_lookup=(func_ip)load(h,ident,"archive_write_disk_set_standard_lookup"); // optional cmake does not support
	parchive_write_data=(func_ipps)load(h,ident,"archive_write_data");
	parchive_write_data_block=(func_ippsl)load(h,ident,"archive_write_data_block");
	parchive_write_set_format_by_name=(func_ipC)load(h,ident,"archive_write_set_format_by_name");
	parchive_write_disk_set_options=(func_ipi)load(h,ident,"archive_write_disk_set_options");
	parchive_write_set_bytes_in_last_block=(func_ipi)load(h,ident,"archive_write_set_bytes_in_last_block");
	parchive_write_header=(func_ipp)load(h,ident,"archive_write_header");
	parchive_write_set_passphrase=(func_ipC)load(h,ident,"archive_write_set_passphrase"); // optional, cf https://github.com/libarchive/libarchive/commit/6c222e59f461bf61962c7de318f946147f58d29b
	parchive_write_add_filter_none=(func_ip)load(h,ident,"archive_write_add_filter_none"); // optional
	parchive_write_set_compression_none=(func_ip)load(h,ident,"archive_write_set_compression_none"); // optional
	parchive_write_add_filter_by_name=(func_ipC)load(h,ident,"archive_write_add_filter_by_name"); // optional cmake does not support
	parchive_write_set_filter_option=(func_ipCCC)load(h,ident,"archive_write_set_filter_option"); // optional
	parchive_write_set_format_option=(func_ipCCC)load(h,ident,"archive_write_set_format_option"); // optional, cf https://github.com/libarchive/libarchive/commit/11e7a909f561bc62272d6c512bfcf1f599036fcb

	parchive_entry_new=(func_p)load(h,ident,"archive_entry_new");
	parchive_entry_clear=(func_ip)load(h,ident,"archive_entry_clear");
	parchive_entry_free=(func_ip)load(h,ident,"archive_entry_free");
	parchive_entry_copy_stat=(func_vpT)load(h,ident,"archive_entry_copy_stat");
	parchive_entry_pathname=(func_Cp)load(h,ident,"archive_entry_pathname");
	parchive_entry_strmode=(func_Cp)load(h,ident,"archive_entry_strmode");
	parchive_entry_size=(func_lp)load(h,ident,"archive_entry_size");
	parchive_entry_filetype=(func_ip)load(h,ident,"archive_entry_filetype");
	parchive_entry_perm=(func_ip)load(h,ident,"archive_entry_perm"); // optional, cf https://github.com/libarchive/libarchive/commit/50f8302a14f0c3b8225e93ebc5007db1a7b6841c
	parchive_entry_mode=(func_ip)load(h,ident,"archive_entry_mode");
	parchive_entry_mtime=(func_tp)load(h,ident,"archive_entry_mtime");
	parchive_entry_atime=(func_tp)load(h,ident,"archive_entry_atime");
	parchive_entry_ctime=(func_tp)load(h,ident,"archive_entry_ctime");
	parchive_entry_set_pathname=(func_vpC)load(h,ident,"archive_entry_set_pathname");
	parchive_entry_set_size=(func_vpl)load(h,ident,"archive_entry_set_size");
	parchive_entry_set_filetype=(func_vpi)load(h,ident,"archive_entry_set_filetype");
	parchive_entry_set_perm=(func_vpi)load(h,ident,"archive_entry_set_perm");
	parchive_entry_set_mode=(func_vpi)load(h,ident,"archive_entry_set_mode");
	parchive_entry_set_mtime=(func_vptl)load(h,ident,"archive_entry_set_mtime");
	parchive_entry_set_atime=(func_vptl)load(h,ident,"archive_entry_set_atime");
	parchive_entry_set_ctime=(func_vptl)load(h,ident,"archive_entry_set_ctime");

	//fprintf(stderr,"%016llx\n",parchive_write_open_filename);
	
	if(!_aliveLibArchive(h)){
		FreeLibrary(h);
		return NULL;
	}

	fprintf(stderr,"libarchive implementation: %s (ver. %d)\n",pathname,parchive_version_number());
	return h;
#endif
}

int openLibArchive(){
	if(aliveLibArchive())return 0;

	hlibarc=NULL;
#ifndef NODLOPEN
	//if(!hlibarc && getenv("MINIARC_IMPL"))hlibarc=chk(getenv("MINIARC_IMPL"));
#if defined(_WIN32) || (!defined(__GNUC__) && !defined(__clang__))
	if(!hlibarc)hlibarc=chk("libarchive-13.dll");
	if(!hlibarc)hlibarc=chk("libarchive.dll");
	if(!hlibarc)hlibarc=chk("C:\\windows\\system32\\archiveint.dll"); // Win64 system version does not suppport lzma :(
	// if(!hlibarc)hlibarc=chk("C:\\windows\\syswow64\\archiveint.dll"); // Win32 system version seems to use stdcall, which is not compatible.
#else
	//user libarchive
	if(!hlibarc)hlibarc=chk("/usr/local/opt/libarchive/lib/libarchive.dylib"); //Homebrew
	if(!hlibarc)hlibarc=chk("/opt/local/lib/libarchive.dylib"); //MacPorts
	if(!hlibarc)hlibarc=chk("/sw/lib/libarchive.dylib"); //Fink
	if(!hlibarc)hlibarc=chk("/home/linuxbrew/.linuxbrew/opt/libarchive/lib/libarchive.so");
	if(!hlibarc){
		char *home = getenv("HOME");
		if(home){
			char buf[256];
			sprintf(buf,"%s/.linuxbrew/opt/libarchive/lib/libarchive.so",home);
			hlibarc=chk(buf);
		}
	}
	//user cmake
	//for very fun, you can use cmake as libarchive.so!
	//but non-macOS needs cmake configured with "-DCMAKE_C_FLAGS:STRING=-fPIE -DCMAKE_CXX_FLAGS:STRING=-fPIE -DCMAKE_EXE_LINKER_FLAGS:STRING='-pie -rdynamic'" ...
	//don't worry, we will not load wrong cmake.
	//also, some symbols are not exported (stripped by ld as unused symbol). miniarc can handle it.
	//bonus: try "nm -add-dyldinfo `which cmake`" on macOS.
	//note: this WILL not be possible on glibc>=2.30 cf https://stackoverflow.com/questions/59074126/loading-executable-or-executing-a-library
	if(!hlibarc)hlibarc=chk("/Applications/CMake.app/Contents/bin/cmake");
	if(!hlibarc)hlibarc=chk("/usr/local/opt/cmake/bin/cmake"); //Homebrew
	if(!hlibarc)hlibarc=chk("/opt/local/bin/cmake"); //MacPorts
	if(!hlibarc)hlibarc=chk("/sw/bin/cmake"); //Fink
	if(!hlibarc)hlibarc=chk("/home/linuxbrew/.linuxbrew/opt/cmake/bin/cmake");
	if(!hlibarc){
		char *home = getenv("HOME");
		if(home){
			char buf[256];
			sprintf(buf,"%s/.linuxbrew/opt/cmake/bin/cmake",home);
			hlibarc=chk(buf);
		}
	}
	//system libarchive
	if(!hlibarc)hlibarc=chk("libarchive.so");
	if(!hlibarc)hlibarc=chk("libarchive.dylib");
	//system cmake
	if(!hlibarc)hlibarc=chk("/usr/bin/cmake");
	if(!hlibarc)hlibarc=chk("/usr/local/bin/cmake");
#endif
#endif
	if(!hlibarc)return 1;

	return 0; //now you can call libarchive.so.
}

static bool _aliveLibArchive(HMODULE h){
	return h
		&& parchive_version_number
		&& parchive_format
		&& parchive_format_name
		&& (parchive_filter_code||parchive_compression)
		&& (parchive_filter_name||parchive_compression_name)
		&& parchive_error_string
		&& parchive_clear_error
		&& parchive_read_new
		&& parchive_read_free
		&& parchive_read_close
		&& parchive_read_support_format_all
		&& parchive_read_support_filter_all
		&& (parchive_read_open||parchive_read_open_filename)
		&& parchive_read_next_header
		&& parchive_read_data_block
		// && parchive_read_add_passphrase
		&& parchive_write_new
		&& parchive_write_free
		&& parchive_write_disk_new
		&& parchive_write_finish_entry
		&& parchive_write_close
		&& (parchive_write_open||parchive_write_open_filename)
		// && parchive_write_disk_set_standard_lookup
		&& parchive_write_data
		&& parchive_write_data_block
		&& parchive_write_set_format_by_name
		&& parchive_write_disk_set_options
		&& parchive_write_set_bytes_in_last_block
		&& parchive_write_header
		// && parchive_write_set_passphrase
		// && parchive_write_add_filter_none
		// && parchive_write_set_compression_none
		// && parchive_write_add_filter_by_name
		// && parchive_write_set_filter_option
		// && parchive_write_set_format_option
		&& parchive_entry_new
		&& parchive_entry_clear
		&& parchive_entry_free
		&& parchive_entry_copy_stat
		&& parchive_entry_pathname
		&& parchive_entry_strmode
		&& parchive_entry_size
		&& parchive_entry_filetype
		// && parchive_entry_perm
		&& parchive_entry_mode
		&& parchive_entry_mtime
		&& parchive_entry_atime
		&& parchive_entry_ctime
		&& parchive_entry_set_pathname
		&& parchive_entry_set_size
		&& parchive_entry_set_filetype
		&& parchive_entry_set_perm
		&& parchive_entry_set_mode
		&& parchive_entry_set_mtime
		&& parchive_entry_set_atime
		&& parchive_entry_set_ctime
	;
}

bool aliveLibArchive(){return _aliveLibArchive(hlibarc);}

int getLibArchiveFileName(char* path, int siz){
	if(!aliveLibArchive())return 0;
	return GetModuleFileNameA(hlibarc,path,siz);
}

int closeLibArchive(){
	if(!aliveLibArchive())return 1;
#ifndef NODLOPEN
	FreeLibrary(hlibarc);
#endif
	hlibarc=NULL;
	return 0;
}
