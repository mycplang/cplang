// _miniz_impl.h - minimal zlib-compatible compress/uncompress using Windows API
// Single-header drop-in replacement for miniz compress/decompress
#pragma once
#include <windows.h>
#include <compressapi.h>
#include <vector>
#include <cstring>

#ifndef COMPRESS_ALGORITHM_MSZIP
#define COMPRESS_ALGORITHM_MSZIP 0x00000002
#endif

namespace cplang_miniz {

inline unsigned long compressBound(unsigned long source_len) {
    // Conservative upper bound for deflate
    return source_len + (source_len >> 12) + (source_len >> 14) + (source_len >> 25) + 13;
}

inline int compress2(unsigned char* pDest, unsigned long* pDest_len,
                     const unsigned char* pSource, unsigned long source_len, int level) {
    COMPRESSOR_HANDLE hCompressor = nullptr;
    if (!CreateCompressor(COMPRESS_ALGORITHM_MSZIP, nullptr, &hCompressor)) {
        // Fallback: try XPRESS
        if (!CreateCompressor(COMPRESS_ALGORITHM_XPRESS, nullptr, &hCompressor)) {
            return -1;  // MZ_PARAM_ERROR
        }
    }
    SIZE_T destSize = *pDest_len;
    BOOL ok = Compress(hCompressor, (void*)pSource, source_len, pDest, destSize, &destSize);
    CloseCompressor(hCompressor);
    if (!ok) return -1;
    *pDest_len = (unsigned long)destSize;
    return 0;  // MZ_OK
}

inline int compress(unsigned char* pDest, unsigned long* pDest_len,
                    const unsigned char* pSource, unsigned long source_len) {
    return compress2(pDest, pDest_len, pSource, source_len, 6);  // default level
}

inline int uncompress(unsigned char* pDest, unsigned long* pDest_len,
                      const unsigned char* pSource, unsigned long source_len) {
    DECOMPRESSOR_HANDLE hDecompressor = nullptr;
    if (!CreateDecompressor(COMPRESS_ALGORITHM_MSZIP, nullptr, &hDecompressor)) {
        if (!CreateDecompressor(COMPRESS_ALGORITHM_XPRESS, nullptr, &hDecompressor)) {
            return -1;
        }
    }
    SIZE_T destSize = *pDest_len;
    BOOL ok = Decompress(hDecompressor, (void*)pSource, source_len, pDest, destSize, &destSize);
    CloseDecompressor(hDecompressor);
    if (!ok) return -1;
    *pDest_len = (unsigned long)destSize;
    return 0;
}

} // namespace cplang_miniz
