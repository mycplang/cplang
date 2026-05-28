// 字节码优化器实现
#include "codegen/bytecode_optimizer.hpp"
#include "core/verbose.hpp"
#include <iostream>
#include <algorithm>
#include <queue>

namespace cplang {

// === 公共接口 ===

VMFunction* BytecodeOptimizer::optimize(VMFunction* func) {
    if (!func || level_ == OptLevel::None) {
        return func;
    }
    
    stats_ = BytecodeOptStats();
    size_t originalSize = func->code.size();
    
    // 按顺序应用优化通道
    if (level_ >= OptLevel::O1) {
        peepholeOptimize(func);
        eliminateDeadInstructions(func);
    }
    
    if (level_ >= OptLevel::O2) {
        propagateConstants(func);
        reorderBasicBlocks(func);
    }
    
    if (level_ >= OptLevel::O3) {
        allocateRegisters(func);
    }
    
    // 计算节省的字节数
    stats_.totalBytesSaved = static_cast<int64_t>(originalSize - func->code.size());
    
    return func;
}

void BytecodeOptimizer::printStats() const {
    VERBOSE(
        std::cout << "=== 字节码优化统计 ===" << std::endl;
        std::cout << "窥孔优化: " << stats_.peepholesApplied << " 次" << std::endl;
        std::cout << "死指令移除: " << stats_.deadInstructionsRemoved << " 条" << std::endl;
        std::cout << "常量传播: " << stats_.constantPropagations << " 次" << std::endl;
        std::cout << "寄存器分配: " << stats_.registersAllocated << " 个" << std::endl;
        std::cout << "节省字节: " << stats_.totalBytesSaved << " B" << std::endl;
    );
}

// === 优化通道 ===

void BytecodeOptimizer::peepholeOptimize(VMFunction* func) {
    if (func->code.size() < 2 * 16) return;  // 每条指令16字节
    
    std::vector<UInt8> newCode;
    size_t i = 0;
    const size_t n = func->code.size();
    const size_t instSize = 16;  // 每条指令16字节
    
    while (i < n) {
        bool optimized = false;

        // 异常处理指令（OP_TRY/OP_ENDTRY/OP_THROW）不参与窥孔优化
        // 直接复制保留，避免被模式匹配误伤
        UInt8 curOp = func->code[i];
        if (curOp == static_cast<UInt8>(Opcode::OP_TRY) ||
            curOp == static_cast<UInt8>(Opcode::OP_ENDTRY) ||
            curOp == static_cast<UInt8>(Opcode::OP_THROW)) {
            for (size_t j = 0; j < instSize; j++) {
                newCode.push_back(func->code[i + j]);
            }
            i += instSize;
            continue;
        }

        // ========== 第一类：单指令优化 ==========
        
        // 模式1：NOP 消除
        if (func->code[i] == static_cast<UInt8>(Opcode::OP_NOP)) {
            stats_.peepholesApplied++;
            stats_.deadInstructionsRemoved++;
            i += instSize;
            optimized = true;
            continue;
        }
        
        // ========== 第二类：双指令优化 ==========
        
        if (i + 2 * instSize <= n) {
            UInt8 op1 = func->code[i];
            UInt8 op2 = func->code[i + instSize];
            UInt8 a1 = func->code[i + 1];
            UInt8 a2 = func->code[i + instSize + 1];
            UInt8 b1 = func->code[i + 2];
            UInt8 b2 = func->code[i + instSize + 2];
            UInt8 c1 = func->code[i + 3];
            UInt8 c2 = func->code[i + instSize + 3];
            
            // 模式2：LOADLOCAL followed by STORELOCAL to same register (冗余)
            if (op1 == static_cast<UInt8>(Opcode::OP_LOADLOCAL) &&
                op2 == static_cast<UInt8>(Opcode::OP_STORELOCAL) &&
                a1 == a2) {
                stats_.peepholesApplied++;
                stats_.deadInstructionsRemoved += 2;
                i += 2 * instSize;
                optimized = true;
                continue;
            }
            
            // 模式3：连续相同的LOADCONST（可合并）
            if (op1 == static_cast<UInt8>(Opcode::OP_LOADCONST) &&
                op2 == static_cast<UInt8>(Opcode::OP_LOADCONST)) {
                // 比较立即数（指令的第4-7字节）
                bool sameConst = true;
                for (int j = 0; j < 4; j++) {
                    if (func->code[i + 4 + j] != func->code[i + instSize + 4 + j]) {
                        sameConst = false;
                        break;
                    }
                }
                if (sameConst && a1 == a2) {
                    stats_.peepholesApplied++;
                    stats_.deadInstructionsRemoved++;
                    // 只保留第一条
                    for (size_t j = 0; j < instSize; j++) {
                        newCode.push_back(func->code[i + j]);
                    }
                    i += 2 * instSize;
                    optimized = true;
                    continue;
                }
            }
            
            // 模式4：LOADINT 0 + NOT → 替换为 LOADBOOL true
            if (op1 == static_cast<UInt8>(Opcode::OP_LOADINT) &&
                op2 == static_cast<UInt8>(Opcode::OP_NOT)) {
                // 检查 LOADINT 是否加载的是 0
                Int32 val = 0;
                val |= static_cast<Int32>(func->code[i + 4]) & 0xFF;
                val |= (static_cast<Int32>(func->code[i + 5]) & 0xFF) << 8;
                val |= (static_cast<Int32>(func->code[i + 6]) & 0xFF) << 16;
                val |= (static_cast<Int32>(func->code[i + 7]) & 0xFF) << 24;
                
                if (val == 0 && a1 == a2) {
                    stats_.peepholesApplied++;
                    // 生成 OP_LOADBOOL true
                    newCode.push_back(static_cast<UInt8>(Opcode::OP_LOADBOOL));
                    newCode.push_back(a1);  // 目标寄存器
                    newCode.push_back(1);   // true
                    newCode.push_back(0);
                    for (int j = 0; j < 12; j++) newCode.push_back(0);
                    i += 2 * instSize;
                    optimized = true;
                    continue;
                }
            }
            
            // 模式5：LOADBOOL true + NOT → 替换为 LOADBOOL false
            if (op1 == static_cast<UInt8>(Opcode::OP_LOADBOOL) &&
                op2 == static_cast<UInt8>(Opcode::OP_NOT) &&
                b1 == 1 && a1 == a2) {
                stats_.peepholesApplied++;
                // 生成 OP_LOADBOOL false
                newCode.push_back(static_cast<UInt8>(Opcode::OP_LOADBOOL));
                newCode.push_back(a1);
                newCode.push_back(0);  // false
                newCode.push_back(0);
                for (int j = 0; j < 12; j++) newCode.push_back(0);
                i += 2 * instSize;
                optimized = true;
                continue;
            }
            
            // 模式6：LOADBOOL false + NOT → 替换为 LOADBOOL true
            if (op1 == static_cast<UInt8>(Opcode::OP_LOADBOOL) &&
                op2 == static_cast<UInt8>(Opcode::OP_NOT) &&
                b1 == 0 && a1 == a2) {
                stats_.peepholesApplied++;
                newCode.push_back(static_cast<UInt8>(Opcode::OP_LOADBOOL));
                newCode.push_back(a1);
                newCode.push_back(1);  // true
                newCode.push_back(0);
                for (int j = 0; j < 12; j++) newCode.push_back(0);
                i += 2 * instSize;
                optimized = true;
                continue;
            }
            
            // 模式7：LOADINT X + LOADINT X → 合并为一个 (如果目标寄存器不同)
            if (op1 == static_cast<UInt8>(Opcode::OP_LOADINT) &&
                op2 == static_cast<UInt8>(Opcode::OP_LOADINT)) {
                bool sameVal = true;
                for (int j = 0; j < 4; j++) {
                    if (func->code[i + 4 + j] != func->code[i + instSize + 4 + j]) {
                        sameVal = false;
                        break;
                    }
                }
                if (sameVal) {
                    stats_.peepholesApplied++;
                    // 保留两条指令，但这个模式我们不处理（寄存器不同）
                }
            }
            
            // 模式8：LOADLOCAL + MOVE 同一寄存器 → 消除
            if (op1 == static_cast<UInt8>(Opcode::OP_LOADLOCAL) &&
                op2 == static_cast<UInt8>(Opcode::OP_MOVE) &&
                a1 == b2 && a1 == a2) {
                stats_.peepholesApplied++;
                stats_.deadInstructionsRemoved++;
                // 只保留 LOADLOCAL
                for (size_t j = 0; j < instSize; j++) {
                    newCode.push_back(func->code[i + j]);
                }
                i += 2 * instSize;
                optimized = true;
                continue;
            }
            
            // ========== 算术运算简化 ==========
            
            // 模式9：LOADINT 0 + ADD → 消除加法（a + 0 = a）
            if (op1 == static_cast<UInt8>(Opcode::OP_LOADINT) &&
                op2 == static_cast<UInt8>(Opcode::OP_ADD)) {
                Int32 val = 0;
                val |= static_cast<Int32>(func->code[i + 4]) & 0xFF;
                val |= (static_cast<Int32>(func->code[i + 5]) & 0xFF) << 8;
                val |= (static_cast<Int32>(func->code[i + 6]) & 0xFF) << 16;
                val |= (static_cast<Int32>(func->code[i + 7]) & 0xFF) << 24;
                
                if (val == 0 && a1 == c2 && b2 == a2) {
                    stats_.peepholesApplied++;
                    stats_.deadInstructionsRemoved++;
                    // 替换为 MOVE b2 → a2
                    newCode.push_back(static_cast<UInt8>(Opcode::OP_MOVE));
                    newCode.push_back(a2);
                    newCode.push_back(b2);
                    newCode.push_back(0);
                    for (int j = 0; j < 12; j++) newCode.push_back(0);
                    i += 2 * instSize;
                    optimized = true;
                    continue;
                }
            }
            
            // 模式10：LOADINT 1 + MUL → 消除乘法（a * 1 = a）
            if (op1 == static_cast<UInt8>(Opcode::OP_LOADINT) &&
                op2 == static_cast<UInt8>(Opcode::OP_MUL)) {
                Int32 val = 0;
                val |= static_cast<Int32>(func->code[i + 4]) & 0xFF;
                val |= (static_cast<Int32>(func->code[i + 5]) & 0xFF) << 8;
                val |= (static_cast<Int32>(func->code[i + 6]) & 0xFF) << 16;
                val |= (static_cast<Int32>(func->code[i + 7]) & 0xFF) << 24;
                
                if (val == 1 && a1 == c2 && b2 == a2) {
                    stats_.peepholesApplied++;
                    stats_.deadInstructionsRemoved++;
                    newCode.push_back(static_cast<UInt8>(Opcode::OP_MOVE));
                    newCode.push_back(a2);
                    newCode.push_back(b2);
                    newCode.push_back(0);
                    for (int j = 0; j < 12; j++) newCode.push_back(0);
                    i += 2 * instSize;
                    optimized = true;
                    continue;
                }
            }
            
            // 模式11：LOADINT 0 + SUB (交换操作数) → 替换为 NEG
            if (op1 == static_cast<UInt8>(Opcode::OP_LOADINT) &&
                op2 == static_cast<UInt8>(Opcode::OP_SUB)) {
                Int32 val = 0;
                val |= static_cast<Int32>(func->code[i + 4]) & 0xFF;
                val |= (static_cast<Int32>(func->code[i + 5]) & 0xFF) << 8;
                val |= (static_cast<Int32>(func->code[i + 6]) & 0xFF) << 16;
                val |= (static_cast<Int32>(func->code[i + 7]) & 0xFF) << 24;
                
                if (val == 0 && a1 == b2 && c2 == a2) {
                    stats_.peepholesApplied++;
                    stats_.deadInstructionsRemoved++;
                    // 0 - a → NEG a
                    newCode.push_back(static_cast<UInt8>(Opcode::OP_NEG));
                    newCode.push_back(a2);
                    newCode.push_back(c2);
                    newCode.push_back(0);
                    for (int j = 0; j < 12; j++) newCode.push_back(0);
                    i += 2 * instSize;
                    optimized = true;
                    continue;
                }
            }
            
            // 模式12：SUB 同一寄存器 → 替换为 LOADINT 0
            if (op2 == static_cast<UInt8>(Opcode::OP_SUB) && b2 == c2) {
                stats_.peepholesApplied++;
                stats_.deadInstructionsRemoved++;
                // 生成 LOADINT 0
                newCode.push_back(static_cast<UInt8>(Opcode::OP_LOADINT));
                newCode.push_back(a2);
                newCode.push_back(0);
                newCode.push_back(0);
                newCode.push_back(0);  // imm = 0
                newCode.push_back(0);
                newCode.push_back(0);
                newCode.push_back(0);
                for (int j = 0; j < 8; j++) newCode.push_back(0);
                i += 2 * instSize;
                optimized = true;
                continue;
            }
        }
        
        // ========== 第三类：三指令优化 ==========
        
        if (i + 3 * instSize <= n) {
            UInt8 op1 = func->code[i];
            UInt8 op2 = func->code[i + instSize];
            UInt8 op3 = func->code[i + 2 * instSize];
            UInt8 a1 = func->code[i + 1];
            UInt8 a2 = func->code[i + instSize + 1];
            UInt8 a3 = func->code[i + 2 * instSize + 1];
            
            // 模式13：PUSH + POP 同一位置 → 消除
            // 注意：这里需要根据实际的指令格式来适配
        }
        
        // 如果没有优化，保留原指令
        if (!optimized) {
            size_t copyEnd = std::min(i + instSize, n);
            for (size_t j = i; j < copyEnd; j++) {
                newCode.push_back(func->code[j]);
            }
            i += instSize;
        }
    }
    
    // 更新优化后的字节码
    func->code = std::move(newCode);
}

void BytecodeOptimizer::eliminateDeadInstructions(VMFunction* func) {
    if (func->code.empty()) return;
    
    const size_t instSize = 16;
    size_t n = func->code.size();
    size_t numInsts = n / instSize;
    if (numInsts == 0) return;
    
    // 第一步：构建控制流图
    std::vector<bool> reachable(numInsts, false);
    std::queue<size_t> worklist;
    
    // 入口点可达
    reachable[0] = true;
    worklist.push(0);
    
    // 宽度优先搜索
    while (!worklist.empty()) {
        size_t pc = worklist.front();
        worklist.pop();
        
        if (pc >= numInsts) continue;
        
        size_t byteOffset = pc * instSize;
        if (byteOffset >= n) continue;
        
        UInt8 op = func->code[byteOffset];
        
        // 根据指令类型确定后续可达指令
        if (op == static_cast<UInt8>(Opcode::OP_JUMP) ||
            op == static_cast<UInt8>(Opcode::OP_JUMPIF) ||
            op == static_cast<UInt8>(Opcode::OP_JUMPNIF)) {
            // 读取跳转偏移
            if (byteOffset + 7 < n) {
                Int32 offset = 0;
                offset |= static_cast<Int32>(func->code[byteOffset + 4]) & 0xFF;
                offset |= (static_cast<Int32>(func->code[byteOffset + 5]) & 0xFF) << 8;
                offset |= (static_cast<Int32>(func->code[byteOffset + 6]) & 0xFF) << 16;
                offset |= (static_cast<Int32>(func->code[byteOffset + 7]) & 0xFF) << 24;
                
                // 跳转目标（offset是字节偏移，需除以instSize转指令索引）
                Int32 targetInst = (static_cast<Int32>(byteOffset) + static_cast<Int32>(instSize) + offset) / static_cast<Int32>(instSize);
                if (targetInst >= 0 && targetInst < static_cast<Int32>(numInsts)) {
                    if (!reachable[targetInst]) {
                        reachable[targetInst] = true;
                        worklist.push(targetInst);
                    }
                }
            }
            
            // 条件跳转还能顺序执行
            if (op != static_cast<UInt8>(Opcode::OP_JUMP)) {
                if (pc + 1 < numInsts && !reachable[pc + 1]) {
                    reachable[pc + 1] = true;
                    worklist.push(pc + 1);
                }
            }
        } else if (op == static_cast<UInt8>(Opcode::OP_RETURN) ||
                   op == static_cast<UInt8>(Opcode::OP_THROW)) {
            // 返回/抛出指令，没有后续顺序执行
            continue;
        } else if (op == static_cast<UInt8>(Opcode::OP_TRY)) {
            // OP_TRY: 需要标记 catch 处理器入口为可达
            // 格式: [OP_TRY][a][0][0][catchPC_imm32][padding8]
            // catchPC 是绝对字节偏移
            if (byteOffset + 7 < n) {
                Int32 catchPC = 0;
                catchPC |= static_cast<Int32>(func->code[byteOffset + 4]) & 0xFF;
                catchPC |= (static_cast<Int32>(func->code[byteOffset + 5]) & 0xFF) << 8;
                catchPC |= (static_cast<Int32>(func->code[byteOffset + 6]) & 0xFF) << 16;
                catchPC |= (static_cast<Int32>(func->code[byteOffset + 7]) & 0xFF) << 24;
                Int32 catchInst = catchPC / static_cast<Int32>(instSize);
                if (catchInst >= 0 && catchInst < static_cast<Int32>(numInsts)) {
                    if (!reachable[catchInst]) {
                        reachable[catchInst] = true;
                        worklist.push(catchInst);
                    }
                }
            }
            // try 体顺序执行
            if (pc + 1 < numInsts && !reachable[pc + 1]) {
                reachable[pc + 1] = true;
                worklist.push(pc + 1);
            }
        } else {
            // 普通指令，顺序执行
            if (pc + 1 < numInsts && !reachable[pc + 1]) {
                reachable[pc + 1] = true;
                worklist.push(pc + 1);
            }
        }
    }
    
    // 第二步：只保留可达指令
    std::vector<UInt8> newCode;
    int removed = 0;
    for (size_t pc = 0; pc < numInsts; pc++) {
        if (reachable[pc]) {
            size_t start = pc * instSize;
            size_t end = std::min(start + instSize, n);
            for (size_t j = start; j < end; j++) {
                newCode.push_back(func->code[j]);
            }
        } else {
            removed++;
        }
    }
    
    stats_.deadInstructionsRemoved += removed;
    func->code = std::move(newCode);
}

void BytecodeOptimizer::propagateConstants(VMFunction* func) {
    const size_t instSize = 16;
    size_t n = func->code.size();
    if (n == 0) return;

    // 追踪寄存器中的常量值
    std::vector<std::pair<bool, Int64>> regConstants(256, {false, 0});
    std::vector<std::pair<bool, bool>> regBools(256, {false, false});

    // 单遍扫描：依次处理每条指令，
    // 常量信息只向前传播，不会影响已经过的指令
    std::vector<UInt8> newCode;
    bool changed = false;
    for (size_t pc = 0; pc + instSize <= n; pc += instSize) {
        UInt8 op = func->code[pc];
        UInt8 a = func->code[pc + 1];
        UInt8 b = func->code[pc + 2];
        UInt8 c = func->code[pc + 3];

        // ─── 常量追踪指令 ─────────────────────────────────
        if (op == static_cast<UInt8>(Opcode::OP_LOADINT)) {
            // 读取立即数并记录常量
            Int32 imm = 0;
            if (pc + 7 < n) {
                imm |= static_cast<Int32>(func->code[pc + 4]) & 0xFF;
                imm |= (static_cast<Int32>(func->code[pc + 5]) & 0xFF) << 8;
                imm |= (static_cast<Int32>(func->code[pc + 6]) & 0xFF) << 16;
                imm |= (static_cast<Int32>(func->code[pc + 7]) & 0xFF) << 24;
            }
            regConstants[a] = {true, imm};
            regBools[a] = {false, false};
            stats_.constantPropagations++;
            // 保留原指令
            for (size_t j = 0; j < instSize; j++) {
                newCode.push_back(func->code[pc + j]);
            }
            continue;
        }

        if (op == static_cast<UInt8>(Opcode::OP_LOADBOOL)) {
            regBools[a] = {true, b != 0};
            regConstants[a] = {false, 0};
            stats_.constantPropagations++;
            for (size_t j = 0; j < instSize; j++) {
                newCode.push_back(func->code[pc + j]);
            }
            continue;
        }

        if (op == static_cast<UInt8>(Opcode::OP_MOVE)) {
            if (regConstants[b].first) {
                regConstants[a] = regConstants[b];
                regBools[a] = {false, false};
                stats_.constantPropagations++;
            } else if (regBools[b].first) {
                regBools[a] = regBools[b];
                regConstants[a] = {false, 0};
                stats_.constantPropagations++;
            } else {
                regConstants[a] = {false, 0};
                regBools[a] = {false, false};
            }
            for (size_t j = 0; j < instSize; j++) {
                newCode.push_back(func->code[pc + j]);
            }
            continue;
        }

        if (op == static_cast<UInt8>(Opcode::OP_THROW)) {
            // OP_THROW 改变控制流，之后不可达，清空所有常量追踪
            for (auto& rc : regConstants) rc = {false, 0};
            for (auto& rb : regBools) rb = {false, false};
            for (size_t j = 0; j < instSize; j++) {
                newCode.push_back(func->code[pc + j]);
            }
            continue;
        }

        if (op == static_cast<UInt8>(Opcode::OP_TRY)) {
            // OP_TRY: a 寄存器将用于接收异常值
            regConstants[a] = {false, 0};
            regBools[a] = {false, false};
            for (size_t j = 0; j < instSize; j++) {
                newCode.push_back(func->code[pc + j]);
            }
            continue;
        }

        // ─── 常量折叠：算术运算 ───────────────────────────
        bool isArithmetic =
            op == static_cast<UInt8>(Opcode::OP_ADD) ||
            op == static_cast<UInt8>(Opcode::OP_SUB) ||
            op == static_cast<UInt8>(Opcode::OP_MUL) ||
            op == static_cast<UInt8>(Opcode::OP_DIV) ||
            op == static_cast<UInt8>(Opcode::OP_MOD) ||
            op == static_cast<UInt8>(Opcode::OP_NEG);

        if (isArithmetic && regConstants[b].first && (c == 0 || regConstants[c].first)) {
            Int64 val1 = regConstants[b].second;
            Int64 val2 = (c != 0) ? regConstants[c].second : 0;
            Int64 result = 0;
            bool folded = false;

            if (op == static_cast<UInt8>(Opcode::OP_ADD)) {
                result = val1 + val2;
                folded = true;
            } else if (op == static_cast<UInt8>(Opcode::OP_SUB)) {
                result = val1 - val2;
                folded = true;
            } else if (op == static_cast<UInt8>(Opcode::OP_MUL)) {
                result = val1 * val2;
                folded = true;
            } else if (op == static_cast<UInt8>(Opcode::OP_NEG)) {
                result = -val1;
                folded = true;
            }

            if (folded) {
                stats_.constantPropagations++;
                changed = true;
                // 记录折叠结果到目标寄存器，使后续指令可以继续折叠
                regConstants[a] = {true, result};
                regBools[a] = {false, false};
                // 生成新的 LOADINT 指令
                newCode.push_back(static_cast<UInt8>(Opcode::OP_LOADINT));
                newCode.push_back(a);
                newCode.push_back(0);
                newCode.push_back(0);
                newCode.push_back(static_cast<UInt8>(result & 0xFF));
                newCode.push_back(static_cast<UInt8>((result >> 8) & 0xFF));
                newCode.push_back(static_cast<UInt8>((result >> 16) & 0xFF));
                newCode.push_back(static_cast<UInt8>((result >> 24) & 0xFF));
                for (int j = 0; j < 8; j++) newCode.push_back(0);
                continue;
            }
        }

        // ─── 常量条件跳转 ─────────────────────────────────
        if (op == static_cast<UInt8>(Opcode::OP_JUMPIF) ||
            op == static_cast<UInt8>(Opcode::OP_JUMPNIF)) {
            if (regBools[b].first) {
                bool cond = regBools[b].second;
                bool shouldJump = (op == static_cast<UInt8>(Opcode::OP_JUMPIF)) ? cond : !cond;

                if (shouldJump) {
                    // 条件恒真：替换为无条件跳转
                    stats_.constantPropagations++;
                    changed = true;
                    newCode.push_back(static_cast<UInt8>(Opcode::OP_JUMP));
                    newCode.push_back(0);
                    newCode.push_back(0);
                    newCode.push_back(0);
                    for (int j = 0; j < 4; j++) {
                        newCode.push_back(func->code[pc + 4 + j]);
                    }
                    for (int j = 0; j < 8; j++) newCode.push_back(0);
                    continue;
                } else {
                    // 条件恒假：移除指令
                    stats_.deadInstructionsRemoved++;
                    changed = true;
                    continue;
                }
            }
        }

        // ─── 其他指令：清除目标寄存器的常量信息 ─────────
        regConstants[a] = {false, 0};
        regBools[a] = {false, false};

        // 复制原指令
        for (size_t j = 0; j < instSize; j++) {
            newCode.push_back(func->code[pc + j]);
        }
    }

    if (changed) {
        func->code = std::move(newCode);
    }
}

void BytecodeOptimizer::reorderBasicBlocks(VMFunction* func) {
    // 简单版本：不改变顺序
    // 完整实现需要profile信息来重排热块
}

void BytecodeOptimizer::allocateRegisters(VMFunction* func) {
    // 为未来寄存器式VM预留
    // 简单计数有多少唯一寄存器被使用
    const size_t instSize = 16;
    std::unordered_set<UInt8> usedRegs;
    
    size_t n = func->code.size();
    for (size_t pc = 0; pc + instSize <= n; pc += instSize) {
        UInt8 a = func->code[pc + 1];
        UInt8 b = func->code[pc + 2];
        UInt8 c = func->code[pc + 3];
        
        if (a < 255) usedRegs.insert(a);
        if (b < 255) usedRegs.insert(b);
        if (c < 255) usedRegs.insert(c);
    }
    
    stats_.registersAllocated = static_cast<int>(usedRegs.size());
}

// === 辅助函数 ===

std::vector<BytecodeOptimizer::BasicBlock> BytecodeOptimizer::buildCFG(VMFunction* func) {
    std::vector<BasicBlock> cfg;
    if (func->code.empty()) return cfg;
    
    const size_t instSize = 16;
    size_t n = func->code.size();
    size_t numInsts = n / instSize;
    if (numInsts == 0) return cfg;
    
    // 识别基本块边界
    std::vector<bool> isLeader(numInsts, false);
    isLeader[0] = true;  // 入口是leader
    
    for (size_t pc = 0; pc < numInsts; pc++) {
        size_t byteOffset = pc * instSize;
        UInt8 op = func->code[byteOffset];
        
        if (op == static_cast<UInt8>(Opcode::OP_JUMP) ||
            op == static_cast<UInt8>(Opcode::OP_JUMPIF) ||
            op == static_cast<UInt8>(Opcode::OP_JUMPNIF) ||
            op == static_cast<UInt8>(Opcode::OP_THROW) ||
            op == static_cast<UInt8>(Opcode::OP_RETURN)) {
            // 跳转目标是leader
            if ((op == static_cast<UInt8>(Opcode::OP_JUMP) ||
                 op == static_cast<UInt8>(Opcode::OP_JUMPIF) ||
                 op == static_cast<UInt8>(Opcode::OP_JUMPNIF)) && byteOffset + 7 < n) {
                Int32 offset = 0;
                offset |= static_cast<Int32>(func->code[byteOffset + 4]) & 0xFF;
                offset |= (static_cast<Int32>(func->code[byteOffset + 5]) & 0xFF) << 8;
                offset |= (static_cast<Int32>(func->code[byteOffset + 6]) & 0xFF) << 16;
                offset |= (static_cast<Int32>(func->code[byteOffset + 7]) & 0xFF) << 24;

                Int32 targetInst = (static_cast<Int32>(byteOffset) + static_cast<Int32>(instSize) + offset) / static_cast<Int32>(instSize);
                if (targetInst >= 0 && targetInst < static_cast<Int32>(numInsts)) {
                    isLeader[targetInst] = true;
                }
            }
            // 跳转/返回后的指令也是leader
            if (pc + 1 < numInsts) {
                isLeader[pc + 1] = true;
            }
        }
        // OP_TRY: catch 处理器入口也是 leader
        if (op == static_cast<UInt8>(Opcode::OP_TRY) && byteOffset + 7 < n) {
            Int32 catchPC = 0;
            catchPC |= static_cast<Int32>(func->code[byteOffset + 4]) & 0xFF;
            catchPC |= (static_cast<Int32>(func->code[byteOffset + 5]) & 0xFF) << 8;
            catchPC |= (static_cast<Int32>(func->code[byteOffset + 6]) & 0xFF) << 16;
            catchPC |= (static_cast<Int32>(func->code[byteOffset + 7]) & 0xFF) << 24;
            Int32 catchInst = catchPC / static_cast<Int32>(instSize);
            if (catchInst >= 0 && catchInst < static_cast<Int32>(numInsts)) {
                isLeader[catchInst] = true;
            }
        }
    }
    
    // 构建基本块
    BasicBlock current;
    current.start = 0;
    for (size_t pc = 1; pc < numInsts; pc++) {
        if (isLeader[pc]) {
            current.end = pc - 1;
            cfg.push_back(current);
            current.start = pc;
        }
    }
    current.end = numInsts - 1;
    cfg.push_back(current);
    
    return cfg;
}

std::unordered_set<size_t> BytecodeOptimizer::findReachableCode(
    VMFunction* func, 
    const std::vector<BasicBlock>& cfg
) {
    std::unordered_set<size_t> reachable;
    if (cfg.empty()) return reachable;
    
    // BFS遍历
    std::queue<size_t> worklist;
    std::unordered_set<size_t> visitedBlocks;
    
    worklist.push(0);
    visitedBlocks.insert(0);
    
    while (!worklist.empty()) {
        size_t blockIdx = worklist.front();
        worklist.pop();
        
        const BasicBlock& block = cfg[blockIdx];
        for (size_t pc = block.start; pc <= block.end; pc++) {
            reachable.insert(pc);
        }
        
        // 添加后继（简化版）
        if (blockIdx + 1 < cfg.size()) {
            if (visitedBlocks.find(blockIdx + 1) == visitedBlocks.end()) {
                visitedBlocks.insert(blockIdx + 1);
                worklist.push(blockIdx + 1);
            }
        }
    }
    
    return reachable;
}

void BytecodeOptimizer::mergeLoads(VMFunction* func) {
    // TODO: 合并连续LOAD
}

void BytecodeOptimizer::removeRedundantStores(VMFunction* func) {
    // TODO: 移除冗余STORE
}

} // namespace cplang
