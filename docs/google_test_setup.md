# Google Test 集成方案

## 方案 A: CMake FetchContent (推荐)

```cmake
# CMakeLists.txt 中添加
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
FetchContent_MakeAvailable(googletest)

# 测试 executable
add_executable(cplang_tests
  tests/gtest/test_vm.cpp
  tests/gtest/test_lexer.cpp
  tests/gtest/test_parser.cpp
)
target_link_libraries(cplang_tests PRIVATE
  GTest::gtest_main
  cplang_vm cplang_lexer cplang_parser cplang_codegen
)
enable_testing()
add_test(NAME cplang_tests COMMAND cplang_tests)
```

## 方案 B: 手动安装

```bash
git clone https://github.com/google/googletest.git
cd googletest && mkdir build && cd build
cmake .. -G Ninja && cmake --build .
cmake --install . --prefix C:/tools/googletest
```

## 测试文件骨架

```cpp
// tests/gtest/test_vm.cpp
#include <gtest/gtest.h>
#include "vm/vm.hpp"

TEST(VMTest, CreateAndDestroy) {
    cplang::VM vm;
    EXPECT_NE(vm.getGlobalsSize(), -1);
}

TEST(VMTest, BasicArithmetic) {
    cplang::VM vm;
    // 1 + 2 = 3
    std::vector<uint32_t> bytecode = {
        OP_LOADINT, 0, 0, 0, 0, 1, 0, 0, 0,
        OP_LOADINT, 0, 1, 0, 0, 2, 0, 0, 0,
        OP_ADD,      0, 0, 1, 0, 0, 0, 0, 0,
        OP_RETURN,   0, 0, 0, 0, 0, 0, 0, 0
    };
    auto result = vm.execute(bytecode);
    EXPECT_EQ(result.getInt(), 3);
}
```

## 待确认

1. **网络**：gtest 直连 GitHub 可能超时，需备国内镜像 URL
2. **Google Test 版本**：v1.15.2 需 C++14 以上
3. **集成方式**：FetchContent 最简单，无需预装