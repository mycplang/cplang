# CP语言 Linux/Android 多平台支持计划

> 状态: 规划中 | 目标: 一套CP源码，三平台（Windows/Linux/Android）零修改运行

## 总览

```
阶段1: 编译器 Linux 自举     ████████░░  2周
阶段2: stdlib 平台抽象层      ██████████  3周
阶段3: Linux 完整验证         ██████░░░░  1.5周
阶段4: Android NDK 移植       ████████░░  2周
阶段5: APK 打包 + 测试        ██████░░░░  1.5周
─────────────────────────────────────────────
总计: 10周（约2.5个月）
```

## 阶段 1: 编译器 Linux 自举（2周）

### 目标
cplang 编译器本身能在 Linux 上编译运行，生成 Linux 可执行文件。

### 现状
- CMakeLists.txt 支持跨平台声明，但实测仅 Windows MSVC 通过
- 代码中混有 MSVC 扩展（`__declspec`、`_popen`、`GetModuleFileNameA`）
- 部分 stdlib 文件标记 `#ifdef _WIN32` 但缺少 `#else`

### 任务清单

#### 1.1 修复编译器的 Windows 专有 API（3天）
| 文件 | Windows API | Linux 替换 |
|------|-----------|-----------|
| `src/main.cpp` | `GetModuleFileNameA` | `readlink("/proc/self/exe")` |
| `src/main.cpp` | `URLDownloadToFileA` | `curl` 或 `libcurl` |
| `src/main.cpp` | `SetConsoleOutputCP` | 无需（Linux 默认 UTF-8） |
| `src/main.cpp` | `_popen` | `popen` |
| `src/jit/jit_compiler.cpp` | `LoadLibrary`/`_popen` | `dlopen`/`popen` |
| `src/jit/orc_jit.cpp` | `__declspec(dllexport)` | `__attribute__((visibility))` |
| `src/codegen/aot_compiler.cpp` | `SECURITY_ATTRIBUTES`/`CreateProcess` | `fork`/`exec` |
| `src/codegen/aot_compiler.cpp` | MSVC 路径检测（vswhere） | pkg-config / llvm-config |

**策略**: 每个 API 用 `#ifdef _WIN32 ... #else ... #endif` 包裹，Windows 路径不动。

#### 1.2 CMake 构建系统完善（2天）
```cmake
# 当前仅 MSVC + Ninja
# 需添加:
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(LLVM REQUIRED)
    find_package(OpenGL REQUIRED)
    find_package(Threads REQUIRED)
    # Raylib 需要单独编译或系统安装
endif()
```

#### 1.3 Linux 依赖验证（1天）
```bash
# Ubuntu/Debian 依赖
sudo apt install -y \
    llvm-18-dev clang-18 libclang-18-dev \
    libglfw3-dev libgl1-mesa-dev \
    libasound2-dev libx11-dev libxrandr-dev \
    libxi-dev libxcursor-dev libxinerama-dev \
    cmake ninja-build

# 编译验证
cd C:\CPLANG  # 假设代码在共享目录
mkdir build_linux && cd build_linux
cmake .. -DCMAKE_BUILD_TYPE=Release -DCPLANG_USE_LLVM=ON -G Ninja
ninja cplang_cli
```

#### 1.4 编译器自举测试（1天）
```bash
# 验证 cplang 在 Linux 上能编译 CP 源码
echo '打印("hello linux");' > test.cp
./cplang -c test.cp        # VM 执行
./cplang -j test.cp        # JIT 执行
./cplang -r                # REPL
```

## 阶段 2: stdlib 平台抽象层（3周）

### 目标
CP 标准库在 Linux 上功能完整，用户代码无需修改。

### 核心策略: 统一抽象接口 + #ifdef 实现

```
                         CP 用户代码
                   打印("hello")  网络_连接("host",80)
                              │
              ┌───────────────┼───────────────┐
              ▼               ▼               ▼
         VM 内置函数    平台抽象层 API       平台抽象层 API
        (纯C++,已跨平台)  pl_net_init()      pl_file_open()
              │               │               │
              └───────────────┼───────────────┘
                              │
              ┌───────────────┼───────────────┐
              ▼               ▼               ▼
         #ifdef _WIN32   #ifdef __linux__  #ifdef __ANDROID__
         WinSock API     BSD socket API    BSD socket API
```

### 2.1 新增平台抽象头文件（2天）

创建 `include/platform/platform.hpp`:
```cpp
#pragma once
namespace cplang::platform {

// ── 网络 ──
bool   net_init();                    // WSAStartup / 空操作
void   net_cleanup();                 // WSACleanup / 空操作
int    net_errno();                   // WSAGetLastError / errno
int    socket_create(int af, int type, int proto);
int    socket_connect(int sock, const char* host, int port);
int    socket_bind(int sock, int port);
int    socket_listen(int sock, int backlog);
int    socket_accept(int sock);
int    socket_recv(int sock, char* buf, int len);
int    socket_send(int sock, const char* buf, int len);
int    socket_close(int sock);

// ── 文件 ──
void*  file_open(const char* path, const char* mode);    // CreateFile / fopen
int    file_read(void* f, char* buf, int len);           // ReadFile / fread
int    file_write(void* f, const char* buf, int len);    // WriteFile / fwrite
void   file_close(void* f);
bool   file_exists(const char* path);
bool   file_delete(const char* path);
int64_t file_size(void* f);

// ── 线程 ──
void*  thread_create(void (*fn)(void*), void* arg);      // CreateThread / pthread_create
void   thread_join(void* thread);
void   thread_detach(void* thread);
void*  mutex_create();
void   mutex_lock(void* m);
void   mutex_unlock(void* m);
void   mutex_destroy(void* m);

// ── 时间 ──
int64_t time_ms();                   // GetTickCount64 / clock_gettime
void   time_sleep_ms(int ms);        // Sleep / usleep

// ── 动态加载 ──
void*  dl_open(const char* path);    // LoadLibrary / dlopen
void*  dl_sym(void* lib, const char* name);  // GetProcAddress / dlsym
void   dl_close(void* lib);

// ── 系统 ──
const char* os_name();              // "windows" / "linux" / "android"
int    cpu_count();
void   random_seed(unsigned int seed);

} // namespace cplang::platform
```

### 2.2 实现 platform_win.cpp（2天）

`src/platform/platform_win.cpp`:
```cpp
#ifdef _WIN32
#include "platform/platform.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
// ... 实现所有 Windows 版本
#endif
```

### 2.3 实现 platform_linux.cpp（3天）

`src/platform/platform_linux.cpp`:
```cpp
#if defined(__linux__) || defined(__ANDROID__)
#include "platform/platform.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <dlfcn.h>
#include <time.h>
// ... 实现所有 Linux/Android 版本
#endif
```

### 2.4 改造 stdlib 5 个平台模块（7天）

逐个替换 `#ifdef _WIN32` 中的 Windows API 调用为 `platform::` 接口。

| 模块 | 文件 | 改动量 | 天数 |
|------|------|--------|------|
| 网络 | `stdlib_net_ws_sql.cpp`, `stdlib_http.cpp`, `stdlib_types_net.cpp` | 替换 WinSock 调用 | 2天 |
| 文件/IO | `stdlib_file.cpp`, `stdlib_io.cpp`, `stdlib_file_log.cpp` | 替换 CreateFile/WriteConsole | 2天 |
| 线程 | `stdlib_threading.cpp` | 替换 CreateThread | 1天 |
| 系统/时间 | `stdlib_time_system.cpp`, `stdlib_time_sys_more.cpp` | 替换 WinAPI | 1天 |
| 进程/控制台 | `stdlib_matrix_color_path_console.cpp`, `stdlib_r10_r11.cpp` | 替换控制台/进程 API | 1天 |

### 2.5 改造主程序 main.cpp（1天）

- `getOwnExePath()` → 用 `platform::` 抽象
- `checkEmbeddedSource()` → 用 `platform::` 抽象
- `packSource()` → Linux 用 ELF 自解压，Android 用 APK asset

## 阶段 3: Linux 完整验证（1.5周）

### 3.1 构建系统集成（1天）
```bash
# 添加 Linux 构建脚本
build_linux.sh:
    cmake -DCMAKE_BUILD_TYPE=Release -DCPLANG_USE_LLVM=ON -G Ninja
    ninja
    # 复制 raylib 到输出目录
    cp third_party/raylib/build_release/raylib/libraylib.a bin/
```

### 3.2 单元测试（2天）
```bash
# 迁移现有测试用例到 Linux
./cplang -c tests/test_math.cp
./cplang -c tests/test_array.cp
./cplang -c tests/test_string.cp
./cplang -c tests/test_table.cp
./cplang -c tests/test_thread.cp
./cplang -c tests/test_regex.cp
# ... 全部 pass 才算通过
```

### 3.3 图形测试（2天）
```bash
# Raylib 图形测试
./cplang -c examples/snake_final.cp     # 贪吃蛇
./cplang -c examples/demo_raylib.cp     # Raylib 演示
./cplang -c examples/hello.cp           # 基础程序
```

### 3.4 SFX 打包验证（1天）
```bash
# Linux SFX 格式: ELF 自解压（源码追加到 elf 末尾）
./cplang -k snake.cp -o snake_linux
./snake_linux                          # 双击运行
```

## 阶段 4: Android NDK 移植（2周）

### 4.1 NDK 工具链配置（2天）

```cmake
# CMakeLists.txt 添加 Android 支持
if(ANDROID)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    # 关闭 Windows 专用模块
    set(CPLANG_USE_LLVM OFF)  # Android JIT 需要额外工作
    # 使用 Android 的 OpenGL ES
    add_definitions(-DGRAPHICS_API_OPENGL_ES2)
    add_definitions(-DPLATFORM_ANDROID)
endif()
```

### 4.2 VM 核心编译为 .so（3天）

```bash
# 交叉编译
cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
      -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 \
      -DCMAKE_BUILD_TYPE=Release ..

# 产出
libcpvm.so          # VM 核心 + 字节码解释器
libcpstdlib.so      # stdlib（不含平台模块）
```

### 4.3 NativeActivity 桥接（3天）

```cpp
// android_main.c - Android 入口
#include <android_native_app_glue.h>
#include "cpvm.hpp"    // CP 语言 VM

void android_main(struct android_app* state) {
    // 初始化 CP VM
    cplang::VM vm;
    cplang::StdLib::registerAll(&vm);

    // 从 APK asset 加载 CP 源码
    AAsset* asset = AAssetManager_open(state->activity->assetManager,
                                       "game.cp", AASSET_MODE_BUFFER);
    const char* source = (const char*)AAsset_getBuffer(asset);

    // 编译并执行
    auto* func = compiler.compile(source, "game.cp");
    vm.loadModule(func);

    // Raylib 需要适配 Android 事件循环
    while (!state->destroyRequested) {
        // 处理 Android 事件
        // 调用 CP 的 update/render
    }
}
```

### 4.4 Raylib Android 集成（2天）

Raylib 已有官方 Android 支持：
```cmake
# raylib 的 Android 构建
cd third_party/raylib/src
make PLATFORM=PLATFORM_ANDROID ANDROID_NDK=$ANDROID_NDK
# 产出 libraylib.a
```

```cpp
// CP 图形包适配
InitWindow → raylib 的 InitWindow (OpenGL ES)
DrawText   → raylib 的 DrawText (Android 字体)
```

## 阶段 5: APK 打包 + 测试（1.5周）

### 5.1 Android 打包脚本（2天）

```bash
# build_apk.sh
#!/bin/bash
cp game.cp app/src/main/assets/          # CP 源码放入 asset
cp libcpvm.so app/src/main/jniLibs/arm64-v8a/
cp libcpstdlib.so app/src/main/jniLibs/arm64-v8a/
cp libraylib.so app/src/main/jniLibs/arm64-v8a/
./gradlew assembleRelease                # 标准 Android 打包
# 产出 app-release.apk
```

### 5.2 cplang 命令行安卓入口（2天）

```bash
# cplang 工具支持 --android 模式
cplang --android game.cp -o game.apk
# 内部: 编译 CP 源码 → 打包为 APK asset → 用 gradle 构建
```

### 5.3 测试验证（3天）

| 测试项 | 目标 |
|--------|------|
| 基础程序 | `打印("hello android")` → logcat 输出 |
| 数组/字符串 | 标准库基础功能 |
| 简单图形 | `清空背景` + `绘制矩形` → 屏幕可见 |
| 2048 游戏 | 触屏交互 → 可玩 |
| 性能 | 60 FPS 无掉帧 |

## 最终验证矩阵

| | Windows | Linux | Android |
|------|---------|-------|---------|
| **编译器** | ✅ 已有 | 🎯 阶段1 | - |
| **VM 核心** | ✅ 已有 | 🎯 阶段1 | 🎯 阶段4 |
| **JIT** | ✅ 已有 | 🎯 阶段1 | ⏳ 后续 |
| **基础 stdlib** | ✅ 已有 | 🎯 阶段2 | 🎯 阶段2 |
| **图形 Raylib** | ✅ 已有 | 🎯 阶段3 | 🎯 阶段4 |
| **SFX 打包** | ✅ 已有 | 🎯 阶段3 | 🎯 阶段5 |
| **2048 游戏** | ✅ 已有 | 🎯 阶段3 | 🎯 阶段5 |

---

> 更新时间: 2026-06-10  
> 总工期: 10 周（约 2.5 个月）  
> 核心原则: Windows 代码不动，只用 #ifdef 添加新平台分支
