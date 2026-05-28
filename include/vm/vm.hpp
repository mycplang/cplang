// CP语言 虚拟机总体头文件
// ⚡ 此文件由拆分工具自动生成，仅包含各子模块的 include
#pragma once

// 1. 基础前向声明 + 对象头 + GC 颜色
#include "vm/vm_fwd.hpp"

// 2. NaN-boxing Value 类型（依赖 vm_fwd.hpp 中的前向声明）
#include "vm/value.hpp"

// 3. 字节码指令枚举
#include "vm/vm_opcodes.hpp"

// 4. 所有 VM 对象结构体定义
#include "vm/vm_object.hpp"

// 5. Value 内联辅助函数（is* / as* / make* 等）
#include "vm/vm_value_helpers.hpp"

// 6. 调用帧 / 异常处理帧
#include "vm/vm_types.hpp"

// 7. VM 类声明
#include "vm/vm_class.hpp"
