// CP语言 代码生成器头文件（依赖分离）
#pragma once

#include "ast/ast.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "vm/vm.hpp"
#include "codegen/bytecode_optimizer.hpp"
#include <vector>
#include <stack>
#include <unordered_map>
#include <memory>

namespace cplang {

// 前向声明（避免循环依赖）

// ═══════════════════════════════════════════════════════════════════
//  代码生成器
// ═══════════════════════════════════════════════════════════════════

class Codegen {
public:
    explicit Codegen(VM* vm = nullptr, SemanticAnalyzer* analyzer = nullptr);
    ~Codegen();

    // 从 AST 生成字节码函数
    VMFunction* compile(Shared<Program> program);

    // 设置源文件名（用于 source_location 和错误报告）
    void setSourceFile(const std::string& f) { sourceFile_ = f; }

    // 设置优化级别
    void setOptLevel(OptLevel level) { optLevel_ = level; }
    
    // 启用/禁用字节码优化
    void setEnableBytecodeOpt(bool enable) { enableBytecodeOpt_ = enable; }

    // 错误
    bool hasError() const { return hasError_; }
    const String& errorMessage() const { return errorMsg_; }
    
    // 源文件行号（编译时设置，用于生成 lineInfo）
    void setLine(int line) { currentLine_ = line; }
    
    // 获取优化统计（如果启用了字节码优化）
    const BytecodeOptStats* getBytecodeOptStats() const { 
        return enableBytecodeOpt_ ? &optStats_ : nullptr; 
    }

private:
    VM* vm_ = nullptr;
    SemanticAnalyzer* analyzer_ = nullptr;
    VMFunction* func_ = nullptr;
    int currentLine_ = 1;
    std::string sourceFile_;
    std::vector<UInt8>* code_ = nullptr;
    
    // 字节码优化相关
    OptLevel optLevel_ = OptLevel::O2;
    bool enableBytecodeOpt_ = true;
    BytecodeOptStats optStats_;

    // 异常处理检测：当函数包含 try/throw 时跳过字节码优化
    bool hasExceptionHandling_ = false;

    int nextReg_ = 0;
    int maxReg_ = 0;
    int maxStack_ = 256;

    // 常量池
    std::vector<Value> constants_;
    std::unordered_map<String, Int32> constIndex_;

    struct LocalVar { int reg; Type* type; };
    std::vector<std::unordered_map<String, LocalVar>> localScopes_;
    [[maybe_unused]] int stackBase_ = 0;

    struct Label {
        int position = 0;
        std::vector<int> unresolved;
    };
    std::vector<Label> labels_;

    struct LoopContext {
        int breakTarget = -1;
        int continueTarget = -1;
        std::vector<int> pendingBreaks;     // OP_JUMP positions to patch on loop exit
        std::vector<int> pendingContinues;  // OP_JUMP positions to patch on continue
    };
    std::vector<LoopContext> loopStack_;

    // 辅助
    void emit(UInt8 op, UInt8 a = 0, UInt8 b = 0, UInt8 c = 0);
    void emitInt(UInt8 op, UInt8 a, Int32 imm);
    void emitJump(UInt8 op, int offset);
    int  emitJumpPlaceholder(UInt8 op, UInt8 a = 0);
    void patchJump(int pos, int target);
    int  allocReg();
    void freeRegs(int n);

    Int32 addConstant(const Value& v);
    int getLocalReg(const String& name);
    LocalVar* findLocal(const String& name);

    void pushScope();
    void popScope();
    int  getScopeHighWater();
    void releaseTempRegs();

    void emitConversion(const Value& from, const Value& to);
    
    // 类型识别辅助
    bool isInt8Type(Type* t) const;
    bool isInt16Type(Type* t) const;
    bool isInt32Type(Type* t) const;
    bool isFloat32Type(Type* t) const;
    
    // 类型化指令发射
    void emitTypedArithmetic(TokenType op, Type* type, int ra, int rb, int rc);
    void emitTypedComparison(TokenType op, Type* type, int ra, int rb, int rc);

    void compileStmt(Shared<Stmt> stmt);
    void compilePackage(Shared<PackageStmt> stmt);
    void compileImport(Shared<ImportStmt> stmt);
    void compileVarDecl(Shared<VarDeclStmt> stmt);
    void compileFuncDecl(Shared<FuncDeclStmt> stmt);
    void compileClassDecl(Shared<ClassDeclStmt> stmt);
    void compileEnumDecl(Shared<EnumDeclStmt> stmt);
    void compileStructDecl(Shared<StructDeclStmt> stmt);
    void compileBlock(Shared<BlockStmt> block);
    void compileIf(Shared<IfStmt> s);
    void compileFor(Shared<ForStmt> s);
    void compileForEach(Shared<ForEachStmt> s);
    void compileWhile(Shared<WhileStmt> s);
    void compileReturn(Shared<ReturnStmt> s);
    void compileBreak(Shared<BreakStmt> s);
    void compileContinue(Shared<ContinueStmt> s);
    void compileTry(Shared<TryStmt> s);
    void compileThrow(Shared<ThrowStmt> s);
    void compileSwitch(Shared<SwitchStmt> s);
    void compileMatch(Shared<MatchStmt> s);
    void compileDoWhile(Shared<DoWhileStmt> s);
    void compileInterfaceDecl(Shared<InterfaceDeclStmt> stmt);
    void compileExprStmt(Shared<ExprStmt> s);

    int compileExpr(Shared<Expr> expr);
    int compileLiteral(Shared<LiteralExpr> expr);
    int compileArray(Shared<ArrayExpr> expr);
    int compileIdentifier(Shared<IdentifierExpr> expr);
    int compileBinary(Shared<BinaryExpr> expr);
    int compileUnary(Shared<UnaryExpr> expr);
    int compileCall(Shared<CallExpr> expr);
    int compileMember(Shared<MemberExpr> expr);
    int compileIndex(Shared<IndexExpr> expr);
    int compileStructLiteral(Shared<StructLiteralExpr> expr);
    int compileAssign(Shared<BinaryExpr> expr);
    int compileTernary(Shared<BinaryExpr> expr);
    int compileNew(Shared<NewExpr> expr);
    int compileLambda(Shared<LambdaExpr> expr);
    int compilePipe(Shared<PipeExpr> expr);

    int compileBinaryOp(TokenType op, int ra, int rb, int rc);
    void compileComparison(TokenType op, int ra, int rb, int rc);

    bool canFold(Shared<Expr> expr) const;
    Value foldConstant(Shared<Expr> expr) const;

    bool hasError_ = false;
    String errorMsg_;
    void reportError(const String& msg);
    
    // 渐进类型：追踪函数是否全 typed 指令
    bool allTyped_ = true;
    
    struct ClassMeta {
        String name;
        std::vector<String> fieldNames;
        std::vector<std::pair<String, VMFunction*>> methods;
    };
    VMFunction* lastCompiledFunc_ = nullptr;
    int lastObjReg_ = -1;
    std::unordered_map<String, ClassMeta> classMeta_;

    // defer 栈：离开作用域时逆序执行
    std::vector<std::vector<Shared<Stmt>>> deferStack_;
    void compileDefer(Shared<DeferStmt> stmt);
    void emitDeferCleanup();
};

// ═══════════════════════════════════════════════════════════════════
//  完整编译器入口
// ═══════════════════════════════════════════════════════════════════

class Compiler {
public:
    Compiler(bool useSlotOpt = true);
    ~Compiler();

    VMFunction* compile(const String& source, const String& sourceFile = "<string>");
    VMFunction* compileFile(const String& filename);
    VM* vm() { return vm_.get(); }

    bool hasError() const { return hasError_; }
    void clearError() { hasError_ = false; errorMsg_.clear(); }
    const String& errorMessage() const { return errorMsg_; }
    void setTraceVM(bool v);
    
    // 字节码优化配置
    void setOptLevel(OptLevel level) { optLevel_ = level; }
    void setEnableBytecodeOpt(bool enable) { enableBytecodeOpt_ = enable; }
    const BytecodeOptStats* getBytecodeOptStats() const { 
        return enableBytecodeOpt_ ? &bytecodeOptStats_ : nullptr; 
    }

private:
    std::unique_ptr<VM> vm_;
    bool useSlotOpt_ = true;
    bool hasError_ = false;
    String errorMsg_;
    OptLevel optLevel_ = OptLevel::O2;
    bool enableBytecodeOpt_ = true;
    BytecodeOptStats bytecodeOptStats_;
    VMFunction* compileInternal(const String& source, const String& sourceFile = "<string>");
};

} // namespace cplang
