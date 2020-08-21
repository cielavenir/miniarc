## miniarc

minimalistic libarchive client.

## motivation

1. this can load `cmake` as libarchive.so if it is compiled using PIE and export-dynamic (but in this way dlsym-ed address can be from unexpected module. we check it using dladdr).
2. Windows10 has libarchive as archiveint.dll (32bit version uses stdcall and incompatible. 64bit version ok). I wanted to dig the capability.
