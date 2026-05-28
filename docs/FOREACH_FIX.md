# CP语言 foreach 循环修复总结

## 问题描述
ForEach 循环 `遍历 (x : arr) { 打印(x); }` 产生无限循环，输出 `10` 无限次。

## 根因分析
在 `compileForEach` 函数中，使用 `emit(OP_LOADINT, oneReg, 1, 0)` 来加载立即数1。

但 `emit` 函数的格式是 `[op][a][b][c][padding7]`，这会将值1存储在字节3的 `b` 字段中。

而 VM 的 `OP_LOADINT` handler 期望立即数在字节4-7（使用 `emitInt` 格式 `[op][a][0][0][imm32]`）。

因此立即数1没有被正确读取，导致索引始终为0，循环无法退出。

## 修复方案
将 `emit(OP_LOADINT, oneReg, 1, 0)` 改为 `emitInt(OP_LOADINT, oneReg, 1)`。

## 修改文件
- `C:\Users\Administrator\Desktop\cplang\src\codegen\codegen.cpp`

## 验证结果
```
变量 arr = [10, 20];
遍历 (x : arr) {
    打印(x);
}
```
输出：
```
10
20
```

正确遍历数组并退出循环。

## 教训
- 16字节指令格式中，`emit` 和 `emitInt` 的立即数位置不同
- `emit`: `[op][a][b][c][padding7]` - 值在 b 字段
- `emitInt`: `[op][a][0][0][imm32]` - 值在 imm32 字段
- 使用错误的 emit 函数会导致指令参数读取错误
