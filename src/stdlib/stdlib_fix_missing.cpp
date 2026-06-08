// stdlib_fix_missing.cpp - 恢复 git reset 丢失的函数
// crc32, hmacMd5, transformArr, procExec/procSystem, envGet/envSet
#include "vm/vm.hpp"
#include "stdlib/stdlib.hpp"
#include "crypto/md5_impl.h"
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#ifndef MINIZ_NO_ARCHIVE_APIS
#define MINIZ_NO_ARCHIVE_APIS
#endif
#include "miniz.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <vector>
#include <string>
#include <cstring>

namespace cplang {

// ═══════════════════ CRC32 ═══════════════════
static UInt32 crc32_table[256];
static bool crc32_table_init = false;

static void crc32_init_table() {
    if (crc32_table_init) return;
    for (UInt32 i = 0; i < 256; i++) {
        UInt32 crc = i;
        for (int j = 0; j < 8; j++)
            crc = (crc >> 1) ^ (0xEDB88320 & -(Int32)(crc & 1));
        crc32_table[i] = crc;
    }
    crc32_table_init = true;
}

static UInt32 crc32_compute(const UInt8* data, size_t len) {
    crc32_init_table();
    UInt32 crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++)
        crc = (crc >> 8) ^ crc32_table[(crc ^ data[i]) & 0xFF];
    return crc ^ 0xFFFFFFFF;
}

namespace fix_missing {
Value crc32Fn(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    auto* vmstr = args[0].asString();
    if (!vmstr) return Value::nil();
    UInt32 crc = crc32_compute((const UInt8*)vmstr->data, vmstr->length);
    char buf[16];
    snprintf(buf, sizeof(buf), "%08x", crc);
    return makeStringVal(VMString::create(std::string(buf)));
}

// ═══════════════════ HMAC-MD5 ═══════════════════

static std::string hmacMd5Compute(const std::string& key, const std::string& data) {
    const size_t BLOCK_SIZE = 64;
    UInt8 key_block[64] = {};
    
    // If key is longer than block size, hash it first
    if (key.length() > BLOCK_SIZE) {
        UInt8 hashed_key[16];
        md5_raw_bytes(key.c_str(), key.length(), hashed_key);
        memcpy(key_block, hashed_key, 16);
    } else {
        memcpy(key_block, key.c_str(), key.length());
    }
    
    // Compute inner and outer padded keys
    UInt8 o_key_pad[64], i_key_pad[64];
    for (int i = 0; i < 64; i++) {
        o_key_pad[i] = key_block[i] ^ 0x5C;
        i_key_pad[i] = key_block[i] ^ 0x36;
    }
    
    // Inner hash: MD5(i_key_pad || data)
    UInt8 inner_hash[16];
    std::string inner_input((const char*)i_key_pad, 64);
    inner_input += data;
    md5_raw_bytes(inner_input.c_str(), inner_input.length(), inner_hash);
    
    // Outer hash: MD5(o_key_pad || inner_hash)
    UInt8 final_hash[16];
    std::string outer_input((const char*)o_key_pad, 64);
    outer_input.append((const char*)inner_hash, 16);
    md5_raw_bytes(outer_input.c_str(), outer_input.length(), final_hash);
    
    // Convert to hex
    char hex[33];
    for (int i = 0; i < 16; i++)
        snprintf(hex + i * 2, 3, "%02x", final_hash[i]);
    return std::string(hex, 32);
}

Value hmacMd5Fn(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    auto* keyStr = args[0].asString();
    auto* dataStr = args[1].asString();
    if (!keyStr || !dataStr) return Value::nil();
    std::string key(keyStr->data, keyStr->length);
    std::string data(dataStr->data, dataStr->length);
    return makeStringVal(VMString::create(hmacMd5Compute(key, data)));
}

// ═══════════════════ transformArr ═══════════════════
Value transformArrFn(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    Value arr = args[0];
    Value fn = args[1];
    if (!arr.isArray() || !fn.isFunction()) return Value::nil();
    
    VMArray* src = arr.asArray();
    VMArray* dst = VMArray::create();
    
    for (Int32 i = 0; i < src->length(); i++) {
        std::vector<Value> cargs = { src->get(i) };
        Value r = VM::current()->callFunction(fn, cargs);
        dst->data.push_back(r);
    }
    return makeArrayVal(dst);
}

// ═══════════════════ procExec ═══════════════════
static std::string readPipe(HANDLE h) {
    std::string result;
    char buf[256];
    DWORD read;
    while (ReadFile(h, buf, sizeof(buf) - 1, &read, nullptr) && read > 0) {
        buf[read] = '\0';
        result += buf;
    }
    return result;
}

Value procExecFn(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    auto* cmdStr = args[0].asString();
    if (!cmdStr) return Value::nil();
    
    std::string cmd(cmdStr->data, cmdStr->length);
    cmd = "cmd /c " + cmd;
    
    HANDLE hRead, hWrite;
    SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
    if (!CreatePipe(&hRead, &hWrite, &sa, 0))
        return Value::nil();
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);
    
    PROCESS_INFORMATION pi = {};
    STARTUPINFOA si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    
    std::vector<char> cmdBuf(cmd.begin(), cmd.end());
    cmdBuf.push_back('\0');
    
    BOOL ok = CreateProcessA(nullptr, cmdBuf.data(), nullptr, nullptr, TRUE,
                             0, nullptr, nullptr, &si, &pi);
    CloseHandle(hWrite);
    
    if (!ok) {
        CloseHandle(hRead);
        return Value::nil();
    }
    
    WaitForSingleObject(pi.hProcess, 30000);
    std::string output = readPipe(hRead);
    
    CloseHandle(hRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    // Trim trailing newline
    while (!output.empty() && (output.back() == '\n' || output.back() == '\r'))
        output.pop_back();
    
    return makeStringVal(VMString::create(output));
}

// ═══════════════════ procSystem ═══════════════════
Value procSystemFn(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    auto* cmdStr = args[0].asString();
    if (!cmdStr) return Value::nil();
    std::string cmd(cmdStr->data, cmdStr->length);
    // UTF-8 → wide for Windows _wsystem (avoids GBK garbling of Chinese paths)
    int wlen = MultiByteToWideChar(CP_UTF8, 0, cmd.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return Value::Int(-1);
    std::wstring wcmd(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, cmd.c_str(), -1, &wcmd[0], wlen);
    int rc = ::_wsystem(wcmd.c_str());
    return Value::Int(rc);
}

// ═══════════════════ envGet ═══════════════════
Value envGetFn(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    auto* nameStr = args[0].asString();
    if (!nameStr) return Value::nil();
    std::string name(nameStr->data, nameStr->length);
    char buf[32768];
    DWORD len = GetEnvironmentVariableA(name.c_str(), buf, sizeof(buf));
    if (len == 0 || len >= sizeof(buf)) return Value::nil();
    return makeStringVal(VMString::create(std::string(buf, len)));
}

// ═══════════════════ envSet ═══════════════════
Value envSetFn(std::vector<Value>& args) {
    if (args.size() < 2) return Value::nil();
    auto* nameStr = args[0].asString();
    auto* valStr = args[1].asString();
    if (!nameStr || !valStr) return Value::nil();
    std::string name(nameStr->data, nameStr->length);
    std::string val(valStr->data, valStr->length);
    BOOL ok = SetEnvironmentVariableA(name.c_str(), val.c_str());
    return Value::Bool(ok);
}

// ═══════════════════ compress / decompress ═══════════════════
Value compressFn(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    auto* src = args[0].asString();
    if (!src) return Value::nil();
    mz_ulong srcLen = static_cast<mz_ulong>(src->length);
    mz_ulong destLen = mz_compressBound(srcLen);
    std::vector<unsigned char> dest(destLen);
    if (mz_compress(dest.data(), &destLen,
                    reinterpret_cast<const unsigned char*>(src->data), srcLen) != MZ_OK)
        return Value::nil();
    return makeStringVal(VMString::create(std::string(
        reinterpret_cast<const char*>(dest.data()), destLen)));
}

Value decompressFn(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    auto* src = args[0].asString();
    if (!src) return Value::nil();
    mz_ulong srcLen = static_cast<mz_ulong>(src->length);
    // Try increasingly larger buffers
    mz_ulong destLen = srcLen * 4;
    if (destLen < 1024) destLen = 1024;
    for (int attempt = 0; attempt < 5; attempt++) {
        std::vector<unsigned char> dest(destLen);
        int rc = mz_uncompress(dest.data(), &destLen,
                               reinterpret_cast<const unsigned char*>(src->data), srcLen);
        if (rc == MZ_OK)
            return makeStringVal(VMString::create(std::string(
                reinterpret_cast<const char*>(dest.data()), destLen)));
        if (rc != MZ_BUF_ERROR) return Value::nil();
        destLen *= 4;
    }
    return Value::nil();
}

// ═══════════════════ gzip ═══════════════════
// Gzip header: ID1 ID2 CM FLG MTIME XFL OS
static const unsigned char gzipHeader[10] = {
    0x1f, 0x8b,  // ID1, ID2
    0x08,         // CM = deflate
    0x00,         // FLG
    0x00, 0x00, 0x00, 0x00,  // MTIME
    0x00,         // XFL
    0xff          // OS = unknown
};

static UInt32 gzipCrc32(const unsigned char* data, size_t len) {
    // Reuse existing crc32 implementation
    return crc32_compute(data, len);
}

Value gzipCompressFn(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    auto* src = args[0].asString();
    if (!src) return Value::nil();
    
    const unsigned char* srcData = reinterpret_cast<const unsigned char*>(src->data);
    mz_ulong srcLen = static_cast<mz_ulong>(src->length);
    
    // Raw deflate (no zlib header)
    mz_ulong rawLen = mz_compressBound(srcLen);
    std::vector<unsigned char> raw(rawLen);
    if (mz_compress2(raw.data(), &rawLen, srcData, srcLen, MZ_DEFAULT_COMPRESSION) != MZ_OK)
        return Value::nil();
    
    // Build gzip: header + raw_deflate + crc32 + original_size
    UInt32 crc = gzipCrc32(srcData, srcLen);
    UInt32 origSize = static_cast<UInt32>(srcLen);
    
    std::string result;
    result.append(reinterpret_cast<const char*>(gzipHeader), 10);
    result.append(reinterpret_cast<const char*>(raw.data()), rawLen);
    result.append(reinterpret_cast<const char*>(&crc), 4);
    result.append(reinterpret_cast<const char*>(&origSize), 4);
    return makeStringVal(VMString::create(result));
}

Value gzipDecompressFn(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    auto* src = args[0].asString();
    if (!src) return Value::nil();
    if (src->length < 18) return Value::nil();  // min gzip: 10 header + 8 footer
    
    const unsigned char* data = reinterpret_cast<const unsigned char*>(src->data);
    size_t totalLen = src->length;
    
    // Verify gzip magic
    if (data[0] != 0x1f || data[1] != 0x8b) return Value::nil();
    
    // Deflate data starts after 10-byte header, ends before 8-byte footer
    size_t deflateOffset = 10;
    size_t deflateLen = totalLen - 18;
    
    // Try decompressing raw deflate
    mz_ulong destLen = static_cast<mz_ulong>(totalLen * 10);
    if (destLen < 4096) destLen = 4096;
    for (int attempt = 0; attempt < 6; attempt++) {
        std::vector<unsigned char> dest(destLen);
        int rc = mz_uncompress(dest.data(), &destLen,
                               data + deflateOffset, static_cast<mz_ulong>(deflateLen));
        if (rc == MZ_OK)
            return makeStringVal(VMString::create(std::string(
                reinterpret_cast<const char*>(dest.data()), destLen)));
        if (rc != MZ_BUF_ERROR) return Value::nil();
        destLen *= 4;
    }
    return Value::nil();
}

// ═══════════════════ 整除 / 取整 ═══════════════════
// 解决 CP 的 / 始终返回浮点的问题
static Value 整除Fn(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isInt() || !args[1].isInt())
        return Value::nil();
    Int64 b = args[1].asInt();
    if (b == 0) return Value::nil();
    return Value::Int(args[0].asInt() / b);
}

static Value 取整Fn(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    if (args[0].isInt()) return args[0];
    if (args[0].isFloat()) return Value::Int(static_cast<Int64>(args[0].asFloat()));
    return Value::Int(0);
}

} // namespace fix_missing

void StdLib::registerFixMissing(VM* vm) {
    registerFunction(vm, "crc32", fix_missing::crc32Fn);
    registerFunction(vm, "hmacMd5", fix_missing::hmacMd5Fn);
    registerFunction(vm, "transformArr", fix_missing::transformArrFn);
    registerFunction(vm, "procExec", fix_missing::procExecFn);
    registerFunction(vm, "procSystem", fix_missing::procSystemFn);
    registerFunction(vm, "envGet", fix_missing::envGetFn);
    registerFunction(vm, "envSet", fix_missing::envSetFn);
    registerFunction(vm, "compress", fix_missing::compressFn);
    registerFunction(vm, "decompress", fix_missing::decompressFn);
    registerFunction(vm, "gzipCompress", fix_missing::gzipCompressFn);
    registerFunction(vm, "gzipDecompress", fix_missing::gzipDecompressFn);

    registerFunction(vm, "整除", fix_missing::整除Fn);
    registerFunction(vm, "取整", fix_missing::取整Fn);
    registerAlias(vm, "divInt", "整除");
    registerAlias(vm, "floor", "取整");

    registerAlias(vm, "CRC32", "crc32");
    registerAlias(vm, "HMAC_MD5", "hmacMd5");
    registerAlias(vm, "数组变换", "transformArr");
    registerAlias(vm, "进程执行", "procExec");
    registerAlias(vm, "进程调用", "procSystem");
    registerAlias(vm, "获取环境", "envGet");
    registerAlias(vm, "设置环境", "envSet");
    registerAlias(vm, "压缩", "compress");
    registerAlias(vm, "解压", "decompress");
    registerAlias(vm, "GZIP压缩", "gzipCompress");
    registerAlias(vm, "GZIP解压", "gzipDecompress");

    // 统一 len 函数：支持字符串和数组
    registerFunction(vm, "len", [](std::vector<Value>& args) -> Value {
        if (args.empty()) return Value::Int(0);
        if (args[0].isString()) {
            auto* s = args[0].asString();
            return Value::Int(s ? static_cast<Int64>(s->length) : 0);
        }
        if (args[0].isArray()) {
            return Value::Int(args[0].asArray()->length());
        }
        return Value::Int(0);
    });
    registerAlias(vm, "长度", "len");
}

} // namespace cplang
