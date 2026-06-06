#include "stdlib/stdlib.hpp"

namespace cplang {

// stdlib_crypto_impl — extracted from stdlib_regex_crypto_string.cpp
void StdLib::registerCrypto(VM* vm) {
    registerFunction(vm, "md5", crypto::md5);
    registerFunction(vm, "sha1", crypto::sha1);
    registerFunction(vm, "sha256", crypto::sha256);
    registerFunction(vm, "base64Encode", crypto::base64Encode);
    registerFunction(vm, "base64Decode", crypto::base64Decode);

    registerAlias(vm, "MD5", "md5");
    registerAlias(vm, "SHA1", "sha1");
    registerAlias(vm, "SHA256", "sha256");
    registerAlias(vm, "Base64编码", "base64Encode");
    registerAlias(vm, "Base64解码", "base64Decode");
}

namespace crypto {

// ---------- MD5 ----------
static void md5Transform(UInt32 state[4], const UInt8 block[64]) {
    UInt32 a = state[0], b = state[1], c = state[2], d = state[3];
    UInt32 x[16];
    for (int i = 0; i < 16; i++) x[i] = block[i*4] | (block[i*4+1] << 8) | (block[i*4+2] << 16) | (block[i*4+3] << 24);
    #define F(x,y,z) ((x & y) | (~x & z))
    #define G(x,y,z) ((x & z) | (y & ~z))
    #define H(x,y,z) (x ^ y ^ z)
    #define I(x,y,z) (y ^ (x | ~z))
    #define ROTL(v,s) ((v << s) | (v >> (32 - s)))
    #define FF(a,b,c,d,x,s,ac) { a += F(b,c,d) + x + ac; a = ROTL(a,s); a += b; }
    #define GG(a,b,c,d,x,s,ac) { a += G(b,c,d) + x + ac; a = ROTL(a,s); a += b; }
    #define HH(a,b,c,d,x,s,ac) { a += H(b,c,d) + x + ac; a = ROTL(a,s); a += b; }
    #define II(a,b,c,d,x,s,ac) { a += I(b,c,d) + x + ac; a = ROTL(a,s); a += b; }
    FF(a,b,c,d,x[ 0], 7,0xd76aa478); FF(d,a,b,c,x[ 1],12,0xe8c7b756); FF(c,d,a,b,x[ 2],17,0x242070db); FF(b,c,d,a,x[ 3],22,0xc1bdceee);
    FF(a,b,c,d,x[ 4], 7,0xf57c0faf); FF(d,a,b,c,x[ 5],12,0x4787c62a); FF(c,d,a,b,x[ 6],17,0xa8304613); FF(b,c,d,a,x[ 7],22,0xfd469501);
    FF(a,b,c,d,x[ 8], 7,0x698098d8); FF(d,a,b,c,x[ 9],12,0x8b44f7af); FF(c,d,a,b,x[10],17,0xffff5bb1); FF(b,c,d,a,x[11],22,0x895cd7be);
    FF(a,b,c,d,x[12], 7,0x6b901122); FF(d,a,b,c,x[13],12,0xfd987193); FF(c,d,a,b,x[14],17,0xa679438e); FF(b,c,d,a,x[15],22,0x49b40821);
    GG(a,b,c,d,x[ 1], 5,0xf61e2562); GG(d,a,b,c,x[ 6], 9,0xc040b340); GG(c,d,a,b,x[11],14,0x265e5a51); GG(b,c,d,a,x[ 0],20,0xe9b6c7aa);
    GG(a,b,c,d,x[ 5], 5,0xd62f105d); GG(d,a,b,c,x[10], 9,0x02441453); GG(c,d,a,b,x[15],14,0xd8a1e681); GG(b,c,d,a,x[ 4],20,0xe7d3fbc8);
    GG(a,b,c,d,x[ 9], 5,0x21e1cde6); GG(d,a,b,c,x[14], 9,0xc33707d6); GG(c,d,a,b,x[ 3],14,0xf4d50d87); GG(b,c,d,a,x[ 8],20,0x455a14ed);
    GG(a,b,c,d,x[13], 5,0xa9e3e905); GG(d,a,b,c,x[ 2], 9,0xfcefa3f8); GG(c,d,a,b,x[ 7],14,0x676f02d9); GG(b,c,d,a,x[12],20,0x8d2a4c8a);
    HH(a,b,c,d,x[ 5], 4,0xfffa3942); HH(d,a,b,c,x[ 8],11,0x8771f681); HH(c,d,a,b,x[11],16,0x6d9d6122); HH(b,c,d,a,x[14],23,0xfde5380c);
    HH(a,b,c,d,x[ 1], 4,0xa4beea44); HH(d,a,b,c,x[ 4],11,0x4bdecfa9); HH(c,d,a,b,x[ 7],16,0xf6bb4b60); HH(b,c,d,a,x[10],23,0xbebfbc70);
    HH(a,b,c,d,x[13], 4,0x289b7ec6); HH(d,a,b,c,x[ 0],11,0xeaa127fa); HH(c,d,a,b,x[ 3],16,0xd4ef3085); HH(b,c,d,a,x[ 6],23,0x04881d05);
    HH(a,b,c,d,x[ 9], 4,0xd9d4d039); HH(d,a,b,c,x[12],11,0xe6db99e5); HH(c,d,a,b,x[15],16,0x1fa27cf8); HH(b,c,d,a,x[ 2],23,0xc4ac5665);
    II(a,b,c,d,x[ 0], 6,0xf4292244); II(d,a,b,c,x[ 7],10,0x432aff97); II(c,d,a,b,x[14],15,0xab9423a7); II(b,c,d,a,x[ 5],21,0xfc93a039);
    II(a,b,c,d,x[12], 6,0x655b59c3); II(d,a,b,c,x[ 3],10,0x8f0ccc92); II(c,d,a,b,x[10],15,0xffeff47d); II(b,c,d,a,x[ 1],21,0x85845dd1);
    II(a,b,c,d,x[ 8], 6,0x6fa87e4f); II(d,a,b,c,x[15],10,0xfe2ce6e0); II(c,d,a,b,x[ 6],15,0xa3014314); II(b,c,d,a,x[13],21,0x4e0811a1);
    II(a,b,c,d,x[ 4], 6,0xf7537e82); II(d,a,b,c,x[11],10,0xbd3af235); II(c,d,a,b,x[ 2],15,0x2ad7d2bb); II(b,c,d,a,x[ 9],21,0xeb86d391);
    #undef F
    #undef G
    #undef H
    #undef I
    #undef ROTL
    #undef FF
    #undef GG
    #undef HH
    #undef II
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
}

static std::string md5Hash(const std::string& input) {
    UInt32 state[4] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};
    UInt64 bitLen = input.size() * 8;
    size_t padLen = ((input.size() + 8) / 64 + 1) * 64;
    std::vector<UInt8> padded(padLen, 0);
    std::memcpy(padded.data(), input.data(), input.size());
    padded[input.size()] = 0x80;
    for (int i = 0; i < 8; i++) padded[padLen - 8 + i] = (bitLen >> (i * 8)) & 0xFF;
    for (size_t i = 0; i < padLen; i += 64) md5Transform(state, &padded[i]);
    std::string result;
    const char* hex = "0123456789abcdef";
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) { result += hex[(state[i] >> (j*8+4)) & 0xF]; result += hex[(state[i] >> (j*8)) & 0xF]; }
    return result;
}

// ---------- SHA-256 ----------
static UInt32 sha256K[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static void sha256Transform(UInt32 state[8], const UInt8 block[64]) {
    UInt32 w[64], s0, s1;
    for (int i = 0; i < 16; i++) w[i] = (block[i*4] << 24) | (block[i*4+1] << 16) | (block[i*4+2] << 8) | block[i*4+3];
    for (int i = 16; i < 64; i++) {
        s0 = ((w[i-15] >> 7) | (w[i-15] << 25)) ^ ((w[i-15] >> 18) | (w[i-15] << 14)) ^ (w[i-15] >> 3);
        s1 = ((w[i-2] >> 17) | (w[i-2] << 15)) ^ ((w[i-2] >> 19) | (w[i-2] << 13)) ^ (w[i-2] >> 10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
    UInt32 a = state[0], b = state[1], c = state[2], d = state[3], e = state[4], f = state[5], g = state[6], h = state[7];
    for (int i = 0; i < 64; i++) {
        UInt32 S1 = ((e >> 6) | (e << 26)) ^ ((e >> 11) | (e << 21)) ^ ((e >> 25) | (e << 7));
        UInt32 ch = (e & f) ^ (~e & g);
        UInt32 temp1 = h + S1 + ch + sha256K[i] + w[i];
        UInt32 S0 = ((a >> 2) | (a << 30)) ^ ((a >> 13) | (a << 19)) ^ ((a >> 22) | (a << 10));
        UInt32 maj = (a & b) ^ (a & c) ^ (b & c);
        UInt32 temp2 = S0 + maj;
        h = g; g = f; f = e; e = d + temp1; d = c; c = b; b = a; a = temp1 + temp2;
    }
    state[0] += a; state[1] += b; state[2] += c; state[3] += d; state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

static std::string sha256Hash(const std::string& input) {
    UInt32 state[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    UInt64 bitLen = input.size() * 8;
    size_t padLen = ((input.size() + 8) / 64 + 1) * 64;
    std::vector<UInt8> padded(padLen, 0);
    std::memcpy(padded.data(), input.data(), input.size());
    padded[input.size()] = 0x80;
    for (int i = 0; i < 8; i++) padded[padLen - 1 - i] = (bitLen >> (i * 8)) & 0xFF;
    for (size_t i = 0; i < padLen; i += 64) sha256Transform(state, &padded[i]);
    std::string result;
    const char* hex = "0123456789abcdef";
    for (int i = 0; i < 8; i++) for (int j = 3; j >= 0; j--) { result += hex[(state[i] >> (j*8+4)) & 0xF]; result += hex[(state[i] >> (j*8)) & 0xF]; }
    return result;
}

// ---------- SHA-1 ----------
static void sha1Transform(UInt32 state[5], const UInt8 block[64]) {
    UInt32 a = state[0], b = state[1], c = state[2], d = state[3], e = state[4];
    UInt32 w[80];
    for (int i = 0; i < 16; i++) w[i] = (block[i*4] << 24) | (block[i*4+1] << 16) | (block[i*4+2] << 8) | block[i*4+3];
    for (int i = 16; i < 80; i++) w[i] = ((w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16]) << 1) | ((w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16]) >> 31);
    for (int i = 0; i < 80; i++) {
        UInt32 f, k;
        if (i < 20) { f = (b & c) | (~b & d); k = 0x5a827999; }
        else if (i < 40) { f = b ^ c ^ d; k = 0x6ed9eba1; }
        else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8f1bbcdc; }
        else { f = b ^ c ^ d; k = 0xca62c1d6; }
        UInt32 temp = ((a << 5) | (a >> 27)) + f + e + k + w[i];
        e = d; d = c; c = ((b << 30) | (b >> 2)); b = a; a = temp;
    }
    state[0] += a; state[1] += b; state[2] += c; state[3] += d; state[4] += e;
}

static std::string sha1Hash(const std::string& input) {
    UInt32 state[5] = {0x67452301,0xefcdab89,0x98badcfe,0x10325476,0xc3d2e1f0};
    UInt64 bitLen = input.size() * 8;
    size_t padLen = ((input.size() + 8) / 64 + 1) * 64;
    std::vector<UInt8> padded(padLen, 0);
    std::memcpy(padded.data(), input.data(), input.size());
    padded[input.size()] = 0x80;
    for (int i = 0; i < 8; i++) padded[padLen - 1 - i] = (bitLen >> (i * 8)) & 0xFF;
    for (size_t i = 0; i < padLen; i += 64) sha1Transform(state, &padded[i]);
    std::string result;
    const char* hex = "0123456789abcdef";
    for (int i = 0; i < 5; i++) for (int j = 3; j >= 0; j--) { result += hex[(state[i] >> (j*8+4)) & 0xF]; result += hex[(state[i] >> (j*8)) & 0xF]; }
    return result;
}

// ---------- Base64 ----------
static const char* base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string base64Enc(const std::string& input) {
    std::string result;
    int i = 0, j = 0;
    UInt8 arr3[3], arr4[4];
    for (char c : input) {
        arr3[i++] = static_cast<UInt8>(c);
        if (i == 3) {
            arr4[0] = (arr3[0] & 0xfc) >> 2;
            arr4[1] = ((arr3[0] & 0x03) << 4) + ((arr3[1] & 0xf0) >> 4);
            arr4[2] = ((arr3[1] & 0x0f) << 2) + ((arr3[2] & 0xc0) >> 6);
            arr4[3] = arr3[2] & 0x3f;
            for (j = 0; j < 4; j++) result += base64Chars[arr4[j]];
            i = 0;
        }
    }
    if (i) {
        for (j = i; j < 3; j++) arr3[j] = 0;
        arr4[0] = (arr3[0] & 0xfc) >> 2;
        arr4[1] = ((arr3[0] & 0x03) << 4) + ((arr3[1] & 0xf0) >> 4);
        arr4[2] = ((arr3[1] & 0x0f) << 2) + ((arr3[2] & 0xc0) >> 6);
        for (j = 0; j < (i + 1); j++) result += base64Chars[arr4[j]];
        while (i++ < 3) result += '=';
    }
    return result;
}

static std::string base64Dec(const std::string& input) {
    std::string result;
    int i = 0, j = 0;
    int inLen = static_cast<int>(input.size());
    UInt8 arr3[3], arr4[4];
    auto idx = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    };
    for (char c : input) {
        int v = idx(c);
        if (v < 0) continue;
        arr4[i++] = static_cast<UInt8>(v);
        if (i == 4) {
            arr3[0] = (arr4[0] << 2) + ((arr4[1] & 0x30) >> 4);
            arr3[1] = ((arr4[1] & 0xf) << 4) + ((arr4[2] & 0x3c) >> 2);
            arr3[2] = ((arr4[2] & 0x3) << 6) + arr4[3];
            for (j = 0; j < 3; j++) result += static_cast<char>(arr3[j]);
            i = 0;
        }
    }
    if (i) {
        for (j = i; j < 4; j++) arr4[j] = 0;
        arr3[0] = (arr4[0] << 2) + ((arr4[1] & 0x30) >> 4);
        arr3[1] = ((arr4[1] & 0xf) << 4) + ((arr4[2] & 0x3c) >> 2);
        arr3[2] = ((arr4[2] & 0x3) << 6) + arr4[3];
        for (j = 0; j < (i - 1); j++) result += static_cast<char>(arr3[j]);
    }
    return result;
}

// ---------- Crypto API ----------
Value md5(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string input(args[0].asString()->data, args[0].asString()->length);
    return Value::String(VMString::create(md5Hash(input)));
}

Value sha1(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string input(args[0].asString()->data, args[0].asString()->length);
    return Value::String(VMString::create(sha1Hash(input)));
}

Value sha256(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string input(args[0].asString()->data, args[0].asString()->length);
    return Value::String(VMString::create(sha256Hash(input)));
}

Value base64Encode(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string input(args[0].asString()->data, args[0].asString()->length);
    return Value::String(VMString::create(base64Enc(input)));
}

Value base64Decode(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string input(args[0].asString()->data, args[0].asString()->length);
    return Value::String(VMString::create(base64Dec(input)));
}
} // namespace crypto

// ═══════════════════════════════════════════════════════════════════
//  编码转换实现
// ═══════════════════════════════════════════════════════════════════

} // namespace cplang
