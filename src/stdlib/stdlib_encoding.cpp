// stdlib_encoding — extracted from stdlib_regex_crypto_string.cpp
void StdLib::registerEncoding(VM* vm) {
    registerFunction(vm, "urlEncode", encoding::urlEncode);
    registerFunction(vm, "urlDecode", encoding::urlDecode);
    registerFunction(vm, "base64Encode", encoding::base64Encode);
    registerFunction(vm, "base64Decode", encoding::base64Decode);
    registerFunction(vm, "hexEncode", encoding::hexEncode);
    registerFunction(vm, "hexDecode", encoding::hexDecode);

    registerAlias(vm, "URL编码", "urlEncode");
    registerAlias(vm, "URL解码", "urlDecode");
    registerAlias(vm, "Base64编码", "base64Encode");
    registerAlias(vm, "Base64解码", "base64Decode");
    registerAlias(vm, "十六进制编码", "hexEncode");
    registerAlias(vm, "十六进制解码", "hexDecode");
}

namespace encoding {
Value urlEncode(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string input(args[0].asString()->data, args[0].asString()->length);
    std::string result;
    const char* hex = "0123456789ABCDEF";
    for (unsigned char c : input) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            result += static_cast<char>(c);
        } else {
            result += '%';
            result += hex[c >> 4];
            result += hex[c & 0xF];
        }
    }
    return Value::String(VMString::create(result));
}

Value urlDecode(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string input(args[0].asString()->data, args[0].asString()->length);
    std::string result;
    for (size_t i = 0; i < input.size(); i++) {
        if (input[i] == '%' && i + 2 < input.size()) {
            int hi = std::isdigit(input[i+1]) ? input[i+1] - '0' : (std::toupper(input[i+1]) - 'A' + 10);
            int lo = std::isdigit(input[i+2]) ? input[i+2] - '0' : (std::toupper(input[i+2]) - 'A' + 10);
            result += static_cast<char>((hi << 4) | lo);
            i += 2;
        } else if (input[i] == '+') {
            result += ' ';
        } else {
            result += input[i];
        }
    }
    return Value::String(VMString::create(result));
}

Value base64Encode(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    static const char* T = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string input(args[0].asString()->data, args[0].asString()->length);
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) { out += T[(val >> valb) & 0x3F]; valb -= 6; }
    }
    if (valb > -6) out += T[((val << 8) >> (valb + 8)) & 0x3F];
    while (out.size() % 4) out += '=';
    return Value::String(VMString::create(out));
}

Value base64Decode(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    static int D[128] = {0};
    static bool init = false;
    if (!init) {
        for (int i = 0; i < 128; i++) D[i] = -1;
        for (int i = 0; i < 64; i++) D[(unsigned char)"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;
        init = true;
    }
    std::string input(args[0].asString()->data, args[0].asString()->length);
    std::string out;
    int val = 0, valb = -8;
    for (unsigned char c : input) {
        if (D[c] == -1) break;
        val = (val << 6) + D[c];
        valb += 6;
        if (valb >= 0) { out += (char)((val >> valb) & 0xFF); valb -= 8; }
    }
    return Value::String(VMString::create(out));
}

Value hexEncode(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string input(args[0].asString()->data, args[0].asString()->length);
    const char* hex = "0123456789abcdef";
    std::string out;
    for (unsigned char c : input) {
        out += hex[c >> 4];
        out += hex[c & 0xF];
    }
    return Value::String(VMString::create(out));
}

Value hexDecode(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string input(args[0].asString()->data, args[0].asString()->length);
    std::string out;
    for (size_t i = 0; i + 1 < input.length(); i += 2) {
        auto hexVal = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return 0;
        };
        out += (char)((hexVal(input[i]) << 4) | hexVal(input[i + 1]));
    }
    return Value::String(VMString::create(out));
}
} // namespace encoding

// ═══════════════════════════════════════════════════════════════════
//  字符串增强实现
// ═══════════════════════════════════════════════════════════════════

