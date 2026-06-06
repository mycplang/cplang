#include "stdlib/stdlib.hpp"

namespace cplang {

// Crypto extensions: HMAC-SHA256, SHA-512
// #include'd from stdlib.cpp, already inside namespace cplang

// ── SHA-512 implementation ──
namespace {
    using U64 = unsigned long long;

    static const U64 sha512_k[80] = {
        0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
        0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
        0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
        0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
        0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
        0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
        0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
        0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
        0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
        0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
        0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
        0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
        0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
        0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
        0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
        0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
        0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
        0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
        0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
        0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
    };

    inline U64 sha512_ror(U64 x, int n) { return (x >> n) | (x << (64-n)); }

    static void sha512_transform(U64 state[8], const unsigned char block[128]) {
        U64 w[80];
        for (int i=0; i<16; i++) {
            w[i] = ((U64)block[i*8]<<56) | ((U64)block[i*8+1]<<48) | ((U64)block[i*8+2]<<40) | ((U64)block[i*8+3]<<32)
                 | ((U64)block[i*8+4]<<24) | ((U64)block[i*8+5]<<16) | ((U64)block[i*8+6]<<8)  | (U64)block[i*8+7];
        }
        for (int i=16; i<80; i++) {
            U64 s0 = sha512_ror(w[i-15],1) ^ sha512_ror(w[i-15],8) ^ (w[i-15]>>7);
            U64 s1 = sha512_ror(w[i-2],19) ^ sha512_ror(w[i-2],61) ^ (w[i-2]>>6);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }
        U64 a=state[0], b=state[1], c=state[2], d=state[3], e=state[4], f=state[5], g=state[6], h=state[7];
        for (int i=0; i<80; i++) {
            U64 S1 = sha512_ror(e,14) ^ sha512_ror(e,18) ^ sha512_ror(e,41);
            U64 ch = (e & f) ^ ((~e) & g);
            U64 tmp1 = h + S1 + ch + sha512_k[i] + w[i];
            U64 S0 = sha512_ror(a,28) ^ sha512_ror(a,34) ^ sha512_ror(a,39);
            U64 maj = (a & b) ^ (a & c) ^ (b & c);
            U64 tmp2 = S0 + maj;
            h=g; g=f; f=e; e=d+tmp1; d=c; c=b; b=a; a=tmp1+tmp2;
        }
        state[0]+=a; state[1]+=b; state[2]+=c; state[3]+=d;
        state[4]+=e; state[5]+=f; state[6]+=g; state[7]+=h;
    }

    std::string sha512_compute(const std::string& input) {
        U64 state[8] = {
            0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL, 0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
            0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL, 0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL
        };
        size_t ml = input.size();
        std::vector<unsigned char> buf = {(unsigned char)(ml>>56),(unsigned char)(ml>>48),(unsigned char)(ml>>40),(unsigned char)(ml>>32),
                                          (unsigned char)(ml>>24),(unsigned char)(ml>>16),(unsigned char)(ml>>8),(unsigned char)ml};
        // Pad: input + 0x80 + zeros + 16-byte length (big-endian)
        // Total padded = ((ml + 17 + 127) / 128) * 128
        size_t padded = ((ml + 17 + 127) / 128) * 128;
        std::vector<unsigned char> data(padded, 0);
        memcpy(data.data(), input.data(), ml);
        data[ml] = 0x80;
        // Append 128-bit length at end (big-endian)
        for (int i=0; i<8; i++) data[padded-8+i] = buf[i];
        for (size_t i=0; i<padded; i+=128) sha512_transform(state, data.data()+i);
        // Format as hex
        char hex[129];
        for (int i=0; i<8; i++) {
            snprintf(hex+i*16, 17, "%016llx", state[i]);
        }
        return std::string(hex, 128);
    }
} // anon

namespace crypto_plus_ns {

// ── SHA-512 ──
Value sha512Raw_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) {
        return makeStringVal(VMString::create(sha512_compute("")));
    }
    std::string input(args[0].asString()->data, args[0].asString()->length);
    return makeStringVal(VMString::create(sha512_compute(input)));
}

// ── HMAC-SHA256 (uses sha256Raw from crypto_ns; accessible via extern) ──
// Reuses sha256's compute function pattern - but we need sha256's raw hash function.
// sha256Raw is registered in crypto_ns; we implement HMAC independently using a local sha256.

// SHA-256 (inline for HMAC use — independent of crypto_ns)
namespace {
    static const UInt32 sha256_k[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba1,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    };
    inline UInt32 sha256_ror(UInt32 x, int n) { return (x>>n)|(x<<(32-n)); }

    static void sha256_transform(UInt32 state[8], const unsigned char block[64]) {
        UInt32 w[64];
        for (int i=0;i<16;i++) w[i]=((UInt32)block[i*4]<<24)|((UInt32)block[i*4+1]<<16)|((UInt32)block[i*4+2]<<8)|block[i*4+3];
        for (int i=16;i<64;i++) {
            UInt32 s0=sha256_ror(w[i-15],7)^sha256_ror(w[i-15],18)^(w[i-15]>>3);
            UInt32 s1=sha256_ror(w[i-2],17)^sha256_ror(w[i-2],19)^(w[i-2]>>10);
            w[i]=w[i-16]+s0+w[i-7]+s1;
        }
        UInt32 a=state[0],b=state[1],c=state[2],d=state[3],e=state[4],f=state[5],g=state[6],h=state[7];
        for (int i=0;i<64;i++) {
            UInt32 S1=sha256_ror(e,6)^sha256_ror(e,11)^sha256_ror(e,25), ch=(e&f)^((~e)&g), tmp1=h+S1+ch+sha256_k[i]+w[i];
            UInt32 S0=sha256_ror(a,2)^sha256_ror(a,13)^sha256_ror(a,22), maj=(a&b)^(a&c)^(b&c), tmp2=S0+maj;
            h=g;g=f;f=e;e=d+tmp1;d=c;c=b;b=a;a=tmp1+tmp2;
        }
        state[0]+=a;state[1]+=b;state[2]+=c;state[3]+=d;state[4]+=e;state[5]+=f;state[6]+=g;state[7]+=h;
    }

    std::string sha256_compute_raw(const unsigned char* data, size_t len) {
        UInt32 state[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
        U64 ml = len * 8; // bit length
        size_t padded = ((len+9+63)/64)*64;
        std::vector<unsigned char> buf(padded, 0);
        memcpy(buf.data(), data, len);
        buf[len]=0x80;
        for(int i=0;i<8;i++) buf[padded-8+i]=static_cast<unsigned char>(ml>>(56-8*i));
        for(size_t i=0;i<padded;i+=64) sha256_transform(state, buf.data()+i);
        std::vector<unsigned char> digest(32);
        for(int i=0;i<8;i++) {
            digest[i*4]=(state[i]>>24)&0xff; digest[i*4+1]=(state[i]>>16)&0xff;
            digest[i*4+2]=(state[i]>>8)&0xff; digest[i*4+3]=state[i]&0xff;
        }
        return std::string(reinterpret_cast<char*>(digest.data()), 32);
    }
} // anon

// HMAC-SHA256: HMAC(K,m) = H((K' xor opad) || H((K' xor ipad) || m))
std::string hmac_sha256_compute(const std::string& key, const std::string& message) {
    const size_t BLOCK_SIZE = 64;
    const unsigned char IPAD = 0x36;
    const unsigned char OPAD = 0x5c;
    
    std::vector<unsigned char> key_block(BLOCK_SIZE, 0);
    if (key.size() > BLOCK_SIZE) {
        // Hash key if too long
        std::string hk = sha256_compute_raw(reinterpret_cast<const unsigned char*>(key.data()), key.size());
        memcpy(key_block.data(), hk.data(), std::min(hk.size(), BLOCK_SIZE));
    } else {
        memcpy(key_block.data(), key.data(), key.size());
    }
    
    // Inner: H(key_block xor ipad || message)
    std::vector<unsigned char> inner(BLOCK_SIZE + message.size());
    for (size_t i=0; i<BLOCK_SIZE; i++) inner[i] = key_block[i] ^ IPAD;
    memcpy(inner.data()+BLOCK_SIZE, message.data(), message.size());
    std::string inner_hash = sha256_compute_raw(inner.data(), inner.size());
    
    // Outer: H(key_block xor opad || inner_hash)
    std::vector<unsigned char> outer(BLOCK_SIZE + 32);
    for (size_t i=0; i<BLOCK_SIZE; i++) outer[i] = key_block[i] ^ OPAD;
    memcpy(outer.data()+BLOCK_SIZE, inner_hash.data(), 32);
    std::string result = sha256_compute_raw(outer.data(), outer.size());
    
    // Hex output
    char hex[65];
    for (int i=0; i<32; i++) snprintf(hex+i*2, 3, "%02x", (unsigned char)result[i]);
    return std::string(hex, 64);
}

Value hmacSha256_(std::vector<Value>& args) {
    if (args.size() < 2) return makeStringVal(VMString::create(""));
    std::string key(args[0].asString()->data, args[0].asString()->length);
    std::string msg(args[1].asString()->data, args[1].asString()->length);
    return makeStringVal(VMString::create(hmac_sha256_compute(key, msg)));
}

// ── Base32 ──
static const char base32_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

Value base32Encode_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return makeStringVal(VMString::create(""));
    std::string input(args[0].asString()->data, args[0].asString()->length);
    std::string out;
    out.reserve(((input.size()+4)/5)*8);
    int buf=0, bits=0;
    for (size_t i=0; i<input.size(); i++) {
        buf = (buf<<8) | (unsigned char)input[i];
        bits += 8;
        while (bits >= 5) {
            out += base32_alphabet[(buf >> (bits-5)) & 31];
            bits -= 5;
        }
    }
    if (bits > 0) out += base32_alphabet[(buf << (5-bits)) & 31];
    // Padding
    while (out.size() % 8 != 0) out += '=';
    return makeStringVal(VMString::create(out));
}

Value base32Decode_(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return makeStringVal(VMString::create(""));
    std::string input(args[0].asString()->data, args[0].asString()->length);
    // Remove padding and build lookup
    static const int lookup[128] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,26,27,28,29,30,31,-1,-1,-1,-1,-1,-1,-1,-1,
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };
    std::string out;
    int buf=0, bits=0;
    for (size_t i=0; i<input.size() && input[i]!='='; i++) {
        int val = lookup[(int)(unsigned char)input[i]];
        if (val < 0) continue;
        buf = (buf<<5) | val;
        bits += 5;
        if (bits >= 8) {
            out += (char)((buf>>(bits-8))&0xff);
            bits -= 8;
        }
    }
    return makeStringVal(VMString::create(out));
}

} // namespace crypto_plus_ns

void StdLib::registerCryptoPlus(VM* vm) {
    using namespace crypto_plus_ns;
    registerFunction(vm, "sha512",        sha512Raw_);
    registerFunction(vm, "hmacSha256",    hmacSha256_);
    registerFunction(vm, "base32Encode",  base32Encode_);
    registerFunction(vm, "base32Decode",  base32Decode_);
    registerAlias(vm, "SHA512",           "sha512");
    registerAlias(vm, "HMAC_SHA256",      "hmacSha256");
    registerAlias(vm, "HMAC签名",         "hmacSha256");
    registerAlias(vm, "Base32编码",       "base32Encode");
    registerAlias(vm, "Base32解码",       "base32Decode");
}

} // namespace cplang
