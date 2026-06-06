// LLVM IR 代码生成器 - CP 语言后端（修复版）
// 使用 LLVM C++ API 直接构建 IR，而非生成文本再解析
#pragma once

#include "ast/ast.hpp"
#include "common/types.hpp"
#include "optimizer/escape_analyzer.hpp"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <memory>

// 前置声明 LLVM 类型（避免头文件依赖 LLVM）
namespace llvm {
    class LLVMContext;
    class Module;
    class IRBuilderBase;
    class Value;
    class Type;
    class Function;
    class BasicBlock;
    class StructType;
    class GlobalVariable;
    class ConstantFolder;
    class IRBuilderDefaultInserter;
    template <typename FolderTy, typename InserterTy> class IRBuilder;
}

namespace cplang {

// 循环上下文，用于 break/continue
struct LoopContext {
    llvm::BasicBlock* condBlock;
    llvm::BasicBlock* endBlock;
    llvm::BasicBlock* incBlock; // for 循环的递增块（continue 跳转目标），while 循环为 nullptr
};

// OptLevel 已移至 common/types.hpp，此处保留兼容性引用

// LLVM IR 生成器（使用 C++ API）
class LLVMCodegen {
public:
    LLVMCodegen();
    ~LLVMCodegen();
    
    // 生成模块（带优化级别）
    // 返回 LLVM Module 的原始指针（调用者不负责释放）
    llvm::Module* generate(Shared<Program> program, OptLevel opt = OptLevel::O2);
    
    // 只生成单个函数的 IR（用于热点编译，避免重复定义其他函数）
    llvm::Module* generateSingleFunction(Shared<Program> program, const std::string& funcName, OptLevel opt = OptLevel::O2);
    
    // 生成 LLVM IR 字符串
    std::string generateIRString(Shared<Program> program, OptLevel opt = OptLevel::O2);
    
    // 生成单个函数的 LLVM IR 字符串
    std::string generateSingleFunctionIRString(Shared<Program> program, const std::string& funcName, OptLevel opt = OptLevel::O2);
    
    // 标识符 sanitize：非 ASCII 字符转义为 LLVM 兼容格式
    static std::string sanitizeName(const std::string& name);
    
    // 设置优化级别
    void setOptLevel(OptLevel opt) { optLevel_ = opt; }
    
    // native 调用检测（OrcJIT 需要访问）
    bool hasNativeCalls() const { return hasNativeCalls_; }
    void resetNativeCallFlag() { hasNativeCalls_ = false; }
    
    // 设置是否跳过 native 调用移除（用于 AOT/--emit-llvm 模式，保留外部符号引用以供链接器解析）
    void setSkipNativeCallRemoval(bool v) { skipNativeCallRemoval_ = v; }
    bool isPureMath() const { return pureMath_; }
    void setPureMath(bool v) { pureMath_ = v; }

    // 获取优化统计
    struct OptStats {
        int constantsFolded = 0;
        int deadCodeRemoved = 0;
        int functionsInlined = 0;
    };
    const OptStats& getOptStats() const { return optStats_; }
    
private:
    // LLVM 上下文和模块（由 this 拥有）
    std::unique_ptr<llvm::LLVMContext> context_;
    std::unique_ptr<llvm::Module> module_;
    llvm::IRBuilder<llvm::ConstantFolder, llvm::IRBuilderDefaultInserter>* builder_;  // 由 LLVMContext/Module 生命周期管理
    
    // 当前程序 AST
    Shared<Program> program_;
    
    // 临时变量计数器
    int tempCounter_;
    
    // 是否已生成 return
    bool hasReturn_;
    
    // 变量映射：变量名 → LLVM Value*
    std::unordered_map<std::string, llvm::Value*> varMap_;
    
    // 函数参数集合（用于区分参数和局部变量）
    std::unordered_set<std::string> funcParams_;
    
    // 循环上下文栈
    std::stack<LoopContext> loopStack_;
    
    // 结构体定义映射：结构体名 → LLVM StructType*
    std::unordered_map<std::string, llvm::StructType*> structTypes_;
    
    // 变量类型映射：变量名 → 结构体名（空字符串表示普通 i64）
    std::unordered_map<std::string, std::string> varTypes_;
    
    // 结构体成员名映射：结构体名 → 成员名列表
    std::unordered_map<std::string, std::vector<std::string>> structMembers_;
    
    // 字符串常量：内容 → 全局变量（不缓存 NaN-boxed 值，因为 NaN-boxing 指令依赖当前插入点）
    std::unordered_map<std::string, llvm::GlobalVariable*> stringConstants_;
    int stringCounter_;
    
    // native 调用标记
    bool hasNativeCalls_;
    
    // 跳过 native 调用移除（AOT 模式使用）
    bool skipNativeCallRemoval_ = false;
    
    // 纯数学模式：跳过 NaN-boxing 类型分派和运行时调用，生成原生 LLVM IR
    bool pureMath_ = false;
    
    // 优化相关
    OptLevel optLevel_;
    OptStats optStats_;
    
    // 当前函数是否完全类型标注（无动态类型开销）
    bool currentFuncIsFullyTyped_ = false;
    
    // 逃逸分析结果：函数名 -> 函数逃逸结果
    std::unordered_map<std::string, FuncEscapeResult> escapeResults_;
    
    // 数组/字符串字面量长度缓存：变量名 → 长度（用于 .length 编译期折叠）
    std::unordered_map<std::string, size_t> literalLenByVar_;
    
    // 当前函数逃逸结果
    const FuncEscapeResult* currentEscapeResult_ = nullptr;
    
    // 最大栈分配对象大小（字节），超过的即使不逃逸也分配在堆上
    const int MAX_STACK_ALLOC_SIZE = 64;
    
    // PGO（剖面引导优化）配置
    bool enablePGOGenerate_ = false;  // 生成PGO插桩代码
    bool enablePGOUse_ = false;       // 使用PGO profile优化
    std::string pgoProfilePath_ = "cplang.profdata";  // PGO profile路径
    
    // === 优化 pass（预留接口）===
    void runOptimizationPasses();
    void foldConstants();
    void eliminateDeadCode();
    void inlineFunctions();
    
    // === 代码生成函数 ===
    void generateProgram(llvm::Module* module, Shared<Program> program);
    void generateStructDecl(llvm::Module* module, Shared<StructDeclStmt> decl);
    llvm::Function* generateFuncDecl(llvm::Module* module, Shared<FuncDeclStmt> decl);
    void generateStatement(llvm::Function* func, Shared<Stmt> stmt);
    void generateVarDecl(llvm::Function* func, Shared<VarDeclStmt> stmt);
    void generateReturnStmt(llvm::Function* func, Shared<ReturnStmt> stmt);
    void generateIfStmt(llvm::Function* func, Shared<IfStmt> stmt);
    void generateWhileStmt(llvm::Function* func, Shared<WhileStmt> stmt);
    void generateForStmt(llvm::Function* func, Shared<ForStmt> stmt);
    void generateBreakStmt(llvm::Function* func, Shared<BreakStmt> stmt);
    void generateContinueStmt(llvm::Function* func, Shared<ContinueStmt> stmt);
    void generateForEachStmt(llvm::Function* func, Shared<ForEachStmt> stmt);
    void generateImportStmt(llvm::Function* func, Shared<ImportStmt> stmt);
    
    // === 表达式生成，返回 LLVM Value* ===
    llvm::Value* generateExpression(llvm::Function* func, Shared<Expr> expr);
    
    // === 辅助函数 ===
    bool isFullyTyped(Shared<FuncDeclStmt> decl);
    llvm::Type* getLLVMType(const std::string& typeName);
    llvm::Value* getTempVar(llvm::Function* func, const std::string& baseName);
    llvm::Value* loadVar(const std::string& varName);
    void storeVar(const std::string& varName, llvm::Value* value);
    
    // === 字符串常量处理 ===
    llvm::Value* registerStringConstant(const std::string& content);
    llvm::Value* nanBoxStringPtr(llvm::Value* rawPtr);
    void emitStringConstants(llvm::Module* module);
    static std::string escapeLLVMString(const std::string& s);
    
    // === 类型转换 ===
    llvm::Value* toDouble(llvm::Value* val);
    llvm::Value* toInt64(llvm::Value* val);
};

} // namespace cplang
