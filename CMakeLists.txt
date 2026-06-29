cmake_minimum_required(VERSION 3.20)
project(cplang
    VERSION 0.9.0
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
# MSVC 特定：允许编译器/STL 版本不匹配
if(MSVC)
    add_compile_definitions(_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH=1)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    # UTF-8 源码编码：确保中文别名等字符串字面量按 UTF-8 编译
    # 与 .cp 源码的词法分析器编码保持一致，防止函数名匹配失败
    add_compile_options("/utf-8")
endif()

# Linux 特定配置
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    find_package(Threads REQUIRED)
    # Raylib Linux 依赖
    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)
endif()

# Android 特定配置（使用 NDK 交叉编译时自动检测）
if(ANDROID)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(CPLANG_USE_LLVM OFF)  # Android 上 JIT 需要额外工作
    # 使用 OpenGL ES 替代 OpenGL
    add_compile_definitions(GRAPHICS_API_OPENGL_ES2 PLATFORM_ANDROID)
    # 关闭测试和示例（Android 上通常不需要）
    set(CPLANG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(CPLANG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    message(STATUS "🔧 Android 构建模式: ABI=${ANDROID_ABI}, API=${ANDROID_PLATFORM}")
endif()

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
# 0. cplang_platform — 平台抽象层（跨平台 API）
# ============================================================================
add_library(cplang_platform STATIC
)
if(WIN32)
    target_sources(cplang_platform PRIVATE src/platform/platform_win.cpp)
    target_sources(cplang_platform PRIVATE src/platform/platform_iopoll_win.cpp)
else()
    target_sources(cplang_platform PRIVATE src/platform/platform_linux.cpp)
    target_sources(cplang_platform PRIVATE src/platform/platform_iopoll_linux.cpp)
endif()
target_include_directories(cplang_platform PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(cplang_platform PUBLIC cplang_common)
set_target_warnings(cplang_platform)

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
    src/vm/event_loop.cpp
    src/vm/value.cpp
)

# vm_opt.cpp 使用 computed goto（GNU 扩展），MSVC 不支持
# MSVC 下使用 vm_opt_stub.cpp 提供所需符号；GCC/Clang 下直接使用 vm_opt.cpp
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
    src/codegen/codegen_stmt.cpp
    src/codegen/codegen_match.cpp
    src/codegen/codegen_expr.cpp
    src/codegen/bytecode_optimizer.cpp
)

# LLVM IR 生成器（用于 JIT）
if(CPLANG_USE_LLVM AND LLVM_FOUND)
    target_sources(cplang_codegen PRIVATE
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
# 10. cplang_jit — JIT 编译（依赖 LLVM，仅 LLVM 可用时才构建）
# ============================================================================
if(CPLANG_USE_LLVM AND LLVM_FOUND)
    add_library(cplang_jit STATIC
        src/jit/jit_compiler.cpp
        src/jit/jit_runtime.cpp
        src/jit/orc_jit.cpp
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
    target_include_directories(cplang_jit SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS})
    target_compile_definitions(cplang_jit PUBLIC CPLANG_HAS_LLVM=1)
    set_target_warnings(cplang_jit)
else()
    # LLVM 不可用时使用 stub 提供所需符号
    add_library(cplang_jit STATIC
        src/jit/jit_dispatch.cpp
        src/jit/hybrid_jit_stub.cpp
    )
    target_include_directories(cplang_jit PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(cplang_jit PUBLIC
        cplang_vm
        cplang_codegen
    )
    set_target_warnings(cplang_jit)
endif()



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
    src/stdlib/stdlib_p5_algo.cpp
    src/stdlib/stdlib_p11_algo.cpp
    src/stdlib/stdlib_p11_graph.cpp
    src/stdlib/stdlib_p11_ds.cpp
    src/stdlib/stdlib_p11_utils.cpp
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
    src/stdlib/stdlib_tcp.cpp
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
    src/stdlib/stdlib_more_utils_extra.cpp
    src/stdlib/stdlib_iopoll.cpp
    src/stdlib/stdlib_image.cpp
    src/stdlib/stdlib_compress.cpp
    src/stdlib/stdlib_argparse.cpp
    src/stdlib/stdlib_config.cpp
    src/stdlib/stdlib_httpserver.cpp
    src/stdlib/stdlib_audio.cpp
    src/stdlib/stdlib_markdown.cpp
        src/stdlib/stdlib_template.cpp
    src/stdlib/stdlib_pdf.cpp
    src/stdlib/stdlib_kvdb.cpp
    src/stdlib/stdlib_testing.cpp
    src/stdlib/stdlib_logging.cpp
    src/stdlib/stdlib_lang.cpp
    src/stdlib/stdlib_async.cpp
    src/stdlib/stdlib_binary.cpp
    src/crypto/md5_impl.cpp
    third_party/miniz/miniz.c
    third_party/miniz/miniz_tdef.c
    third_party/miniz/miniz_tinfl.c
    third_party/miniz/miniz_zip.c
    third_party/sqlite/sqlite3.c
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/rlImGui.cpp
    # Raylib 核心源码（直接编译，避免 OBJECT 库在 NMake 下被跳过）
    third_party/raylib/src/rcore.c
    third_party/raylib/src/rshapes.c
    third_party/raylib/src/rtextures.c
    third_party/raylib/src/rtext.c
    third_party/raylib/src/rmodels.c
    third_party/raylib/src/raudio.c
    third_party/raylib/src/rglfw.c
)
# Windows 专用模块
if(WIN32)
    target_sources(cplang_stdlib PRIVATE
        src/stdlib/stdlib_db.cpp
        src/stdlib/stdlib_redis.cpp
        src/stdlib/stdlib_mongo.cpp
        src/stdlib/stdlib_r10_r11.cpp
        src/stdlib/stdlib_http.cpp
        src/stdlib/stdlib_p3_util.cpp
    )
endif()
target_include_directories(cplang_stdlib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/src/external/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
)
target_compile_definitions(cplang_stdlib PRIVATE 
    GRAPHICS_API_OPENGL_21 PLATFORM_DESKTOP NO_FONT_AWESOME
)
target_link_libraries(cplang_stdlib PUBLIC cplang_vm cplang_platform)
set_target_warnings(cplang_stdlib)

# ============================================================================
# 12b. cplang_stdlib_core — 核心标准库（轻量运行时，不含图形/数据库等重模块）
# ============================================================================
add_library(cplang_stdlib_core STATIC
    src/stdlib/stdlib_core.cpp
)
target_include_directories(cplang_stdlib_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(cplang_stdlib_core PUBLIC cplang_vm cplang_platform)
set_target_warnings(cplang_stdlib_core)

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
# 14b. cplang_game_engine — 游戏引擎标准库 (v0.4.0，开箱即用)
# ============================================================================
add_library(cplang_game_engine OBJECT
    # 核心引擎实现
    src/game/game_config.cpp
    # game_office.cpp -- xlsx needs amalgamated miniz
    # 标准库注册层 (8个模块)
    src/stdlib/stdlib_game_net.cpp
    src/stdlib/stdlib_game_battle.cpp
    src/stdlib/stdlib_game_map.cpp
    src/stdlib/stdlib_game_item.cpp
    src/stdlib/stdlib_game_role.cpp
    src/stdlib/stdlib_game_sprite.cpp
    src/stdlib/stdlib_game_db.cpp
    src/stdlib/stdlib_game_config.cpp
    src/stdlib/stdlib_web.cpp
)
target_include_directories(cplang_game_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/miniz
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/src/external/glfw/include
)
target_link_libraries(cplang_game_engine INTERFACE
    cplang_vm
    cplang_platform
    cplang_common
    cplang_core
    cplang_stdlib
)
set_target_warnings(cplang_game_engine)

# ============================================================================
# 共享库路径配置
# ============================================================================
# Raylib 库路径（分平台）
if(WIN32)
    set(RAYLIB_LIB "${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/build_release/raylib/raylib.lib")
else()
    set(RAYLIB_LIB "${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/build_release/raylib/libraylib.a")
endif()

# LLVM 库路径（由 FindLLVM.cmake 提供，通过 llvm-config --libfiles 获取）
if(CPLANG_USE_LLVM AND LLVM_FOUND)
    set(LLVM_LIB_FILES ${LLVM_LIBRARIES})
    list(LENGTH LLVM_LIB_FILES _LLVM_COUNT)
    message(STATUS "LLVM 库数量: ${_LLVM_COUNT}")
endif()

# 系统库（分平台）
if(WIN32)
    set(SYSTEM_LIBS ws2_32 winhttp urlmon opengl32 gdi32 winmm ole32 comctl32 user32 shell32)
elseif(ANDROID)
    set(SYSTEM_LIBS android log EGL GLESv3)
else()
    set(SYSTEM_LIBS pthread dl GL X11 asound)
endif()

# ============================================================================
# 14c. Raylib 编译选项 — raylib 源码已移入 cplang_stdlib，此处配置编译抑制
# ============================================================================
# raylib C 源码需要特定的编译定义和警告抑制
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/src/rcore.c
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/src/rshapes.c
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/src/rtextures.c
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/src/rtext.c
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/src/rmodels.c
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/src/raudio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/src/rglfw.c
    PROPERTIES
        COMPILE_DEFINITIONS "PLATFORM_DESKTOP;GRAPHICS_API_OPENGL_33;_DEFAULT_SOURCE;_GNU_SOURCE"
)
if(MSVC)
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/src/rcore.c
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/src/rshapes.c
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/src/rtextures.c
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/src/rtext.c
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/src/rmodels.c
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/src/raudio.c
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib/src/rglfw.c
        PROPERTIES COMPILE_OPTIONS "/W0;/wd4201;/wd4100;/wd4189;/wd4244;/wd4267;/wd4456;/wd4457;/wd4458;/wd4459"
    )
endif()

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
    cplang_platform
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
     $<TARGET_OBJECTS:cplang_game_engine>
     ${LLVM_LIB_FILES}
     ${SYSTEM_LIBS}
)
# /OPT:NOREF keeps ALL code including raylib+game_engine (function-pointer registration needs this)
# raylib 已移入 cplang_stdlib，不再需要单独 OBJECT 库
if(MSVC)
    target_link_options(cplang_cli PRIVATE "/OPT:NOREF")
endif()
set_target_warnings(cplang_cli)
set_target_properties(cplang_cli PROPERTIES
    OUTPUT_NAME cplang
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
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
)
# cpkg 只链接 SYSTEM_LIBS，无重复符号，/FORCE:MULTIPLE 不需要

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
     $<TARGET_OBJECTS:cplang_game_engine>
     ${LLVM_LIB_FILES}
     ${SYSTEM_LIBS}
)
# /OPT:NOREF keeps ALL code in OBJECT files (raylib+game_engine need this for function-pointer registration)
# raylib 已移入 cplang_stdlib，不再需要单独 OBJECT 库
if(MSVC)
    target_link_options(cplang_repl PRIVATE "/OPT:NOREF")
endif()
set_target_warnings(cplang_repl)
target_compile_definitions(cplang_repl PRIVATE CPLANG_REPL_STANDALONE)
set_target_properties(cplang_repl PROPERTIES
    OUTPUT_NAME cplang_repl
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
# P0修复: repl.cpp standalone main 已条件编译保护，移除 /FORCE:MULTIPLE

# ============================================================================
# 15d. cplsp — LSP 语言服务器（语义级补全/跳转/悬停）
# ============================================================================
add_executable(cplsp
    tools/cplsp/cplsp_main.cpp
    tools/cplsp/main.cpp
    tools/cplsp/document_manager.cpp
    tools/cplsp/lsp_diagnostics.cpp
    tools/cplsp/lsp_completion.cpp
    tools/cplsp/lsp_definition.cpp
    tools/cplsp/lsp_hover.cpp
    tools/cplsp/lsp_references.cpp
)
target_include_directories(cplsp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/cplsp
)
target_link_libraries(cplsp PRIVATE
    cplang_lexer cplang_parser cplang_semantic cplang_ast cplang_token cplang_common
)
target_compile_features(cplsp PRIVATE cxx_std_17)
set_target_warnings(cplsp)
set_target_properties(cplsp PROPERTIES
    OUTPUT_NAME cplsp
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ============================================================================
# 16. cplang_android — Android NativeActivity 共享库（仅 Android）
# ============================================================================
if(ANDROID)
    add_library(cplang_android SHARED
        src/android/android_main.cpp
    )
    target_include_directories(cplang_android PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(cplang_android PRIVATE
        cplang_platform
        cplang_core
        cplang_lexer
        cplang_parser
        cplang_semantic
        cplang_codegen
        cplang_vm
        cplang_stdlib
        cplang_module
        ${SYSTEM_LIBS}
    )
    set_target_warnings(cplang_android)
endif()

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
    
    # 测试可执行文件（使用自带轻量测试框架，无外部依赖）
    add_executable(cplang_tests
        tests/gtest/test_main.cpp
        tests/gtest/test_lexer.cpp
        tests/gtest/test_parser.cpp
        tests/gtest/test_vm.cpp
        tests/gtest/test_optimizer.cpp
        tests/gtest/test_semantic.cpp
        tests/gtest/test_e2e.cpp
        tests/gtest/test_stdlib.cpp
    )

    target_link_libraries(cplang_tests PRIVATE
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

    # 注册测试（CTest 直接运行可执行文件）
    add_test(NAME cplang_unit_tests COMMAND cplang_tests)

    # ================================================================
    #  CP 语言集成测试 — 自动注册 tests/ 目录下的核心 .cp 测试
    # ================================================================
    set(CP_TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    set(CP_TEST_WORKDIR "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

    # 核心语言特性测试
    set(CP_CORE_TESTS
        test_stdlib_new      test_all           test_bool
        test_cfg             test_cfg2          test_cfg3
        test_debug           test_debug2        test_final
        test_full_flow       test_game          test_hello
        test_int             test_integrated    test_items
        test_lang            test_load          test_map
        test_map_arr         test_map2          test_map3
        test_map4            test_map5          test_minimal
        test_rect            test_role          test_simple
        test_summary         test_table         test_v3_quick
        test_v4              test_v4_step       test_xproc
        test_alias           test_client        test_csv
        test_db              test_loopback
    )
    foreach(T ${CP_CORE_TESTS})
        add_test(NAME ${T}
            COMMAND cplang_cli -c "${CP_TEST_DIR}/${T}.cp"
            WORKING_DIRECTORY ${CP_TEST_WORKDIR}
        )
    endforeach()

    # 泛型测试
    set(CP_GENERIC_TESTS
        generic_test         generic_test2      generic_struct
        generic_struct_simple generic_pair       generic_constraint_test
        generic_constraint_test2 generic_constraint_comprehensive
        generic_nested_test   generic_struct_comprehensive
    )
    foreach(T ${CP_GENERIC_TESTS})
        add_test(NAME ${T}
            COMMAND cplang_cli -c "${CP_TEST_DIR}/${T}.cp"
            WORKING_DIRECTORY ${CP_TEST_WORKDIR}
        )
    endforeach()

    # JIT 测试（LLVM 可用时）
    if(CPLANG_USE_LLVM AND LLVM_FOUND)
        set(CP_JIT_TESTS
            jit_minimal    jit_pure_test    jit_pure_fib
            jit_bench      jit_step2        jit_while
            jit_simple_while jit_prime_test jit_fib_print
        )
        foreach(T ${CP_JIT_TESTS})
            add_test(NAME ${T}
                COMMAND cplang_cli -c "${CP_TEST_DIR}/${T}.cp"
                WORKING_DIRECTORY ${CP_TEST_WORKDIR}
            )
        endforeach()
    endif()

    # 扩展：更多语言/标准库测试（避免与 CP_CORE_TESTS/CP_GENERIC_TESTS 重复）
    set(CP_EXTRA_TESTS
        test_step1 test_step2 test_step3 test_step4 test_step5 test_step6
        test_ret_no_print test_print_inside test_println
        test_tostr test_v5_util test_transform_ret
        test_tail_rec test_switch test_loop_unroll test_inline
        test_func_basic test_func_check
        test_full test_generic2 test_generic_struct
        test_nested test_nested_struct test_nested_struct_cn
        test_for_each_main test_for_each_fn5 test_for_each_simple
        test_for_each_only test_for_each_sep
        test_jit test_jit_fib test_jit_loop test_jit_simple test_jit_mini
        test_jit_hotspot test_jit_fib20
        test_simple_fn test_simple_call test_simple_ret
        test_for_simple test_for_nested test_for_both
        test_num test_numstr
        test_inc test_inc_dec
        test_ternary test_match_def test_match_int test_match_tok
        test_global_var test_global_var2
        test_simple_import
        test_stdlib test_stdlib_cn test_stdlib_ext
        test_new_features test_new_util
        test_no_import test_no_native
        test_array test_array_index
        test_forEach_simple
        test_fn_return test_fn_call_arr test_fn_with_arr
        test_struct test_struct_access test_struct_access_cn
        test_struct_array test_struct_array_cn
        test_struct_cn test_struct_simple test_struct_simple2
        test_struct_infer test_struct_lit_simple test_struct_literal
        test_struct_literal_cn test_simple_generic_struct
        two_field_struct access_field circular_a circular_b
        test_bilingual test_bilingual_symbols
        test_md5_check test_aes test_crypto
        test_dns_min test_dns_min2 test_http test_websocket
        test_channel test_channel2 test_threading
        test_matrix test_json test_json_proc_env
        test_repl_ops test_src
        test_render_t test_render_full
        test_raylib test_raylib_basics test_raylib_debug
        test_raylib_english test_raylib_minimal
        test_raylib_probe test_raylib_single
        test_rl_bare_loop test_rl_loop_min
        test_rl_min test_llvm test_optimizer
        test_shapeonly test_color_text
        test_min_font test_text_diag
        test_fib test_fib20 test_perf
        test_p0p1 test_p2 test_p2_more test_p3
        test_basic test_basic_cn
        test_enum test_enum2 test_enum3 test_enum_basic test_enum_only
        test_class test_class2
        test_break test_break_only test_break_fixed
        test_continue_mod test_continue_simple test_continue_if
        test_div test_ft test_while_int
        test_while_bare test_defer test_dynamic_type
        test_exception test_escape test_fe
        test_ftest test_chinese test_let test_neg
        test_dbg test_qt
        test_bench_fib test_bench_fib38
        test_bench_primes test_gtest
        test_sqlite test_sqlite_advanced
    )
    foreach(T ${CP_EXTRA_TESTS})
        if(EXISTS "${CP_TEST_DIR}/${T}.cp")
            add_test(NAME ${T}
                COMMAND cplang_cli -c "${CP_TEST_DIR}/${T}.cp"
                WORKING_DIRECTORY ${CP_TEST_WORKDIR}
            )
        endif()
    endforeach()

    # Python 端到端测试（需要 Python 3）
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_FOUND)
        add_test(NAME py_test_cases
            COMMAND ${Python3_EXECUTABLE} "${CP_TEST_DIR}/test_cases.py"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        add_test(NAME py_test_features
            COMMAND ${Python3_EXECUTABLE} "${CP_TEST_DIR}/test_features.py"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        message(STATUS "Python 测试已启用 (${Python3_EXECUTABLE}): py_test_cases, py_test_features")
    else()
        message(STATUS "未找到 Python3，跳过 Python 端到端测试")
    endif()

    message(STATUS "测试构建已启用 (C++单元测试 + CP集成测试 + Python端到端测试)")
endif()

# ============================================================================
# PGO (Profile-Guided Optimization) — 支持 MSVC/GCC/Clang 三种工具链
# ============================================================================
if(CPLANG_USE_LLVM AND LLVM_FOUND)
    if(MSVC)
        # MSVC: /GENPROFILE + /USEPROFILE
        add_executable(cplang_pgo_train EXCLUDE_FROM_ALL
            src/main.cpp src/repl.cpp
        )
        target_compile_options(cplang_pgo_train PRIVATE /GL- /O2)
        target_link_options(cplang_pgo_train PRIVATE /GENPROFILE)
        target_link_libraries(cplang_pgo_train PRIVATE
            cplang_core cplang_lexer cplang_parser cplang_semantic
            cplang_vm cplang_codegen cplang_stdlib cplang_platform
            cplang_llvm_codegen cplang_jit cplang_optimizer
            cplang_module cplang_exception $<TARGET_OBJECTS:cplang_game_engine>
            ${LLVM_LIB_FILES} ${SYSTEM_LIBS}
        )

        add_custom_target(cplang_pgo_merge
            COMMAND ${CMAKE_COMMAND} -E echo "Merging PGO profiles..."
            COMMAND "${LLVM_TOOLS_BINARY_DIR}/llvm-profdata" merge -o
                    "${CMAKE_BINARY_DIR}/cplang.profdata"
                    "${CMAKE_BINARY_DIR}/*.profraw"
            COMMENT "PGO: merge .profraw -> cplang.profdata"
        )

        add_executable(cplang_pgo_optimized EXCLUDE_FROM_ALL
            src/main.cpp src/repl.cpp
        )
        target_compile_options(cplang_pgo_optimized PRIVATE /GL /O2)
        target_link_options(cplang_pgo_optimized PRIVATE /LTCG
            "/USEPROFILE:${CMAKE_BINARY_DIR}/cplang.profdata")
        target_link_libraries(cplang_pgo_optimized PRIVATE
            cplang_core cplang_lexer cplang_parser cplang_semantic
            cplang_vm cplang_codegen cplang_stdlib cplang_platform
            cplang_llvm_codegen cplang_jit cplang_optimizer
            cplang_module cplang_exception $<TARGET_OBJECTS:cplang_game_engine>
            ${LLVM_LIB_FILES} ${SYSTEM_LIBS}
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # GCC/Clang: -fprofile-generate + -fprofile-use
        add_executable(cplang_pgo_train EXCLUDE_FROM_ALL
            src/main.cpp src/repl.cpp
        )
        target_compile_options(cplang_pgo_train PRIVATE -O2 -fprofile-generate="${CMAKE_BINARY_DIR}/pgo_data")
        target_link_options(cplang_pgo_train PRIVATE -fprofile-generate="${CMAKE_BINARY_DIR}/pgo_data")
        target_link_libraries(cplang_pgo_train PRIVATE
            cplang_core cplang_lexer cplang_parser cplang_semantic
            cplang_vm cplang_codegen cplang_stdlib cplang_platform
            cplang_llvm_codegen cplang_jit cplang_optimizer
            cplang_module cplang_exception $<TARGET_OBJECTS:cplang_game_engine>
            ${LLVM_LIB_FILES} ${SYSTEM_LIBS}
        )

        add_custom_target(cplang_pgo_merge
            COMMAND ${CMAKE_COMMAND} -E echo "使用 GCC/Clang 自动合并 profile（-fprofile-use 时编译器自动读取）"
            COMMENT "PGO: GCC/Clang 自动处理 profile 数据"
        )

        add_executable(cplang_pgo_optimized EXCLUDE_FROM_ALL
            src/main.cpp src/repl.cpp
        )
        target_compile_options(cplang_pgo_optimized PRIVATE -O3 -fprofile-use="${CMAKE_BINARY_DIR}/pgo_data" -fprofile-correction)
        target_link_options(cplang_pgo_optimized PRIVATE -fprofile-use="${CMAKE_BINARY_DIR}/pgo_data" -fprofile-correction)
        target_link_libraries(cplang_pgo_optimized PRIVATE
            cplang_core cplang_lexer cplang_parser cplang_semantic
            cplang_vm cplang_codegen cplang_stdlib cplang_platform
            cplang_llvm_codegen cplang_jit cplang_optimizer
            cplang_module cplang_exception $<TARGET_OBJECTS:cplang_game_engine>
            ${LLVM_LIB_FILES} ${SYSTEM_LIBS}
        )
    endif()

    if(MSVC OR CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "PGO 已启用: cplang_pgo_train -> cplang_pgo_merge -> cplang_pgo_optimized")
    endif()
endif()
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
    cplang_game_engine
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
set_target_properties(cplang_game_engine PROPERTIES FOLDER "cplang/05-game")
set_target_properties(cplang_cli    PROPERTIES FOLDER "cplang/99-cli")
set_target_properties(cplang_repl   PROPERTIES FOLDER "cplang/99-cli")