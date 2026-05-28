// md5_impl.cpp - isolated MD5 implementation (no CP headers)
#include <vector>
#include <string>
#include <cstring>
#include <stdint.h>
#include <vector>

static const uint32_t md5_T[64] = {
    0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,
    0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
    0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,
    0x6b901122,0xfd987193,0xa679438e,0x49b40821,
    0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,
    0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
    0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,
    0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
    0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,
    0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
    0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,
    0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
    0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,
    0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
    0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,
    0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391
};

static const unsigned md5_S[64] = {
    7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22,
    5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
    4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,
    6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21
};

// Returns 32-char hex string (caller must free with md5_free_result)
char* md5_compute(const char* data, size_t len) {
    uint32_t h0=0x67452301,h1=0xefcdab89,h2=0x98badcfe,h3=0x10325476;
    uint64_t bl = (uint64_t)len*8;
    std::vector<unsigned char> msg(data, data+len);
    msg.push_back(0x80);
    while ((msg.size()%64)!=56) msg.push_back(0);
    for (int i=0;i<8;i++) msg.push_back((unsigned char)(bl>>(i*8)));
    for (size_t off=0;off<msg.size();off+=64) {
        uint32_t w[16];
        for (int i=0;i<16;i++)
            w[i]=(uint32_t)msg[off+i*4]|((uint32_t)msg[off+i*4+1]<<8)|((uint32_t)msg[off+i*4+2]<<16)|((uint32_t)msg[off+i*4+3]<<24);
        uint32_t a=h0,b=h1,c=h2,d=h3;
        for (int i=0;i<64;i++) {
            uint32_t f; int g;
            if(i<16){f=(b&c)|(~b&d);g=i;}
            else if(i<32){f=(d&b)|(~d&c);g=(5*i+1)%16;}
            else if(i<48){f=b^c^d;g=(3*i+5)%16;}
            else{f=c^(b|~d);g=(7*i)%16;}
            uint32_t temp = d;
            d = c;
            c = b;
            uint32_t sum = a + f + md5_T[i] + w[g];
            b = b + ((sum << md5_S[i]) | (sum >> (32u - md5_S[i])));
            a = temp;
        }
        h0+=a;h1+=b;h2+=c;h3+=d;
    }
    // Hex output
    char* result = new char[33];
    const char* hex = "0123456789abcdef";
    for (int i = 0; i < 4; i++) {
        result[i*2]   = hex[(h0>>(i*8+4))&0xF];
        result[i*2+1] = hex[(h0>>(i*8))&0xF];
    }
    for (int i = 0; i < 4; i++) {
        result[8+i*2]   = hex[(h1>>(i*8+4))&0xF];
        result[8+i*2+1] = hex[(h1>>(i*8))&0xF];
    }
    for (int i = 0; i < 4; i++) {
        result[16+i*2]   = hex[(h2>>(i*8+4))&0xF];
        result[16+i*2+1] = hex[(h2>>(i*8))&0xF];
    }
    for (int i = 0; i < 4; i++) {
        result[24+i*2]   = hex[(h3>>(i*8+4))&0xF];
        result[24+i*2+1] = hex[(h3>>(i*8))&0xF];
    }
    result[32] = 0;
    return result;
}

// Returns 16 raw bytes as std::string (for HMAC etc.)
#ifdef _WIN32
__declspec(dllexport) void md5_raw_bytes(const char* data, size_t len, unsigned char out[16]) {
#else
void md5_raw_bytes(const char* data, size_t len, unsigned char out[16]) {
#endif
    // Same algorithm as md5_compute, but returns raw bytes
    static const uint32_t T[64] = {
        0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,
        0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
        0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,
        0x6b901122,0xfd987193,0xa679438e,0x49b40821,
        0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,
        0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
        0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,
        0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
        0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,
        0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
        0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,
        0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
        0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,
        0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
        0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,
        0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391
    };
    static const unsigned S[64] = {
        7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22,
        5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
        4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,
        6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21
    };
    uint32_t h0=0x67452301,h1=0xefcdab89,h2=0x98badcfe,h3=0x10325476;
    uint64_t bl = (uint64_t)len*8;
    std::vector<unsigned char> msg(data, data+len);
    msg.push_back(0x80);
    while ((msg.size()%64)!=56) msg.push_back(0);
    for (int i=0;i<8;i++) msg.push_back((unsigned char)(bl>>(i*8)));
    for (size_t off=0;off<msg.size();off+=64) {
        uint32_t w[16];
        for (int i=0;i<16;i++)
            w[i]=(uint32_t)msg[off+i*4]|((uint32_t)msg[off+i*4+1]<<8)|((uint32_t)msg[off+i*4+2]<<16)|((uint32_t)msg[off+i*4+3]<<24);
        uint32_t a=h0,b=h1,c=h2,d=h3;
        for (int i=0;i<64;i++) {
            uint32_t f; int g;
            if(i<16){f=(b&c)|(~b&d);g=i;}
            else if(i<32){f=(d&b)|(~d&c);g=(5*i+1)%16;}
            else if(i<48){f=b^c^d;g=(3*i+5)%16;}
            else{f=c^(b|~d);g=(7*i)%16;}
            uint32_t temp = d; d = c; c = b;
            uint32_t sum = a + f + T[i] + w[g];
            b = b + ((sum << S[i]) | (sum >> (32u - S[i])));
            a = temp;
        }
        h0+=a;h1+=b;h2+=c;h3+=d;
    }
    // Store in little-endian
    for (int i=0;i<4;i++) { out[i]=(unsigned char)(h0>>(i*8)); out[4+i]=(unsigned char)(h1>>(i*8)); out[8+i]=(unsigned char)(h2>>(i*8)); out[12+i]=(unsigned char)(h3>>(i*8)); }
}

void md5_free_result(char* p) {
    delete[] p;
}

// Self-test entry point
const char* md5_selftest() {
    char* h = md5_compute("", 0);
    if (strcmp(h, "d41d8cd98f00b204e9800998ecf8427e") != 0) {
        delete[] h;
        return "FAIL: empty string";
    }
    delete[] h;
    h = md5_compute("hello", 5);
    if (strcmp(h, "5d41402abc4b2a76b9719d911017c592") != 0) {
        delete[] h;
        return "FAIL: hello";
    }
    delete[] h;
    return "OK";
}
