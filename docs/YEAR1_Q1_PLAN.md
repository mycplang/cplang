# Year 1 Q1 详细任务计划

## 时间范围
2026-04-21 — 2026-07-21（3个月）

## 核心里程碑

### M1：LLVM JIT 集成（优先级最高）

**当前问题**：
```
当前流程：字节码 → 生成 LLVM IR 文件 → clang.exe 编译 → lld-link 链接 → LoadLibrary
问题：
1. 每次编译启动外部进程（~50-200ms 开销）
2. 需要写文件到磁盘（I/O 开销）
3. 链接器启动慢
```

**目标**：
```
目标流程：字节码 → LLVM IR（内存）→ ORC JIT 编译（内存）→ 直接执行
优点：
1. 无外部进程开销
2. 内存中完成，无磁盘 I/O
3. 编译延迟 < 10ms
```

**任务拆解**：

#### Week 1-2：LLVM ORC JIT 基础设施

1. **安装 LLVM 开发库**
   - 下载 LLVM 源码或预编译开发包
   - 配置 CMake 构建
   - 链接 LLVM 库到 cplang

2. **创建 ORC JIT 类**
   ```cpp
   // include/jit/orc_jit.hpp
   class OrcJIT {
   public:
       OrcJIT();
       ~OrcJIT();
       
       // 添加 LLVM 模块
       Error addModule(std::unique_ptr<Module> M);
       
       // 查找符号
       Expected<JITEvaluatedSymbol> lookup(StringRef Name);
       
       // 编译并获取函数指针
       void* compile(const std::string& ir);
   private:
       std::unique_ptr<ExecutionSession> ES_;
       std::unique_ptr<RTDyldObjectLinkingLayer> ObjectLayer_;
       std::unique_ptr<IRCompileLayer> CompileLayer_;
       std::unique_ptr<IRTransformLayer> TransformLayer_;
   };
   ```

3. **集成测试**
   - 编写简单的 LLVM IR 字符串
   - 通过 ORC JIT 编译执行
   - 验证函数调用正确

#### Week 3-4：字节码 → LLVM IR 转换

1. **完善 LLVMCodegen**
   - 当前已有 `include/codegen/llvm_codegen.hpp`
   - 需要确保生成有效的 LLVM IR

2. **集成热点检测**
   ```cpp
   // 在 VM 中集成
   if (jit_->shouldCompile(currentFunction_)) {
       void* compiled = jit_->compile(currentFunction_);
       if (compiled) {
           currentFunction_->setCompiledCode(compiled);
       }
   }
   ```

3. **性能测试**
   - fibonacci(35) 性能对比
   - 编译延迟测试
   - 内存占用测试

---

### M2：字节码优化器（并行进行）

**当前状态**：
- 已有 `constant_folder.hpp` — 常量折叠
- 已有 `dead_code_eliminator.hpp` — 死代码消除
- 已有 `optimizer.hpp` — 优化器入口

**任务**：

#### Week 5-6：集成优化器到编译管线

1. **创建优化管线**
   ```cpp
   class BytecodeOptimizer {
   public:
       void optimize(std::vector<Instruction>& bytecode);
   private:
       ConstantFolder constantFolder_;
       DeadCodeEliminator dce_;
       // 后续添加更多优化
   };
   ```

2. **优化策略**
   ```
   Pass 1: 常量折叠
      var x = 1 + 2  →  var x = 3
   
   Pass 2: 死代码消除
      var unused = 10  →  (删除)
   
   Pass 3: 基本块重排
      优化跳转目标，减少分支
   ```

3. **效果测试**
   - 测试用例优化前后字节码大小
   - 执行时间对比

---

### M3：内联缓存（Week 7-12）

**目标**：加速方法调用和全局变量访问

#### Week 7-8：全局变量内联缓存

**当前问题**：
```cpp
// 每次访问全局变量都要查表
OP_GET_GLOBAL:
    name = readString(ip);
    value = globals[name];  // O(n) 或 O(log n)
```

**优化方案**：
```cpp
// 第一次访问时记录 slot
OP_GET_GLOBAL_CACHED:
    slot = ip->imm32;       // 编译时或首次执行时分配
    if (slot == -1) {
        // 首次访问，查表并缓存
        slot = getOrCreateGlobalSlot(name);
    }
    value = globalSlots[slot];  // O(1)
```

**实现任务**：
1. 为每个全局变量分配固定 slot
2. 修改 codegen 生成 `OP_GET_GLOBAL_CACHED` 指令
3. VM 中实现缓存逻辑

#### Week 9-10：方法调用内联缓存

**当前问题**：
```cpp
// 每次方法调用都要查找方法
obj.method(args)  →  查找 obj 的类型 → 查找方法 → 调用
```

**优化方案**：
```cpp
// 内联缓存：记录上次的查找结果
struct InlineCache {
    void* lastClass;      // 上次的类
    void* lastMethod;     // 上次的方法
};

// 调用时
if (cache.lastClass == obj->class) {
    // 缓存命中，直接调用
    cache.lastMethod(obj, args);
} else {
    // 缓存未命中，查找并更新
    method = lookupMethod(obj->class, methodName);
    cache.lastClass = obj->class;
    cache.lastMethod = method;
    method(obj, args);
}
```

**实现任务**：
1. 定义 InlineCache 结构
2. 修改 call 指令支持缓存
3. 性能测试

#### Week 11-12：集成测试和性能基准

1. **Benchmark 套件**
   ```
   benchmarks/
   ├── bench_fibonacci.cp    // 递归测试
   ├── bench_loop.cp         // 循环测试
   ├── bench_array.cp        // 数组操作测试
   ├── bench_string.cp       // 字符串测试
   └── bench_method.cp       // 方法调用测试
   ```

2. **性能对比表**
   | 测试项 | 解释执行 | +字节码优化 | +内联缓存 | +JIT |
   |-------|---------|-----------|----------|------|
   | fib(35) | 2.5s | 2.3s | 2.2s | 0.05s |
   | loop 100M | 0.8s | 0.6s | 0.5s | 0.01s |
   | ... | ... | ... | ... | ... |

3. **性能报告**
   - 生成性能对比图表
   - 分析瓶颈
   - 确定下一步优化方向

---

## 资源需求

### 开发环境

1. **LLVM 开发库**
   ```bash
   # 下载地址
   https://github.com/llvm/llvm-project/releases
   
   # 或使用 CMake 构建
   cmake -DLLVM_ENABLE_PROJECTS=orcjit ...
   ```

2. **构建系统**
   - 当前使用 CMakeLists.txt
   - 需要添加 LLVM 依赖

### 人力投入

| 任务 | 预计工时 | 技能要求 |
|-----|---------|---------|
| LLVM ORC JIT 集成 | 80h | C++、LLVM API |
| 字节码优化器 | 40h | 编译原理 |
| 内联缓存 | 60h | VM 实现经验 |
| 测试和文档 | 40h | 测试方法 |
| **总计** | **220h** | |

---

## 风险和对策

### 风险 1：LLVM ORC API 复杂

**可能性**：高
**影响**：中
**对策**：
- 参考 LLVM 官方示例
- 从最小可用版本开始，逐步完善
- 可考虑使用 MCJIT 作为过渡方案

### 风险 2：性能提升不如预期

**可能性**：中
**影响**：高
**对策**：
- 设定阶段性目标，逐步验证
- 性能分析定位瓶颈
- 调整优化策略

### 风险 3：编译延迟影响用户体验

**可能性**：中
**影响**：中
**对策**：
- 后台线程编译
- 分层编译（快速编译 + 后续优化）
- 预编译常用函数

---

## 成功标准

### M1：LLVM JIT 集成
- [ ] ORC JIT 可用，无外部进程
- [ ] 编译延迟 < 50ms（简单函数）
- [ ] fibonacci(35) < 0.1s

### M2：字节码优化器
- [ ] 常量折叠正常工作
- [ ] 死代码消除正常工作
- [ ] 字节码大小减少 10%+

### M3：内联缓存
- [ ] 全局变量访问 O(1)
- [ ] 方法调用缓存命中率 > 90%
- [ ] 整体性能提升 20%+

---

## 下一步行动

### 本周任务（2026-04-21 ~ 2026-04-27）

1. **安装 LLVM 开发库**
   - 下载 LLVM 18.x 预编译包
   - 配置开发环境
   - 编写测试程序验证环境

2. **学习 ORC JIT API**
   - 阅读 LLVM 官方文档
   - 运行官方示例
   - 理解核心概念

3. **创建 ORC JIT 基础类**
   - 定义接口
   - 实现最小可用版本
   - 单元测试

---

**文档版本**：v1.0
**创建日期**：2026-04-21
**负责人**：待定
