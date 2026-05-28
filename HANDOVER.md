# CP语言项目交接文档

**日期**: 2026-05-04  
**交接人**: AI Assistant  
**项目路径**: `C:\cplang`

---

## 1. 项目概况

CP语言是一个自研的中文编程语言，包含编译器+虚拟机全链路实现，基于C++17和LLVM 18.1.8。

- **定位**: 生产级中文编程语言，对标C++
- **当前阶段**: Year 1 Q1-Q2 "JIT基础设施"初期，Phase 0收尾
- **构建方式**: MSVC编译器（`build_msvc.bat`），ninja/cmake未安装
- **总函数数**: ~282+，中文别名~110+

---

## 2. 已修复：全局变量Slot化立即数偏移Bug ✅

### 2.1 问题
编译时 `mean` 映射到 slot 278，运行时 `OP_LOADGLOBAL` 却读到 slot 0。

### 2.2 根因
`emitInt` 格式: `[op][a][0][0][imm32小端][pad8]`
op 取出后 pc 指向 a，imm32 在 `pc+3`。

但 `OP_LOADGLOBAL` 和 `OP_STOREGLOBAL` 用了 `pc+4~7`，差1字节。
所有其他立即数指令（LOADINT/LOADSTR/JUMP等）都正确用 `pc+3~6`。

### 2.3 修复
`src/vm/vm.cpp`: 两处解码偏移 `pc+4~7` → `pc+3~6`

### 2.4 验证
- `test_mean_debug3.cp`: slot 278 编译时与运行时一致
- `test_mean_debug2.cp`: 变量 a slot 317，正确输出 `[1, 2, 3]`

---

## 3. 已完成的修复

### 3.1 VMTable哈希表改造 ✅
- `std::vector` → `std::unordered_map`，O(n)→O(1)

### 3.2 OP_LOADGLOBAL/STOREGLOBAL解码偏移 ✅
- `pc+4~7` → `pc+3~6`

### 3.3 globalSlots_越界保护 ✅
- `registerNative`/`registerGlobal` 添加 resize 检查

### 3.4 字符串/数组拼接 ✅
- `OP_ADD` 支持字符串和数组拼接

### 3.5 结构体字段双存储 ✅
- 整数索引+字符串键双存储

---

## 4. 待修复问题

### 4.1 中优先级
1. **示例文件旧版API** - `escape_demo.cpp`, `tail_rec_demo.cpp`等仍用旧Lexer/Parser API
2. **CMake模块化重构** - 仍用bat脚本
3. **JIT编译器框架** - 未接入编译管线

### 4.2 低优先级
4. **scripts/fixes/下40+临时修复脚本**待清理
5. **Clang 18.1.8与MSVC STL 14.44不兼容** - 需升级至Clang 19+

---

## 5. 标准库实现状态

| 模块 | 状态 | 备注 |
|------|------|------|
| 数学 | ✅ | 45+函数 |
| 字符串 | ✅ | 45+函数 |
| 数组 | ✅ | 31+函数 |
| 表 | ✅ | 6+函数 |
| 集合/栈/队列 | ✅ | 各7-9函数 |
| 位运算/算法 | ✅ | 10+/13函数 |
| 随机数/正则 | ✅ | 5/3函数 |
| 加密哈希/编码 | ✅ | 5/2函数 |
| IO/时间/系统 | ✅ | 各4-7函数 |
| 文件操作 | ✅ | 15函数 |
| 日期时间 | ✅ | 12函数 |
| 进程信息 | ✅ | 4函数 |
| HTTP客户端 | ✅ | WinHTTP实现 |
| TCP/UDP | ✅ | Winsock实现 |
| JSON处理 | ✅ | 自研实现 |
| 矩阵/向量 | ✅ | 9函数 |
| **统计函数** | ✅ | 已修复，可正常加载 |

---

## 6. 关键文件路径

| 文件 | 说明 |
|------|------|
| `src/vm/vm.cpp` | VM主循环，含OP_LOADGLOBAL/STOREGLOBAL |
| `src/codegen/codegen.cpp` | 代码生成器，含emitInt |
| `include/vm/vm.hpp` | VM头文件，含globalSlots_声明 |
| `src/stdlib/stdlib.cpp` | 标准库实现 |
| `include/stdlib/stdlib.hpp` | 标准库头文件 |
| `build_msvc.bat` | MSVC构建脚本 |
| `STDLIB_ROADMAP.md` | 标准库路线图 |

---

## 7. 构建命令

```bash
# MSVC构建
cmd /c build_msvc.bat

# 运行测试
.\build\cplang.exe -c test_file.cp
```

---

## 8. 下一步行动

1. 修复示例文件旧版 API 兼容
2. 清理临时修复脚本
3. 启动 CMake 模块化重构

---

## 9. 标准库新增（2026-05-04）

### 新增 VM 类型
- **VMDeque**（TAG_DEQUE=6, T_DEQUE=17）：双端队列，基于 std::deque
- **VMPriorityQueue**（TAG_PRIORITY_QUEUE=7, T_PRIORITY_QUEUE=18）：优先队列，最大堆
- **VMLinkedList**（TAG_LINKEDLIST=16, T_LINKEDLIST=19）：双向链表，基于 std::list
- **VMSLinkedList**（TAG_SLINKEDLIST=17, T_SLINKEDLIST=20）：单向链表，基于 std::forward_list
- **VMMultiSet**（TAG_MULTISET=18, T_MULTISET=21）：多重集合，基于 std::multiset + ValueLess 比较器
- **VMMultiMap**（TAG_MULTIMAP=19, T_MULTIMAP=22）：多重映射，基于 std::multimap + ValueLess 比较器
- **VMUnorderedSet**（TAG_UNORDERED_SET=20, T_UNORDERED_SET=23）：无序集合，基于 std::unordered_set + ValueHash/ValueEqual
- **VMUnorderedMultiSet**（TAG_UNORDERED_MULTISET=21, T_UNORDERED_MULTISET=24）：无序多重集合
- **VMUnorderedMap**（TAG_UNORDERED_MAP=22, T_UNORDERED_MAP=25）：无序映射，基于 std::unordered_map + ValueHash/ValueEqual
- **VMUnorderedMultiMap**（TAG_UNORDERED_MULTIMAP=23, T_UNORDERED_MULTIMAP=26）：无序多重映射

### 新增标准库模块
| 模块 | 函数数 | 中文别名 | 对标 C++ | 实现方式 |
|------|--------|----------|----------|----------|
| 双端队列 | 12 | 12 | std::deque | VMDeque 原生类型 |
| 优先队列 | 8 | 8 | std::priority_queue | VMPriorityQueue 原生类型 |
| 链表 | 16 | 16 | std::list | VMLinkedList 原生类型 |
| 单向链表 | 15 | 15 | std::forward_list | VMSLinkedList 原生类型 |
| 多重集合 | 12 | 12 | std::multiset | VMMultiSet 原生类型 |
| 无序集合 | 9 | 9 | std::unordered_set | VMUnorderedSet + ValueHash |
| 无序多重集合 | 10 | 10 | std::unordered_multiset | VMUnorderedMultiSet |
| 无序映射 | 10 | 10 | std::unordered_map | VMUnorderedMap |
| 无序多重映射 | 11 | 11 | std::unordered_multimap | VMUnorderedMultiMap |
| 复数 | 10 | 10 | std::complex | 表存储 {real, imag} |
| 对组 | 4 | 4 | std::pair | 表存储 {first, second} |

### 算法扩展（2026-05-04）
新增 15 个算法函数：prevPermutation, isSortedUntil, partialSort, nthElement, merge, inplaceMerge, setUnion, setIntersection, setDifference, setSymmetricDiff, minElement, maxElement, unique, rotate

### 随机分布扩展
新增 4 个分布函数：randomUniformInt, randomExponential, randomBernoulli, randomPoisson

### ObjectHeader::Tag 碰撞修复
原 TAG_CLOSURE=7 与 TAG_PRIORITY_QUEUE=7 冲突，TAG_CLASS=8 与 TAG_FUNCTION=8 冲突。
修复后分配：TAG_CLOSURE=9, TAG_CLASS=10, TAG_INSTANCE=11, TAG_UPVALUE=12, TAG_NATIVE=13, TAG_STRUCT=14, TAG_USERDATA=15, TAG_LINKEDLIST=16

### GC 增强
- 为 TAG_SET/TAG_STACK/TAG_QUEUE/TAG_DEQUE/TAG_PRIORITY_QUEUE 添加了 gcMarkObject 内部元素追踪

### 当前总计
- **总函数数：~580+**，中文别名：~330+
- 编译状态：MSVC build_msvc.bat 通过，仅预存 size_t 截断警告