// CP 语言 核心标准库（AOT 模式最小运行时）
// 仅包含绝对必要的基础模块，避免链接全量 stdlib
#include "stdlib/stdlib.hpp"

namespace cplang {

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
    registerNetwork(vm);      // 基础网络
    registerFormat(vm);       // 格式化
    registerPath(vm);         // 路径处理
    registerMemory(vm);       // 内存
    registerFixMissing(vm);   // 修复缺失
    registerRandom(vm);       // 随机数（getRandomValue 等）
    registerIOPoll(vm);       // IO轮询
}

} // namespace cplang
