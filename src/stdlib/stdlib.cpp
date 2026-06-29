// 鏍囧噯搴撳疄鐜?

#include "stdlib/stdlib.hpp"
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <cstdio>
#include <regex>
#include <filesystem>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winhttp.h>
#include <crypto/md5_impl.h>
#endif
#include <sqlite/sqlite3.h>

namespace cplang {

// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?
//  StdLib 瀹炵幇
// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?

void StdLib::registerAll(VM* vm) {
    registerMath(vm);
    registerArray(vm);
    registerString(vm);       // 需在 array 之后，统一 find 函数
    registerTable(vm);
    registerSet(vm);
     registerStack(vm);
     registerQueue(vm);
     registerDeque(vm);
     registerPriorityQueue(vm);
     registerLinkedList(vm);
     registerSLinkedList(vm);
     registerMultiSet(vm);
     registerMultiMap(vm);
     registerUnorderedSet(vm);
     registerUnorderedMultiSet(vm);
     registerUnorderedMap(vm);
     registerUnorderedMultiMap(vm);
     registerBitset(vm);
     registerComplex(vm);
     registerPair(vm);
     registerIO(vm);
     registerTime(vm);
     registerSystem(vm);
     registerReflection(vm);
     registerFile(vm);
     registerNetwork(vm);
     registerJSON(vm);
     registerTypes(vm);
     registerBitwise(vm);
     registerAlgorithms(vm);
     registerRandom(vm);
     registerRegex(vm);
     registerStringExt(vm);
     registerCrypto(vm);
     registerEncoding(vm);
     registerStringMore(vm);
     registerArrayMore(vm);
     registerFileMore(vm);
     registerTimeMore(vm);
     registerSystemMore(vm);
     registerProcess(vm);
     registerMathMore(vm);
     registerStatistics(vm);
     registerUtils(vm);
     registerStringCase(vm);
     registerMoreUtils(vm);
     registerHTTP(vm);
     registerMatrix(vm);
     registerColor(vm);
     registerPath(vm);
     registerConsole(vm);
     registerOptional(vm);
     registerVariant(vm);
     registerAny(vm);
     registerTuple(vm);
     registerNumericLimits(vm);
     registerHeap(vm);
     registerStringSearch(vm);
     registerAlgoExt(vm);
     registerP5Algo(vm);
     registerMathConst(vm);
     registerFormat(vm);
     registerResult(vm);
     registerFunctional(vm);
     registerSpan(vm);
     registerIterator(vm);
      registerAsync(vm);      // 原生异步（P9.3）
     registerCharconv(vm);
     registerSourceLoc(vm);
     registerMemory(vm);
     registerThreading(vm);

     registerWebSocket(vm);
     registerSqlite(vm);
     registerMysql(vm);
     registerPg(vm);
     registerRedis(vm);
     registerMongo(vm);
     registerHttp(vm);
     registerImGui(vm);
     registerTCP(vm);
     registerRaylib(vm);
     registerCryptoPlus(vm);
     registerAes(vm);
     registerFFI(vm);

    registerR10Misc(vm);
    registerR11Misc(vm);
    registerMathSpecial(vm);
    registerAlgoMissing(vm);
    registerCharconvFloat(vm);
    registerSpanEnhance(vm);
    registerResultMonad(vm);
    registerTupleEnhance(vm);
    registerBinaryIO(vm);
    registerCallOnce(vm);
    registerMap(vm);
    registerCharset(vm);
    registerFileSeek(vm);
    registerFileWalk(vm);
    registerLogger(vm);
    registerTemp(vm);
    registerFileStat(vm);
    registerDuration(vm);
    registerDir(vm);
    registerCsvWrite(vm);
    registerLogPlus(vm);
    registerStrCi(vm);
    registerFixMissing(vm);
    registerIOPoll(vm);
    registerImage(vm);          // 图片处理
    registerCompress(vm);       // ZIP压缩 (自写格式)
    registerArgparse(vm);       // 命令行参数
    registerConfig(vm);         // 配置文件
    registerHTTPServer(vm);     // HTTP服务端
    registerTemplate(vm);       // 模板引擎
    registerAudio(vm);          // 音频 (raylib API)
    registerMarkdown(vm);       // Markdown转换
    registerPDF(vm);            // PDF生成
    registerKVDB(vm);           // 键值数据库
    registerTesting(vm);        // 测试框架
    registerLogging(vm);        // 日志框架
    registerLangEnhance(vm);    // 函数式+子进程

    // P11 标准库生态增强
    registerBinary(vm);         // 二进制数据（字节数组）
    registerP11Algo(vm);
    registerP11Graph(vm);
    registerP11DS(vm);
    registerP11Utils(vm);

    // 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?
    //  娓告垙寮曟搸妯″潡 (v0.4.0 鈥?寮€绠卞嵆鐢紝闆跺鍏?
    // 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?
    registerGameNet(vm);
    registerGameBattle(vm);
    registerGameMap(vm);
    registerGameItem(vm);
    registerGameRole(vm);
    registerGameSprite(vm);
    registerGameDb(vm);
    registerGameConfig(vm);
    registerWeb(vm);

        registerColors(vm);  // 涓存椂娉ㄩ噴锛屾帓鏌ラ棶棰?
}

// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?
//  鏍稿績娉ㄥ唽锛堟渶灏忚繍琛屾椂锛屽彧娉ㄥ唽缁濆蹇呰鐨勫熀纭€妯″潡锛?
// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?

void StdLib::registerCore(VM* vm) {
    registerIO(vm);           // 打印/输入/类型转换/数组基础/表基础
    registerMath(vm);         // 基础数学运算
    registerArray(vm);        // 数组操作
    registerString(vm);       // 基础字符串操作（需在 array 之后，统一 find 函数）
    registerTable(vm);        // 表操作
    registerTypes(vm);        // 绫诲瀷鍒ゆ柇(isNil/isBool绛?
    registerReflection(vm);   // 鍙嶅皠
    registerTime(vm);         // 鏃堕棿
    registerSystem(vm);       // 绯荤粺
    registerFile(vm);         // 鍩虹鏂囦欢鎿嶄綔
    registerNetwork(vm);      // 鍩虹缃戠粶(network.h涓殑鍚屾枃浠?
    registerFormat(vm);       // 鏍煎紡鍖?
    registerPath(vm);         // 璺緞澶勭悊
    registerMemory(vm);       // 鍐呭瓨
    registerFixMissing(vm);   // 淇缂哄け
    registerIOPoll(vm);       // IO杞
}

// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?
//  鎸夐渶娉ㄥ唽锛堜粎娉ㄥ唽鎸囧畾妯″潡锛?
// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?

void StdLib::registerModules(VM* vm, const std::vector<std::string>& modules) {
    for (const auto& m : modules) {
        if (m == "graphics" || m == "@cp/graphics" || m == "鍥惧舰" || m == "@cp/鍥惧舰") {
            registerTCP(vm);
     registerRaylib(vm);
            registerImGui(vm);
        } else if (m == "database" || m == "@cp/database") {
            registerSqlite(vm);
            registerMysql(vm);
            registerPg(vm);
            registerRedis(vm);
            registerMongo(vm);
            registerWebSocket(vm);
        } else if (m == "crypto" || m == "@cp/crypto" || m == "鍔犲瘑" || m == "@cp/鍔犲瘑") {
            registerCrypto(vm);
            registerCryptoPlus(vm);
            registerAes(vm);
            registerEncoding(vm);
        } else if (m == "ffi" || m == "@cp/ffi" || m == "澶栭儴鎺ュ彛" || m == "@cp/澶栭儴鎺ュ彛") {
            registerFFI(vm);
        } else if (m == "net" || m == "@cp/net" || m == "缃戠粶" || m == "@cp/缃戠粶") {
            registerJSON(vm);
            registerHTTP(vm);
            registerHttp(vm);
        } else if (m == "container" || m == "@cp/container" || m == "瀹瑰櫒" || m == "@cp/瀹瑰櫒") {
            registerSet(vm);
            registerStack(vm);
            registerQueue(vm);
            registerDeque(vm);
            registerPriorityQueue(vm);
            registerLinkedList(vm);
            registerSLinkedList(vm);
            registerMultiSet(vm);
            registerMultiMap(vm);
            registerUnorderedSet(vm);
            registerUnorderedMultiSet(vm);
            registerUnorderedMap(vm);
            registerUnorderedMultiMap(vm);
            registerBitset(vm);
            registerHeap(vm);
            registerComplex(vm);
            registerPair(vm);
        } else if (m == "concurrent" || m == "@cp/concurrent") {
            registerThreading(vm);
        } else if (m == "string_ext" || m == "@cp/string_ext") {
            registerStringExt(vm);
            registerStringMore(vm);
            registerStringSearch(vm);
            registerStringCase(vm);
            registerStrCi(vm);
            registerRegex(vm);
            registerStatistics(vm);
            registerUtils(vm);
            registerMoreUtils(vm);
        } else if (m == "charset" || m == "@cp/charset") {
            registerCharset(vm);
        } else if (m == "algorithm" || m == "@cp/algorithm") {
            registerAlgorithms(vm);
            registerAlgoExt(vm);
            registerAlgoMissing(vm);
            registerP5Algo(vm);
            registerBitwise(vm);
            registerRandom(vm);
        }
    }
}
// ========== 各模块实现已提取到独立 .cpp 文件 ==========
// 非 Windows 平台空实现（链接用）
#ifndef _WIN32
static void registerMysql(cplang::VM*) {}
static void registerPg(cplang::VM*) {}
static void registerRedis(cplang::VM*) {}
static void registerMongo(cplang::VM*) {}
static void registerHttp(cplang::VM*) {}
static void registerLogPlus(cplang::VM*) {}
#endif

void StdLib::registerColors(VM* vm) {
    struct ColorDef { const char* name; uint8_t r, g, b, a; };
    ColorDef colors[] = {
        {"亮蓝",   80, 140, 255, 255},
        {"深蓝",   15,  15,  35, 255},
        {"白色",  255, 255, 255, 255},
        {"金色",  255, 215,   0, 255},
        {"灰色",  150, 150, 160, 255},
        {"红色",  255,  60,  60, 255},
        {"黑色",    0,   0,   0, 255},
        {"绿色",   80, 220,  80, 255},
        {"酸橙色",  0, 255,   0, 255},
        {"天蓝",  135, 206, 235, 255},
        {"粉色",  255, 182, 193, 255},
        {"紫色",  160,  32, 240, 255},
        {"橙色",  255, 165,   0, 255},
        {"棕色",  139,  69,  19, 255},
        {"青色",    0, 255, 255, 255},
        {"洋红",  255,   0, 255, 255},
        {"黄色",  255, 255,   0, 255},
        {"透明",    0,   0,   0,   0},
        {"背景色", 187, 173, 160, 255},
        {"空格色", 205, 193, 180, 255},
        {"深字色", 119, 110, 101, 255},
        {"浅字色", 249, 246, 242, 255},
        {"遮罩色",   0,   0,   0, 180},
    };
    for (auto& c : colors) {
        auto t = VMTable::create();
        t->set(makeStringVal(VMString::create("r")), Value::Int(c.r));
        t->set(makeStringVal(VMString::create("g")), Value::Int(c.g));
        t->set(makeStringVal(VMString::create("b")), Value::Int(c.b));
        t->set(makeStringVal(VMString::create("a")), Value::Int(c.a));
        vm->trackGC(reinterpret_cast<VMObject*>(t));
        vm->setGlobal(c.name, makeTableVal(t));
    }
}

// ========== Web 框架注册 ==========
void StdLib::registerWeb(VM* vm) {
    registerFunction(vm, "web_serve",   cplang::web::serve);
}
} // namespace cplang