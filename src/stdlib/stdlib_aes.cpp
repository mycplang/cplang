#include "stdlib/stdlib.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace cplang {

// AES-128/256-CBC encryption + cryptographically secure random
// #include'd from stdlib.cpp, already inside namespace cplang

namespace aes_ns {

// ── AES S-box and inverse ──
static const unsigned char sbox[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

static const unsigned char inv_sbox[256] = {
    0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
    0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
    0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
    0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
    0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
    0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
    0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
    0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
    0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
    0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
    0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
    0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
    0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
    0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
    0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
    0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d
};

// Rcon for key expansion
static const unsigned char Rcon[15] = {
    0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1b,0x36,0x6c,0xd8,0xab,0x4d,0x9a
};

// Galois field multiply by 2 (for MixColumns)
static inline unsigned char gmul2(unsigned char v) {
    return (v & 0x80) ? (unsigned char)((v << 1) ^ 0x1b) : (unsigned char)(v << 1);
}
static inline unsigned char gmul3(unsigned char v) { return gmul2(v) ^ v; }
static inline unsigned char gmul(unsigned char a, unsigned char b) {
    unsigned char p = 0;
    for (int i = 0; i < 8; i++) {
        if (b & 1) p ^= a;
        bool hi = (a & 0x80) != 0;
        a = (unsigned char)(a << 1);
        if (hi) a ^= 0x1b;
        b >>= 1;
    }
    return p;
}

// ── Key expansion ──
// Expands key into roundKeys (must be pre-allocated: 176 bytes for AES-128, 240 for AES-256)
static int expandKey(const std::vector<unsigned char>& key, std::vector<unsigned char>& roundKeys) {
    size_t Nk = key.size() / 4; // 4 for AES-128, 8 for AES-256
    int Nr = (Nk == 4) ? 10 : (Nk == 8) ? 14 : -1; // rounds
    if (Nr < 0) return -1;
    
    size_t expandedSize = 16 * (Nr + 1);
    roundKeys.resize(expandedSize);
    memcpy(roundKeys.data(), key.data(), key.size());
    
    size_t i = Nk;
    while (i < 4 * (Nr + 1)) {
        unsigned char temp[4];
        memcpy(temp, roundKeys.data() + (i-1)*4, 4);
        if (i % Nk == 0) {
            // RotWord
            unsigned char t = temp[0]; temp[0] = temp[1]; temp[1] = temp[2]; temp[2] = temp[3]; temp[3] = t;
            // SubWord
            for (int j=0; j<4; j++) temp[j] = sbox[temp[j]];
            temp[0] ^= Rcon[i/Nk - 1];
        } else if (Nk > 6 && i % Nk == 4) {
            // SubWord only for AES-256
            for (int j=0; j<4; j++) temp[j] = sbox[temp[j]];
        }
        for (int j=0; j<4; j++) {
            roundKeys[i*4 + j] = roundKeys[(i-Nk)*4 + j] ^ temp[j];
        }
        i++;
    }
    return Nr;
}

// ── Core AES operations ──
static void addRoundKey(unsigned char state[16], const unsigned char* rk) {
    for (int i=0; i<16; i++) state[i] ^= rk[i];
}
static void subBytes(unsigned char state[16]) {
    for (int i=0; i<16; i++) state[i] = sbox[state[i]];
}
static void invSubBytes(unsigned char state[16]) {
    for (int i=0; i<16; i++) state[i] = inv_sbox[state[i]];
}
static void shiftRows(unsigned char state[16]) {
    unsigned char t;
    // Row 1: shift left 1
    t=state[1]; state[1]=state[5]; state[5]=state[9]; state[9]=state[13]; state[13]=t;
    // Row 2: shift left 2
    t=state[2]; state[2]=state[10]; state[10]=t;
    t=state[6]; state[6]=state[14]; state[14]=t;
    // Row 3: shift left 3 (right 1)
    t=state[15]; state[15]=state[11]; state[11]=state[7]; state[7]=state[3]; state[3]=t;
}
static void invShiftRows(unsigned char state[16]) {
    unsigned char t;
    // Row 1: shift right 1
    t=state[13]; state[13]=state[9]; state[9]=state[5]; state[5]=state[1]; state[1]=t;
    // Row 2: shift right 2
    t=state[2]; state[2]=state[10]; state[10]=t;
    t=state[6]; state[6]=state[14]; state[14]=t;
    // Row 3: shift right 3 (left 1)
    t=state[3]; state[3]=state[7]; state[7]=state[11]; state[11]=state[15]; state[15]=t;
}
static void mixColumns(unsigned char state[16]) {
    for (int c=0; c<4; c++) {
        int i = c*4;
        unsigned char a=state[i], b=state[i+1], c2=state[i+2], d=state[i+3];
        state[i]   = gmul2(a) ^ gmul3(b) ^ c2 ^ d;
        state[i+1] = a ^ gmul2(b) ^ gmul3(c2) ^ d;
        state[i+2] = a ^ b ^ gmul2(c2) ^ gmul3(d);
        state[i+3] = gmul3(a) ^ b ^ c2 ^ gmul2(d);
    }
}
static void invMixColumns(unsigned char state[16]) {
    for (int c=0; c<4; c++) {
        int i = c*4;
        unsigned char a=state[i], b=state[i+1], c2=state[i+2], d=state[i+3];
        state[i]   = gmul(a,0x0e) ^ gmul(b,0x0b) ^ gmul(c2,0x0d) ^ gmul(d,0x09);
        state[i+1] = gmul(a,0x09) ^ gmul(b,0x0e) ^ gmul(c2,0x0b) ^ gmul(d,0x0d);
        state[i+2] = gmul(a,0x0d) ^ gmul(b,0x09) ^ gmul(c2,0x0e) ^ gmul(d,0x0b);
        state[i+3] = gmul(a,0x0b) ^ gmul(b,0x0d) ^ gmul(c2,0x09) ^ gmul(d,0x0e);
    }
}

// ── AES encrypt/decrypt single block (16 bytes) ──
static void aesEncryptBlock(const unsigned char in[16], unsigned char out[16],
                            const std::vector<unsigned char>& roundKeys, int Nr) {
    memcpy(out, in, 16);
    addRoundKey(out, roundKeys.data());
    for (int r=1; r<Nr; r++) {
        subBytes(out); shiftRows(out); mixColumns(out);
        addRoundKey(out, roundKeys.data() + r*16);
    }
    subBytes(out); shiftRows(out);
    addRoundKey(out, roundKeys.data() + Nr*16);
}

static void aesDecryptBlock(const unsigned char in[16], unsigned char out[16],
                            const std::vector<unsigned char>& roundKeys, int Nr) {
    memcpy(out, in, 16);
    addRoundKey(out, roundKeys.data() + Nr*16);
    for (int r=Nr-1; r>0; r--) {
        invShiftRows(out); invSubBytes(out);
        addRoundKey(out, roundKeys.data() + r*16);
        invMixColumns(out);
    }
    invShiftRows(out); invSubBytes(out);
    addRoundKey(out, roundKeys.data());
}

// ── CBC mode with PKCS7 padding ──
// encrypt: plaintext + iv → ciphertext (always a multiple of 16)
static std::vector<unsigned char> pkcs7Pad(const std::vector<unsigned char>& data) {
    size_t pad = 16 - (data.size() % 16);
    std::vector<unsigned char> out(data.size() + pad);
    memcpy(out.data(), data.data(), data.size());
    memset(out.data() + data.size(), (unsigned char)pad, pad);
    return out;
}

static bool pkcs7Unpad(std::vector<unsigned char>& data) {
    if (data.empty() || data.size() % 16 != 0) return false;
    unsigned char pad = data.back();
    if (pad == 0 || pad > 16) return false;
    for (size_t i = data.size() - pad; i < data.size(); i++) {
        if (data[i] != pad) return false;
    }
    data.resize(data.size() - pad);
    return true;
}

static std::vector<unsigned char> xorBlocks(const unsigned char* a, const unsigned char* b, size_t len) {
    std::vector<unsigned char> out(len);
    for (size_t i=0; i<len; i++) out[i] = a[i] ^ b[i];
    return out;
}

// CBC encrypt
static std::vector<unsigned char> cbcEncrypt(const std::vector<unsigned char>& plaintext,
                                             const std::vector<unsigned char>& iv,
                                             const std::vector<unsigned char>& roundKeys, int Nr) {
    std::vector<unsigned char> padded = pkcs7Pad(plaintext);
    std::vector<unsigned char> ciphertext(padded.size());
    unsigned char prev[16];
    memcpy(prev, iv.data(), 16);
    for (size_t i=0; i<padded.size(); i+=16) {
        auto xored = xorBlocks(padded.data()+i, prev, 16);
        aesEncryptBlock(xored.data(), ciphertext.data()+i, roundKeys, Nr);
        memcpy(prev, ciphertext.data()+i, 16);
    }
    return ciphertext;
}

// CBC decrypt
static std::vector<unsigned char> cbcDecrypt(const std::vector<unsigned char>& ciphertext,
                                             const std::vector<unsigned char>& iv,
                                             const std::vector<unsigned char>& roundKeys, int Nr) {
    if (ciphertext.size() % 16 != 0) return {};
    std::vector<unsigned char> plaintext(ciphertext.size());
    unsigned char prev[16];
    memcpy(prev, iv.data(), 16);
    for (size_t i=0; i<ciphertext.size(); i+=16) {
        unsigned char decrypted[16];
        aesDecryptBlock(ciphertext.data()+i, decrypted, roundKeys, Nr);
        auto xored = xorBlocks(decrypted, prev, 16);
        memcpy(plaintext.data()+i, xored.data(), 16);
        memcpy(prev, ciphertext.data()+i, 16);
    }
    if (!pkcs7Unpad(plaintext)) return {};
    return plaintext;
}

// Bytes to hex string
static std::string bytesToHex(const std::vector<unsigned char>& data) {
    char hex[3];
    std::string out;
    out.reserve(data.size()*2);
    for (auto b : data) { snprintf(hex, 3, "%02x", b); out += hex; }
    return out;
}

// Hex string to bytes
static std::vector<unsigned char> hexToBytes(const std::string& hex) {
    std::vector<unsigned char> out;
    out.reserve(hex.size()/2);
    for (size_t i=0; i+1<hex.size(); i+=2) {
        unsigned int b;
        sscanf_s(hex.c_str()+i, "%2x", &b);
        out.push_back((unsigned char)b);
    }
    return out;
}

// ── Public API ──

Value aesEncrypt_(std::vector<Value>& args) {
    // aesEncrypt(plaintext, key, iv)
    // Returns hex ciphertext, or nil on error
    if (args.size() < 3) return Value::nil();
    if (!args[0].isString() || !args[1].isString() || !args[2].isString()) return Value::nil();
    
    std::string pt(args[0].asString()->data, args[0].asString()->length);
    std::string keyStr(args[1].asString()->data, args[1].asString()->length);
    std::string ivStr(args[2].asString()->data, args[2].asString()->length);
    
    if (ivStr.size() != 16) return Value::nil();
    if (keyStr.size() != 16 && keyStr.size() != 32) return Value::nil();
    
    std::vector<unsigned char> key(keyStr.begin(), keyStr.end());
    std::vector<unsigned char> iv(ivStr.begin(), ivStr.end());
    std::vector<unsigned char> plaintext(pt.begin(), pt.end());
    
    std::vector<unsigned char> roundKeys;
    int Nr = expandKey(key, roundKeys);
    if (Nr < 0) return Value::nil();
    
    auto ct = cbcEncrypt(plaintext, iv, roundKeys, Nr);
    return makeStringVal(VMString::create(bytesToHex(ct)));
}

Value aesDecrypt_(std::vector<Value>& args) {
    // aesDecrypt(hexCiphertext, key, iv)
    // Returns plaintext string, or nil on error
    if (args.size() < 3) return Value::nil();
    if (!args[0].isString() || !args[1].isString() || !args[2].isString()) return Value::nil();
    
    std::string hexCt(args[0].asString()->data, args[0].asString()->length);
    std::string keyStr(args[1].asString()->data, args[1].asString()->length);
    std::string ivStr(args[2].asString()->data, args[2].asString()->length);
    
    if (ivStr.size() != 16) return Value::nil();
    if (keyStr.size() != 16 && keyStr.size() != 32) return Value::nil();
    
    std::vector<unsigned char> key2(keyStr.begin(), keyStr.end());
    std::vector<unsigned char> iv2(ivStr.begin(), ivStr.end());
    auto ct = hexToBytes(hexCt);
    
    std::vector<unsigned char> roundKeys;
    int Nr = expandKey(key2, roundKeys);
    if (Nr < 0) return Value::nil();
    
    auto pt = cbcDecrypt(ct, iv2, roundKeys, Nr);
    if (pt.empty() && !hexCt.empty()) return Value::nil();
    return makeStringVal(VMString::create(std::string(pt.begin(), pt.end())));
}

// Cryptographically secure random bytes (BCryptGenRandom on Windows)
Value randomBytes_(std::vector<Value>& args) {
    int count = 32;
    if (args.size() >= 1 && args[0].isInt()) {
        count = static_cast<int>(args[0].asInt());
        if (count < 1) count = 1;
        if (count > 1048576) count = 1048576; // 1MB max
    }
    
    std::vector<unsigned char> buf(count);
    
    // Use BCryptGenRandom for cryptographically secure randomness
    HMODULE hBcrypt = LoadLibraryA("bcrypt.dll");
    if (hBcrypt) {
        typedef LONG (WINAPI *BCryptGenRandom_t)(HANDLE, PUCHAR, ULONG, ULONG);
        auto BCryptGenRandom_fn = (BCryptGenRandom_t)GetProcAddress(hBcrypt, "BCryptGenRandom");
        if (BCryptGenRandom_fn) {
            BCryptGenRandom_fn(nullptr, buf.data(), count, 2); // BCRYPT_USE_SYSTEM_PREFERRED_RNG
        } else {
            // Fallback to basic rand
            for (int i=0; i<count; i++) buf[i] = (unsigned char)(rand() ^ (rand()>>8));
        }
        FreeLibrary(hBcrypt);
    } else {
        for (int i=0; i<count; i++) buf[i] = (unsigned char)(rand() ^ (rand()>>8));
    }
    
    return makeStringVal(VMString::create(std::string(buf.begin(), buf.end())));
}

} // namespace aes_ns

void StdLib::registerAes(VM* vm) {
    using namespace aes_ns;
    registerFunction(vm, "aesEncrypt",   aesEncrypt_);
    registerFunction(vm, "aesDecrypt",   aesDecrypt_);
    registerFunction(vm, "randomBytes",  randomBytes_);
    registerAlias(vm, "AES加密",         "aesEncrypt");
    registerAlias(vm, "AES解密",         "aesDecrypt");
    registerAlias(vm, "随机字节",        "randomBytes");
}

// ── Dir make (recursive) ──
namespace dir_ns {

// Recursively create directories (like mkdir -p)
static bool mkdirRecursive(const std::wstring& path) {
    if (CreateDirectoryW(path.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS)
        return true;
    if (GetLastError() != ERROR_PATH_NOT_FOUND)
        return false;
    // Try parent — check both \ and /
    size_t pos = path.rfind(L'\\');
    if (pos == std::wstring::npos) pos = path.rfind(L'/');
    if (pos == std::wstring::npos) return false;
    if (!mkdirRecursive(path.substr(0, pos))) return false;
    return CreateDirectoryW(path.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS;
}

Value dirMake_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Bool(false);
    std::string path(args[0].asString()->data, args[0].asString()->length);
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return Value::Bool(false);
    std::vector<wchar_t> wb(wlen);
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wb.data(), wlen);
    return Value::Bool(mkdirRecursive(std::wstring(wb.data())));
}

} // namespace dir_ns

void StdLib::registerDir(VM* vm) {
    using namespace dir_ns;
    registerFunction(vm, "dirMake",   dirMake_);
    registerAlias(vm, "创建目录",     "dirMake");
}

} // namespace cplang
