# FindLLVM.cmake
# 查找 LLVM 安装路径，支持多种安装方式
# 优先级: llvm-config → CMake find_package(LLVM CONFIG) → 默认路径
#
# 注意: llvm-config 优先于 find_package，因为:
#   - find_package(LLVM CONFIG) 设置的 LLVM_LIBRARIES 是 CMake target 名称
#     （如 LLVM::Core、LLVM::OrcJIT），不是 .lib 文件路径
#   - 而 target_link_libraries 需要 .lib 文件路径列表才能用于 build_msvc.bat
#     一致的链接
#   - llvm-config --libfiles 直接输出完整 .lib 路径列表（~180+ 个）
#
# 输出变量:
#   LLVM_FOUND          - 是否找到
#   LLVM_INCLUDE_DIRS   - 头文件路径
#   LLVM_LIBRARIES      - 完整库路径列表（用于 target_link_libraries）
#   LLVM_LIB_DIRS       - 库目录
#   LLVM_CONFIG_EXECUTABLE - llvm-config 路径
#   LLVM_VERSION        - LLVM 版本号

# ============================================================
# 搜索路径列表
# ============================================================
set(LLVM_SEARCH_PATHS
    "C:/Program Files/LLVM"
    "C:/Program Files (x86)/LLVM"
    "C:/LLVM"
    "D:/LLVM"
    "$ENV{USERPROFILE}/LLVM"
    "$ENV{HOME}/llvm-dev"
    "C:/CPLANG/llvm-dev"
    "C:/cplang/llvm-dev"
)

if(DEFINED ENV{LLVM_DIR})
    list(INSERT LLVM_SEARCH_PATHS 0 "$ENV{LLVM_DIR}")
endif()

# ============================================================
# 策略 1: 使用 llvm-config（首选，因为它提供完整 .lib 路径）
# ============================================================
find_program(LLVM_CONFIG_EXECUTABLE llvm-config
    PATHS ${LLVM_SEARCH_PATHS}
    PATH_SUFFIXES bin
)

if(LLVM_CONFIG_EXECUTABLE)
    message(STATUS "找到 llvm-config: ${LLVM_CONFIG_EXECUTABLE}")

    # 获取头文件目录
    execute_process(
        COMMAND ${LLVM_CONFIG_EXECUTABLE} --includedir
        OUTPUT_VARIABLE LLVM_INCLUDE_DIRS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # 获取库目录
    execute_process(
        COMMAND ${LLVM_CONFIG_EXECUTABLE} --libdir
        OUTPUT_VARIABLE LLVM_LIB_DIRS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # 获取 LLVM 版本
    execute_process(
        COMMAND ${LLVM_CONFIG_EXECUTABLE} --version
        OUTPUT_VARIABLE LLVM_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # 使用 --libfiles 获取全部库路径列表
    # 不指定组件名 = 返回所有 LLVM 库（~130+ 个）
    # 修复: 旧代码用 --libnames 指定 4 个组件但不够，导致链接缺少
    # X86Target、ORCJIT 内部符号等。用 --libfiles 不带参数获得全部。
    execute_process(
        COMMAND ${LLVM_CONFIG_EXECUTABLE} --libfiles
        OUTPUT_VARIABLE _LLVM_LIB_FILES
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(_LLVM_LIB_FILES)
        # 将空格分隔的路径转换为 CMake 分号分隔列表
        string(REPLACE " " ";" LLVM_LIBRARIES "${_LLVM_LIB_FILES}")
        list(LENGTH LLVM_LIBRARIES _LLVM_LIB_COUNT)
        message(STATUS "LLVM 库文件数: ${_LLVM_LIB_COUNT}")
    else()
        # 降级: 如果 --libfiles 不可用（旧版 LLVM），用 --libnames
        execute_process(
            COMMAND ${LLVM_CONFIG_EXECUTABLE} --libnames
                core orcjit native irreader
            OUTPUT_VARIABLE _LLVM_LIB_NAMES
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        string(REPLACE " " ";" _LLVM_LIB_NAMES_LIST "${_LLVM_LIB_NAMES}")
        foreach(lib ${_LLVM_LIB_NAMES_LIST})
            # 检查是否已有 .lib 后缀（Windows MSVC 会带）
            if(lib MATCHES "\\.lib$")
                list(APPEND LLVM_LIBRARIES "${LLVM_LIB_DIRS}/${lib}")
            else()
                list(APPEND LLVM_LIBRARIES "${LLVM_LIB_DIRS}/${lib}.lib")
            endif()
        endforeach()
    endif()

    set(LLVM_FOUND TRUE)
    set(LLVM_FOUND_BY "llvm-config")
    message(STATUS "LLVM 版本:   ${LLVM_VERSION}")
    message(STATUS "LLVM Include: ${LLVM_INCLUDE_DIRS}")
    message(STATUS "LLVM Lib:     ${LLVM_LIB_DIRS}")

    # 去重（--libfiles 可能因依赖传递包含重复项）
    if(LLVM_LIBRARIES)
        list(REMOVE_DUPLICATES LLVM_LIBRARIES)
        list(LENGTH LLVM_LIBRARIES _LLVM_LIB_COUNT_AFTER_DEDUP)
        message(STATUS "LLVM 库文件数(去重后): ${_LLVM_LIB_COUNT_AFTER_DEDUP}")
    endif()
endif()

# ============================================================
# 策略 2: 使用 CMake find_package（仅当 llvm-config 未找到时）
# LLVM 自带的 CMake 配置通常位于:
#   <prefix>/lib/cmake/llvm/LLVMConfig.cmake
# ============================================================
if(NOT LLVM_CONFIG_EXECUTABLE)
    find_package(LLVM QUIET CONFIG)

    if(LLVM_FOUND)
        set(LLVM_FOUND_BY "find_package(LLVM CONFIG)")
        message(STATUS "通过 find_package(LLVM CONFIG) 找到 LLVM")

        # LLVM 自带的 CMake config 可能设置 LLVM_LIBRARIES 为 target 名称
        # 尝试解析为实际文件路径
        if(NOT LLVM_LIBRARIES)
            # 如果 LLVM_LIBRARIES 为空，尝试从 LLVM_AVAILABLE_LIBS 构造
            if(DEFINED LLVM_AVAILABLE_LIBS)
                set(LLVM_LIBRARIES ${LLVM_AVAILABLE_LIBS})
            endif()
        endif()

        # 确保 LLVM_LIBRARIES 是文件路径而非 target 名称
        # 如果 LLVM_LIBRARIES 包含 ::，说明是 CMake targets，转成文件路径
        if(LLVM_LIBRARIES)
            set(_converted_libs "")
            foreach(_lib ${LLVM_LIBRARIES})
                if(_lib MATCHES "::")
                    # CMake target，提取目标名并查找实际 .lib 文件
                    string(REPLACE "::" "" _target_name "${_lib}")
                    if(LLVM_LIB_DIRS)
                        list(APPEND _converted_libs "${LLVM_LIB_DIRS}/${_target_name}.lib")
                    endif()
                elseif(_lib MATCHES "\\.lib$")
                    list(APPEND _converted_libs "${_lib}")
                elseif(LLVM_LIB_DIRS)
                    list(APPEND _converted_libs "${LLVM_LIB_DIRS}/${_lib}.lib")
                else()
                    list(APPEND _converted_libs "${_lib}.lib")
                endif()
            endforeach()
            if(_converted_libs)
                set(LLVM_LIBRARIES ${_converted_libs})
            endif()
            list(LENGTH LLVM_LIBRARIES _LLVM_LIB_COUNT)
            message(STATUS "LLVM 库文件数: ${_LLVM_LIB_COUNT}")
        else()
            message(WARNING "⚠️  find_package(LLVM CONFIG) 未设置 LLVM_LIBRARIES")
            message(STATUS "   将尝试使用 llvm-config 作为后备...")
        endif()

        if(NOT LLVM_INCLUDE_DIRS)
            # 尝试从 LLVM 的 CMake 目标获取 include 目录
            get_target_property(_llvm_include_dirs LLVM::Core INTERFACE_INCLUDE_DIRECTORIES)
            if(_llvm_include_dirs)
                set(LLVM_INCLUDE_DIRS ${_llvm_include_dirs})
            endif()
        endif()

        if(NOT LLVM_VERSION)
            # 尝试从 find_package 获取版本
            if(DEFINED LLVM_PACKAGE_VERSION)
                set(LLVM_VERSION ${LLVM_PACKAGE_VERSION})
            endif()
        endif()

        message(STATUS "LLVM 版本:   ${LLVM_VERSION}")
        message(STATUS "LLVM Include: ${LLVM_INCLUDE_DIRS}")
        if(LLVM_LIB_DIRS)
            message(STATUS "LLVM Lib:     ${LLVM_LIB_DIRS}")
        endif()
    endif()
endif()

# ============================================================
# 策略 3: 手动搜索（仅当以上均失败时）
# ============================================================
if(NOT LLVM_FOUND)
    find_path(LLVM_INCLUDE_DIRS
        NAMES "llvm/IR/Module.h"
        PATHS ${LLVM_SEARCH_PATHS}
        PATH_SUFFIXES include
    )

    if(LLVM_INCLUDE_DIRS)
        # 查找核心库作为 LLVM 安装的确认
        find_library(LLVM_CORE_LIB
            NAMES LLVMCore LLVMCore.lib
            PATHS ${LLVM_SEARCH_PATHS}
            PATH_SUFFIXES lib
        )
        if(LLVM_CORE_LIB)
            set(LLVM_FOUND TRUE)
            set(LLVM_FOUND_BY "manual")
            set(LLVM_LIBRARIES ${LLVM_CORE_LIB})
            message(STATUS "LLVM 手动找到于: ${LLVM_INCLUDE_DIRS}")
            message(WARNING "⚠️  手动模式只找到 LLVMCore，JIT 链接可能失败")
            message(STATUS "   建议安装 llvm-config 以获得完整库列表")
        endif()
    endif()
endif()

# ============================================================
# 结果处理
# ============================================================
if(LLVM_FOUND)
    message(STATUS "✅ LLVM 已找到，JIT 编译已启用")

    # 去重（--libfiles 可能因依赖传递包含重复项）
    if(LLVM_LIBRARIES)
        list(REMOVE_DUPLICATES LLVM_LIBRARIES)
    endif()

    # 导出 LLVM_LIBRARIES 供 CMakeLists.txt 使用（作为后备）
    # 注意: 如果用了 find_package(LLVM CONFIG)，LLVM 自带的
    # LLVM_LIBRARIES 变量会包含所有依赖。我们优先用 llvm-config
    # 的结果以保证 bat 与 CMake 的一致性。
else()
    message(WARNING "⚠️  未找到 LLVM，JIT 编译将禁用")
    message(STATUS "   如需启用 JIT: 安装 LLVM 18+ 并设置 LLVM_DIR 环境变量")
    message(STATUS "   或确保 llvm-config 在 PATH 中")
endif()
