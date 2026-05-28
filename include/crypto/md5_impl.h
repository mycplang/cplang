#pragma once
#include <cstddef>
// Returns 32-char hex string (caller must free with md5_free_result)
char* md5_compute(const char* data, size_t len);
void  md5_free_result(char* p);
// Returns 16 raw bytes in out buffer
void  md5_raw_bytes(const char* data, size_t len, unsigned char out[16]);
const char* md5_selftest();
