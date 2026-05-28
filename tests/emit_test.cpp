#include "codegen/codegen.hpp"
#include "vm/vm.hpp"
#include <cstdio>
using namespace cplang;

int main() {
    printf("OP_RETURN from vm.hpp = %d\n", (int)OP_RETURN);
    
    // Create a minimal codegen and emit RETURN
    VM vm;
    Codegen cg(&vm, nullptr);
    
    // Manually test emit
    std::vector<UInt8> code;
    code.push_back(OP_RETURN);
    code.push_back(0);
    code.push_back(0);
    code.push_back(0);
    
    printf("Bytecode: ");
    for (auto b : code) printf("%d ", (int)b);
    printf("\n");
    
    return 0;
}
