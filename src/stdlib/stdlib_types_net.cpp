#include "stdlib/stdlib.hpp"

namespace cplang {

// Network and Types functions
// #include'd from stdlib.cpp, already inside namespace cplang

void StdLib::registerNetwork(VM* vm) {
    registerFunction(vm, "nHttpGet", network::httpGet);
    registerFunction(vm, "nHttpPost", network::httpPost);
    registerFunction(vm, "nDownload", network::download);
    registerFunction(vm, "nUpload", network::upload);
    registerFunction(vm, "resolve", network::resolve);
    registerFunction(vm, "tcpConnect", network::tcpConnect);
    registerFunction(vm, "udpSocket", network::udpSocket);
    registerAlias(vm, "HTTP获取", "nHttpGet");
    registerAlias(vm, "HTTP提交", "nHttpPost");
    registerAlias(vm, "HTTP下载", "nDownload");
    registerAlias(vm, "HTTP上传", "nUpload");
    registerAlias(vm, "解析地址", "resolve");
    registerAlias(vm, "TCP连接", "tcpConnect");
    registerAlias(vm, "UDP套接字", "udpSocket");
    
    // net:: namespace functions
    registerFunction(vm, "tcpListen", net::tcpListen);
    registerFunction(vm, "tcpAccept", net::tcpAccept);
    registerFunction(vm, "tcpSend", net::tcpSend);
    registerFunction(vm, "tcpReceive", net::tcpReceive);
    registerFunction(vm, "tcpClose", net::tcpClose);
    registerFunction(vm, "udpSend", net::udpSend);
    registerFunction(vm, "udpReceive", net::udpReceive);
    
    registerAlias(vm, "TCP监听", "tcpListen");
    registerAlias(vm, "TCP接受", "tcpAccept");
    registerAlias(vm, "TCP发送", "tcpSend");
    registerAlias(vm, "TCP接收", "tcpReceive");
    registerAlias(vm, "TCP关闭", "tcpClose");
    registerAlias(vm, "UDP发送", "udpSend");
    registerAlias(vm, "UDP接收", "udpReceive");
}

namespace network {
    Value httpGet(std::vector<Value>&) {
        return Value::String(VMString::create("[network not supported in this build]"));
    }
    Value httpPost(std::vector<Value>&) {
        return Value::String(VMString::create("[network not supported in this build]"));
    }
    Value download(std::vector<Value>&) {
        return Value::String(VMString::create("[network not supported in this build]"));
    }
    Value upload(std::vector<Value>&) {
        return Value::String(VMString::create("[network not supported in this build]"));
    }
    Value resolve(std::vector<Value>&) { return Value::nil(); }
    Value tcpConnect(std::vector<Value>&) { return Value::nil(); }
    Value udpSocket(std::vector<Value>&) { return Value::nil(); }
}

// ═══════════════════════════════════════════════════════════════════
//  类型检查函数
// ═══════════════════════════════════════════════════════════════════

void StdLib::registerTypes(VM* vm) {
    registerFunction(vm, "isNil", types::isNil);
    registerFunction(vm, "isBool", types::isBool);
    registerFunction(vm, "isInt", types::isInt);
    registerFunction(vm, "isFloat", types::isFloat);
    registerFunction(vm, "isString", types::isString);
    registerFunction(vm, "isArray", types::isArray);
    registerFunction(vm, "isFunction", types::isFunction);
    registerFunction(vm, "isNumber", types::isNumber);
    registerFunction(vm, "isObject", types::isObject);
    registerFunction(vm, "typeOf", types::typeOf);
    registerFunction(vm, "toString", types::toString);
    registerFunction(vm, "toInt", types::toInt);
    registerFunction(vm, "toFloat", types::toFloat);
    registerFunction(vm, "toBool", types::toBool);
    registerAlias(vm, "是空", "isNil");
    registerAlias(vm, "是布尔", "isBool");
    registerAlias(vm, "是整数", "isInt");
    registerAlias(vm, "是浮点", "isFloat");
    registerAlias(vm, "是字符串", "isString");
    registerAlias(vm, "是数组", "isArray");
    registerAlias(vm, "是函数", "isFunction");
    registerAlias(vm, "是数字", "isNumber");
    registerAlias(vm, "是对象", "isObject");
    registerAlias(vm, "取类型", "typeOf");
    registerAlias(vm, "转字符串", "toString");
    registerAlias(vm, "转整数", "toInt");
    registerAlias(vm, "转浮点", "toFloat");
    registerAlias(vm, "转布尔", "toBool");
}

namespace types {
    Value isNil(std::vector<Value>& args) {
        return Value::Bool(args.empty() || args[0].isNil());
    }
    Value isBool(std::vector<Value>& args) {
        return Value::Bool(!args.empty() && args[0].isBool());
    }
    Value isInt(std::vector<Value>& args) {
        return Value::Bool(!args.empty() && (args[0].isInt() || args[0].isInt64()));
    }
    Value isFloat(std::vector<Value>& args) {
        return Value::Bool(!args.empty() && args[0].isFloat());
    }
    Value isString(std::vector<Value>& args) {
        return Value::Bool(!args.empty() && args[0].isString());
    }
    Value isArray(std::vector<Value>& args) {
        return Value::Bool(!args.empty() && args[0].isArray());
    }
    Value isFunction(std::vector<Value>& args) {
        if (args.empty()) return Value::Bool(false);
        return Value::Bool(args[0].tag == Value::T_FUNCTION || args[0].tag == Value::T_CFUNCTION || args[0].tag == Value::T_CLOSURE);
    }
    Value isNumber(std::vector<Value>& args) {
        return Value::Bool(!args.empty() && (args[0].isInt() || args[0].isFloat()));
    }
    Value isObject(std::vector<Value>& args) {
        return Value::Bool(!args.empty() && args[0].isObject());
    }
    Value typeOf(std::vector<Value>& args) {
        if (args.empty()) return Value::String(VMString::create("nil"));
        if (args[0].isNil()) return Value::String(VMString::create("nil"));
        if (args[0].isBool()) return Value::String(VMString::create("bool"));
        if (args[0].isInt() || args[0].isInt64()) return Value::String(VMString::create("int"));
        if (args[0].isFloat() || args[0].isFloat32()) return Value::String(VMString::create("float"));
        if (args[0].isString()) return Value::String(VMString::create("string"));
        if (args[0].isArray()) return Value::String(VMString::create("array"));
        if (args[0].isTable()) return Value::String(VMString::create("table"));
        if (args[0].isFunction() || args[0].isClosure()) return Value::String(VMString::create("function"));
        if (args[0].isClass()) return Value::String(VMString::create("class"));
        return Value::String(VMString::create("object"));
    }
    Value toString(std::vector<Value>& args) {
        if (args.empty()) return Value::String(VMString::create("nil"));
        if (args[0].isString()) return args[0];
        if (args[0].isInt() || args[0].isInt64()) {
            std::string s = std::to_string(args[0].asInt());
            return Value::String(VMString::create(s.c_str(), (UInt32)s.size()));
        }
        if (args[0].isFloat()) {
            std::string s = std::to_string(args[0].asFloat());
            return Value::String(VMString::create(s.c_str(), (UInt32)s.size()));
        }
        if (args[0].isBool())
            return Value::String(VMString::create(args[0].asBool() ? "true" : "false"));
        return typeOf(args);
    }
    Value toInt(std::vector<Value>& args) {
        if (args.empty()) return Value::Int(0);
        if (args[0].isInt() || args[0].isInt64()) return args[0];
        if (args[0].isFloat()) return Value::Int((Int64)args[0].asFloat());
        if (args[0].isBool()) return Value::Int(args[0].asBool() ? 1 : 0);
        if (args[0].isString()) {
            try {
                std::string s(args[0].asString()->data, args[0].asString()->length);
                return Value::Int(std::stoll(s));
            } catch (...) { return Value::Int(0); }
        }
        return Value::Int(0);
    }
    Value toFloat(std::vector<Value>& args) {
        if (args.empty()) return Value::Float(0.0);
        if (args[0].isFloat()) return args[0];
        if (args[0].isInt()) return Value::Float((double)args[0].asInt());
        if (args[0].isBool()) return Value::Float(args[0].asBool() ? 1.0 : 0.0);
        if (args[0].isString()) {
            try {
                std::string s(args[0].asString()->data, args[0].asString()->length);
                return Value::Float(std::stod(s));
            } catch (...) { return Value::Float(0.0); }
        }
        return Value::Float(0.0);
    }
    Value toBool(std::vector<Value>& args) {
        if (args.empty()) return Value::Bool(false);
        if (args[0].isBool()) return args[0];
        if (args[0].isNil()) return Value::Bool(false);
        if (args[0].isInt()) return Value::Bool(args[0].asInt() != 0);
        if (args[0].isFloat()) return Value::Bool(args[0].asFloat() != 0.0);
        if (args[0].isString()) {
            std::string s(args[0].asString()->data, args[0].asString()->length);
            return Value::Bool(!s.empty() && s != "false" && s != "0");
        }
        return Value::Bool(true);
    }
}

} // namespace cplang
