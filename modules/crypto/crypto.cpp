// @cp/crypto — CP语言 加密模块（AES + SHA + MD5 + Base64 + 编码）
// 独立编译为 cplang_crypto.lib，AOT 按需链接

#include "stdlib/stdlib_fwd.hpp"

extern "C" __declspec(dllexport) void cplang_module_crypto_register(cplang::VM* vm) {
    cplang::StdLib::registerCrypto(vm);
    cplang::StdLib::registerCryptoPlus(vm);
    cplang::StdLib::registerAes(vm);
    cplang::StdLib::registerEncoding(vm);
}
