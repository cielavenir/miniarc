## miniarc

minimalistic libarchive client.

as a very special feature, this can load `cmake` as libarchive.so if it is compiled using `-DCMAKE_EXE_LINKER_FLAGS:STRING=-Wl,-E`.

but in this way dlsym-ed address can be from unexpected module. we check it using dladdr. 
