#include <cstdio>
#include <string>
#include "codegen/codegen.hpp"
#include "vm/vm.hpp"
using namespace cplang;

int main() {
    std::fprintf(stderr, "Starting...\n");
    try {
        Compiler compiler;
        std::fprintf(stderr, "Compiler created\n");
        VMFunction* func = compiler.compile(u8"变量 x = 42;");
        std::fprintf(stderr, "Compiled: %p\n", (void*)func);
        if (func) {
            VM* vm = compiler.vm();
            std::fprintf(stderr, "VM: %p\n", (void*)vm);
            if (vm) {
                vm->loadModule(func);
                std::fprintf(stderr, "Loaded\n");
            }
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Exception: %s\n", e.what());
    } catch (...) {
        std::fprintf(stderr, "Unknown exception\n");
    }
    std::fprintf(stderr, "Done\n");
    return 0;
}
