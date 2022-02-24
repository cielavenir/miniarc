#if 0
libarchive_musl's archive_entry_copy_stat is safe to call from test_glibc.

how to check:

on alpine container, do:

```
apk add zlib-static
apk add acl-static
apk add bzip2-static
apk add expat-static
apk add lz4-static
apk add zstd-static
apk add openssl-libs-static
apk add libarchive-static
```

on host, do:

```
CONTAINER_NAME=xxx
docker cp $CONTAINER_NAME:/lib/libz.a ./
docker cp $CONTAINER_NAME:/lib/libacl.a ./
docker cp $CONTAINER_NAME:/usr/lib/libbz2.a ./
docker cp $CONTAINER_NAME:/usr/lib/libexpat.a ./
docker cp $CONTAINER_NAME:/usr/lib/liblz4.a ./
docker cp $CONTAINER_NAME:/usr/lib/libzstd.a ./
docker cp $CONTAINER_NAME:/usr/lib/libssl.a ./
docker cp $CONTAINER_NAME:/usr/lib/libcrypto.a ./
docker cp $CONTAINER_NAME:/usr/lib/libarchive.a ./

gcc -O2 test.c *.a -pthread -ldl -llzma
./a.out test.zip compat.h
```
#endif

#include <archive.h>
#include <archive_entry.h>

static char buf[65536];
int main(int argc, char **argv){
    void *a = archive_write_new();
    int status = archive_write_set_format_zip(a);
	if(status < ARCHIVE_OK){
		fprintf(stderr,"%s\n",archive_error_string(a));
		return 0;
	}
	status = archive_write_open_filename(a, argv[1]);
	if(status < ARCHIVE_OK){
		fprintf(stderr,"%s\n",archive_error_string(a));
		return 0;
	}
	void *entry = archive_entry_new();
	for(int i=2;i<argc;i++){
		fprintf(stderr,"Adding %s...\n",argv[i]);
		archive_entry_clear(entry);
		struct stat st;
		stat(argv[i],&st);
#if 0
//defined(_WIN32) || (!defined(__GNUC__) && !defined(__clang__))
		// there are quite many CRT dialects and passing struct stat to 3rdparty library could be unstable.
		archive_entry_set_size(entry,st.st_size);
		archive_entry_set_mtime(entry,st.st_mtime,0);
		archive_entry_set_ctime(entry,st.st_ctime,0);
		archive_entry_set_atime(entry,st.st_atime,0);
		archive_entry_set_mode(entry,st.st_mode); // seems required as not defaulting to S_IFREG.
#else
		archive_entry_copy_stat(entry,&st);
#endif
		archive_entry_set_pathname(entry,argv[i]);
		status = archive_write_header(a,entry);
		if(status < ARCHIVE_OK){
			fprintf(stderr,"%s\n",archive_error_string(a));
			return 0;
		}
		FILE *f = fopen(argv[i],"rb");
		for(;;){
			int len = fread(buf,1,sizeof(buf),f);
			if(len<=0)break;
			status = archive_write_data(a,buf,len);
			if(status < ARCHIVE_OK){
				fprintf(stderr,"%s\n",archive_error_string(a));
				return 0;
			}
		}
		fclose(f);
	}
	archive_entry_free(entry);
	archive_write_close(a);
	archive_write_free(a);
    return 0;
}
