#include "vm/vm.hpp"
#include "vm/value.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>

namespace cplang {

// 基础工具函数
static std::string readFile(const std::string& path) {
    std::ifstream t(path);
    std::stringstream buffer;
    buffer << t.rdbuf();
    return buffer.str();
}

static bool writeFile(const std::string& path, const std::string& content) {
    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << content;
    return true;
}

// 注册基础标准库
void registerSystem(VM* vm) {
    // 打印函数
    vm->registerNative("打印", [](std::vector<Value>& args) -> Value {
        for (size_t i=0; i<args.size(); i++) {
            if (args[i].isString()) {
                printf("%s", args[i].asString()->data);
            } else if (args[i].isInt()) {
                printf("%lld", args[i].asInt());
            } else if (args[i].isFloat()) {
                printf("%f", args[i].asFloat());
            } else if (args[i].isBool()) {
                printf(args[i].asBool() ? "真" : "假");
            } else if (args[i].isNil()) {
                printf("nil");
            } else if (args[i].isArray()) {
                printf("[array]");
            } else if (args[i].isTable()) {
                printf("[table]");
            } else {
                printf("[object]");
            }
            if (i != args.size() - 1) printf(" ");
        }
        printf("\n");
        return Value();
    });

    // 读取文件
    vm->registerNative("读文件", [](std::vector<Value>& args) -> Value {
        if (args.empty() || !args[0].isString()) return Value();
        std::string path(args[0].asString()->data, args[0].asString()->length);
        std::string content = readFile(path);
        if (content.empty()) return Value();
        return makeStringVal(VMString::create(content.c_str()));
    });

    // 写入文件
    vm->registerNative("写文件", [](std::vector<Value>& args) -> Value {
        if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return Value::Bool(false);
        std::string path(args[0].asString()->data, args[0].asString()->length);
        std::string content(args[1].asString()->data, args[1].asString()->length);
        return Value::Bool(writeFile(path, content));
    });

    // 时间戳
    vm->registerNative("时间戳", [](std::vector<Value>& args) -> Value {
        return Value::Int((int64_t)time(nullptr));
    });

    // 随机数
    vm->registerNative("随机数", [](std::vector<Value>& args) -> Value {
        if (args.empty()) return Value::Int(rand());
        if (args.size() == 1 && args[0].isInt()) {
            int max = args[0].asInt();
            return Value::Int(rand() % max);
        }
        if (args.size() == 2 && args[0].isInt() && args[1].isInt()) {
            int min = args[0].asInt();
            int max = args[1].asInt();
            return Value::Int(min + rand() % (max - min));
        }
        return Value();
    });

    // 数学函数
    vm->registerNative("正弦", [](std::vector<Value>& args) -> Value {
        if (args.empty() || !args[0].isNumber()) return Value::fromFloat(0);
        return Value::fromFloat(sin(args[0].asFloat()));
    });

    vm->registerNative("余弦", [](std::vector<Value>& args) -> Value {
        if (args.empty() || !args[0].isNumber()) return Value::fromFloat(0);
        return Value::fromFloat(cos(args[0].asFloat()));
    });

    vm->registerNative("平方根", [](std::vector<Value>& args) -> Value {
        if (args.empty() || !args[0].isNumber()) return Value::fromFloat(0);
        return Value::fromFloat(sqrt(args[0].asFloat()));
    });

    // 字符串函数
    vm->registerNative("字符串长度", [](std::vector<Value>& args) -> Value {
        if (args.empty() || !args[0].isString()) return Value::Int(0);
        return Value::Int(args[0].asString()->length);
    });

    vm->registerNative("转字符串", [](std::vector<Value>& args) -> Value {
        if (args.empty()) return makeStringVal(VMString::create(""));
        if (args[0].isString()) return args[0];
        if (args[0].isInt()) {
            return makeStringVal(VMString::create(std::to_string(args[0].asInt()).c_str()));
        }
        if (args[0].isFloat()) {
            return makeStringVal(VMString::create(std::to_string(args[0].asFloat()).c_str()));
        }
        if (args[0].isBool()) {
            return makeStringVal(VMString::create(args[0].asBool() ? "真" : "假"));
        }
        return makeStringVal(VMString::create(""));
    });

    // 数组函数
    vm->registerNative("数组长度", [](std::vector<Value>& args) -> Value {
        if (args.empty() || !args[0].isArray()) return Value::Int(0);
        return Value::Int(args[0].asArray()->length());
    });

    // 系统函数
    vm->registerNative("退出", [](std::vector<Value>& args) -> Value {
        int code = 0;
        if (!args.empty() && args[0].isInt()) code = args[0].asInt();
        exit(code);
        return Value();
    });

    // 初始化随机数种子
    srand((unsigned)time(nullptr));
}

}