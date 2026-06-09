// CP语言 Android NativeActivity 桥接
// 用法：将 .cp 源码放入 APK assets/ 目录，在 android_main 中加载执行
//
// NDK 构建: cmake -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake
//                -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 ...
//
// 产物: libcpandroid.so → 放入 APK lib/arm64-v8a/

#include <android_native_app_glue.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>

#include "vm/vm.hpp"
#include "stdlib/stdlib.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "codegen/codegen.hpp"

#define LOG_TAG "CPLang"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace cplang;

// ── 全局状态 ──
static VM*      g_vm        = nullptr;
static bool     g_inited    = false;
static bool     g_shouldExit = false;

// ── 从 APK asset 加载文件内容 ──
static std::string loadAsset(AAssetManager* mgr, const char* filename) {
    if (!mgr || !filename) return "";
    AAsset* asset = AAssetManager_open(mgr, filename, AASSET_MODE_BUFFER);
    if (!asset) {
        LOGE("无法打开 asset: %s", filename);
        return "";
    }
    size_t size = AAsset_getLength(asset);
    std::string content((const char*)AAsset_getBuffer(asset), size);
    AAsset_close(asset);
    LOGI("已加载 asset: %s (%zu 字节)", filename, size);
    return content;
}

// ── 编译 CP 源码 → VM 函数 ──
static CompiledFunction* compileSource(const std::string& source, const char* filename) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    if (lexer.hasError()) {
        LOGE("词法分析失败: %s", lexer.getError().c_str());
        return nullptr;
    }

    Parser parser(tokens, filename);
    auto ast = parser.parse();
    if (!ast || parser.hasError()) {
        LOGE("语法分析失败: %s", parser.getError().c_str());
        return nullptr;
    }

    CodeGen cg;
    auto* func = cg.compile(ast, filename);
    if (!func) {
        LOGE("代码生成失败");
        return nullptr;
    }

    LOGI("编译成功: %s", filename);
    return func;
}

// ── 初始化 CP 语言运行时 ──
static void initRuntime(AAssetManager* mgr) {
    if (g_inited) return;

    LOGI("CP 语言运行时初始化...");

    // 创建 VM
    g_vm = new VM();
    VM::setCurrent(g_vm);

    // 注册标准库
    StdLib::registerAll(g_vm);

    // 可选：从 asset 加载初始化脚本
    std::string initSrc = loadAsset(mgr, "init.cp");
    if (!initSrc.empty()) {
        auto* initFunc = compileSource(initSrc, "init.cp");
        if (initFunc) {
            g_vm->loadModule(initFunc);
            g_vm->callFunction(*initFunc, {});
        }
    }

    g_inited = true;
    LOGI("CP 语言运行时就绪");
}

// ── 加载并执行主游戏脚本 ──
static void loadAndRun(AAssetManager* mgr, const char* gameFile) {
    if (!g_vm) return;

    std::string source = loadAsset(mgr, gameFile);
    if (source.empty()) return;

    auto* func = compileSource(source, gameFile);
    if (!func) return;

    // 加载到 VM
    g_vm->loadModule(func);
    LOGI("模块已加载: %s", gameFile);

    // 查找入口函数
    Value entry = g_vm->getGlobal("main");
    if (!entry.isNil() && (entry.isFunction() || entry.isCFunction())) {
        LOGI("调用 main()...");
        g_vm->callFunction(entry, {});
    } else {
        // 没有 main，直接执行 module 的顶层代码
        LOGI("执行顶层代码...");
        g_vm->callFunction(*func, {});
    }
}

// ── Android 主入口 ──
extern "C" void android_main(struct android_app* state) {
    LOGI("=== CP 语言 Android NativeActivity 启动 ===");

    // 初始化运行时
    initRuntime(state->activity->assetManager);

    // 从 asset 加载游戏源码（默认 game.cp）
    loadAndRun(state->activity->assetManager, "game.cp");

    // 事件循环
    LOGI("进入事件循环...");
    while (!state->destroyRequested && !g_shouldExit) {
        int events;
        struct android_poll_source* source;

        // 等待事件
        while (ALooper_pollAll(0, nullptr, &events, (void**)&source) >= 0) {
            if (source) {
                source->process(state, source);
            }

            if (state->destroyRequested) {
                break;
            }
        }

        // 这里可以添加帧更新逻辑
        // 如果 CP 代码注册了 frame_update 函数，每帧调用它
        if (g_vm) {
            static int frameCount = 0;
            Value frameFn = g_vm->getGlobal("frameUpdate");
            if (!frameFn.isNil() && (frameFn.isFunction() || frameFn.isCFunction())) {
                if (frameCount == 0) LOGI("调用 frameUpdate...");
                g_vm->callFunction(frameFn, {Value::Int(frameCount)});
                frameCount++;
                if (frameCount >= 1000000) frameCount = 0;
            }
        }
    }

    LOGI("CP 语言运行时正在退出...");
    if (g_vm) {
        delete g_vm;
        g_vm = nullptr;
    }

    // 调用 ANativeActivity_finish
    ANativeActivity_finish(state->activity);
}