/// mini7z - minimalistic 7z.dll client written in C ///
/// multibyte filenames are not tested ///

#include "../lib/libminiarc.h"
#include "../lib/xutil.h" // ismatchwildcard

#include <stdio.h>
//#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

static int match(const char *name, int argc, const char **argv){
  for(int i=0;i<argc;i++){
	if(matchwildcard(argv[i],name))return 0;
  }
  return 1;
}

static int copy_data(void *r, void *w){
	int status;
	void *buff;
	size_t size;
	long long offset=0;
	for(;;){
		status = parchive_read_data_block(r, &buff, &size, &offset);
		if(status == ARCHIVE_EOF)return ARCHIVE_OK;
		if(status < ARCHIVE_OK){
			fprintf(stderr,"%s\n",parchive_error_string(r));
			return status;
		}
		status = parchive_write_data_block(w, buff, size, offset);
		if(status < ARCHIVE_OK){
			fprintf(stderr,"%s\n",parchive_error_string(w));
			return status;
		}
	}
}

static int extract(const char *password,const char *arc, const char *dir, int argc, const char **argv){
	if(openLibArchive())return 1;
	int flags=0;
	flags |= ARCHIVE_EXTRACT_TIME;
	flags |= ARCHIVE_EXTRACT_PERM;
	flags |= ARCHIVE_EXTRACT_ACL;
	flags |= ARCHIVE_EXTRACT_FFLAGS;

	void *a = parchive_read_new();
	parchive_read_support_format_all(a);
	parchive_read_support_filter_all(a);
	if(password&&*password){
		if(parchive_read_add_passphrase){
			parchive_read_add_passphrase(a,password);
		}else{
			fprintf(stderr,"[warn] password is not supported on this version of libarchive\n");
		}
	}

	void *ext = parchive_write_disk_new();
	parchive_write_disk_set_options(ext, flags);
	if(parchive_write_disk_set_standard_lookup){
		parchive_write_disk_set_standard_lookup(ext);
	}else{
		// possibly cmake
	}
	int status = myarchive_read_open_filename(a, arc, 65536);
	void *entry;
	for(int idx=0;;idx++){
		status = parchive_read_next_header(a, &entry);
		if(status != ARCHIVE_OK && status != ARCHIVE_EOF){
			fprintf(stderr,"%s\n",parchive_error_string(a));
			break;
		}
		if(idx==0){
			int arctype = parchive_format(a);
			const char* s_arctype = parchive_format_name(a);
			int comptype = myarchive_compression(a);
			const char* s_comptype = myarchive_compression_name(a);
			fprintf(stderr,"arctype = %s (0x%x) .\n",s_arctype,arctype);
			fprintf(stderr,"comptype = %s (0x%x) .\n",s_comptype,comptype);
			fprintf(stderr,"\n");
		}
		if(status == ARCHIVE_EOF){
			break;
		}

		const char *pathname = parchive_entry_pathname(entry);
		if(!match(pathname,argc,argv)){
			fprintf(stderr,"Extracting %s...\n",pathname);
			status = parchive_write_header(ext, entry);
			if(status < ARCHIVE_OK){
				fprintf(stderr,"%s\n",parchive_error_string(ext));
			}else if(parchive_entry_size(entry) > 0){
				status = copy_data(a, ext);
				if(status < ARCHIVE_OK){
					//fprintf(stderr, "%s\n", archive_error_string(ext));
					if(status < ARCHIVE_WARN){
						break;
					}
				}
			}
			status = parchive_write_finish_entry(ext);
			if(status < ARCHIVE_OK){
				fprintf(stderr, "%s\n", parchive_error_string(ext));
				if(status < ARCHIVE_WARN){
					break;
				}
			}
		}
	}
	parchive_write_close(ext);
	parchive_write_free(ext);
	parchive_read_close(a);
	parchive_read_free(a);
	closeLibArchive();
	return 0;
}

static int list(const char *password,const char *arc, int argc, const char **argv){
	if(openLibArchive())return 1;
	void *a = parchive_read_new();
	parchive_read_support_format_all(a);
	parchive_read_support_filter_all(a);

	if(password&&*password){
		if(parchive_read_add_passphrase){
			parchive_read_add_passphrase(a,password);
		}else{
			fprintf(stderr,"[warn] password is not supported on this version of libarchive\n");
		}
	}

	int status = myarchive_read_open_filename(a, arc, 65536);
	void *entry;
	for(int idx=0;;idx++){
		status = parchive_read_next_header(a, &entry);
		if(status != ARCHIVE_OK && status != ARCHIVE_EOF){
			fprintf(stderr,"%s\n",parchive_error_string(a));
			break;
		}
		if(idx==0){
			int arctype = parchive_format(a);
			const char* s_arctype = parchive_format_name(a);
			int comptype = myarchive_compression(a);
			const char* s_comptype = myarchive_compression_name(a);
			printf("arctype = %s (0x%x) .\n",s_arctype,arctype);
			printf("comptype = %s (0x%x) .\n",s_comptype,comptype);
			printf("\n");
			printf("Size       Time                Name              \n");
			printf("---------- ------------------- --------------------\n");
		}
		if(status == ARCHIVE_EOF){
			printf("---------- ------------------- --------------------\n");
			break;
		}

		const char *pathname = parchive_entry_pathname(entry);
		if(!match(pathname,argc,argv)){
			time_t t = parchive_entry_mtime(entry);
			struct tm tt;
#if defined(_WIN32) || (!defined(__GNUC__) && !defined(__clang__))
			localtime_s(&tt,&t);
#else
			localtime_r(&t,&tt);
#endif
			strftime(cbuf,99,"%Y-%m-%d %H:%M:%S",&tt);
			printf("%10"LLU" %s %-20s\n",parchive_entry_size(entry),cbuf,pathname);
		}
	}
	parchive_read_close(a);
	parchive_read_free(a);
	closeLibArchive();
	return 0;
}

static int add(const char *password,const char* arctype,/*int level,*/const char *arc, int argc, const char **argv){
	if(openLibArchive())return 1;
	void *a = parchive_write_new();
	if(password&&*password){
		if(parchive_write_set_passphrase){
			parchive_write_set_passphrase(a,password);
		}else{
			fprintf(stderr,"[warn] password is not supported on this version of libarchive\n");
		}
	}
	int status;
	//status = parchive_write_set_compression_none(a);
	//status = parchive_write_add_filter_none(a);
	status = parchive_write_set_format_by_name(a,arctype);
	if(status < ARCHIVE_OK){
		fprintf(stderr,"%s\n",parchive_error_string(a));
		return 0;
	}
	status = myarchive_write_open_filename(a, arc);
	if(status < ARCHIVE_OK){
		fprintf(stderr,"%s\n",parchive_error_string(a));
		return 0;
	}
	void *entry = parchive_entry_new();
	for(int i=0;i<argc;i++){
		fprintf(stderr,"Adding %s...\n",argv[i]);
		parchive_entry_clear(entry);
		struct stat st;
		stat(argv[i],&st);
		parchive_entry_copy_stat(entry,&st);
		parchive_entry_set_pathname(entry,argv[i]);
		status = parchive_write_header(a,entry);
		if(status < ARCHIVE_OK){
			fprintf(stderr,"%s\n",parchive_error_string(a));
			return 0;
		}
		FILE *f = fopen(argv[i],"rb");
		for(;;){
			int len = fread(buf,1,sizeof(buf),f);
			if(len<=0)break;
			status = parchive_write_data(a,buf,len);
			if(status < ARCHIVE_OK){
				fprintf(stderr,"%s\n",parchive_error_string(a));
				return 0;
			}
		}
		fclose(f);
	}
	parchive_entry_free(entry);
	parchive_write_close(a);
	parchive_write_free(a);
	closeLibArchive();
	return 0;
}

#ifdef STANDALONE
unsigned char buf[BUFLEN];
int main(int argc, const char **argv){
#else
int miniarc(int argc, const char **argv){
#endif
  printf(
  	"minimal libarchive client\n"
  	"Usage:\n"
  	"miniarc [xl][PASSWORD] arc.7z [extract_dir] [filespec]\n"
	"miniarc a[PASSWORD] TYPE arc.7z [filespec] (cannot handle wildcard nor directories)\n"
	"(possibly) find filespec -type f | xargs miniarc a 7zip arc.7z\n"
	"\n"
	"known TYPE list (avilability depends on libarchive.so):\n"
	"7zip ar arbsd argnu arsvr4 bsdtar cd9660 cpio gnutar iso iso9660 mtree mtree-classic newc\n"
	"odc oldtar pax paxr posix raw rpax shar shardump ustar v7tar v7 warc xar zip\n"
  	"\n"
  );
  if(argc<3)return -1;
  const char *w="*";
  
  switch(argv[1][0]){
	case 'x':{
		const char *password=argv[1]+1;
		const char *arc=argv[2];
		const char *dir="./";
		argv+=3;argc-=3;
		//if(argv[0]&&(LastByte(argv[0])=='/'||LastByte(argv[0])=='\\')){makedir(argv[0]);dir=argv[0];argv++;argc--;}
		return extract(password,arc,dir,argc?argc:1,argc?argv:&w);
	}
	case 'l':return list(argv[1]+1,argv[2],argc-3?argc-3:1,argc-3?argv+3:&w);
	case 'a':{
		if(argc<5)return -1;
		return add(argv[1]+1,argv[2],argv[3],argc-4,argv+4);
	}
	default:return -1;
  }
}
