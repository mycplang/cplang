#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "codegen/codegen.hpp"
#include "vm/vm.hpp"
#include "module/module_system.hpp"
#include "exception/exception_handler.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace cplang;

std::string readFile(const std::string& path) {
    std::ifstream t(path);
    std::stringstream buffer;
    buffer << t.rdbuf();
    return buffer.str();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "用法: cplang <源文件路径>" << std::endl;
        return 1;
    }

    std::string source = readFile(argv[1]);
    if (source.empty()) {
        std::cerr << "错误: 无法读取文件 " << argv[1] << std::endl;
        return 1;
    }

    try {
        // 词法分析
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        
        // 语法分析
        Parser parser(tokens);
        auto program = parser.parseProgram();
        
        // 语义分析
        SemanticAnalyzer analyzer;
        analyzer.analyze(program);
        
        // 代码生成
        Codegen codegen;
        auto vmProgram = codegen.compile(program);
        
        // 初始化虚拟机
        VM vm;
        registerSystem(&vm);
        registerStubImpls(&vm);
        
        // 运行程序
        ExecContext ctx;
        ctx.program = vmProgram.get();
        vm.run(&ctx);
        
    } catch (const std::exception& e) {
        std::cerr << "运行错误: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}