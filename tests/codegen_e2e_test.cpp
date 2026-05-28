#include "codegen/codegen.hpp"
#include "vm/vm.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>

using namespace cplang;

static void dumpBC(VMFunction* func, int indent = 0) {
    if (!func) return;
    std::printf("BC(%zu): ", func->code.size());
    for (size_t i = 0; i < func->code.size(); i++) {
        std::printf("%d ", (int)func->code[i]);
    }
    std::printf("\nCONST(%zu): ", func->constants.size());
    for (size_t i = 0; i < func->constants.size(); i++) {
        const Value& v = func->constants[i];
        std::printf("[%zu]=", i);
        if (v.tag == 2) std::printf("2:%lld", (long long)v.asInt());
        else if (v.tag == 3) std::printf("3:%.6g", v.asFloat());
        else if (v.tag == 4 && v.asString()) std::printf("4:%s", v.asString()->data);
        else if (v.tag == 7) {
            // T_FUNCTION - dump sub-function
            std::printf("7:<func>");
        }
        else std::printf("%d:?", (int)v.tag);
        if (i + 1 < func->constants.size()) std::printf(" ");
    }
    std::printf("\n");
    // Dump sub-functions
    for (size_t i = 0; i < func->constants.size(); i++) {
        const Value& v = func->constants[i];
        if (v.tag == 7 && v.func) {
            std::printf("  SUB-FUNC[%zu]:\n  ", i);
            dumpBC(v.func, indent + 1);
        }
    }
}

static void printVal(const Value& v) {
    if (v.tag == 0) { std::cout << "nil"; return; }           // T_NIL
    if (v.tag == 1) { std::cout << (v.i ? "true" : "false"); return; }  // T_BOOL
    if (v.tag == 2) { std::cout << v.asInt(); return; }       // T_INT
    if (v.tag == 3) { std::cout << v.asFloat(); return; }     // T_FLOAT
    if (v.tag == 4 && v.asString()) { std::cout << v.asString()->data; return; } // T_STRING
    if (v.tag == 5 && v.asArray()) {                           // T_ARRAY
        std::cout << "[";
        auto& data = v.asArray()->data;
        for (size_t i = 0; i < data.size(); i++) {
            if (i > 0) std::cout << ", ";
            printVal(data[i]);
        }
        std::cout << "]";
        return;
    }
    if (v.tag == 7 && v.func) { std::cout << "<function>"; return; }  // T_FUNCTION
    std::cout << "<" << (int)v.tag << ">";
}

int main(int argc, char** argv) {
    std::string source;
    if (argc > 1) {
        std::ifstream f(argv[1], std::ios::binary);
        if (f) {
            std::string raw((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            if (raw.size() >= 3 && (unsigned char)raw[0] == 0xEF && (unsigned char)raw[1] == 0xBB && (unsigned char)raw[2] == 0xBF)
                raw = raw.substr(3);
            source = raw;
        } else {
            source = argv[1];
        }
    } else {
        std::getline(std::cin, source);
    }
    
    std::printf("SRC: %s\n", source.c_str());
    std::fflush(stdout);
    
    Compiler compiler;
    VMFunction* func = compiler.compile(source.c_str());
    if (compiler.hasError() || !func) {
        std::fprintf(stderr, "COMPILE ERR: %s\n", compiler.errorMessage().c_str());
        return 1;
    }
    
    dumpBC(func);
    std::fflush(stdout);
    
    VM* vm = compiler.vm();
    // 原生函数已在VM构造函数中注册，包括中文别名
    // 只需确保编译前所有需要的原生函数都已注册
    
    std::printf("TEST: about to call loadModule\n");
    std::fflush(stdout);
    bool ok = vm->loadModule(func);
    std::printf("TEST: loadModule returned %d\n", ok ? 1 : 0);
    std::fflush(stdout);
    
    if (!ok) {
        std::printf("VM ERROR: %s\n", vm->error().c_str());
    }
    
    return 0;
}
