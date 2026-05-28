#include "codegen/codegen.hpp"
#include "vm/vm.hpp"
#include <cstdio>
#include <string>
using namespace cplang;

int main() {
    // Create compiler
    Compiler compiler;
    
    // Compile a simple source
    const char* src = u8"打印(42);";
    VMFunction* func = compiler.compile(src);
    
    if (!func) {
        printf("Compile failed!\n");
        return 1;
    }
    
    printf("Bytecode size: %zu\n", func->code.size());
    printf("Constants: %zu\n", func->constants.size());
    
    for (size_t i = 0; i < func->constants.size(); i++) {
        const Value& v = func->constants[i];
        printf("  const[%zu]: tag=%d ", i, v.tag);
        if (v.tag == 4 && v.obj) {
            VMString* s = reinterpret_cast<VMString*>(v.obj);
            printf("string='%s' ptr=%p\n", s->data, (void*)s);
        } else if (v.tag == 2) {
            printf("int=%lld\n", v.asInt());
        } else {
            printf("obj=%p\n", (void*)v.obj);
        }
    }
    
    // Check if the string is interned
    VM* vm = compiler.vm();
    VMString* interned = vm->internString(u8"打印");
    printf("\ninterned '打印': %p\n", (void*)interned);
    
    return 0;
}
