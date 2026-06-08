#include "stdlib/stdlib.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winhttp.h>

namespace cplang {

// JSON and HTTP functions
// #include'd from stdlib.cpp, already inside namespace cplang

void StdLib::registerJSON(VM* vm) {
    registerFunction(vm, "jsonParse", json::jsonParse);
    registerFunction(vm, "jsonStringify", json::jsonStringify);
    registerFunction(vm, "jsonPretty", json::jsonPretty);
    registerFunction(vm, "jsonValidate", json::jsonValidate);

    registerAlias(vm, "JSON解析", "jsonParse");
    registerAlias(vm, "转JSON", "jsonStringify");
    registerAlias(vm, "JSON序列化", "jsonStringify");
    registerAlias(vm, "JSON美化", "jsonPretty");
    registerAlias(vm, "JSON验证", "jsonValidate");
}

namespace json {

// ---------- 解析器 ----------

class JsonParser {
    const char* str;
    size_t pos;
    size_t len;

    void skipSpace() {
        while (pos < len && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r')) pos++;
    }

    Value parseValue() {
        skipSpace();
        if (pos >= len) return Value::nil();
        char c = str[pos];
        if (c == 'n') { expect("null"); return Value::nil(); }
        if (c == 't') { expect("true"); return Value::Bool(true); }
        if (c == 'f') { expect("false"); return Value::Bool(false); }
        if (c == '"') return parseString();
        if (c == '[') return parseArray();
        if (c == '{') return parseObject();
        return parseNumber();
    }

    void expect(const char* s) {
        size_t n = std::strlen(s);
        if (pos + n <= len && std::memcmp(str + pos, s, n) == 0) pos += n;
    }

    Value parseString() {
        pos++; // skip "
        std::string result;
        while (pos < len && str[pos] != '"') {
            if (str[pos] == '\\' && pos + 1 < len) {
                char esc = str[pos + 1];
                if (esc == '"') result += '"';
                else if (esc == '\\') result += '\\';
                else if (esc == '/') result += '/';
                else if (esc == 'b') result += '\b';
                else if (esc == 'f') result += '\f';
                else if (esc == 'n') result += '\n';
                else if (esc == 'r') result += '\r';
                else if (esc == 't') result += '\t';
                else result += esc;
                pos += 2;
            } else {
                result += str[pos++];
            }
        }
        if (pos < len && str[pos] == '"') pos++;
        return Value::String(VMString::create(result));
    }

    Value parseNumber() {
        size_t start = pos;
        if (str[pos] == '-') pos++;
        while (pos < len && std::isdigit(str[pos])) pos++;
        bool isFloat = false;
        if (pos < len && str[pos] == '.') {
            isFloat = true; pos++;
            while (pos < len && std::isdigit(str[pos])) pos++;
        }
        if (pos < len && (str[pos] == 'e' || str[pos] == 'E')) {
            isFloat = true; pos++;
            if (pos < len && (str[pos] == '+' || str[pos] == '-')) pos++;
            while (pos < len && std::isdigit(str[pos])) pos++;
        }
        std::string numStr(str + start, pos - start);
        if (isFloat) return Value::Float(std::stod(numStr));
        return Value::Int(std::stoll(numStr));
    }

    Value parseArray() {
        pos++; // skip [
        VMArray* arr = VMArray::create(0);
        skipSpace();
        if (pos < len && str[pos] == ']') { pos++; return Value::Array(arr); }
        while (true) {
            arr->data.push_back(parseValue());
            skipSpace();
            if (pos < len && str[pos] == ',') { pos++; continue; }
            if (pos < len && str[pos] == ']') { pos++; break; }
            break;
        }
        return Value::Array(arr);
    }

    Value parseObject() {
        pos++; // skip {
        VMTable* tbl = VMTable::create();
        skipSpace();
        if (pos < len && str[pos] == '}') { pos++; return Value::Table(tbl); }
        while (true) {
            skipSpace();
            Value key = parseValue();
            skipSpace();
            if (pos < len && str[pos] == ':') pos++;
            Value val = parseValue();
            if (key.isString()) {
                tbl->set(key, val);
            }
            skipSpace();
            if (pos < len && str[pos] == ',') { pos++; continue; }
            if (pos < len && str[pos] == '}') { pos++; break; }
            break;
        }
        return Value::Table(tbl);
    }

public:
    JsonParser(const char* s, size_t l) : str(s), pos(0), len(l) {}
    Value parse() { return parseValue(); }
};

// ---------- 序列化器 ----------

static void escapeString(std::string& out, const std::string& s) {
    for (unsigned char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\b') out += "\\b";
        else if (c == '\f') out += "\\f";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c < 0x20) {
            char buf[8];
            std::snprintf(buf, sizeof(buf), "\\u%04x", c);
            out += buf;
        }
        else out += static_cast<char>(c);
    }
}

static void stringifyValue(std::string& out, const Value& v, int indent, int depth) {
    std::string prefix(depth * indent, ' ');
    if (v.isNil()) out += "null";
    else if (v.isBool()) out += v.asInt() ? "true" : "false";
    else if (v.isInt()) out += std::to_string(v.asInt());
    else if (v.isFloat()) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.6g", v.asFloat());
        out += buf;
    }
    else if (v.isString()) {
        out += '"';
        escapeString(out, std::string(v.asString()->data, v.asString()->length));
        out += '"';
    }
    else if (v.isArray()) {
        auto* arr = v.asArray();
        out += '[';
        if (indent > 0 && !arr->data.empty()) out += '\n';
        for (size_t i = 0; i < arr->data.size(); i++) {
            if (indent > 0) out += prefix + std::string(indent, ' ');
            stringifyValue(out, arr->data[i], indent, depth + 1);
            if (i + 1 < arr->data.size()) out += ',';
            if (indent > 0) out += '\n';
        }
        if (indent > 0 && !arr->data.empty()) out += prefix;
        out += ']';
    }
    else if (v.isTable()) {
        auto* tbl = v.asTable();
        out += '{';
        if (indent > 0 && tbl->size() > 0) out += '\n';
        size_t idx = 0;
        for (auto& p : tbl->data) {
            if (indent > 0) out += prefix + std::string(indent, ' ');
            stringifyValue(out, p.first, 0, 0);
            out += ": ";
            stringifyValue(out, p.second, indent, depth + 1);
            if (++idx < tbl->size()) out += ',';
            if (indent > 0) out += '\n';
        }
        if (indent > 0 && tbl->size() > 0) out += prefix;
        out += '}';
    }
    else out += "null";
}

// ---------- API ----------

Value jsonParse(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string input(args[0].asString()->data, args[0].asString()->length);
    JsonParser parser(input.c_str(), input.size());
    return parser.parse();
}

Value jsonStringify(std::vector<Value>& args) {
    if (args.empty()) return Value::String(VMString::create("null"));
    std::string result;
    stringifyValue(result, args[0], 0, 0);
    return Value::String(VMString::create(result));
}

Value jsonPretty(std::vector<Value>& args) {
    if (args.empty()) return Value::String(VMString::create("null"));
    int indent = (args.size() >= 2 && args[1].isNumber()) ? static_cast<int>(args[1].asFloat()) : 2;
    std::string result;
    stringifyValue(result, args[0], indent, 0);
    return Value::String(VMString::create(result));
}

Value jsonValidate(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::Bool(false);
    try {
        std::string input(args[0].asString()->data, args[0].asString()->length);
        JsonParser parser(input.c_str(), input.size());
        parser.parse();
        return Value::Bool(true);
    } catch (...) { return Value::Bool(false); }
}

} // namespace json

// ═══════════════════════════════════════════════════════════════════
//  HTTP 客户端实现（Windows WinHTTP API）
// ═══════════════════════════════════════════════════════════════════

void StdLib::registerHTTP(VM* vm) {
    registerFunction(vm, "httpGet", http::httpGet);
    registerFunction(vm, "httpPost", http::httpPost);
    registerFunction(vm, "httpDownload", http::httpDownload);

    registerAlias(vm, "HTTP获取", "httpGet");
    registerAlias(vm, "HTTP提交", "httpPost");
    registerAlias(vm, "HTTP下载", "httpDownload");
}

namespace http {

static std::string getStr(const Value& v) {
    if (!v.isString()) return "";
    return std::string(v.asString()->data, v.asString()->length);
}

static std::wstring toWstr(const std::string& s) {
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring result(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &result[0], len);
    return result;
}

static std::string doRequest(const std::wstring& method, const std::string& urlStr, const std::string& body = "", const std::string& contentType = "") {
    // 解析 URL
    std::string protocol, host, path;
    size_t pos = urlStr.find("://");
    if (pos != std::string::npos) {
        protocol = urlStr.substr(0, pos);
        pos += 3;
    }
    size_t slash = urlStr.find('/', pos);
    if (slash != std::string::npos) {
        host = urlStr.substr(pos, slash - pos);
        path = urlStr.substr(slash);
    } else {
        host = urlStr.substr(pos);
        path = "/";
    }

    bool isHttps = (protocol == "https");
    HINTERNET hSession = WinHttpOpen(L"CP-Language/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return "";

    HINTERNET hConnect = WinHttpConnect(hSession, toWstr(host).c_str(), isHttps ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return ""; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, method.c_str(), toWstr(path).c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, isHttps ? WINHTTP_FLAG_SECURE : 0);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return ""; }

    std::wstring headers;
    if (!contentType.empty()) {
        headers = L"Content-Type: " + toWstr(contentType) + L"\r\n";
    }

    BOOL result = WinHttpSendRequest(hRequest, headers.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headers.c_str(), headers.empty() ? 0 : static_cast<DWORD>(headers.length()), body.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)body.c_str(), body.empty() ? 0 : static_cast<DWORD>(body.length()), body.empty() ? 0 : static_cast<DWORD>(body.length()), 0);
    if (!result) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return ""; }

    result = WinHttpReceiveResponse(hRequest, nullptr);
    if (!result) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return ""; }

    std::string response;
    DWORD dwSize = 0;
    do {
        dwSize = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
        if (dwSize == 0) break;
        std::vector<char> buf(dwSize + 1, '\0');
        DWORD dwRead = 0;
        if (WinHttpReadData(hRequest, buf.data(), dwSize, &dwRead)) {
            response.append(buf.data(), dwRead);
        }
    } while (dwSize > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return response;
}

Value httpGet(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string url = getStr(args[0]);
    return Value::String(VMString::create(doRequest(L"GET", url)));
}

Value httpPost(std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) return Value::nil();
    std::string url = getStr(args[0]);
    std::string body = (args.size() >= 2 && args[1].isString()) ? getStr(args[1]) : "";
    std::string ctype = (args.size() >= 3 && args[2].isString()) ? getStr(args[2]) : "application/x-www-form-urlencoded";
    return Value::String(VMString::create(doRequest(L"POST", url, body, ctype)));
}

Value httpDownload(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Bool(false);
    std::string url = getStr(args[0]);
    std::string filepath = getStr(args[1]);
    std::string data = doRequest(L"GET", url);
    if (data.empty()) return Value::Bool(false);
    std::ofstream file(filepath, std::ios::binary);
    if (!file) return Value::Bool(false);
    file.write(data.c_str(), data.size());
    return Value::Bool(file.good());
}
} // namespace http

// ═══════════════════════════════════════════════════════════════════
//  矩阵/向量运算实现（基于数组约定）
// ═══════════════════════════════════════════════════════════════════

} // namespace cplang
