// LLVM代码生成测试

#include "codegen/llvm_codegen.hpp"
#include "parser/parser.hpp"
#include "lexer/lexer.hpp"
#include <iostream>
#include <fstream>

using namespace cplang;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "用法: " << argv[0] << " <cp源文件>\n";
        return 1;
    }
    
    // 读取源文件
    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "无法打开文件: " << argv[1] << "\n";
        return 1;
    }
    
    String source((std::istreambuf_iterator<char>(file)),
                   std::istreambuf_iterator<char>());
    
    std::cout << "=== CP源代码 ===\n";
    std::cout << source << "\n\n";
    
    // 词法分析
    auto lexer = std::make_unique<Lexer>(source);
    
    // 语法分析
    Parser parser(lexer.get());
    auto module = parser.parse();
    
    if (parser.hasError()) {
        std::cerr << "解析错误\n";
        // 打印所有token用于调试
        auto lexer2 = std::make_unique<Lexer>(source);
        Token tok;
        int n = 0;
        while (true) {
            tok = lexer2->nextToken();
            if (tok.type == TokenType::END_OF_FILE) break;
            std::cerr << "  [" << tok.line << "] " << (int)tok.type << " '" << tok.text << "'\n";
            if (++n > 80) { std::cerr << "  ...(truncated)\n"; break; }
        }
        return 1;
    }
    
    std::cout << "=== 生成LLVM IR ===\n";
    
    // LLVM代码生成
    LLVMCodegen codegen;
    String ir = codegen.generate(module);
    
    std::cout << ir << "\n";
    
    // 保存到文件
    String outFile = String(argv[1]) + ".ll";
    std::ofstream out(outFile);
    out << ir;
    out.close();
    
    std::cout << "LLVM IR已保存到: " << outFile << "\n";
    std::cout << "\n编译命令: llc " << outFile << " -o output.s\n";
    std::cout << "          clang output.s -o output.exe\n";
    
    return 0;
}
