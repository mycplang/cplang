# CP语言编译器 - 字节码格式修复

## 已完成的修复

1. **codegen.cpp** - 统一为8字节格式
   - `emit()`: [op][a][b][c][padding4] = 8字节
   - `emitInt()`: [op][a][b][c][imm32] = 8字节
   - `emitJump()`: [op][a][b][c][offset32] = 8字节
   - `emitJumpPlaceholder()`: [op][a][b][c][offset32=0] = 8字节
   - `patchJump()`: 写4字节偏移量

2. **vm.cpp** - 需要对应的修复
   - 主循环读取: [op][a][b][c] 共4字节，然后各case按需读取后续4字节imm
   - emitInt类指令(LOADINT/LOADCONST/LOADGLOBAL/STOREGLOBAL等): 读4字节imm
   - ABC类指令(ADD/SUB/MOVE/CALL等): 直接用a,b,c，跳过后续4字节

## 下一步

手动修复vm.cpp中所有指令case:
1. 更新宏定义: `#define RR_a (base[a_reg])` 等
2. 每个case根据指令类型处理字节读取
3. ABC类: `ctx->pc += 4` 跳过padding
4. Imm类: 读取后续4字节作为imm值
