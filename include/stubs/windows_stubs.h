#pragma once
// Linux stubs for Windows APIs used in JIT module
#ifdef __linux__
#define HMODULE void*
inline void* LoadLibraryA(const char*) { return nullptr; }
inline void FreeLibrary(void*) {}
inline void* GetProcAddress(void*, const char*) { return nullptr; }
#endif
