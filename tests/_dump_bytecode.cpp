#include "codegen/codegen.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include <iostream>
#include <iomanip>
#include <vector>

using namespace cplang;

static void dumpBytecode(VMFunction* func, const char* label) {
    std::cout << "=== " << label << " ===" << std::endl;
    std::cout << "Bytecode size: " << func->code.size() << " bytes" << std::endl;
    const size_t instSize = 16;
    for (size_t i = 0; i < func->code.size(); i += instSize) {
        std::cout << "  [" << std::setw(3) << (i/instSize) << "] ";
        for (size_t j = 0; j < instSize && i+j < func->code.size(); j++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                     << (int)func->code[i+j] << " ";
        }
        std::cout << std::dec << std::endl;
    }
}

int main() {
    std::string source = R"(
打印("abs(-10) = ", abs(-10));
打印("abs(-3.14) = ", abs(-3.14));
)";
    
    Lexer lexer(source);
    Parser parser(&lexer);
    auto program = parser.parse();
    if (!program) {
        std::cerr << "Parse failed: " << parser.errorMessage() << std::endl;
        return 1;
    }
    
    SemanticAnalyzer analyzer;
    if (!analyzer.analyze(program)) {
        std::cerr << "Semantic failed: " << analyzer.errorMessage() << std::endl;
        return 1;
    }
    
    Compiler compiler;
    compiler.setEnableBytecodeOpt(false);
    VMFunction* func = compiler.compile(source);
    if (!func) {
        std::cerr << "Compile failed: " << compiler.errorMessage() << std::endl;
        return 1;
    }
    //dumpBytecode(func, "Unoptimized");
    
    // Now compile with optimization
    Compiler compiler2;
    compiler2.setEnableBytecodeOpt(true);
    VMFunction* func2 = compiler2.compile(source);
    if (!func2) {
        std::cerr << "Compile2 failed: " << compiler2.errorMessage() << std::endl;
        return 1;
    }
    dumpBytecode(func, "Unoptimized");
    dumpBytecode(func2, "Optimized");
    
    return 0;
}
