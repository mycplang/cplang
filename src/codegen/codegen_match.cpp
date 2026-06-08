// 模式匹配编译（独立编译单元）
#include "codegen/codegen.hpp"

namespace cplang {

void Codegen::compileMatch(Shared<MatchStmt> s) {
    printf("DEBUG: compileMatch called, cases=%zu\n", s->cases.size());
    // 编译匹配表达式
    int matchReg = compileExpr(s->expr);

    std::vector<int> allEndJumps;

    // 为每个分支生成代码（值比较，与 compileSwitch 相同的策略）
    for (auto& mc : s->cases) {
        int caseValReg;

        // 从语义分析器获取变体信息
        if (analyzer_) {
            Symbol* sym = analyzer_->lookup(mc.variantName);
            if (sym && sym->kind == Symbol::ENUM_VARIANT) {
                // 枚举变体：加载其 tag 值作为整数
                Int64 tag = sym->enumValue;
                caseValReg = allocReg();
                emitInt(OP_LOADINT, static_cast<UInt8>(caseValReg), static_cast<Int32>(tag));
            } else {
                // 非枚举标识符：编译为常规表达式
                auto idExpr = Shared<IdentifierExpr>(new IdentifierExpr());
                idExpr->name = mc.variantName;
                caseValReg = compileExpr(idExpr);
            }
        } else {
            auto idExpr = Shared<IdentifierExpr>(new IdentifierExpr());
            idExpr->name = mc.variantName;
            caseValReg = compileExpr(idExpr);
        }

        // 比较 matchReg == caseValReg
        int cmpReg = allocReg();
        emit(OP_CMPEQ, static_cast<UInt8>(cmpReg),
             static_cast<UInt8>(matchReg), static_cast<UInt8>(caseValReg));

        // 如果不等，跳过此分支体
        int skipJump = emitJumpPlaceholder(OP_JUMPNIF, static_cast<UInt8>(cmpReg));

        // 推送绑定作用域
        pushScope();

        // 绑定变体字段到局部变量
        for (size_t i = 0; i < mc.bindings.size(); i++) {
            int fieldReg = allocReg();
            emit(OP_GETVARIANTFIELD, static_cast<UInt8>(fieldReg),
                 static_cast<UInt8>(matchReg), static_cast<UInt8>(i + 1));
            LocalVar lv{fieldReg, nullptr};
            localScopes_.back()[mc.bindings[i]] = lv;
        }

        // 编译分支体
        if (mc.body) {
            compileStmt(mc.body);
        }

        // 弹出绑定作用域
        popScope();

        // 跳转到匹配结束
        int endJump = emitJumpPlaceholder(OP_JUMP);
        allEndJumps.push_back(endJump);

        // 修补跳过跳转
        patchJump(skipJump, static_cast<int>(code_->size()));
    }

    // 默认分支
    if (s->defaultCase) {
        compileStmt(s->defaultCase);
    }

    printf("DEBUG: compileMatch: patching end jumps\n"); fflush(stdout);
    // 修补所有结束跳转
    int endTarget = static_cast<int>(code_->size());
    for (int jmp : allEndJumps) {
        patchJump(jmp, endTarget);
    }
    printf("DEBUG: compileMatch: done\n"); fflush(stdout);
}

} // namespace cplang
