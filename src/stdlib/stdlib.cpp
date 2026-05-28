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
}

void StdLib::registerFunction(VM* vm, const char* name, VMNativeFunc::Fn fn) {
    vm->registerNative(name, fn);
}

void StdLib::registerAlias(VM* vm, const char* alias, const char* original) {
    vm->registerNativeAlias(alias, original);
}

// ═══════════════════════════════════════════════════════════════════
//  提取到独立文件的模块 (extracted modules)
// ═══════════════════════════════════════════════════════════════════

#include "stdlib_numeric_limits.cpp"
#include "stdlib_heap.cpp"
#include "stdlib_str_search.cpp"
#include "stdlib_algo_ext.cpp"
#include "stdlib_reflect.cpp"
#include "stdlib_math.cpp"
#include "stdlib_string.cpp"
#include "stdlib_array.cpp"
#include "stdlib_table.cpp"
#include "stdlib_io.cpp"
#include "stdlib_time_system.cpp"
#include "stdlib_file.cpp"
#include "stdlib_types_net.cpp"
#include "stdlib_containers.cpp"
#include "stdlib_bitset.cpp"
#include "stdlib_complex_pair.cpp"
#include "stdlib_algo_bitwise.cpp"
#include "stdlib_regex.cpp"
#include "stdlib_string_ext.cpp"
#include "stdlib_crypto_impl.cpp"
#include "stdlib_encoding.cpp"
#include "stdlib_string_more_impl.cpp"
#include "stdlib_array_file_more.cpp"
#include "stdlib_time_sys_more.cpp"
#include "stdlib_stats_utils.cpp"
#include "stdlib_json_http.cpp"
#include "stdlib_matrix_color_path_console.cpp"
#include "stdlib_variant_utils.cpp"
#include "stdlib_net_ws_sql.cpp"

#include "stdlib_threading.cpp"
#include "stdlib_iterator.cpp"
#include "stdlib_misc_modules.cpp"
#include "stdlib_math_special.cpp"
#include "stdlib_algo_missing.cpp"
#include "stdlib_p1_enhance.cpp"
#include "stdlib_map.cpp"
#include "stdlib_charset.cpp"
#include "stdlib_crypto_plus.cpp"
#include "stdlib_file_log.cpp"
#include "stdlib_p2_more.cpp"
#include "stdlib_aes.cpp"
#ifdef _WIN32
#include "stdlib_db.cpp"
#include "stdlib_redis.cpp"
#include "stdlib_r10_r11.cpp"
#include "stdlib_http.cpp"
#include "stdlib_p3_util.cpp"
#else
static void registerMysql(cplang::VM*) {}
static void registerPg(cplang::VM*) {}
static void registerRedis(cplang::VM*) {}
static void registerHttp(cplang::VM*) {}
static void registerLogPlus(cplang::VM*) {}
#endif
// stdlib_raylib.cpp → compiled separately as stdlib_raylib_unit.cpp to isolate raylib.h from windows.h conflicts

} // namespace cplang
