# CP语言 AOT 编译经验文档

> 基于 2026年6月 贪吃蛇AOT修复实战总结

## 一、AOT 架构概述

AOT 编译将 CP 源码编译为 Windows 原生 PE 可执行文件（.exe）。

```
源码(.cp) → Lexer → Parser → SemanticAnalyzer → LLVMCodegen
                                                      │
                                          LLVM IR (.ll 文本)
                                                      │
                                          llc.exe -O0 -filetype=obj
                                                      │
                                          原生 .obj + aot_vm_bridge.obj
                                          + cplang_aot.lib + jit_runtime.lib
                                                      │
                                          link.exe → .exe
```

**关键特征**：
- 用户代码（控制流/算术）→ 原生机器码
- 标准库调用 → `aot_call_native` 桥接到内嵌 VM
- 图形函数 → bridge 直接调用 Raylib C 函数（绕过 VM）
- LLVM 优化级别强制 -O0（防止 NaN-boxing 算术被破坏）

## 二、已修复的 Bug 及根因

### Bug 1: CRT 运行时库不匹配

**症状**：AOT 链接失败，76 个 `LNK2038` 错误

**根因**：cplang_aot.lib 用 /MD 编译，aot_vm_bridge.obj 用 /MT 编译，链接器拒绝混合 CRT。

**修复**：
- `aot_compiler.cpp` 的 `linkToExecutable()` 统一使用动态 CRT
- 排除所有默认 CRT 库的自动选择，手动指定 msvcrt.lib + libvcruntime.lib + libucrt.lib
- 启用 `/FORCE:MULTIPLE` 兜底

### Bug 2: 数组操作失败（len返回垃圾值）

**症状**：`长度(arr)` 返回 6（错误），`arr[0]` 返回 nil

**根因**：LLVM codegen 对数组有两条路径：
1. `[]` 空数组 → 编译为 `jit_table_create()` → 创建 **TableData**（standalone 运行时）
2. `追加`/`弹出`/`插入` → 编译为 standalone `push`/`pop`/`insert` → 操作 TableData
3. `长度` → 编译为 `jit_len` → **只识别 VM 对象，不识别 TableData**

三条路径用三种不同数据表示（TableData / VMArray / VMTable），互相不兼容。

**修复**：
- `jit_runtime.cpp` 的 `jit_len`：添加 TableData 检测（首8字节 = TABLE_MAGIC）
- `llvm_codegen.cpp`：添加中文函数名 standalone 映射（`追加`→`push`，`弹出`→`pop`，`插入`→`insert`）
- `jit_runtime_standalone.cpp`：`len`/`arrlen`/`push`/`pop`/`insert` 添加 VMArray 路由
- `aot_vm_bridge.cpp`：`toVMValue`/`aot_ptr_to_value` 用 typeTag 识别 VM 对象类型
- `cplang_aot.lib`：移除 `cplang_jit.lib`（其 VMTable 版 jit_table_* 与 standalone TableData 版冲突）
- `jit_runtime_standalone.cpp`：添加 `jitTryCallDispatch` C++ 桩函数

### Bug 3: var 表变量导致白色窗口/无响应

**症状**：使用 `var 颜色 = {r:..., g:..., b:..., a:...}` 声明 2 个以上颜色表变量时，窗口白色背景、无响应、3-4 秒后消失

**根因**：LLVM codegen 对多个 `var` 表变量的编译有 bug。表变量在循环中被引用时表现异常。**此 bug 根因未完全修复**，仅通过代码规避。

**规避方法**：不要使用 `var` 或 `常量` 存储颜色表。颜色全部用内联表字面量直接传参。

### Bug 4: 内联表字面量导致内存泄漏

**症状**：蛇身和食物不显示，仅背景和 FPS 可见

**根因**：内联表字面量（如 `{r:80, g:80, b:80, a:255}`）在 LLVM codegen 中每帧被重新创建（`jit_table_create` + 4×`jit_table_set`），分配在 `tracked_malloc` 堆上永不释放。循环中 1200 次/帧的表创建导致快速内存耗尽。

**规避方法**：减少循环内内联表的使用量。每帧控制在 10 次以内。

### Bug 5: bridge 绕过不匹配中文函数名

**症状**：图形函数（绘制矩形/清空背景）走慢路径（VM native），表参数转换失败

**根因**：`aot_vm_bridge.cpp` 的直接 Raylib 绕过只检查英文函数名（`drawRectangle`），但 LLVM codegen 传递的是中文原始函数名（`绘制矩形`）。

**修复**：在绕过判断中添加中文 UTF-8 字节匹配。

## 三、AOT 兼容的 CP 代码规范

### ✅ DO

```cp
// 变量声明用 变量（不用 var 声明表）
变量 x = 42;
变量 arr = [];
追加(arr, 10);

// 颜色全部用内联表字面量直接传参
清空背景({r: 30, g: 30, b: 50, a: 255});
绘制矩形(x, y, w, h, {r: 0, g: 228, b: 48, a: 255});

// 字符串操作正常
打印("Hello " + 转字符串(x));

// 键盘检测正常
如果 (键盘按下(键_右)) { ... }

// 随机值正常
变量 rx = 随机值(0, 100);
```

### ❌ DON'T

```cp
// 不要用 var 或 常量 声明颜色表（≥2个会触发 white-screen bug）
var 黑 = {r: 0, g: 0, b: 0, a: 255};   // ❌
常量 红 = {r: 255, g: 0, b: 0, a: 255}; // ❌

// 不要在大循环内创建内联表（每帧超过 ~50 次会内存泄漏）
当 (i < 1200) {
    绘制矩形(i, j, 10, 10, {r: 80, g: 80, b: 80, a: 255}); // ❌ 1200次/帧
    i = i + 1;
}

// 不要使用 import 模块（AOT 不支持运行时导入）
导入("mymodule"); // ❌
```

### 规避 var 表 bug 的颜色方案

| 方案 | 可行性 | 说明 |
|------|--------|------|
| 内联表字面量 `{r:..., g:..., b:..., a:...}` | ✅ | 推荐，每帧 ≤ 10 次 |
| 1 个 var 表变量 | ✅ | dbg2 验证可行 |
| ≥2 个 var 表变量 | ❌ | white-screen bug |
| 常量 | ❌ | 和 var 行为相同 |

## 四、构建流程

### 前提

- Visual Studio 2022 (MSVC v143)
- LLVM 18+ 开发包（放在 `C:\CPLANG\llvm-dev\`）
- CMake 3.20+

### 完整构建步骤

```powershell
# 1. 清理
Remove-Item C:\CPLANG\build_msvc\CMakeCache.txt -Force

# 2. CMake 配置
cd C:\CPLANG\build_msvc
cmake C:\CPLANG -DCMAKE_BUILD_TYPE=Release -DCPLANG_USE_LLVM=ON -G Ninja

# 3. 编译 cplang.exe
cmake --build . --config Release

# 4. 编译 AOT 支持文件
cl /c /EHsc /std:c++17 /O2 /MD /I../include /Fo:aot_vm_bridge.obj ../src/aot/aot_vm_bridge.cpp
cl /c /EHsc /std:c++17 /O2 /MD /I../include /Fo:jit_runtime_standalone.obj ../src/jit/jit_runtime_standalone.cpp
lib /OUT:jit_runtime.lib jit_runtime_standalone.obj

# 5. 合并 cplang_aot.lib（不含 cplang_jit.lib！）
lib /OUT:bin/cplang_aot.lib cplang_stdlib.lib cplang_vm.lib cplang_codegen.lib cplang_parser.lib cplang_semantic.lib cplang_lexer.lib cplang_optimizer.lib cplang_module.lib cplang_debug.lib cplang_exception.lib cplang_core.lib
copy /Y bin/cplang_aot.lib bin/cplang_graphics.lib

# 6. 部署
copy aot_vm_bridge.obj bin\
copy jit_runtime.lib bin\
copy ..\third_party\raylib\build_release\raylib\raylib.lib bin\
```

### cplang_aot.lib 组成说明

```
cplang_aot.lib = cplang_stdlib.lib   (标准库)
               + cplang_vm.lib       (虚拟机)
               + cplang_codegen.lib  (代码生成)
               + cplang_parser.lib   (语法分析)
               + cplang_semantic.lib (语义分析)
               + cplang_lexer.lib    (词法分析)
               + cplang_optimizer.lib(优化器)
               + cplang_module.lib   (模块系统)
               + cplang_debug.lib    (调试器)
               + cplang_exception.lib(异常处理)
               + cplang_core.lib     (运行时核心)

注意：不要包含 cplang_jit.lib！
其 jit_table_* 的 VMTable 版本与 jit_runtime.lib 的 TableData 版本冲突。
```

## 五、故障排查

### AOT 编译失败

| 症状 | 可能原因 | 检查方法 |
|------|---------|---------|
| `LNK2038: RuntimeLibrary不匹配` | CRT 不匹配 | 确认所有 obj 用统一 /MD 或 /MT |
| `LNK2019: 无法解析的外部符号 LLVM*` | LLVM lib 路径错误 | 检查 `aot_compiler.cpp` 中 LLVM lib 路径 |
| `jitTryCallDispatch` 缺失 | cplang_jit.lib 被移除但桩函数缺失 | 检查 `jit_runtime_standalone.cpp` 中 C++ 桩 |

### AOT exe 运行异常

| 症状 | 可能原因 | 解决 |
|------|---------|------|
| 白色窗口+无响应 | var 表变量 bug | 去掉所有 var/常量 表变量 |
| 画面有背景但无图形 | 内联表内存泄漏 | 减少循环内内联表，< 50次/帧 |
| 数组操作异常 | TableData/VMTable 冲突 | 检查 cplang_aot.lib 不含 cplang_jit.lib |
| 蛇身不显示 | 绘制矩形未走绕过 | 检查 bridge 中文名匹配 |
| 闪退无日志 | 依赖 DLL 缺失 | `dumpbin /dependents` 检查 MSVCP140.dll 等 |

### 调试方法

```
# 查看 AOT bridge 日志
type aot_debug.log

# 查看 AOT 链接详情（在 cplang.exe 中）
.\cplang.exe -a -o test.exe test.cp -v 2>&1

# 查看 exe DLL 依赖
dumpbin /dependents test.exe

# 逐步测试（从简到繁）
test_simple.exe   → 只打印和变量
test_array.exe    → 数组操作
test_window.exe   → 基础窗口
mini_snake.exe    → 最简蛇（1个方块移动）
snake_v6.exe      → 完整蛇（无网格）
```

## 六、项目文件索引

| 文件 | 角色 | AOT 相关修改 |
|------|------|-------------|
| `src/codegen/aot_compiler.cpp` | AOT 编译流程控制 | CRT 链接、LLVM 扫描、链接命令 |
| `src/aot/aot_vm_bridge.cpp` | AOT 运行时桥接 | typeTag 识别、VMArray 适配、中文绕过、jit_len |
| `src/jit/jit_runtime.cpp` | JIT/VM 运行时 | jit_len TableData 检测 |
| `src/jit/jit_runtime_standalone.cpp` | AOT standalone 运行时 | VMArray 路由、jitTryCallDispatch 桩 |
| `src/codegen/llvm_codegen.cpp` | LLVM IR 代码生成 | 中文函数 standalone 映射、LLVM lib 路径 |
| `build.ps1` | 快速构建脚本 | aot_vm_bridge 编译、AOT 文件部署 |

---

> 最后更新：2026-06-10  
> AOT 状态：基础功能可用，完整贪吃蛇经 snake_v6.cp 验证通过
