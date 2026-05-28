#include <iostream>
#include <fstream>
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "codegen/codegen.hpp"

using namespace cplang;

int main() {
    std::ifstream ifs("tests/simple_module.cp");
    String source((std::istreambuf_iterator<char>(ifs)),
                  std::istreambuf_iterator<char>());
    
    Compiler compiler;
    VMFunction* func = compiler.compile(source);
    
    if (!func) {
        std::cout << "编译失败: " << compiler.errorMessage() << "\n";
        return 1;
    }
    
    std::cout << "模块字节码（" << func->code.size() << " 字节）:\n";
    for (size_t i = 0; i < func->code.size(); i++) {
        printf("%02X ", func->code[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    std::cout << "\n\n常量池（" << func->constants.size() << " 个）:\n";
    for (size_t i = 0; i < func->constants.size(); i++) {
        std::cout << "[" << i << "]: " << func->constants[i].toString() << "\n";
    }
    
    return 0;
}
