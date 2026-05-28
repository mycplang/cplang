// CP语言 优化代码生成器 — 4字节指令格式
#include "vm/vm_opt.hpp"
#include "ast/ast.hpp"
#include <unordered_map>
#include <stack>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  代码生成器
// ═══════════════════════════════════════════════════════════════════
class CodegenOpt {
public:
    explicit CodegenOpt(VM* vm) : vm_(vm) {}
    
    VMFunction* compile(struct Program* program);
    bool hasError() const { return hasError_; }
    const std::string& errorMessage() const { return errorMsg_; }

private:
    VM* vm_;
    VMFunction* func_ = nullptr;
    std::vector<UInt8>* code_ = nullptr;
    std::vector<Value> constants_;
    
    int nextReg_ = 0;
    int maxReg_ = 0;
    int stackBase_ = 0;
    
    struct LocalVar { int reg; };
    std::vector<std::unordered_map<std::string, LocalVar>> localScopes_;
    
    struct LoopContext { int breakTarget; int continueTarget; };
    std::vector<LoopContext> loopStack_;
    
    // 指令发射（4字节格式）
    void emit(UInt8 op, UInt8 a, UInt8 b, UInt8 c);
    void emitI(UInt8 op, UInt8 a, Int16 sbx);  // I格式
    int emitJump(UInt8 op, UInt8 a);  // 返回位置用于patch
    void patchJump(int pos, int target);
    
    int allocReg();
    void freeReg(int n) { if (nextReg_ > n) nextReg_ = n; }
    void releaseTemps();
    
    Int32 addConstant(const Value& v);
    Int32 getGlobalSlot(const std::string& name);
    
    void pushScope() { localScopes_.emplace_back(); }
    void popScope() { localScopes_.pop_back(); }
    
    // 编译函数
    void compileStmt(struct Stmt* stmt);
    void compileExprStmt(struct ExprStmt* s);
    void compileVarDecl(struct VarDeclStmt* s);
    void compileFuncDecl(struct FuncDeclStmt* s);
    void compileBlock(struct BlockStmt* s);
    void compileIf(struct IfStmt* s);
    void compileWhile(struct WhileStmt* s);
    void compileFor(struct ForStmt* s);
    void compileReturn(struct ReturnStmt* s);
    void compileBreak(struct BreakStmt* s);
    void compileContinue(struct ContinueStmt* s);
    
    int compileExpr(struct Expr* expr);
    int compileLiteral(struct LiteralExpr* e);
    int compileIdentifier(struct IdentifierExpr* e);
    int compileBinary(struct BinaryExpr* e);
    int compileUnary(struct UnaryExpr* e);
    int compileCall(struct CallExpr* e);
    int compileArray(struct ArrayExpr* e);
    int compileIndex(struct IndexExpr* e);
    int compileAssign(class AssignExpr* e);
    
    int compileBinaryOp(TokenType op, int ra, int rb, int rc);
    
    bool hasError_ = false;
    std::string errorMsg_;
    void error(const std::string& msg) { hasError_ = true; errorMsg_ = msg; }
};

// ═══════════════════════════════════════════════════════════════════
//  指令发射实现
// ═══════════════════════════════════════════════════════════════════
void CodegenOpt::emit(UInt8 op, UInt8 a, UInt8 b, UInt8 c) {
    code_->push_back(op);
    code_->push_back(a);
    code_->push_back(b);
    code_->push_back(c);
    if (a < 255 && a >= maxReg_) maxReg_ = a + 1;
    if (b < 255 && b >= maxReg_) maxReg_ = b + 1;
    if (c < 255 && c >= maxReg_) maxReg_ = c + 1;
}

void CodegenOpt::emitI(UInt8 op, UInt8 a, Int16 sbx) {
    code_->push_back(op);
    code_->push_back(a);
    code_->push_back(static_cast<UInt8>(sbx & 0xFF));
    code_->push_back(static_cast<UInt8>((sbx >> 8) & 0xFF));
    if (a < 255 && a >= maxReg_) maxReg_ = a + 1;
}

int CodegenOpt::emitJump(UInt8 op, UInt8 a) {
    int pos = static_cast<int>(code_->size());
    emitI(op, a, 0);  // 占位
    return pos;
}

void CodegenOpt::patchJump(int pos, int target) {
    Int16 offset = static_cast<Int16>((target - (pos + 4)) / 4);
    (*code_)[pos + 2] = static_cast<UInt8>(offset & 0xFF);
    (*code_)[pos + 3] = static_cast<UInt8>((offset >> 8) & 0xFF);
}

int CodegenOpt::allocReg() {
    return nextReg_++;
}

void CodegenOpt::releaseTemps() {
    // 简化：在语句边界回收寄存器
    // 实际应追踪寄存器生命周期
}

Int32 CodegenOpt::addConstant(const Value& v) {
    for (Int32 i = 0; i < static_cast<Int32>(constants_.size()); i++) {
        if (constants_[i].equals(v)) return i;
    }
    Int32 idx = static_cast<Int32>(constants_.size());
    constants_.push_back(v);
    return idx;
}

Int32 CodegenOpt::getGlobalSlot(const std::string& name) {
    return vm_->getGlobalSlot(name.c_str());
}

// ═══════════════════════════════════════════════════════════════════
//  表达式编译
// ═══════════════════════════════════════════════════════════════════
int CodegenOpt::compileExpr(struct Expr* /*expr*/) {
    // 简化：根据类型分发
    // 实际应使用visitor模式或switch on type
    return 0;
}

// ═══════════════════════════════════════════════════════════════════
//  主编译入口
// ═══════════════════════════════════════════════════════════════════
VMFunction* CodegenOpt::compile(struct Program* /*program*/) {
    func_ = new VMFunction();
    func_->typeTag = T_FUNCTION;
    code_ = &func_->code;
    nextReg_ = 0;
    maxReg_ = 0;
    
    // 编译程序体
    // ... 遍历AST并编译
    
    func_->maxStack = maxReg_ + 16;
    func_->constants = constants_;
    
    return hasError_ ? nullptr : func_;
}

} // namespace cplang
