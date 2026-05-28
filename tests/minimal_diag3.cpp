#include <cstdio>
#include "minimal_test.hpp"
#include "codegen/codegen.hpp"
using namespace cplang;

TEST(Minimal, Compile) {
    Compiler compiler;
    VMFunction* func = compiler.compile(u8"变量 x = 42;");
    EXPECT_NE(func, nullptr);
}

TEST_MAIN()
