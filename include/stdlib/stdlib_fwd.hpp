#pragma once
#include "vm/vm.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  标准库函数注册（前向声明）
// ═══════════════════════════════════════════════════════════════════

class StdLib {
public:
    // 注册所有标准库函数到 VM
    static void registerAll(VM* vm);
    
    // 按类别注册（各模块实现在对应子文件中）
    static void registerMath(VM* vm);
    static void registerString(VM* vm);
    static void registerArray(VM* vm);
    static void registerTable(VM* vm);
    static void registerIO(VM* vm);
    static void registerTime(VM* vm);
    static void registerSystem(VM* vm);
    static void registerFile(VM* vm);
    static void registerNetwork(VM* vm);
    static void registerJSON(VM* vm);
    static void registerHTTP(VM* vm);
    static void registerMatrix(VM* vm);
    static void registerColor(VM* vm);
    static void registerSocket(VM* vm);
    static void registerRegex(VM* vm);
    static void registerCrypto(VM* vm);
    static void registerEncoding(VM* vm);
    static void registerReflection(VM* vm);
    static void registerTypes(VM* vm);
    static void registerBitwise(VM* vm);
    static void registerAlgorithms(VM* vm);
    static void registerRandom(VM* vm);
    static void registerStringExt(VM* vm);
    static void registerStringMore(VM* vm);
    static void registerArrayMore(VM* vm);
    static void registerFileMore(VM* vm);
    static void registerTimeMore(VM* vm);
    static void registerSystemMore(VM* vm);
    static void registerProcess(VM* vm);
    static void registerMathMore(VM* vm);
    static void registerStatistics(VM* vm);
    static void registerUtils(VM* vm);
    static void registerStringCase(VM* vm);
    static void registerPath(VM* vm);
    static void registerConsole(VM* vm);
    static void registerOptional(VM* vm);
    static void registerVariant(VM* vm);
    static void registerAny(VM* vm);
    static void registerTuple(VM* vm);
    static void registerMoreUtils(VM* vm);
    static void registerSet(VM* vm);
    static void registerStack(VM* vm);
    static void registerQueue(VM* vm);
    static void registerDeque(VM* vm);
    static void registerPriorityQueue(VM* vm);
    static void registerLinkedList(VM* vm);
    static void registerSLinkedList(VM* vm);
    static void registerMultiSet(VM* vm);
    static void registerMultiMap(VM* vm);
    static void registerUnorderedSet(VM* vm);
    static void registerUnorderedMultiSet(VM* vm);
    static void registerUnorderedMap(VM* vm);
    static void registerUnorderedMultiMap(VM* vm);
    static void registerOrderedSet(VM* vm);
    static void registerOrderedMap(VM* vm);
    static void registerBitset(VM* vm);
    static void registerComplex(VM* vm);
    static void registerPair(VM* vm);
    static void registerNumericLimits(VM* vm);
    static void registerHeap(VM* vm);
    static void registerStringSearch(VM* vm);
    static void registerAlgoExt(VM* vm);
    static void registerMathConst(VM* vm);
    static void registerFormat(VM* vm);
    static void registerResult(VM* vm);
    static void registerFunctional(VM* vm);
    static void registerSpan(VM* vm);
    static void registerIterator(VM* vm);
    static void registerCharconv(VM* vm);
    static void registerSourceLoc(VM* vm);
    static void registerMemory(VM* vm);
    static void registerThreading(VM* vm);
    static void registerWebSocket(VM* vm);
    static void registerSqlite(VM* vm);
    static void registerMysql(VM* vm);
    static void registerPg(VM* vm);
    static void registerRedis(VM* vm);
    static void registerR10Misc(VM* vm);
    static void registerR11Misc(VM* vm);
    static void registerHttp(VM* vm);
    static void registerImGui(VM* vm);
    static void registerMathSpecial(VM* vm);
    static void registerAlgoMissing(VM* vm);
    static void registerCharconvFloat(VM* vm);
    static void registerSpanEnhance(VM* vm);
    static void registerResultMonad(VM* vm);
    static void registerTupleEnhance(VM* vm);
    static void registerBinaryIO(VM* vm);
    static void registerCallOnce(VM* vm);
    static void registerMap(VM* vm);
    static void registerCharset(VM* vm);
    static void registerCryptoPlus(VM* vm);
    static void registerFileSeek(VM* vm);
    static void registerFileWalk(VM* vm);
    static void registerLogger(VM* vm);
    static void registerTemp(VM* vm);
    static void registerFileStat(VM* vm);
    static void registerDuration(VM* vm);
    static void registerAes(VM* vm);
    static void registerDir(VM* vm);
    static void registerCsvWrite(VM* vm);
    static void registerLogPlus(VM* vm);
    static void registerStrCi(VM* vm);
    static void registerRaylib(VM* vm);
    static void registerFixMissing(VM* vm);
    static void registerFFI(VM* vm);
    static void registerIOPoll(VM* vm);

    // 核心注册（AOT 模式使用，只注册绝对必要的基础模块）
    static void registerCore(VM* vm);
    
    // 按需注册接口（AOT 模式使用，仅注册实际需要的模块）
    static void registerModules(VM* vm, const std::vector<std::string>& modules);

private:
    // 辅助函数
    static void registerFunction(VM* vm, const char* name, VMNativeFunc::Fn fn);
    static void registerAlias(VM* vm, const char* alias, const char* original);
};

// ── 内联辅助实现（定义在类外，方便模块独立编译）──
#ifndef CPLANG_STDLIB_IMPL_ONLY
inline void StdLib::registerFunction(VM* vm, const char* name, VMNativeFunc::Fn fn) {
    vm->registerNative(name, fn);
}
inline void StdLib::registerAlias(VM* vm, const char* alias, const char* original) {
    vm->registerNativeAlias(alias, original);
}
#endif

} // namespace cplang
