// CP 语言 核心标准库（AOT 模式最小运行时）
// 仅包含绝对必要的基础模块，避免链接全量 stdlib
#include "stdlib/stdlib.hpp"

namespace cplang {

static void registerColors(VM* vm) {
    // 内置颜色常量
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
    };
    for (auto& c : colors) {
        auto t = VMTable::create();
        t->set(makeStringVal(VMString::create("r")), Value::Int(c.r));
        t->set(makeStringVal(VMString::create("g")), Value::Int(c.g));
        t->set(makeStringVal(VMString::create("b")), Value::Int(c.b));
        t->set(makeStringVal(VMString::create("a")), Value::Int(c.a));
        vm->setGlobal(c.name, makeTableVal(t));
    }
}

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
    registerColors(vm);       // 内置颜色常量
}

} // namespace cplang
