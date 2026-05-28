// CP语言 测试入口
// 所有测试用例通过 #include 包含至此文件
// 使用最小化测试框架（minimal_test.hpp）

#include "minimal_test.hpp"

// ===== 词法分析器测试 =====
#include "gtest/test_lexer.cpp"

// ===== 语法分析器测试 =====
#include "gtest/test_parser.cpp"

// ===== 优化器测试 =====
#include "gtest/test_optimizer.cpp"

// ===== 虚拟机测试 =====
#include "gtest/test_vm.cpp"

// ===== 语义分析测试 =====
#include "gtest/test_semantic.cpp"

// ===== 端到端测试 =====
#include "gtest/test_e2e.cpp"

TEST_MAIN()
