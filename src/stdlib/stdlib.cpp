// 标准库实现

#include "stdlib/stdlib.hpp"
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
    // registerSocket(vm); // 已合并到 registerNetwork
    registerMathConst(vm);
    registerFormat(vm);
    registerResult(vm);
    registerFunctional(vm);
    registerSpan(vm);
    registerIterator(vm);
    registerCharconv(vm);
    registerSourceLoc(vm);
    registerMemory(vm);
    registerThreading(vm);

    registerWebSocket(vm);
    registerSqlite(vm);
    registerMysql(vm);
    registerPg(vm);
    registerRedis(vm);
    registerR10Misc(vm);
    registerR11Misc(vm);
    registerHttp(vm);
    registerImGui(vm);
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
    registerCryptoPlus(vm);
    registerFileSeek(vm);
    registerFileWalk(vm);
    registerLogger(vm);
    registerTemp(vm);
    registerFileStat(vm);
    registerDuration(vm);
    registerAes(vm);
    registerDir(vm);
    registerCsvWrite(vm);
    registerLogPlus(vm);
    registerStrCi(vm);
    registerRaylib(vm);
    registerFixMissing(vm);
    registerFFI(vm);
}

void StdLib::registerFunction(VM* vm, const char* name, VMNativeFunc::Fn fn) {
    vm->registerNative(name, fn);
}

void StdLib::registerAlias(VM* vm, const char* alias, const char* original) {
    vm->registerNativeAlias(alias, original);
}

// ═══════════════════════════════════════════════════════════════════
//  各模块的实现已提取到独立 .cpp 文件，由 CMake 统一编译链接
//  (之前通过 #include .cpp 文本拼接，现改为独立翻译单元)
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