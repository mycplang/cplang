#include "codegen/codegen.hpp"
#include "vm/vm.hpp"
#include <cstdio>
using namespace cplang;

int main() {
    VM vm;
    
    // Register print (Chinese)
    vm.registerNative(u8"打印", [](std::vector<Value>& args) -> Value {
        printf("打印 called with %zu args\n", args.size());
        return Value::nil();
    });
    
    // Intern "打印" the same way codegen does
    VMString* name1 = vm.internString(u8"打印");
    printf("internString returned: %p\n", (void*)name1);
    printf("name1->data: %s\n", name1->data);
    
    return 0;
}
