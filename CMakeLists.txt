cmake_minimum_required(VERSION 3.20)
project(cplang
    VERSION 1.0.0
    DESCRIPTION "CP语言 — 中文编程语言编译器与虚拟机"
    LANGUAGES C CXX
)

# ============================================================================
# 全局 C++ 标准
# ============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# Clang 18 < MSVC 2022 STL (requires Clang 19+), suppress version check
# ============================================================================
add_compile_definitions(_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH=1)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

# ============================================================================
# 选项
# ============================================================================
option(CPLANG_BUILD_TESTS   "构建测试"      ON)
option(CPLANG_BUILD_EXAMPLES "构建示例"      ON)
option(CPLANG_USE_LLVM      "启用 LLVM JIT"  ON)
option(CPLANG_ENABLE_SANITIZERS "启用 Sanitizers" OFF)
option(CPLANG_WARNINGS_AS_ERRORS "警告即错误" OFF)

# ============================================================================
# CMake 模块路径
# ============================================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ============================================================================
# 编译警告
# ============================================================================
include(CompilerWarnings)

# ============================================================================
# LLVM 查找
# ============================================================================
if(CPLANG_USE_LLVM)
    include(FindLLVM)
endif()

# ============================================================================
# Sanitizers（仅 Debug）
# ============================================================================
if(CPLANG_ENABLE_SANITIZERS)
    include(Sanitizers)
endif()

# ============================================================================
# 0. cplang_core — 运行时配置（verbose 状态）
# ============================================================================
add_library(cplang_core STATIC
    src/core/verbose.cpp
)
target_include_directories(cplang_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(cplang_core PUBLIC cplang_common)
set_target_warnings(cplang_core)

# ============================================================================
# 1. cplang_common — 基础类型（纯头文件，无依赖）
# ============================================================================
add_library(cplang_common INTERFACE)
target_include_directories(cplang_common INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_features(cplang_common INTERFACE cxx_std_17)

# ============================================================================
# 2. cplang_token — Token 类型（纯头文件）
# ============================================================================
add_library(cplang_token INTERFACE)
target_include_directories(cplang_token INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(cplang_token INTERFACE cplang_common)

# ============================================================================
# 3. cplang_lexer — 词法分析器
# ============================================================================
add_library(cplang_lexer STATIC
    src/lexer/lexer.cpp
)
target_include_directories(cplang_lexer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(cplang_lexer PUBLIC cplang_token)
set_target_warnings(cplang_lexer)

# ============================================================================
# 4. cplang_ast — AST 定义（纯头文件）
# ============================================================================
add_library(cplang_ast INTERFACE)
target_include_directories(cplang_ast INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(cplang_ast INTERFACE cplang_token cplang_common)

# ============================================================================
# 5. cplang_parser — 语法分析器（已拆分: 核心 + 声明 + 语句 + 表达式）
# ============================================================================
add_library(cplang_parser STATIC
    src/parser/parser.cpp
    src/parser/parser_decl.cpp
    src/parser/parser_stmt.cpp
    src/parser/parser_expr.cpp
)
target_include_directories(cplang_parser PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(cplang_parser PUBLIC cplang_ast cplang_lexer)
set_target_warnings(cplang_parser)

# ============================================================================
# 6. cplang_semantic — 语义分析器
# ============================================================================
add_library(cplang_semantic STATIC
    src/semantic/semantic_analyzer.cpp
)
target_include_directories(cplang_semantic PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(cplang_semantic PUBLIC cplang_ast cplang_lexer)
set_target_warnings(cplang_semantic)

# ============================================================================
# 7. cplang_vm — 虚拟机核心（已拆分: 核心 + 容器 + 对象 + 执行循环）
# ============================================================================
add_library(cplang_vm STATIC
    src/vm/vm.cpp
    src/vm/vm_containers.cpp
    src/vm/vm_objects.cpp
    src/vm/vm_exec.cpp
    src/vm/value.cpp
)

# vm_opt.cpp 使用 computed goto（GNU 扩展），MSVC 不支持
# MSVC 下使用 vm_opt_stub.cpp 提供所需符号
if(NOT MSVC)
    target_sources(cplang_vm PRIVATE src/vm/vm_opt.cpp)
else()
    target_sources(cplang_vm PRIVATE src/vm/vm_opt_stub.cpp)
endif()
target_include_directories(cplang_vm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(cplang_vm PUBLIC cplang_common)
set_target_warnings(cplang_vm)

# vm_exec.cpp 中包含条件 JIT 代码（#ifdef CPLANG_HAS_LLVM）
# 需要宏定义和 LLVM include 路径才能编译
if(CPLANG_USE_LLVM AND LLVM_FOUND)
    target_compile_definitions(cplang_vm PRIVATE CPLANG_HAS_LLVM=1)
    target_include_directories(cplang_vm SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
endif()

# ============================================================================
# 8. cplang_codegen — 代码生成（字节码 + LLVM IR）
# ============================================================================
add_library(cplang_codegen STATIC
    src/codegen/codegen.cpp
    src/codegen/codegen_opt.cpp
    src/codegen/codegen_stmt.cpp
    src/codegen/codegen_expr.cpp
    src/codegen/bytecode_optimizer.cpp
)

# aot_compiler.cpp 和 llvm_codegen.cpp 依赖 LLVM
if(CPLANG_USE_LLVM AND LLVM_FOUND)
    target_sources(cplang_codegen PRIVATE
        src/codegen/aot_compiler.cpp
        src/codegen/llvm_codegen.cpp
    )
    target_compile_definitions(cplang_codegen PRIVATE CPLANG_HAS_LLVM=1)
    target_include_directories(cplang_codegen SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
endif()
target_include_directories(cplang_codegen PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(cplang_codegen PUBLIC
    cplang_ast
    cplang_semantic
    cplang_vm
)
set_target_warnings(cplang_codegen)

# ============================================================================
# 9. cplang_optimizer — 优化器（纯 AST 变换，不依赖 JIT）
# ============================================================================
add_library(cplang_optimizer STATIC
    src/optimizer/optimizer.cpp
    src/optimizer/constant_folder.cpp
    src/optimizer/dead_code_eliminator.cpp
    src/optimizer/escape_analyzer.cpp
    src/optimizer/function_inliner.cpp
    src/optimizer/loop_unroller.cpp
    src/optimizer/tail_recursion_optimizer.cpp
    src/optimizer/llvm_optimizer.cpp
)
target_include_directories(cplang_optimizer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(cplang_optimizer PUBLIC
    cplang_ast
)
set_target_warnings(cplang_optimizer)

# ============================================================================
# 10. cplang_jit — JIT 编译（依赖 LLVM）
# ============================================================================
add_library(cplang_jit STATIC
    src/jit/jit_compiler.cpp
    src/jit/jit_runtime.cpp
    src/jit/orc_jit.cpp
    src/jit/jit_dispatch.cpp
    src/jit/jit_dispatch.cpp
    src/jit/hybrid_jit.cpp
)
target_include_directories(cplang_jit PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(cplang_jit PUBLIC
    cplang_vm
    cplang_codegen
)
if(CPLANG_USE_LLVM AND LLVM_FOUND)
    target_include_directories(cplang_jit SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS})
    target_compile_definitions(cplang_jit PUBLIC CPLANG_HAS_LLVM=1)
endif()
set_target_warnings(cplang_jit)

# ============================================================================
# 11. cplang_module — 模块系统
# ============================================================================
add_library(cplang_module STATIC
    src/module/module_system.cpp
)
target_include_directories(cplang_module PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(cplang_module PUBLIC cplang_vm cplang_codegen)
set_target_warnings(cplang_module)

# ============================================================================
# 12. cplang_stdlib — 标准库
# ============================================================================
add_library(cplang_stdlib STATIC
    src/stdlib/stdlib.cpp
    src/stdlib/stdlib_stubs.cpp
    src/stdlib/stdlib_raylib.cpp
    src/stdlib/stdlib_imgui.cpp
    src/stdlib/stdlib_fix_missing.cpp
    # 独立模块实现（从 stdlib.cpp #include 拼接中提取）
    src/stdlib/stdlib_aes.cpp
    src/stdlib/stdlib_algo_bitwise.cpp
    src/stdlib/stdlib_algo_ext.cpp
    src/stdlib/stdlib_algo_missing.cpp
    src/stdlib/stdlib_array.cpp
    src/stdlib/stdlib_array_file_more.cpp
    src/stdlib/stdlib_bitset.cpp
    src/stdlib/stdlib_charset.cpp
    src/stdlib/stdlib_complex_pair.cpp
    src/stdlib/stdlib_containers.cpp
    src/stdlib/stdlib_crypto_impl.cpp
    src/stdlib/stdlib_crypto_plus.cpp
    src/stdlib/stdlib_encoding.cpp
    src/stdlib/stdlib_ffi.cpp
    src/stdlib/stdlib_file.cpp
    src/stdlib/stdlib_file_log.cpp
    src/stdlib/stdlib_heap.cpp
    src/stdlib/stdlib_io.cpp
    src/stdlib/stdlib_iterator.cpp
    src/stdlib/stdlib_json_http.cpp
    src/stdlib/stdlib_map.cpp
    src/stdlib/stdlib_math.cpp
    src/stdlib/stdlib_math_special.cpp
    src/stdlib/stdlib_matrix_color_path_console.cpp
    src/stdlib/stdlib_misc_modules.cpp
    src/stdlib/stdlib_net_ws_sql.cpp
    src/stdlib/stdlib_numeric_limits.cpp
    src/stdlib/stdlib_p1_enhance.cpp
    src/stdlib/stdlib_p2_more.cpp
    src/stdlib/stdlib_reflect.cpp
    src/stdlib/stdlib_regex.cpp
    src/stdlib/stdlib_stats_utils.cpp
    src/stdlib/stdlib_str_search.cpp
    src/stdlib/stdlib_string.cpp
    src/stdlib/stdlib_string_ext.cpp
    src/stdlib/stdlib_string_more_impl.cpp
    src/stdlib/stdlib_table.cpp
    src/stdlib/stdlib_threading.cpp
    src/stdlib/stdlib_time_sys_more.cpp
    src/stdlib/stdlib_time_system.cpp
    src/stdlib/stdlib_types_net.cpp
    src/stdlib/stdlib_variant_utils.cpp
    src/miniz.c
    src/miniz_tdef.c
    src/miniz_tinfl.c
    src/crypto/md5_impl.cpp
    src/sqlite/sqlite3.c
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/rlImGui.cpp
)
# Windows 专用模块
if(WIN32)
    target_sources(cplang_stdlib PRIVATE
        src/stdlib/stdlib_db.cpp
        src/stdlib/stdlib_redis.cpp
        src/stdlib/stdlib_r10_r11.cpp
        src/stdlib/stdlib_http.cpp
        src/stdlib/stdlib_p3_util.cpp
    )
endif()
target_include_directories(cplang_stdlib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/src/external/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
)
target_compile_definitions(cplang_stdlib PRIVATE 
    GRAPHICS_API_OPENGL_33 PLATFORM_DESKTOP NO_FONT_AWESOME
)
target_link_libraries(cplang_stdlib PUBLIC cplang_vm)
set_target_warnings(cplang_stdlib)

# ============================================================================
# 13. cplang_debug — 调试器
# ============================================================================
add_library(cplang_debug STATIC
    src/debug/debugger.cpp
)
target_include_directories(cplang_debug PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(cplang_debug PUBLIC cplang_vm)
set_target_warnings(cplang_debug)

# ============================================================================
# 14. cplang_exception — 异常处理
# ============================================================================
add_library(cplang_exception STATIC
    src/exception/exception_handler.cpp
)
target_include_directories(cplang_exception PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(cplang_exception PUBLIC cplang_vm)
set_target_warnings(cplang_exception)

# ============================================================================
# 共享库路径配置
# ============================================================================
set(RAYLIB_LIB "${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/build_release/raylib/Release/raylib.lib")
# LLVM 库路径（由 FindLLVM.cmake 提供，通过 llvm-config --libfiles 获取）
if(CPLANG_USE_LLVM AND LLVM_FOUND)
    set(LLVM_LIB_FILES ${LLVM_LIBRARIES})
    list(LENGTH LLVM_LIB_FILES _LLVM_COUNT)
    message(STATUS "LLVM 库数量: ${_LLVM_COUNT}")
endif()
# 系统库（两个可执行目标共享）
set(SYSTEM_LIBS ws2_32 winhttp urlmon opengl32 gdi32 winmm ole32 comctl32 user32 shell32)

# ============================================================================
# 15. cplang_cli — 命令行入口（可执行文件，main.cpp）
# ============================================================================
add_executable(cplang_cli
    src/main.cpp
    src/repl.cpp
)
target_include_directories(cplang_cli PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(cplang_cli PRIVATE
    cplang_core
    cplang_lexer
    cplang_parser
    cplang_semantic
    cplang_codegen
    cplang_vm
    cplang_stdlib
    cplang_module
    cplang_optimizer
    cplang_jit
    cplang_debug
    cplang_exception
    ${LLVM_LIB_FILES}
    ${RAYLIB_LIB}
    ${SYSTEM_LIBS}
)
set_target_warnings(cplang_cli)
set_target_properties(cplang_cli PROPERTIES
    OUTPUT_NAME cplang
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LINK_FLAGS "/FORCE:MULTIPLE"
    LINK_FLAGS "/FORCE:MULTIPLE"
)

# ============================================================================
# 15b. cplang_cpkg — 包管理器（独立 exe）
# ============================================================================
add_executable(cplang_cpkg
    src/cpkg/cpkg.cpp
)
target_include_directories(cplang_cpkg PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(cplang_cpkg PRIVATE
    ${SYSTEM_LIBS}
)
set_target_warnings(cplang_cpkg)
set_target_properties(cplang_cpkg PROPERTIES
    OUTPUT_NAME cpkg
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LINK_FLAGS "/FORCE:MULTIPLE"
    LINK_FLAGS "/FORCE:MULTIPLE"
)

# ============================================================================
# 15c. cplang_repl — 交互式 REPL（repl.cpp，独立入口）
# ============================================================================
add_executable(cplang_repl
    src/repl.cpp
)
target_include_directories(cplang_repl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(cplang_repl PRIVATE
    cplang_core
    cplang_lexer
    cplang_parser
    cplang_semantic
    cplang_codegen
    cplang_vm
    cplang_stdlib
    cplang_module
    cplang_optimizer
    cplang_jit
    cplang_debug
    cplang_exception
    ${LLVM_LIB_FILES}
    ${RAYLIB_LIB}
    ${SYSTEM_LIBS}
)
set_target_warnings(cplang_repl)
set_target_properties(cplang_repl PROPERTIES
    OUTPUT_NAME cplang_repl
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LINK_FLAGS "/FORCE:MULTIPLE"
    LINK_FLAGS "/FORCE:MULTIPLE"
)

# ============================================================================
# 16. cplang_single — 单文件整合入口（可选）
# ============================================================================
# add_executable(cplang_single
#     src/cplang_build.cpp
# )
# target_include_directories(cplang_single PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
# target_link_libraries(cplang_single PRIVATE
#     cplang_lexer cplang_parser cplang_semantic cplang_codegen
#     cplang_vm cplang_stdlib cplang_optimizer cplang_jit
#     cplang_module cplang_debug cplang_exception
# )
# set_target_warnings(cplang_single)

# ============================================================================
# Examples（可选）
# ============================================================================
if(CPLANG_BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples")
    file(GLOB EXAMPLE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp")
    foreach(example_src ${EXAMPLE_SOURCES})
        get_filename_component(example_name ${example_src} NAME_WE)
        add_executable(example_${example_name} ${example_src})
        target_include_directories(example_${example_name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        )
        target_link_libraries(example_${example_name} PRIVATE
            cplang_core
            cplang_lexer cplang_parser cplang_semantic cplang_codegen
            cplang_vm cplang_stdlib cplang_optimizer cplang_jit
            cplang_module cplang_debug cplang_exception
        )
        set_target_warnings(example_${example_name})
        set_target_properties(example_${example_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
        )
    endforeach()
endif()

# ============================================================================
# Tests (Google Test 集成)
# ============================================================================
if (CPLANG_BUILD_TESTS)
    enable_testing()
    
    # 使用 FetchContent 获取 Google Test
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    )
    
    # 防止 Google Test 覆盖父项目的编译器/链接器设置
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    
    # 测试可执行文件
    add_executable(cplang_tests
        tests/gtest/test_lexer.cpp
        tests/gtest/test_parser.cpp
        tests/gtest/test_vm.cpp
        tests/gtest/test_optimizer.cpp
        tests/gtest/test_semantic.cpp
        tests/gtest/test_e2e.cpp
    )

    target_link_libraries(cplang_tests PRIVATE
        GTest::gtest_main
        cplang_core
        cplang_lexer
        cplang_parser
        cplang_semantic
        cplang_vm
        cplang_codegen
        cplang_stdlib
        cplang_module
        cplang_optimizer
        ${SYSTEM_LIBS}
    )
    
    target_include_directories(cplang_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    
    set_target_warnings(cplang_tests)
    
    # 注册测试
    include(GoogleTest)
    gtest_discover_tests(cplang_tests)

    # 添加 CP 语言集成测试（直接运行 cplang.exe 执行 .cp 测试文件）
    add_test(NAME test_stdlib_new
        COMMAND cplang_cli -c "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_new_stdlib.cp"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
    )

    message(STATUS "测试构建已启用 (Google Test 已集成)")
endif()

# ============================================================================
# 安装规则
# ============================================================================
include(GNUInstallDirs)

install(TARGETS cplang_cli cplang_cpkg cplang_repl
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(TARGETS
    cplang_lexer cplang_parser cplang_semantic cplang_vm
    cplang_codegen cplang_optimizer cplang_jit
    cplang_module cplang_stdlib cplang_debug cplang_exception
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cplang
    FILES_MATCHING PATTERN "*.hpp"
)

# ============================================================================
# IDE 支持
# ============================================================================
# 为 Visual Studio / CLion 生成文件夹分组
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_target_properties(cplang_core   PROPERTIES FOLDER "cplang/00-core")
set_target_properties(cplang_common PROPERTIES FOLDER "cplang/00-core")
set_target_properties(cplang_token  PROPERTIES FOLDER "cplang/00-core")
set_target_properties(cplang_lexer  PROPERTIES FOLDER "cplang/01-frontend")
set_target_properties(cplang_ast    PROPERTIES FOLDER "cplang/00-core")
set_target_properties(cplang_parser PROPERTIES FOLDER "cplang/01-frontend")
set_target_properties(cplang_semantic PROPERTIES FOLDER "cplang/01-frontend")
set_target_properties(cplang_vm     PROPERTIES FOLDER "cplang/02-backend")
set_target_properties(cplang_codegen PROPERTIES FOLDER "cplang/02-backend")
set_target_properties(cplang_optimizer PROPERTIES FOLDER "cplang/03-optimizer")
set_target_properties(cplang_jit    PROPERTIES FOLDER "cplang/03-jit")
set_target_properties(cplang_module PROPERTIES FOLDER "cplang/04-runtime")
set_target_properties(cplang_stdlib PROPERTIES FOLDER "cplang/04-runtime")
set_target_properties(cplang_debug  PROPERTIES FOLDER "cplang/04-runtime")
set_target_properties(cplang_exception PROPERTIES FOLDER "cplang/04-runtime")
set_target_properties(cplang_cli    PROPERTIES FOLDER "cplang/99-cli")
set_target_properties(cplang_repl   PROPERTIES FOLDER "cplang/99-cli")
