#pragma once
// Linux compatibility stubs for Windows APIs
#ifdef __linux__
#include <dlfcn.h>
#define HMODULE void*
inline void* LoadLibraryA(const char* path) { return dlopen(path, RTLD_LAZY); }
inline void FreeLibrary(void* h) { if(h) dlclose(h); }
inline void* GetProcAddress(void* h, const char* name) { return dlsym(h, name); }
#endif
