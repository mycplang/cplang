// CP语言 语义分析测试
#include "common/types.hpp"
#include "lexer/lexer.hpp"
#include "lexer/token.hpp"
#include "parser/parser.hpp"
#include "ast/ast.hpp"
#include "semantic/semantic_analyzer.hpp"
#include <iostream>

using namespace cplang;

void testSemantic(const String& name, const String& source) {
    std::cout << "===== Test: " << name << " =====" << std::endl;
    
    Lexer lexer(source);
    Parser parser(&lexer);
    auto program = parser.parse();
    
    if (parser.hasError()) {
        std::cout << "Parser Error: " << parser.errorMessage() << std::endl;
        return;
    }
    
    SemanticAnalyzer analyzer;
    bool ok = analyzer.analyze(program);
    
    if (analyzer.hasError()) {
        std::cout << "Semantic Error: " << analyzer.errorMessage() << std::endl;
        analyzer.printErrors();
        std::cout << "Result: FAILED" << std::endl;
    } else {
        std::cout << "Result: OK" << std::endl;
    }
    
    std::cout << std::endl;
}

int main() {
    // Test 1: 基本变量和函数
    testSemantic("基本变量和函数", R"(
func add(int a, int b) -> int {
    int result = a + b;
    return result;
}
)");
    
    // Test 2: 类定义
    testSemantic("类定义", R"(
class Player {
    private:
        string name;
        int hp = 100;
        
    public:
        func attack(int damage) -> bool {
            if (damage > 10) {
                return true;
            }
            return false;
        }
        
        func getHp() -> int {
            return hp;
        }
}
)");
    
    // Test 3: 类型检查
    testSemantic("类型检查", R"(
func main() {
    int num = 42;
    float pi = 3.14159;
    bool flag = true;
    string msg = "Hello";
    
    float mixed = num + pi;
    string concat = msg + " World";
}
)");
    
    // Test 4: 控制流
    testSemantic("控制流", R"(
func loop(int n) -> int {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        if (i % 2 == 0) {
            sum = sum + i;
        } else {
            continue;
        }
    }
    return sum;
}
)");
    
    // Test 5: 完整程序
    testSemantic("完整程序", R"(
package game.core;

import std.io;
import std.math;

class Vector2 {
    private:
        float x;
        float y;
        
    public:
        func getX() -> float { return x; }
        func getY() -> float { return y; }
}

func main() {
    int count = 0;
    while (count < 10) {
        count = count + 1;
    }
    
    Vector2 pos;
    pos.x = 1.0;
    pos.y = 2.0;
}
)");
    
    std::cout << "===== All tests completed =====" << std::endl;
    return 0;
}