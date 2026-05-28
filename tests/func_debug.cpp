#include "codegen/codegen.hpp"
#include "vm/vm.hpp"
#include <cstdio>

using namespace cplang;

void dumpFunc(VMFunction* f, const char* name) {
    printf("Function %s: %zu bytes, %u params, %zu constants\n", 
           name, f->code.size(), f->numParams, f->constants.size());
    printf("  Code: ");
    for (size_t i = 0; i < f->code.size() && i < 40; i++) {
        printf("%d ", (int)f->code[i]);
    }
    printf("\n");
    for (size_t i = 0; i < f->constants.size() && i < 5; i++) {
        const Value& v = f->constants[i];
        printf("  const[%zu]: tag=%d\n", i, v.tag);
    }
}

int main() {
    const char* src = "func f() { return 42; } print(f());";
    printf("Source: %s\n\n", src);
    
    Compiler compiler;
    VMFunction* func = compiler.compile(src);
    
    if (!func) {
        printf("Compile failed: %s\n", compiler.errorMessage().c_str());
        return 1;
    }
    
    printf("=== Main function ===\n");
    dumpFunc(func, "main");
    
    for (size_t i = 0; i < func->constants.size(); i++) {
        const Value& v = func->constants[i];
        if (v.tag == Value::T_FUNCTION) {
            VMFunction* userFunc = reinterpret_cast<VMFunction*>(v.obj);
            printf("\n=== User function ===\n");
            dumpFunc(userFunc, "f");
        }
    }
    
    return 0;
}
