// 逃逸分析演示
// 展示逃逸分析的效果

#include <iostream>
#include "optimizer/escape_analyzer.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_analyzer.hpp"

using namespace cplang;

// 测试用例
struct EscapeTest {
    String name;
    String source;
    String expected;
};

int main() {
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║        逃逸分析演示                                 ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
    
    std::vector<EscapeTest> tests = {
        {
            "不逃逸的变量",
            R"(
                函数 test() {
                    整数 a = 10;
                    整数 b = 20;
                    返回 a + b;
                }
            )",
            "栈上分配"
        },
        {
            "作为返回值",
            R"(
                函数 create_array() {
                    整数数组 arr[10];
                    返回 arr;
                }
            )",
            "堆上分配（逃逸）"
        },
        {
            "作为参数传递",
            R"(
                函数 modify(整数数组 arr) {
                    arr[0] = 1;
                }
                函数 test() {
                    整数数组 arr[10];
                    modify(arr);
                }
            )",
            "堆上分配（逃逸）"
        },
        {
            "局部循环变量",
            R"(
                函数 test() {
                    整数 sum = 0;
                    对于 整数 i = 0; i < 100; i = i + 1 则 {
                        sum = sum + i;
                    }
                    返回 sum;
                }
            )",
            "栈上分配"
        },
        {
            "结构体返回值",
            R"(
                结构体 Point { 整数 x; 整数 y; }
                函数 create() {
                    Point p;
                    p.x = 10;
                    返回 p;
                }
            )",
            "堆上分配（逃逸）"
        }
    };
    
    EscapeAnalyzer analyzer;
    
    std::cout << "逃逸分析测试:\n";
    std::cout << "════════════════════════════════════════════════════\n\n";
    
    for (const auto& test : tests) {
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "【" << test.name << "】\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "预期: " << test.expected << "\n\n";
        
        // 编译
        Lexer lexer;
        auto tokens = lexer.tokenize(test.source);
        
        Parser parser;
        auto program = parser.parse(tokens);
        
        SemanticAnalyzer semantic;
        semantic.analyze(program);
        
        // 逃逸分析
        auto result = analyzer.analyze(program);
        
        // 打印结果
        analyzer.printResult(result);
        
        std::cout << "\n";
    }
    
    // 性能对比
    std::cout << "\n\n╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║        性能对比                                      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "不使用逃逸分析:\n";
    std::cout << "  - 所有数组/结构体默认堆分配\n";
    std::cout << "  - GC 压力增大\n";
    std::cout << "  - 性能下降 20-40%\n\n";
    
    std::cout << "使用逃逸分析:\n";
    std::cout << "  - 不逃逸的对象栈上分配\n";
    std::cout << "  - 减少 GC 压力\n";
    std::cout << "  - 性能提升 20-40%\n\n";
    
    std::cout << "性能提升示例:\n";
    std::cout << "  - 短期存活对象（如循环计数器）: 避免 malloc/free\n";
    std::cout << "  - 局部数组（不返回）: 栈上分配，无 GC 开销\n";
    std::cout << "  - 临时结构体（不逃逸）: 栈上分配，零拷贝\n\n";
    
    // 逃逸级别说明
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║        逃逸级别说明                                  ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "EscapeLevel::None (栈上分配):\n";
    std::cout << "  - 变量只在当前作用域使用\n";
    std::cout << "  - 不作为参数传递\n";
    std::cout << "  - 不作为返回值\n\n";
    
    std::cout << "EscapeLevel::Arg (参数逃逸):\n";
    std::cout << "  - 作为函数参数传递\n";
    std::cout << "  - 需要堆分配或特殊处理\n\n";
    
    std::cout << "EscapeLevel::Return (返回值逃逸):\n";
    std::cout << "  - 作为函数返回值\n";
    std::cout << "  - 需要堆分配\n\n";
    
    std::cout << "EscapeLevel::Global (全局逃逸):\n";
    std::cout << "  - 赋值给全局变量\n";
    std::cout << "  - 必须堆分配\n\n";
    
    std::cout << "EscapeLevel::Heap (堆逃逸):\n";
    std::cout << "  - 逃逸到堆\n";
    std::cout << "  - 交给 GC 管理\n";
    
    std::cout << "\n完成！\n";
    return 0;
}
