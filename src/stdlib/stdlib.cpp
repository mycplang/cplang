// 标准库实现

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

// ═══════════════════════════════════════════════════════════════════
//  StdLib 实现
// ═══════════════════════════════════════════════════════════════════

void StdLib::registerAll(VM* vm) {
    registerMath(vm);
    registerString(vm);
    registerArray(vm);
    registerTable(vm);
    // @cp/container: 以下高级容器已拆分为独立模块包
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
    // registerJSON(vm);    // → @cp/net 模块
    registerTypes(vm);
    // @cp/algorithm: 以下算法函数已拆分
    registerBitwise(vm);
    registerAlgorithms(vm);
    registerRandom(vm);
    // @cp/string_ext: 以下高级字符串功能已拆分
    registerRegex(vm);
    registerStringExt(vm);
    // registerCrypto(vm);    // → @cp/crypto 模块
    // registerEncoding(vm);  // → @cp/crypto 模块
    registerStringMore(vm);
    registerArrayMore(vm);
    registerFileMore(vm);
    registerTimeMore(vm);
    registerSystemMore(vm);
    registerProcess(vm);
    registerMathMore(vm);
    registerStatistics(vm);   // → 同上 @cp/string_ext
    registerUtils(vm);         // → 同上 @cp/string_ext
    registerStringCase(vm);    // → 同上 @cp/string_ext
    registerMoreUtils(vm);     // → 同上 @cp/string_ext
    // registerHTTP(vm);       // → @cp/net 模块
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
    registerStringSearch(vm);  // → @cp/string_ext
    registerAlgoExt(vm);       // → @cp/algorithm
    registerMathConst(vm);
    registerFormat(vm);
    registerResult(vm);
    registerFunctional(vm);
    registerSpan(vm);
    registerIterator(vm);
    registerCharconv(vm);
    registerSourceLoc(vm);
    registerMemory(vm);
    registerThreading(vm);     // → @cp/concurrent

    // ── 以下已拆分为独立模块包，VM 模式仍全量注册 ──
    // @cp/database: registerSqlite + registerMysql + registerPg + registerRedis + registerWebSocket
    registerWebSocket(vm);
    registerSqlite(vm);
    registerMysql(vm);
    registerPg(vm);
    registerRedis(vm);
    // @cp/net: registerJSON + registerHTTP + registerHttp
    registerJSON(vm);
    registerHTTP(vm);
    registerHttp(vm);
    // @cp/graphics: registerImGui + registerRaylib
    registerImGui(vm);
    registerRaylib(vm);
    // @cp/crypto: registerCrypto + registerCryptoPlus + registerAes + registerEncoding
    registerCrypto(vm);
    registerCryptoPlus(vm);
    registerAes(vm);
    registerEncoding(vm);
    // @cp/ffi: registerFFI
    registerFFI(vm);

    registerR10Misc(vm);
    registerR11Misc(vm);
    registerMathSpecial(vm);
    registerAlgoMissing(vm);   // → @cp/algorithm
    registerCharconvFloat(vm);
    registerSpanEnhance(vm);
    registerResultMonad(vm);
    registerTupleEnhance(vm);
    registerBinaryIO(vm);
    registerCallOnce(vm);
    registerMap(vm);
    registerCharset(vm);        // → @cp/charset
    registerFileSeek(vm);
    registerFileWalk(vm);
    registerLogger(vm);
    registerTemp(vm);
    registerFileStat(vm);
    registerDuration(vm);
    registerDir(vm);
    registerCsvWrite(vm);
    registerLogPlus(vm);
    registerStrCi(vm);          // → @cp/string_ext
    registerFixMissing(vm);
    registerIOPoll(vm);
}

// registerFunction / registerAlias 已内联至 include/stdlib/stdlib_fwd.hpp，
// 方便模块独立编译。此处不再重复定义。

// ═══════════════════════════════════════════════════════════════════
//  核心注册（AOT 模式使用，只注册绝对必要的基础模块）
//  这些模块任何 CP 程序都需要，体积小（~3000行），保留在 core 中
// ═══════════════════════════════════════════════════════════════════

void StdLib::registerCore(VM* vm) {
    registerIO(vm);           // 打印/输入/类型转换/数组基础/表基础
    registerMath(vm);         // 基础数学运算
    registerString(vm);       // 基础字符串操作
    registerArray(vm);        // 数组操作
    registerTable(vm);        // 表操作
    registerTypes(vm);        // 类型判断(isNil/isBool等)
    registerReflection(vm);   // 反射
    registerTime(vm);         // 时间
    registerSystem(vm);       // 系统
    registerFile(vm);         // 基础文件操作
    registerNetwork(vm);      // 基础网络(network.h中的同文件)
    registerFormat(vm);       // 格式化
    registerPath(vm);         // 路径处理
    registerMemory(vm);       // 内存
    registerFixMissing(vm);   // 修复缺失
    registerIOPoll(vm);       // IO轮询
}

// ═══════════════════════════════════════════════════════════════════
//  按需注册（AOT 模式使用，仅注册指定模块）
// ═══════════════════════════════════════════════════════════════════

void StdLib::registerModules(VM* vm, const std::vector<std::string>& modules) {
    for (const auto& m : modules) {
        if (m == "graphics" || m == "@cp/graphics" || m == "图形" || m == "@cp/图形") {
            registerRaylib(vm);
            registerImGui(vm);
        } else if (m == "database" || m == "@cp/database" || m == "数据库" || m == "@cp/数据库") {
            registerSqlite(vm);
            registerMysql(vm);
            registerPg(vm);
            registerRedis(vm);
            registerWebSocket(vm);
        } else if (m == "crypto" || m == "@cp/crypto" || m == "加密" || m == "@cp/加密") {
            registerCrypto(vm);
            registerCryptoPlus(vm);
            registerAes(vm);
            registerEncoding(vm);
        } else if (m == "ffi" || m == "@cp/ffi" || m == "外部接口" || m == "@cp/外部接口") {
            registerFFI(vm);
        } else if (m == "net" || m == "@cp/net" || m == "网络" || m == "@cp/网络") {
            registerJSON(vm);
            registerHTTP(vm);
            registerHttp(vm);
        } else if (m == "container" || m == "@cp/container" || m == "容器" || m == "@cp/容器") {
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
        } else if (m == "concurrent" || m == "@cp/concurrent" || m == "并发" || m == "@cp/并发") {
            registerThreading(vm);
        } else if (m == "string_ext" || m == "@cp/string_ext" || m == "字符串扩展" || m == "@cp/字符串扩展") {
            registerStringExt(vm);
            registerStringMore(vm);
            registerStringSearch(vm);
            registerStringCase(vm);
            registerStrCi(vm);
            registerRegex(vm);
            registerStatistics(vm);
            registerUtils(vm);
            registerMoreUtils(vm);
        } else if (m == "charset" || m == "@cp/charset" || m == "字符集" || m == "@cp/字符集") {
            registerCharset(vm);
        } else if (m == "algorithm" || m == "@cp/algorithm" || m == "算法" || m == "@cp/算法") {
            registerAlgorithms(vm);
            registerAlgoExt(vm);
            registerAlgoMissing(vm);
            registerBitwise(vm);
            registerRandom(vm);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
//  各模块的实现已提取到独立 .cpp 文件，由 CMake 统一编译链接
//  拆分出去的模块见 modules/ 目录，可通过 cpkg 安装
// ═══════════════════════════════════════════════════════════════════

// 非 Windows 平台的空壳实现（链接用）
#ifndef _WIN32
static void registerMysql(cplang::VM*) {}
static void registerPg(cplang::VM*) {}
static void registerRedis(cplang::VM*) {}
static void registerHttp(cplang::VM*) {}
static void registerLogPlus(cplang::VM*) {}
#endif

} // namespace cplang