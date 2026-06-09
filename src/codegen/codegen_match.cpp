// 妯″紡鍖归厤缂栬瘧锛堢嫭绔嬬紪璇戝崟鍏冿級
#include "codegen/codegen.hpp"

namespace cplang {

void Codegen::compileMatch(Shared<MatchStmt> s) {
    // 缂栬瘧鍖归厤琛ㄨ揪寮?
    int matchReg = compileExpr(s->expr);

    std::vector<int> allEndJumps;

    // 涓烘瘡涓垎鏀敓鎴愪唬鐮侊紙鍊兼瘮杈冿紝涓?compileSwitch 鐩稿悓鐨勭瓥鐣ワ級
    for (auto& mc : s->cases) {
        int caseValReg;

        // 浠庤涔夊垎鏋愬櫒鑾峰彇鍙樹綋淇℃伅
        if (analyzer_) {
            Symbol* sym = analyzer_->lookup(mc.variantName);
            if (sym && sym->kind == Symbol::ENUM_VARIANT) {
                // 鏋氫妇鍙樹綋锛氬姞杞藉叾 tag 鍊间綔涓烘暣鏁?
                Int64 tag = sym->enumValue;
                caseValReg = allocReg();
                emitInt(OP_LOADINT, static_cast<UInt8>(caseValReg), static_cast<Int32>(tag));
            } else {
                // 闈炴灇涓炬爣璇嗙锛氱紪璇戜负甯歌琛ㄨ揪寮?
                auto idExpr = Shared<IdentifierExpr>(new IdentifierExpr());
                idExpr->name = mc.variantName;
                caseValReg = compileExpr(idExpr);
            }
        } else {
            auto idExpr = Shared<IdentifierExpr>(new IdentifierExpr());
            idExpr->name = mc.variantName;
            caseValReg = compileExpr(idExpr);
        }

        // 姣旇緝 matchReg == caseValReg
        int cmpReg = allocReg();
        emit(OP_CMPEQ, static_cast<UInt8>(cmpReg),
             static_cast<UInt8>(matchReg), static_cast<UInt8>(caseValReg));

        // 濡傛灉涓嶇瓑锛岃烦杩囨鍒嗘敮浣?
        int skipJump = emitJumpPlaceholder(OP_JUMPNIF, static_cast<UInt8>(cmpReg));

        // 鎺ㄩ€佺粦瀹氫綔鐢ㄥ煙
        pushScope();

        // 缁戝畾鍙樹綋瀛楁鍒板眬閮ㄥ彉閲?
        for (size_t i = 0; i < mc.bindings.size(); i++) {
            int fieldReg = allocReg();
            emit(OP_GETVARIANTFIELD, static_cast<UInt8>(fieldReg),
                 static_cast<UInt8>(matchReg), static_cast<UInt8>(i + 1));
            LocalVar lv{fieldReg, nullptr};
            localScopes_.back()[mc.bindings[i]] = lv;
        }

        // 缂栬瘧鍒嗘敮浣?
        if (mc.body) {
            compileStmt(mc.body);
        }

        // 寮瑰嚭缁戝畾浣滅敤鍩?
        popScope();

        // 璺宠浆鍒板尮閰嶇粨鏉?
        int endJump = emitJumpPlaceholder(OP_JUMP);
        allEndJumps.push_back(endJump);

        // 淇ˉ璺宠繃璺宠浆
        patchJump(skipJump, static_cast<int>(code_->size()));
    }

    // 榛樿鍒嗘敮
    if (s->defaultCase) {
        compileStmt(s->defaultCase);
    }

    // 淇ˉ鎵€鏈夌粨鏉熻烦杞?
    int endTarget = static_cast<int>(code_->size());
    for (int jmp : allEndJumps) {
        patchJump(jmp, endTarget);
    }
}

} // namespace cplang
