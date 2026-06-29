#pragma once
#include "vm/vm.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace cplang {

// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?
//  鏍囧噯搴撳嚱鏁版敞鍐岋紙鍓嶅悜澹版槑锛?
// 鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺愨晲鈺?

class StdLib {
public:
    // 娉ㄥ唽鎵€鏈夋爣鍑嗗簱鍑芥暟鍒?VM
    static void registerAll(VM* vm);
    
    // 鎸夌被鍒敞鍐岋紙鍚勬ā鍧楀疄鐜板湪瀵瑰簲瀛愭枃浠朵腑锛?
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
    static void registerP5Algo(VM* vm);
    static void registerMathConst(VM* vm);
    static void registerFormat(VM* vm);
    static void registerResult(VM* vm);
    static void registerFunctional(VM* vm);
    static void registerSpan(VM* vm);
    static void registerIterator(VM* vm);
    static void registerAsync(VM* vm);      // 原生异步（P9.3）
    static void registerCharconv(VM* vm);
    static void registerSourceLoc(VM* vm);
    static void registerMemory(VM* vm);
    static void registerThreading(VM* vm);
    static void registerWebSocket(VM* vm);
    static void registerSqlite(VM* vm);
    static void registerMysql(VM* vm);
    static void registerPg(VM* vm);
    static void registerRedis(VM* vm);
    static void registerMongo(VM* vm);
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
    static void registerImage(VM* vm);     // 图片处理 (stb_image)
    static void registerCompress(VM* vm);  // 压缩解压 (miniz)
    static void registerArgparse(VM* vm);  // 命令行参数
    static void registerConfig(VM* vm);    // 配置文件 (TOML+INI)
    static void registerHTTPServer(VM* vm); // HTTP服务端 (Mongoose)
    static void registerTemplate(VM* vm);     // 模板引擎
    static void registerAudio(VM* vm);      // 音频播放 (miniaudio)
    static void registerMarkdown(VM* vm);   // Markdown转换
    static void registerPDF(VM* vm);        // PDF生成
    static void registerKVDB(VM* vm);       // 键值数据库
    static void registerTesting(VM* vm);    // 测试框架
    static void registerLogging(VM* vm);    // 日志框架
    static void registerLangEnhance(VM* vm);// 函数式+子进程

    // P11 标准库生态增强
    static void registerP11Algo(VM* vm);
    static void registerP11Graph(VM* vm);
    static void registerP11DS(VM* vm);
    static void registerP11Utils(VM* vm);

    // 娓告垙寮曟搸妯″潡 (v0.4.0 鈥?寮€绠卞嵆鐢?
    static void registerGameNet(VM* vm);
    static void registerGameBattle(VM* vm);
    static void registerGameMap(VM* vm);
    static void registerGameItem(VM* vm);
    static void registerGameRole(VM* vm);
    static void registerGameSprite(VM* vm);
    static void registerGameDb(VM* vm);
    static void registerGameConfig(VM* vm);
    static void registerWeb(VM* vm);
    static void registerTCP(VM* vm);

    // 二进制数据
    static void registerBinary(VM* vm);

    // 鏍稿績娉ㄥ唽锛圓OT 妯″紡浣跨敤锛屽彧娉ㄥ唽缁濆蹇呰鐨勫熀纭€妯″潡锛?
    static void registerCore(VM* vm);
    static void registerColors(VM* vm);  // 鍐呯疆棰滆壊甯搁噺
    
    // 鎸夐渶娉ㄥ唽鎺ュ彛锛圓OT 妯″紡浣跨敤锛屼粎娉ㄥ唽瀹為檯闇€瑕佺殑妯″潡锛?
    static void registerModules(VM* vm, const std::vector<std::string>& modules);

    // 杈呭姪鍑芥暟锛堝叕鏈夛紝渚?AOT 妗ユ帴澶栭儴娉ㄥ唽浣跨敤锛?
    static void registerFunction(VM* vm, const char* name, VMNativeFunc::Fn fn);
    static void registerAlias(VM* vm, const char* alias, const char* original);
};

// 鈹€鈹€ 鍐呰仈杈呭姪瀹炵幇锛堝畾涔夊湪绫诲锛屾柟渚挎ā鍧楃嫭绔嬬紪璇戯級鈹€鈹€
#ifndef CPLANG_STDLIB_IMPL_ONLY
inline void StdLib::registerFunction(VM* vm, const char* name, VMNativeFunc::Fn fn) {
    vm->registerNative(name, fn);
}
inline void StdLib::registerAlias(VM* vm, const char* alias, const char* original) {
    vm->registerNativeAlias(alias, original);
}
#endif

} // namespace cplang