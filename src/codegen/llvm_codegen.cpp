// LLVM IR 代码生成器 - CP 语言后端（修复版）
// 使用 LLVM C++ API 直接构建 IR，而非生成文本再解析

// LLVM 头文件必须放在最前面（先于任何 cplang 命名空间头文件）
#ifdef CPLANG_HAS_LLVM
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/PassInstrumentation.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/StringRef.h>

// LLVM 18: getProcessTriple / Triple
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>

// Pass 管理
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/OptimizationLevel.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/Transforms/IPO/GlobalOpt.h>
#include <llvm/Transforms/IPO/ConstantMerge.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Scalar/Reassociate.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/Scalar/DCE.h>
#include <llvm/Transforms/Vectorize/LoopVectorize.h>
#include <llvm/Transforms/Vectorize/SLPVectorizer.h>
#include <llvm/Transforms/Utils/Mem2Reg.h>
#include <llvm/Transforms/IPO/ModuleInliner.h>

// PGO
#include <llvm/Support/VirtualFileSystem.h>
#include <llvm/Transforms/Instrumentation/PGOInstrumentation.h>
#include <llvm/Transforms/Instrumentation/InstrProfiling.h>
#include <llvm/ProfileData/InstrProfReader.h>
#endif

#include "codegen/llvm_codegen.hpp"
#include "parser/parser.hpp"
#include "stdlib/stdlib.hpp"

#include <sstream>
#include <fstream>
#include <iostream>
#include <cctype>
#include <unordered_set>

#ifdef CPLANG_HAS_LLVM
namespace cplang {

// === 构造函数和析构函数 ===

LLVMCodegen::LLVMCodegen()
    : tempCounter_(0)
    , hasReturn_(false)
    , stringCounter_(0)
    , hasNativeCalls_(false)
    , optLevel_(OptLevel::O2)
{
    context_ = std::make_unique<llvm::LLVMContext>();
    builder_ = new llvm::IRBuilder<llvm::ConstantFolder, llvm::IRBuilderDefaultInserter>(*context_);
}

LLVMCodegen::~LLVMCodegen() {
    // builder_ 由 LLVMContext 生命周期管理，无需手动释放
}

// === 静态辅助函数 ===

std::string LLVMCodegen::sanitizeName(const std::string& name) {
    if (name.empty()) return "anonymous";
    
    std::string result;
    for (size_t i = 0; i < name.length(); ) {
        unsigned char c = name[i];
        if (c < 0x80) {
            // ASCII 字符
            if (std::isalnum(c) || c == '_') {
                result += c;
            } else {
                result += '_';
            }
            i++;
        } else {
            // UTF-8 多字节字符
            unsigned unicode = 0;
            int extra = 0;
            if ((c & 0xE0) == 0xC0) { unicode = c & 0x1F; extra = 1; }
            else if ((c & 0xF0) == 0xE0) { unicode = c & 0x0F; extra = 2; }
            else if ((c & 0xF8) == 0xF0) { unicode = c & 0x07; extra = 3; }
            
            for (int j = 0; j < extra && i + 1 < name.length(); j++) {
                i++;
                unicode = (unicode << 6) | (name[i] & 0x3F);
            }
            i++;
            
            char buf[16];
            std::snprintf(buf, sizeof(buf), "__u%04X__", unicode);
            result += buf;
        }
    }
    return result;
}

std::string LLVMCodegen::escapeLLVMString(const std::string& s) {
    std::string result;
    for (char c : s) {
        if (c == '\\') result += "\\\\";
        else if (c == '"') result += "\\\"";
        else if (c == '\n') result += "\\0A";
        else if (c == '\t') result += "\\09";
        else if (static_cast<unsigned char>(c) < 0x20 || c == 0x7F) {
            char buf[8];
            std::snprintf(buf, sizeof(buf), "\\%02X", static_cast<unsigned char>(c));
            result += buf;
        } else {
            result += c;
        }
    }
    return result;
}

// === 主编译入口 ===

llvm::Module* LLVMCodegen::generate(Shared<Program> program, OptLevel opt) {
    program_ = program;
    optLevel_ = opt;
    hasReturn_ = false;
    tempCounter_ = 0;
    varMap_.clear();
    funcParams_.clear();
    while (!loopStack_.empty()) loopStack_.pop();
    structTypes_.clear();
    varTypes_.clear();
    stringConstants_.clear();
    stringCounter_ = 0;
    
    // 先运行逃逸分析
    EscapeAnalyzer escapeAnalyzer;
    ProgramEscapeResult programEscapeResult = escapeAnalyzer.analyze(program);
    escapeResults_.clear();
    for (const auto& funcResult : programEscapeResult.functionResults) {
        // 找到对应的函数名
        for (const auto& stmt : program->statements) {
            if (auto func = std::dynamic_pointer_cast<FuncDeclStmt>(stmt)) {
                // 匹配函数参数数量和名称来确认是同一个函数
                bool match = true;
                if (func->params.size() != funcResult.varEscape.size()) continue;
                for (const auto& param : func->params) {
                    if (funcResult.varEscape.find(param.first) == funcResult.varEscape.end()) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    escapeResults_[func->name] = funcResult;
                    break;
                }
            }
        }
    }
    
    // 创建新模块
    module_ = std::make_unique<llvm::Module>("cplang_module", *context_);
    module_->setDataLayout(llvm::DataLayout(""));
    module_->setTargetTriple(llvm::sys::getDefaultTargetTriple());
    
    // 启用SIMD优化：设置目标CPU特性，支持AVX2、FMA等向量指令
    if (optLevel_ >= OptLevel::O2) {
        llvm::Triple targetTriple(module_->getTargetTriple());
        if (targetTriple.isX86()) {
            // 启用x86平台的AVX2、FMA、BMI2等现代CPU特性
            module_->addModuleFlag(llvm::Module::Override, "target-features", 
                llvm::ConstantDataArray::getString(*context_, "+avx2,+fma,+bmi2,+popcnt,+sse4.2"));
            // 开启快速数学模式，允许浮点运算重排，提升向量化效果
            module_->addModuleFlag(llvm::Module::Override, "unsafe-fp-math", llvm::ConstantInt::getTrue(*context_));
            module_->addModuleFlag(llvm::Module::Override, "no-infs-fp-math", llvm::ConstantInt::getTrue(*context_));
            module_->addModuleFlag(llvm::Module::Override, "no-nans-fp-math", llvm::ConstantInt::getTrue(*context_));
        }
    }
    
    // 生成代码
    generateProgram(module_.get(), program);
    
    // 移除包含 native 调用的函数（AOT 模式下跳过，保留外部符号供链接器解析）
    if (!skipNativeCallRemoval_) {
        std::unordered_set<llvm::Function*> toRemove;
        // Pass 1: 直接检测——调用外部声明（未定义函数）的函数
        // 跳过 jit_* 前缀的函数（它们是已注册的 JIT 运行时符号，可在 JIT 链接时解析）
        for (auto& func : module_->functions()) {
            if (func.isDeclaration()) continue;
            for (auto& bb : func) {
                for (auto& inst : bb) {
                    if (auto* callInst = llvm::dyn_cast<llvm::CallInst>(&inst)) {
                        if (auto* calledFunc = callInst->getCalledFunction()) {
                            if (calledFunc->isDeclaration()) {
                                std::string calleeName = calledFunc->getName().str();
                                // 已知的 JIT 运行时符号：这些在 OrcJIT::Impl::initialize() 中已注册
                                if (calleeName == "jit_printv" || calleeName == "jit_strcat" ||
                                    calleeName == "jit_table_create" || calleeName == "jit_table_get" ||
                                    calleeName == "jit_table_set" || calleeName == "jit_tick" || calleeName == "jit_call_native" ||
                                    calleeName == "jit_len" || calleeName == "jit_toString" ||
                                    calleeName == "jit_get_function_value" || calleeName == "jit_call_value") {
                                    continue; // 跳过，这些是已注册的 JIT 符号
                                }
                                toRemove.insert(&func);
                                break;
                            }
                        }
                    }
                }
                if (toRemove.count(&func)) break;
            }
        }
        // Pass 2+: 传递依赖——调用已被标记删除函数的函数也加入删除列表
        bool changed;
        do {
            changed = false;
            for (auto& func : module_->functions()) {
                if (func.isDeclaration()) continue;
                if (toRemove.count(&func)) continue;
                for (auto& bb : func) {
                    for (auto& inst : bb) {
                        if (auto* callInst = llvm::dyn_cast<llvm::CallInst>(&inst)) {
                            if (auto* calledFunc = callInst->getCalledFunction()) {
                                if (toRemove.count(calledFunc)) {
                                    toRemove.insert(&func);
                                    changed = true;
                                    break;
                                }
                            }
                        }
                    }
                    if (changed) break;
                }
                if (changed) break;
            }
        } while (changed);
        
        // 统一删除
        for (auto* func : toRemove) {
            func->eraseFromParent();
        }
    }
    
    // 运行优化
    if (opt != OptLevel::None) {
        runOptimizationPasses();
    }
    
    return module_.get();
}

llvm::Module* LLVMCodegen::generateSingleFunction(Shared<Program> program, const std::string& funcName, OptLevel opt) {
    program_ = program;
    optLevel_ = opt;
    hasReturn_ = false;
    tempCounter_ = 0;
    varMap_.clear();
    funcParams_.clear();
    while (!loopStack_.empty()) loopStack_.pop();
    structTypes_.clear();
    varTypes_.clear();
    stringConstants_.clear();
    stringCounter_ = 0;
    hasNativeCalls_ = false;
    
    // 创建新模块
    std::string moduleName = "cplang_hot_" + funcName;
    module_ = std::make_unique<llvm::Module>(moduleName, *context_);
    module_->setDataLayout(llvm::DataLayout(""));
    module_->setTargetTriple(llvm::sys::getProcessTriple());
    
    // 查找并生成目标函数
    bool found = false;
    for (auto& stmt : program->statements) {
        if (auto funcDecl = std::dynamic_pointer_cast<FuncDeclStmt>(stmt)) {
            if (funcDecl->name == funcName) {
                generateFuncDecl(module_.get(), funcDecl);
                found = true;
                break;
            }
        }
    }
    
    if (!found) {
        std::cerr << "[LLVMCodegen] 错误: 函数 '" << funcName << "' 未找到\n";
        return nullptr;
    }
    
    // 运行优化
    if (opt != OptLevel::None) {
        runOptimizationPasses();
    }
    
    return module_.get();
}

// === 程序生成 ===

void LLVMCodegen::generateProgram(llvm::Module* module, Shared<Program> program) {
    // 第一遍：生成结构体声明
    for (auto& stmt : program->statements) {
        if (auto structDecl = std::dynamic_pointer_cast<StructDeclStmt>(stmt)) {
            generateStructDecl(module, structDecl);
        }
    }
    
    // 第二遍：生成函数定义
    for (auto& stmt : program->statements) {
        if (auto funcDecl = std::dynamic_pointer_cast<FuncDeclStmt>(stmt)) {
            generateFuncDecl(module, funcDecl);
        }
    }
    
    // 第三遍：收集顶层脚本语句（非函数/非结构体），包装为合成入口函数
    bool hasTopLevelStmts = false;
    for (auto& stmt : program->statements) {
        if (!std::dynamic_pointer_cast<StructDeclStmt>(stmt) &&
            !std::dynamic_pointer_cast<FuncDeclStmt>(stmt)) {
            hasTopLevelStmts = true;
            break;
        }
    }
    
    if (hasTopLevelStmts) {
        // 清理跨函数状态
        varMap_.clear();
        varTypes_.clear();
        funcParams_.clear();
        loopStack_ = {};
        currentFuncIsFullyTyped_ = false;
        currentEscapeResult_ = nullptr;
        hasReturn_ = false;
        
        // 创建入口函数类型：i64 () - 返回 CP Value (NaN-boxed i64)
        llvm::Type* returnType = llvm::Type::getInt64Ty(*context_);
        llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, {}, false);
        
        // 创建 __cplang_entry 函数
        llvm::Function* entryFunc = llvm::Function::Create(
            funcType, llvm::Function::ExternalLinkage, "__cplang_entry", module);
        
        // 创建入口基本块
        llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(*context_, "entry", entryFunc);
        builder_->SetInsertPoint(entryBlock);
        
        // 生成所有顶层语句
        for (auto& stmt : program->statements) {
            if (std::dynamic_pointer_cast<StructDeclStmt>(stmt) ||
                std::dynamic_pointer_cast<FuncDeclStmt>(stmt)) {
                continue;
            }
            generateStatement(entryFunc, stmt);
            // 检查当前块是否已终止（如顶层 return），已终止则停止
            llvm::BasicBlock* curBlock = builder_->GetInsertBlock();
            if (curBlock && !curBlock->empty() && curBlock->back().isTerminator()) {
                break;
            }
        }
        
        // 确保终止指令
        llvm::BasicBlock* currentBlock = builder_->GetInsertBlock();
        if (currentBlock && (currentBlock->empty() || !currentBlock->back().isTerminator())) {
            llvm::Value* zero = llvm::ConstantInt::get(*context_, llvm::APInt(64, 0));
            builder_->CreateRet(zero);
        }
        
        // 验证函数
        if (llvm::verifyFunction(*entryFunc, &llvm::errs())) {
            std::cerr << "[LLVMCodegen] 警告: __cplang_entry 合成函数验证失败\n";
        }
    }
    
    // 如果源代码定义了 func main() 但没有顶层语句，
    // 创建 __cplang_entry 包装它，供 AOT 链接器生成正确的 main 入口
    if (!hasTopLevelStmts) {
        // 检查是否有用户定义的 main 函数（已重命名为 __cp_main）
        llvm::Function* existingMain = module->getFunction("__cp_main");
        // 也检查原始的 @main（如 JIT 模式或未重命名情况）
        if (!existingMain) {
            existingMain = module->getFunction("main");
        }
        if (existingMain) {
            varMap_.clear();
            varTypes_.clear();
            llvm::FunctionType* entryType = llvm::FunctionType::get(
                llvm::Type::getInt64Ty(*context_), {}, false);
            llvm::Function* entryFunc = llvm::Function::Create(
                entryType, llvm::Function::ExternalLinkage, "__cplang_entry", module);
            
            llvm::BasicBlock* block = llvm::BasicBlock::Create(*context_, "entry", entryFunc);
            builder_->SetInsertPoint(block);
            
            // 调用 @main() 执行副作用，但始终返回 0（AOT 入口统一返回码）
            (void)builder_->CreateCall(existingMain, {}, "main_call");
            builder_->CreateRet(llvm::ConstantInt::get(*context_, llvm::APInt(64, 0)));
        }
    }
}

// === 辅助函数 ===

llvm::Value* LLVMCodegen::getTempVar(llvm::Function* func, const std::string& baseName) {
    llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());
    llvm::Type* i64Type = llvm::Type::getInt64Ty(*context_);
    llvm::Value* alloca = tmpBuilder.CreateAlloca(i64Type, nullptr, baseName);
    return alloca;
}

llvm::Value* LLVMCodegen::loadVar(const std::string& varName) {
    auto it = varMap_.find(varName);
    if (it == varMap_.end()) {
        // AOT 模式：未定义的变量尝试从 VM 全局变量加载
        if (skipNativeCallRemoval_ && !pureMath_) {
            llvm::LLVMContext& ctx = *context_;
            llvm::Type* i64Ty = llvm::Type::getInt64Ty(ctx);
            llvm::Type* i8PtrTy = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(ctx));
            llvm::Constant* nameStr = llvm::ConstantDataArray::getString(ctx, varName);
            llvm::GlobalVariable* nameGlobal = new llvm::GlobalVariable(
                *module_, nameStr->getType(), true,
                llvm::GlobalValue::InternalLinkage, nameStr, ".global_name");
            llvm::Value* zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx), 0);
            llvm::Value* namePtr = builder_->CreateInBoundsGEP(nameStr->getType(), nameGlobal,
                {zero, zero}, "global_name_ptr");
            llvm::Function* getGlobalFunc = module_->getFunction("aot_get_global");
            if (!getGlobalFunc) {
                llvm::FunctionType* ft = llvm::FunctionType::get(i64Ty, {i8PtrTy}, false);
                getGlobalFunc = llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                    "aot_get_global", module_.get());
            }
            return builder_->CreateCall(getGlobalFunc, {namePtr}, "global");
        }
        // 检查是否是函数名 → 运行时查函数值
        if (program_) {
            for (auto& stmt : program_->statements) {
                if (auto fd = std::dynamic_pointer_cast<FuncDeclStmt>(stmt)) {
                    if (fd->name == varName) {
                        // 生成 jit_get_function_value("funcName") 调用
                        llvm::Type* i8PtrTy = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(*context_));
                        llvm::Function* getFuncVal = module_->getFunction("jit_get_function_value");
                        if (!getFuncVal) {
                            llvm::FunctionType* ft = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context_), {i8PtrTy}, false);
                            getFuncVal = llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                                "jit_get_function_value", module_.get());
                        }
                        llvm::Constant* nameStr = llvm::ConstantDataArray::getString(*context_, varName);
                        llvm::GlobalVariable* nameGV = new llvm::GlobalVariable(
                            *module_, nameStr->getType(), true,
                            llvm::GlobalValue::InternalLinkage, nameStr, ".fn_name");
                        llvm::Value* zero = llvm::ConstantInt::get(*context_, llvm::APInt(32, 0));
                        llvm::Value* namePtr = builder_->CreateInBoundsGEP(
                            nameStr->getType(), nameGV, {zero, zero}, "fn_name_ptr");
                        return builder_->CreateCall(getFuncVal, {namePtr}, "fn_val");
                    }
                }
            }
        }
        std::cerr << "[LLVMCodegen] 错误: 变量 '" << varName << "' 未定义\n";
        return llvm::ConstantInt::get(*context_, llvm::APInt(64, 0));
    }
    llvm::Value* ptr = it->second;
    return builder_->CreateLoad(llvm::Type::getInt64Ty(*context_), ptr, "load_" + varName);
}

void LLVMCodegen::storeVar(const std::string& varName, llvm::Value* value) {
    auto it = varMap_.find(varName);
    if (it == varMap_.end()) {
        std::cerr << "[LLVMCodegen] 错误: 变量 '" << varName << "' 未定义\n";
        return;
    }
    llvm::Value* ptr = it->second;
    builder_->CreateStore(value, ptr);
}

// === 辅助函数 ===
// 判断函数是否完全类型标注（所有参数+返回值都有类型）
bool LLVMCodegen::isFullyTyped(Shared<FuncDeclStmt> decl) {
    if (!decl->returnType.has_value()) return false;
    for (auto& param : decl->params) {
        if (!param.second.has_value()) return false;
    }
    return true;
}

llvm::Type* LLVMCodegen::getLLVMType(const std::string& typeName) {
    if (typeName.empty()) return llvm::Type::getInt64Ty(*context_);
    if (typeName == "int" || typeName == "整数") return llvm::Type::getInt64Ty(*context_);
    if (typeName == "float" || typeName == "浮点数") return llvm::Type::getDoubleTy(*context_);
    if (typeName == "bool" || typeName == "布尔") return llvm::Type::getInt1Ty(*context_);
    if (typeName == "string" || typeName == "字符串") return llvm::PointerType::getUnqual(*context_);
    
    // 结构体类型
    auto it = structTypes_.find(typeName);
    if (it != structTypes_.end()) {
        return it->second;
    }
    
    return llvm::Type::getInt64Ty(*context_);
}

llvm::Value* LLVMCodegen::toDouble(llvm::Value* val) {
    llvm::Type* valType = val->getType();
    if (valType->isDoubleTy()) return val;
    if (valType->isIntegerTy(64)) {
        return builder_->CreateSIToFP(val, llvm::Type::getDoubleTy(*context_), "toDouble");
    }
    return val;
}

llvm::Value* LLVMCodegen::toInt64(llvm::Value* val) {
    llvm::Type* valType = val->getType();
    if (valType->isIntegerTy(64)) return val;
    if (valType->isDoubleTy()) {
        return builder_->CreateFPToSI(val, llvm::Type::getInt64Ty(*context_), "toInt64");
    }
    return val;
}

// === 字符串常量处理 ===

llvm::Value* LLVMCodegen::registerStringConstant(const std::string& content) {
    auto it = stringConstants_.find(content);
    if (it != stringConstants_.end()) {
        // 已有全局变量，获取其 i8* 指针并在此处新鲜 NaN-box
        llvm::GlobalVariable* existingGV = it->second;
        llvm::ArrayType* arrayTy = llvm::cast<llvm::ArrayType>(existingGV->getValueType());
        llvm::Value* zero = llvm::ConstantInt::get(*context_, llvm::APInt(32, 0));
        llvm::Value* indices[] = {zero, zero};
        llvm::Value* strPtr = builder_->CreateGEP(arrayTy, existingGV, indices, "strPtr");
        return nanBoxStringPtr(strPtr);
    }
    
    // 创建全局字符串常量
    llvm::LLVMContext& ctx = *context_;
    llvm::Module* module = module_.get();
    
    std::string globalName = "str_" + std::to_string(stringCounter_++);
    
    // 创建字符数组类型
    llvm::Type* i8Type = llvm::Type::getInt8Ty(ctx);
    llvm::ArrayType* arrayType = llvm::ArrayType::get(i8Type, content.size() + 1);
    
    // 创建全局变量
    llvm::Constant* strConstant = llvm::ConstantDataArray::getString(ctx, content, true);
    llvm::GlobalVariable* globalVar = new llvm::GlobalVariable(
        *module,
        arrayType,
        true,  // isConstant
        llvm::GlobalValue::PrivateLinkage,
        strConstant,
        globalName
    );
    
    // 获取指向字符串的指针（i8*），然后构造 NaN-boxed 的 CP Value（i64）
    llvm::Value* zero = llvm::ConstantInt::get(ctx, llvm::APInt(32, 0));
    llvm::Value* indices[] = {zero, zero};
    llvm::Value* strPtr = builder_->CreateGEP(arrayType, globalVar, indices, "strPtr");
    llvm::Value* taggedVal = nanBoxStringPtr(strPtr);
    
    // 缓存全局变量（非 NaN-boxed 值，因为 NaN-boxing 指令依赖当前插入点）
    stringConstants_[content] = globalVar;
    return taggedVal;
}

llvm::Value* LLVMCodegen::nanBoxStringPtr(llvm::Value* rawPtr) {
    // NaN-boxed 字符串格式: 0xFFFF_0000_00000000 | (ptr & 0x0000_FFFF_FFFF_FFFF)
    llvm::Value* ptrInt = builder_->CreatePtrToInt(rawPtr, llvm::Type::getInt64Ty(*context_), "ptrInt");
    llvm::Value* ptrMasked = builder_->CreateAnd(ptrInt,
        llvm::ConstantInt::get(*context_, llvm::APInt(64, 0x0000FFFFFFFFFFFFULL)), "ptrMasked");
    llvm::Value* nanTag = llvm::ConstantInt::get(*context_, llvm::APInt(64, 0xFFFF000000000000ULL));
    return builder_->CreateOr(ptrMasked, nanTag, "strVal");
}

// === 结构体生成 ===

void LLVMCodegen::generateStructDecl(llvm::Module* module, Shared<StructDeclStmt> decl) {
    std::string structName = decl->name;
    std::string safeName = sanitizeName(structName);
    
    // 检查是否已定义
    if (structTypes_.find(structName) != structTypes_.end()) {
        return;  // 已定义，跳过
    }
    
    // 创建结构体类型
    std::vector<llvm::Type*> memberTypes;
    for (auto& member : decl->members) {
        auto vdecl = std::dynamic_pointer_cast<VarDeclStmt>(member);
        if (vdecl) {
            llvm::Type* memberType = getLLVMType(vdecl->type.value_or(""));
            memberTypes.push_back(memberType);
        }
    }
    
    llvm::StructType* structType = llvm::StructType::create(*context_, memberTypes, safeName);
    structTypes_[structName] = structType;
    
    // 记录成员索引
    for (size_t i = 0; i < decl->members.size(); i++) {
        auto vdecl = std::dynamic_pointer_cast<VarDeclStmt>(decl->members[i]);
        if (vdecl) {
            structMembers_[structName].push_back(vdecl->name);
        }
    }
}

// === 函数生成 ===

llvm::Function* LLVMCodegen::generateFuncDecl(llvm::Module* module, Shared<FuncDeclStmt> decl) {
    std::string funcName = decl->name;
    std::string safeName = sanitizeName(funcName);
    
    // 重命名 main 函数以避免与 CRT 的 main(i32, ptr) 冲突
    bool isMainFunc = (safeName == "main");
    if (isMainFunc) {
        safeName = "__cp_main";
    }
    
    // 检查函数是否已定义
    llvm::Function* func = module->getFunction(safeName);
    if (func) {
        return func;  // 已定义，跳过
    }
    
    // 清理跨函数变量映射
    varMap_.clear();
    varTypes_.clear();
    funcParams_.clear();
    loopStack_ = {};
    
    // 判断是否完全类型标注
    currentFuncIsFullyTyped_ = isFullyTyped(decl);
    
    // 获取当前函数的逃逸分析结果
    auto it = escapeResults_.find(funcName);
    if (it != escapeResults_.end()) {
        currentEscapeResult_ = &it->second;
    } else {
        currentEscapeResult_ = nullptr;
    }
    
    // 确定函数类型
    llvm::Type* returnType;
    if (currentFuncIsFullyTyped_) {
        returnType = getLLVMType(*decl->returnType);
    } else {
        returnType = llvm::Type::getInt64Ty(*context_);  // 默认返回 i64（动态类型装箱值）
    }
    
    std::vector<llvm::Type*> paramTypes;
    
    for (auto& param : decl->params) {
        llvm::Type* paramType = getLLVMType(param.second.value_or(""));
        paramTypes.push_back(paramType);
    }
    
    llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, paramTypes, false);
    
    // 创建函数
    func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, safeName, module);
    
    // 设置参数名
    int paramIdx = 0;
    for (auto& arg : func->args()) {
        if (paramIdx < static_cast<int>(decl->params.size())) {
            arg.setName(decl->params[paramIdx].first);
            paramIdx++;
        }
    }
    
    // 创建基本块
    llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(*context_, "entry", func);
    builder_->SetInsertPoint(entryBlock);
    
    // 为参数创建 alloca
    for (auto& arg : func->args()) {
        llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());
        llvm::Value* alloca = tmpBuilder.CreateAlloca(arg.getType(), nullptr, arg.getName().str() + "_addr");
        builder_->CreateStore(&arg, alloca);
        varMap_[arg.getName().str()] = alloca;
        funcParams_.insert(arg.getName().str());
        // 保存参数类型
        if (decl->params[paramIdx-1].second.has_value()) {
            varTypes_[arg.getName().str()] = *decl->params[paramIdx-1].second;
        }
    }
    
    // 生成函数体
    hasReturn_ = false;
    for (auto& stmt : decl->body->statements) {
        generateStatement(func, stmt);
        // 检查当前块是否已终止（如顶层 return），已终止则后续语句是死代码
        llvm::BasicBlock* curBlock = builder_->GetInsertBlock();
        if (curBlock && curBlock->back().isTerminator()) {
            break;
        }
    }
    
    // 确保当前块有终止指令
    llvm::BasicBlock* currentBlock = builder_->GetInsertBlock();
    if (currentBlock && !currentBlock->back().isTerminator()) {
            if (currentFuncIsFullyTyped_) {
                // 完全类型函数返回对应类型的零值
                if (returnType->isIntegerTy()) {
                    llvm::Value* zero = llvm::ConstantInt::get(*context_, llvm::APInt(returnType->getIntegerBitWidth(), 0));
                    builder_->CreateRet(zero);
                } else if (returnType->isDoubleTy()) {
                    llvm::Value* zero = llvm::ConstantFP::get(*context_, llvm::APFloat(0.0));
                    builder_->CreateRet(zero);
                } else if (returnType->isPointerTy()) {
                    llvm::Value* nullPtr = llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(*context_));
                    builder_->CreateRet(nullPtr);
                }
            } else {
                // 默认返回0
                llvm::Value* zero = llvm::ConstantInt::get(*context_, llvm::APInt(64, 0));
                builder_->CreateRet(zero);
            }
        }

    
    // 验证函数
    {
        std::string errStr;
        llvm::raw_string_ostream errStream(errStr);
        if (llvm::verifyFunction(*func, &errStream)) {
            std::cerr << "[LLVMCodegen] 错误: 函数 '" << funcName << "' 验证失败:\n" << errStr << "\n";
            func->eraseFromParent();
            return nullptr;
        }
    }
    
    // 重置标记
    currentFuncIsFullyTyped_ = false;
    currentEscapeResult_ = nullptr;
    return func;
}

// === 语句生成 ===

void LLVMCodegen::generateStatement(llvm::Function* func, Shared<Stmt> stmt) {
    if (!stmt) return;
    
    if (auto varDecl = std::dynamic_pointer_cast<VarDeclStmt>(stmt)) {
        generateVarDecl(func, varDecl);
    } else if (auto returnStmt = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
        generateReturnStmt(func, returnStmt);
    } else if (auto ifStmt = std::dynamic_pointer_cast<IfStmt>(stmt)) {
        generateIfStmt(func, ifStmt);
    } else if (auto whileStmt = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
        generateWhileStmt(func, whileStmt);
    } else if (auto forStmt = std::dynamic_pointer_cast<ForStmt>(stmt)) {
        generateForStmt(func, forStmt);
    } else if (auto breakStmt = std::dynamic_pointer_cast<BreakStmt>(stmt)) {
        generateBreakStmt(func, breakStmt);
    } else if (auto continueStmt = std::dynamic_pointer_cast<ContinueStmt>(stmt)) {
        generateContinueStmt(func, continueStmt);
    } else if (auto importStmt = std::dynamic_pointer_cast<ImportStmt>(stmt)) {
        generateImportStmt(func, importStmt);
    } else if (auto exprStmt = std::dynamic_pointer_cast<ExprStmt>(stmt)) {
        generateExpression(func, exprStmt->expr);
    }
}

void LLVMCodegen::generateImportStmt(llvm::Function* func, Shared<ImportStmt> stmt) {
    if (!skipNativeCallRemoval_) return;
    llvm::LLVMContext& ctx = *context_;
    llvm::Type* i32Ty = llvm::Type::getInt32Ty(ctx);
    llvm::Type* i8PtrTy = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(ctx));

    llvm::Constant* nameStr = llvm::ConstantDataArray::getString(ctx, stmt->moduleName);
    llvm::GlobalVariable* nameGlobal = new llvm::GlobalVariable(
        *module_, nameStr->getType(), true,
        llvm::GlobalValue::InternalLinkage, nameStr, ".import_name");
    llvm::Value* namePtr = builder_->CreateInBoundsGEP(nameStr->getType(), nameGlobal,
        {llvm::ConstantInt::get(i32Ty, 0), llvm::ConstantInt::get(i32Ty, 0)}, "import_name_ptr");

    llvm::Function* importFunc = module_->getFunction("aot_import_module");
    if (!importFunc) {
        llvm::FunctionType* ft = llvm::FunctionType::get(i32Ty, {i8PtrTy}, false);
        importFunc = llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
            "aot_import_module", module_.get());
    }
    builder_->CreateCall(importFunc, {namePtr}, "import");
    (void)func;
}

void LLVMCodegen::generateVarDecl(llvm::Function* func, Shared<VarDeclStmt> stmt) {
    // 隐式变量声明（如 s = expr，没有 "变量" 关键字）：
    // 如果变量已存在，说明是赋值而非声明，直接存储值并返回
    if (stmt->isImplicit) {
        auto it = varMap_.find(stmt->name);
        if (it != varMap_.end()) {
            if (stmt->init) {
                llvm::Value* initVal = generateExpression(func, stmt->init);
                builder_->CreateStore(initVal, it->second);
            }
            return;
        }
    }
    
    llvm::Type* varType = nullptr;
    bool canStackAlloc = false;
    
    // 检查是否可以栈上分配
    if (currentEscapeResult_ != nullptr) {
        auto varIt = currentEscapeResult_->varEscape.find(stmt->name);
        if (varIt != currentEscapeResult_->varEscape.end()) {
            // 不逃逸且类型大小<=MAX_STACK_ALLOC_SIZE可以栈分配
            if (varIt->second.level == EscapeLevel::None) {
                // 计算类型大小
                if (stmt->type.has_value()) {
                    llvm::Type* llvmType = getLLVMType(*stmt->type);
                    auto& dl = module_->getDataLayout();
                    uint64_t typeSize = dl.getTypeAllocSize(llvmType);
                    if (typeSize <= MAX_STACK_ALLOC_SIZE) {
                        canStackAlloc = true;
                        varType = llvmType;
                    }
                }
            }
        }
    }
    
    // 默认类型
    if (!varType) {
        varType = llvm::Type::getInt64Ty(*context_);  // 动态类型默认i64
    }
    
    llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());
    llvm::Value* alloca = tmpBuilder.CreateAlloca(varType, nullptr, stmt->name);
    
    // 存入变量映射
    varMap_[stmt->name] = alloca;
    // 保存变量类型
    if (stmt->type.has_value()) {
        varTypes_[stmt->name] = *stmt->type;
    }
    
    // 如果有初始化表达式，生成并存储
    if (stmt->init) {
        llvm::Value* initVal = generateExpression(func, stmt->init);
        // 如果是栈分配的小对象，不需要装箱，直接存储原生值
        if (canStackAlloc && stmt->type.has_value()) {
            // 类型转换为对应原生类型
            if (varType->isIntegerTy() && !initVal->getType()->isIntegerTy()) {
                initVal = builder_->CreateFPToSI(initVal, varType);
            } else if (varType->isDoubleTy() && !initVal->getType()->isDoubleTy()) {
                initVal = builder_->CreateSIToFP(initVal, varType);
            }
        }
        builder_->CreateStore(initVal, alloca);
    }
    
    // 记录数组/字符串字面量长度（用于 .length 编译期折叠）
    if (stmt->init) {
        if (auto arrLit = std::dynamic_pointer_cast<ArrayExpr>(stmt->init)) {
            literalLenByVar_[stmt->name] = arrLit->elements.size();
        } else if (auto strLit = std::dynamic_pointer_cast<LiteralExpr>(stmt->init)) {
            if (auto* s = std::get_if<String>(&strLit->value)) {
                literalLenByVar_[stmt->name] = s->length();
            }
        }
    }
}

void LLVMCodegen::generateReturnStmt(llvm::Function* func, Shared<ReturnStmt> stmt) {
    if (stmt->value) {
        llvm::Value* retVal = generateExpression(func, stmt->value);
        builder_->CreateRet(retVal);
    } else {
        llvm::Value* zero = llvm::ConstantInt::get(*context_, llvm::APInt(64, 0));
        builder_->CreateRet(zero);
    }
    hasReturn_ = true;
}

void LLVMCodegen::generateIfStmt(llvm::Function* func, Shared<IfStmt> stmt) {
    llvm::BasicBlock* thenBlock = llvm::BasicBlock::Create(*context_, "if.then", func);
    auto thenBlockStmt = std::dynamic_pointer_cast<BlockStmt>(stmt->thenBranch);
    auto elseBlockStmt = stmt->elseBranch ? std::dynamic_pointer_cast<BlockStmt>(stmt->elseBranch) : nullptr;
    llvm::BasicBlock* elseBlock = elseBlockStmt ? llvm::BasicBlock::Create(*context_, "if.else", func) : nullptr;
    llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(*context_, "if.merge", func);
    
    // 生成条件
    llvm::Value* condVal = generateExpression(func, stmt->condition);
    llvm::Value* condBool = builder_->CreateICmpNE(condVal, llvm::ConstantInt::get(*context_, llvm::APInt(64, 0)), "if.cond");
    
    // 条件分支
    if (elseBlock) {
        builder_->CreateCondBr(condBool, thenBlock, elseBlock);
    } else {
        builder_->CreateCondBr(condBool, thenBlock, mergeBlock);
    }
    
    // Then 块
    builder_->SetInsertPoint(thenBlock);
    if (thenBlockStmt) {
        for (auto& s : thenBlockStmt->statements) {
            generateStatement(func, s);
        }
    }
    // 检查最后一块是否有终止指令（非 thenBlock 第一个块，因为生成语句可能创建新块）
    {
        llvm::BasicBlock* curThenBlock = builder_->GetInsertBlock();
        if (curThenBlock && (curThenBlock->empty() || !curThenBlock->back().isTerminator())) {
            builder_->CreateBr(mergeBlock);
        }
    }
    
    // Else 块
    if (elseBlock && elseBlockStmt) {
        builder_->SetInsertPoint(elseBlock);
        for (auto& s : elseBlockStmt->statements) {
            generateStatement(func, s);
        }
        {
            llvm::BasicBlock* curElseBlock = builder_->GetInsertBlock();
            if (curElseBlock && (curElseBlock->empty() || !curElseBlock->back().isTerminator())) {
                builder_->CreateBr(mergeBlock);
            }
        }
    }
    
    // Merge 块
    builder_->SetInsertPoint(mergeBlock);
}

void LLVMCodegen::generateWhileStmt(llvm::Function* func, Shared<WhileStmt> stmt) {
    llvm::BasicBlock* condBlock = llvm::BasicBlock::Create(*context_, "while.cond", func);
    llvm::BasicBlock* bodyBlock = llvm::BasicBlock::Create(*context_, "while.body", func);
    llvm::BasicBlock* endBlock = llvm::BasicBlock::Create(*context_, "while.end", func);
    
    // 进入条件块
    builder_->CreateBr(condBlock);
    builder_->SetInsertPoint(condBlock);
    
    // 生成条件
    llvm::Value* condVal = generateExpression(func, stmt->condition);
    llvm::Value* condBool = builder_->CreateICmpNE(condVal, llvm::ConstantInt::get(*context_, llvm::APInt(64, 0)), "while.cond");
    
    // 条件分支
    builder_->CreateCondBr(condBool, bodyBlock, endBlock);
    
    // 循环体
    builder_->SetInsertPoint(bodyBlock);
    
    // 保存循环上下文
    loopStack_.push({condBlock, endBlock, nullptr});
    
    auto loopBody = std::dynamic_pointer_cast<BlockStmt>(stmt->body);
    if (loopBody) {
        for (auto& s : loopBody->statements) {
            generateStatement(func, s);
        }
    }
    
    loopStack_.pop();
    
    // 检查当前块是否有 terminator（可能因内部 if/while 改变了插入点）
    llvm::BasicBlock* curBlock = builder_->GetInsertBlock();
    if (curBlock && !curBlock->back().isTerminator()) {
        builder_->CreateBr(condBlock);
    }
    
    // 结束块
    builder_->SetInsertPoint(endBlock);
}

void LLVMCodegen::generateForStmt(llvm::Function* func, Shared<ForStmt> stmt) {
    llvm::BasicBlock* condBlock = llvm::BasicBlock::Create(*context_, "for.cond", func);
    llvm::BasicBlock* bodyBlock = llvm::BasicBlock::Create(*context_, "for.body", func);
    llvm::BasicBlock* incBlock = llvm::BasicBlock::Create(*context_, "for.inc", func);
    llvm::BasicBlock* endBlock = llvm::BasicBlock::Create(*context_, "for.end", func);
    
    // 初始化
    if (stmt->init) {
        generateStatement(func, stmt->init);
    }
    
    // 进入条件块
    builder_->CreateBr(condBlock);
    builder_->SetInsertPoint(condBlock);
    
    // 条件
    if (stmt->condition) {
        llvm::Value* condVal = generateExpression(func, stmt->condition);
        llvm::Value* condBool = builder_->CreateICmpNE(condVal, llvm::ConstantInt::get(*context_, llvm::APInt(64, 0)), "for.cond");
        builder_->CreateCondBr(condBool, bodyBlock, endBlock);
    } else {
        // 无条件 = 无限循环
        builder_->CreateBr(bodyBlock);
    }
    
    // 循环体
    builder_->SetInsertPoint(bodyBlock);
    
    // 保存循环上下文
    loopStack_.push({condBlock, endBlock, incBlock});
    
    auto forBody = std::dynamic_pointer_cast<BlockStmt>(stmt->body);
    if (forBody) {
        for (auto& s : forBody->statements) {
            generateStatement(func, s);
        }
    }
    
    loopStack_.pop();
    
    // 递增（检查当前插入点块，可能因内部 if/while 改变了）
    llvm::BasicBlock* forCurBlock = builder_->GetInsertBlock();
    if (forCurBlock && !forCurBlock->back().isTerminator()) {
        builder_->CreateBr(incBlock);
    }
    
    builder_->SetInsertPoint(incBlock);
    if (stmt->update) {
        generateExpression(func, stmt->update);
    }
    builder_->CreateBr(condBlock);
    
    // 结束块
    builder_->SetInsertPoint(endBlock);
}

void LLVMCodegen::generateBreakStmt(llvm::Function* func, Shared<BreakStmt> stmt) {
    if (!loopStack_.empty()) {
        llvm::BasicBlock* endBlock = loopStack_.top().endBlock;
        builder_->CreateBr(endBlock);
    }
}

void LLVMCodegen::generateContinueStmt(llvm::Function* func, Shared<ContinueStmt> stmt) {
    if (!loopStack_.empty()) {
        auto& ctx = loopStack_.top();
        llvm::BasicBlock* target = ctx.incBlock ? ctx.incBlock : ctx.condBlock;
        builder_->CreateBr(target);
    }
}

// === 表达式生成 ===

llvm::Value* LLVMCodegen::generateExpression(llvm::Function* func, Shared<Expr> expr) {
    if (!expr) return llvm::ConstantInt::get(*context_, llvm::APInt(64, 0));
    
    // 优先处理内置函数调用，直接生成LLVM指令，消除C++调用开销
    if (auto callExpr = std::dynamic_pointer_cast<CallExpr>(expr)) {
        if (auto callee = std::dynamic_pointer_cast<IdentifierExpr>(callExpr->callee)) {
            const std::string& funcName = callee->name;
            // 间接调用：函数值/变量（fn 在 varMap_ 或 funcParams_ 中，不是直接函数名）
            if (varMap_.find(funcName) != varMap_.end() || funcParams_.count(funcName)) {
                llvm::Value* fnVal = loadVar(funcName);
                // 生成参数数组
                int nArgs = (int)callExpr->arguments.size();
                llvm::Type* i64Ty = llvm::Type::getInt64Ty(*context_);
                llvm::Type* i32Ty = llvm::Type::getInt32Ty(*context_);
                llvm::ArrayType* arrTy = llvm::ArrayType::get(i64Ty, nArgs ? nArgs : 1);
                llvm::Value* argsAlloca = builder_->CreateAlloca(arrTy, nullptr, "indirect_args");
                llvm::Value* zero = llvm::ConstantInt::get(i32Ty, 0);
                for (int i = 0; i < nArgs; i++) {
                    llvm::Value* argVal = generateExpression(func, callExpr->arguments[i]);
                    llvm::Value* gep = builder_->CreateInBoundsGEP(arrTy, argsAlloca,
                        {zero, llvm::ConstantInt::get(i32Ty, i)}, "indirect_arg");
                    builder_->CreateStore(argVal, gep);
                }
                // 调用 jit_call_value(fnValue, argc, args)
                llvm::Function* callValFunc = module_->getFunction("jit_call_value");
                if (!callValFunc) {
                    llvm::FunctionType* ft = llvm::FunctionType::get(i64Ty,
                        {i64Ty, i32Ty, llvm::PointerType::getUnqual(i64Ty)}, false);
                    callValFunc = llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                        "jit_call_value", module_.get());
                }
                llvm::Value* argsBase = builder_->CreateInBoundsGEP(arrTy, argsAlloca,
                    {zero, zero}, "indirect_args_base");
                return builder_->CreateCall(callValFunc,
                    {fnVal, llvm::ConstantInt::get(i32Ty, nArgs), argsBase}, "indirect_call");
            }
            // 单参数数学函数
            if (callExpr->arguments.size() == 1) {
                llvm::Value* arg = generateExpression(func, callExpr->arguments[0]);
                if (arg) {
                    if (funcName == "abs" || funcName == "绝对值") {
                        if (arg->getType()->isIntegerTy()) {
                            // 整数绝对值：生成 xor + sub 指令
                            llvm::Value* zero = llvm::ConstantInt::get(arg->getType(), 0);
                            llvm::Value* neg = builder_->CreateNeg(arg);
                            llvm::Value* isNeg = builder_->CreateICmpSLT(arg, zero);
                            return builder_->CreateSelect(isNeg, neg, arg);
                        } else if (arg->getType()->isDoubleTy()) {
                            // 浮点数绝对值：调用llvm.fabs intrinsic
                            return builder_->CreateUnaryIntrinsic(llvm::Intrinsic::fabs, arg);
                        }
                    } else if (funcName == "sqrt" || funcName == "平方根") {
                        if (arg->getType()->isDoubleTy()) {
                            return builder_->CreateUnaryIntrinsic(llvm::Intrinsic::sqrt, arg);
                        }
                    } else if (funcName == "floor" || funcName == "向下取整") {
                        if (arg->getType()->isDoubleTy()) {
                            return builder_->CreateUnaryIntrinsic(llvm::Intrinsic::floor, arg);
                        }
                    } else if (funcName == "ceil" || funcName == "向上取整") {
                        if (arg->getType()->isDoubleTy()) {
                            return builder_->CreateUnaryIntrinsic(llvm::Intrinsic::ceil, arg);
                        }
                    } else if (funcName == "round" || funcName == "四舍五入") {
                        if (arg->getType()->isDoubleTy()) {
                            return builder_->CreateUnaryIntrinsic(llvm::Intrinsic::round, arg);
                        }
                    }
                }
            }
            // 双参数数学函数
            else if (callExpr->arguments.size() == 2) {
                llvm::Value* arg0 = generateExpression(func, callExpr->arguments[0]);
                llvm::Value* arg1 = generateExpression(func, callExpr->arguments[1]);
                if (arg0 && arg1) {
                    if (funcName == "pow" || funcName == "幂") {
                        if (arg0->getType()->isDoubleTy() && arg1->getType()->isDoubleTy()) {
                            return builder_->CreateBinaryIntrinsic(llvm::Intrinsic::pow, arg0, arg1);
                        }
                    }
                }
            }
        }
    }
    
    // 字面量
    if (auto lit = std::dynamic_pointer_cast<LiteralExpr>(expr)) {
        if (std::holds_alternative<Int64>(lit->value)) {
            Int64 val = std::get<Int64>(lit->value);
            return llvm::ConstantInt::get(*context_, llvm::APInt(64, val));
        } else if (std::holds_alternative<Float64>(lit->value)) {
            Float64 val = std::get<Float64>(lit->value);
            return llvm::ConstantFP::get(*context_, llvm::APFloat(val));
        } else if (std::holds_alternative<std::string>(lit->value)) {
            const std::string& content = std::get<std::string>(lit->value);
            return registerStringConstant(content);
        } else if (std::holds_alternative<bool>(lit->value)) {
            bool val = std::get<bool>(lit->value);
            return llvm::ConstantInt::get(*context_, llvm::APInt(64, val ? 1 : 0));
        }
    }
    
    // 标识符
    if (auto ident = std::dynamic_pointer_cast<IdentifierExpr>(expr)) {
        return loadVar(ident->name);
    }
    
    // 二元表达式
    if (auto binary = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        // 处理赋值（先求右值，因为左值可能是需要特殊处理的下标/成员表达式）
        if (binary->op == TokenType::OP_ASSIGN) {
            if (auto leftIdent = std::dynamic_pointer_cast<IdentifierExpr>(binary->left)) {
                llvm::Value* rightVal = generateExpression(func, binary->right);
                storeVar(leftIdent->name, rightVal);
                return rightVal;
            }
            // 数组元素赋值: arr[i] = val
            if (auto leftIndex = std::dynamic_pointer_cast<IndexExpr>(binary->left)) {
                llvm::Value* arrayVal = generateExpression(func, leftIndex->array);
                llvm::Value* indexVal = generateExpression(func, leftIndex->index);
                llvm::Value* rightVal = generateExpression(func, binary->right);
                llvm::Type* i64Ty = llvm::Type::getInt64Ty(*context_);
                
                // 运行时派发：表走 jit_table_set，栈数组走 GEP+store
                llvm::Value* hi48 = builder_->CreateLShr(arrayVal, 48, "hi48_w");
                llvm::Value* isTable = builder_->CreateICmpEQ(hi48, llvm::ConstantInt::get(i64Ty, 0xFFFF), "istable_w");
                llvm::Function* currFn = builder_->GetInsertBlock()->getParent();
                llvm::BasicBlock* tblBlock = llvm::BasicBlock::Create(*context_, "tbl_write", currFn);
                llvm::BasicBlock* arrBlock = llvm::BasicBlock::Create(*context_, "arr_write", currFn);
                llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(*context_, "idx_write_end", currFn);
                builder_->CreateCondBr(isTable, tblBlock, arrBlock);
                
                // 表路径: jit_table_set
                builder_->SetInsertPoint(tblBlock);
                llvm::Function* setFunc = module_->getFunction("jit_table_set");
                if (!setFunc) {
                    llvm::FunctionType* ft = llvm::FunctionType::get(i64Ty, {i64Ty, i64Ty, i64Ty}, false);
                    setFunc = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "jit_table_set", module_.get());
                }
                builder_->CreateCall(setFunc, {arrayVal, indexVal, rightVal}, "tbl_set");
                builder_->CreateBr(mergeBlock);
                
                // 栈数组路径: GEP + store
                builder_->SetInsertPoint(arrBlock);
                llvm::Value* basePtr = builder_->CreateIntToPtr(arrayVal, llvm::PointerType::getUnqual(*context_), "arrbase_w");
                llvm::Value* elemPtr = builder_->CreateGEP(i64Ty, basePtr, indexVal, "elem_w");
                builder_->CreateStore(rightVal, elemPtr);
                builder_->CreateBr(mergeBlock);
                
                // 合并：返回右值
                builder_->SetInsertPoint(mergeBlock);
                return rightVal;
            }
            // 表属性赋值: p.x = val
            if (auto leftMember = std::dynamic_pointer_cast<MemberExpr>(binary->left)) {
                llvm::Value* tableVal = generateExpression(func, leftMember->object);
                llvm::Value* keyVal = registerStringConstant(leftMember->member);
                llvm::Value* rightVal = generateExpression(func, binary->right);
                llvm::Type* i64Ty = llvm::Type::getInt64Ty(*context_);
                llvm::Function* setFunc = module_->getFunction("jit_table_set");
                if (!setFunc) {
                    llvm::FunctionType* ft = llvm::FunctionType::get(i64Ty, {i64Ty, i64Ty, i64Ty}, false);
                    setFunc = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "jit_table_set", module_.get());
                }
                builder_->CreateCall(setFunc, {tableVal, keyVal, rightVal}, "tbl_set");
                return rightVal;
            }
        }
        
        // 非赋值运算符：生成左右操作数
        llvm::Value* left = generateExpression(func, binary->left);
        llvm::Value* right = generateExpression(func, binary->right);
        
        // 运行时字符串类型检测（OP_PLUS 可能表示字符串拼接）
        // 用 NaN-boxing 编码检测：hi 16 bits = 0xFFFF + bit 47 = 0（对象指针）
        // 纯数学模式下：跳过 NaN-boxing 分派，直接生成原生 i64 算术指令
        if (binary->op == TokenType::OP_PLUS && !pureMath_) {
            auto buildIsString = [&](llvm::Value* v) -> llvm::Value* {
                llvm::Value* hi = builder_->CreateLShr(v, 48, "hi48");
                llvm::Value* tagged = builder_->CreateICmpEQ(hi,
                    llvm::ConstantInt::get(*context_, llvm::APInt(64, 0xFFFF)), "istag");
                llvm::Value* immBit = builder_->CreateAnd(v,
                    llvm::ConstantInt::get(*context_, llvm::APInt(64, 0x0000800000000000ULL)), "immbit");
                llvm::Value* isPtr = builder_->CreateICmpEQ(immBit,
                    llvm::ConstantInt::get(*context_, llvm::APInt(64, 0)), "isptr");
                return builder_->CreateAnd(tagged, isPtr, "isstr");
            };
            
            llvm::Value* leftIsStr = buildIsString(left);
            llvm::Value* rightIsStr = buildIsString(right);
            llvm::Value* anyStr = builder_->CreateOr(leftIsStr, rightIsStr, "anystr");
            
            llvm::Function* curFunc = builder_->GetInsertBlock()->getParent();
            llvm::BasicBlock* concatBB = llvm::BasicBlock::Create(*context_, "strconcat", curFunc);
            llvm::BasicBlock* arithBB = llvm::BasicBlock::Create(*context_, "arithplus", curFunc);
            llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(*context_, "plusmerge", curFunc);
            
            builder_->CreateCondBr(anyStr, concatBB, arithBB);
            
            // 字符串拼接路径：调用 jit_strcat（已被 ORC JIT 注册的运行时函数）
            builder_->SetInsertPoint(concatBB);
            llvm::Function* strcatFunc = module_->getFunction("jit_strcat");
            if (!strcatFunc) {
                llvm::Type* i64Ty = llvm::Type::getInt64Ty(*context_);
                llvm::FunctionType* st = llvm::FunctionType::get(i64Ty, {i64Ty, i64Ty}, false);
                strcatFunc = llvm::Function::Create(st, llvm::Function::ExternalLinkage,
                    "jit_strcat", module_.get());
            }
            llvm::Value* concatRes = builder_->CreateCall(strcatFunc, {left, right}, "strcat");
            builder_->CreateBr(mergeBB);
            
            // 算术路径：整数或浮点数加法
            builder_->SetInsertPoint(arithBB);
            llvm::Value* arithRes = nullptr;
            bool useFloat = left->getType()->isDoubleTy() || right->getType()->isDoubleTy();
            if (useFloat) {
                arithRes = builder_->CreateFAdd(toDouble(left), toDouble(right), "fadd");
            } else {
                arithRes = builder_->CreateAdd(left, right, "add");
            }
            builder_->CreateBr(mergeBB);
            
            // 通过 PHI 节点合并两条路径的结果
            builder_->SetInsertPoint(mergeBB);
            llvm::PHINode* phi = builder_->CreatePHI(llvm::Type::getInt64Ty(*context_), 2, "plusres");
            phi->addIncoming(concatRes, concatBB);
            phi->addIncoming(arithRes, arithBB);
            return phi;
        }
        
        // 算术运算
        bool useFloat = left->getType()->isDoubleTy() || right->getType()->isDoubleTy();
        
        if (useFloat) {
            left = toDouble(left);
            right = toDouble(right);
            switch (binary->op) {
                case TokenType::OP_PLUS:  return builder_->CreateFAdd(left, right, "fadd");
                case TokenType::OP_MINUS: return builder_->CreateFSub(left, right, "fsub");
                case TokenType::OP_MUL:   return builder_->CreateFMul(left, right, "fmul");
                case TokenType::OP_DIV:   return builder_->CreateFDiv(left, right, "fdiv");
                default: break;
            }
        } else {
            switch (binary->op) {
                case TokenType::OP_PLUS:  return builder_->CreateAdd(left, right, "add");
                case TokenType::OP_MINUS: return builder_->CreateSub(left, right, "sub");
                case TokenType::OP_MUL:   return builder_->CreateMul(left, right, "mul");
                case TokenType::OP_DIV:   return builder_->CreateSDiv(left, right, "div");
                case TokenType::OP_MOD:   return builder_->CreateSRem(left, right, "rem");
                case TokenType::OP_EQ: {
                    llvm::Value* cmp = builder_->CreateICmpEQ(left, right, "cmpeq");
                    return builder_->CreateZExt(cmp, llvm::Type::getInt64Ty(*context_), "zext");
                }
                case TokenType::OP_NE: {
                    llvm::Value* cmp = builder_->CreateICmpNE(left, right, "cmpne");
                    return builder_->CreateZExt(cmp, llvm::Type::getInt64Ty(*context_), "zext");
                }
                case TokenType::OP_LT: {
                    llvm::Value* cmp = builder_->CreateICmpSLT(left, right, "cmplt");
                    return builder_->CreateZExt(cmp, llvm::Type::getInt64Ty(*context_), "zext");
                }
                case TokenType::OP_GT: {
                    llvm::Value* cmp = builder_->CreateICmpSGT(left, right, "cmpgt");
                    return builder_->CreateZExt(cmp, llvm::Type::getInt64Ty(*context_), "zext");
                }
                case TokenType::OP_LE: {
                    llvm::Value* cmp = builder_->CreateICmpSLE(left, right, "cmple");
                    return builder_->CreateZExt(cmp, llvm::Type::getInt64Ty(*context_), "zext");
                }
                case TokenType::OP_GE: {
                    llvm::Value* cmp = builder_->CreateICmpSGE(left, right, "cmpge");
                    return builder_->CreateZExt(cmp, llvm::Type::getInt64Ty(*context_), "zext");
                }
                case TokenType::OP_AND: return builder_->CreateAnd(left, right, "and");
                case TokenType::OP_OR:  return builder_->CreateOr(left, right, "or");
                default: break;
            }
        }
    }
    
    // 一元表达式
    if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        llvm::Value* operand = generateExpression(func, unary->operand);
        switch (unary->op) {
            case TokenType::OP_MINUS:
                if (operand->getType()->isDoubleTy()) {
                    return builder_->CreateFNeg(operand, "fneg");
                } else {
                    return builder_->CreateNeg(operand, "neg");
                }
            case TokenType::OP_NOT: {
                llvm::Value* cmp = builder_->CreateICmpEQ(operand, llvm::ConstantInt::get(*context_, llvm::APInt(64, 0)), "not");
                return builder_->CreateZExt(cmp, llvm::Type::getInt64Ty(*context_), "zext");
            }
            default: break;
        }
    }
    
    // 函数调用
    if (auto call = std::dynamic_pointer_cast<CallExpr>(expr)) {
        // 获取函数名
        std::string funcName;
        if (auto calleeIdent = std::dynamic_pointer_cast<IdentifierExpr>(call->callee)) {
            funcName = calleeIdent->name;
        } else {
        }
        
        if (funcName.empty()) {
            return llvm::ConstantInt::get(*context_, llvm::APInt(64, 0));
        }
        
        // 检查是否是原生函数
        if (funcName == "打印" || funcName == "print" || funcName == "println") {
            // 纯数学模式下：跳过 print（无运行时支持）
            if (pureMath_) {
                std::cerr << "[LLVMCodegen] 警告: pure-math 模式下跳过 print() 调用（需链接运行时库支持）\n";
                return llvm::ConstantInt::get(*context_, llvm::APInt(64, 0));
            }
            // 调用 jit_printv（已在 ORC JIT 中注册的可解析符号）
            int argCount = (int)call->arguments.size();

            // 生成参数
            std::vector<llvm::Value*> argValues;
            for (auto& arg : call->arguments) {
                argValues.push_back(generateExpression(func, arg));
            }

            // 在栈上分配参数数组: [n x i64]
            llvm::Type* i64Ty = llvm::Type::getInt64Ty(*context_);
            llvm::Type* i32Ty = llvm::Type::getInt32Ty(*context_);
            llvm::ArrayType* arrTy = llvm::ArrayType::get(i64Ty, argCount);
            llvm::Value* argsAlloca = builder_->CreateAlloca(arrTy, nullptr, "print_args");

            // 存储每个参数到数组中
            llvm::Value* zero = llvm::ConstantInt::get(i32Ty, 0);
            for (int i = 0; i < argCount; i++) {
                llvm::Value* idx = llvm::ConstantInt::get(i32Ty, i);
                llvm::Value* gep = builder_->CreateGEP(arrTy, argsAlloca, {zero, idx}, "arg_ptr");
                builder_->CreateStore(argValues[i], gep);
            }

            // 获取数组基址指针
            llvm::Value* basePtr = builder_->CreateGEP(arrTy, argsAlloca, {zero, zero}, "args_base");

            // 调用 jit_printv
            llvm::Function* printvFunc = module_->getFunction("jit_printv");
            if (!printvFunc) {
                llvm::FunctionType* printvTy = llvm::FunctionType::get(
                    llvm::Type::getVoidTy(*context_), {i32Ty, llvm::PointerType::getUnqual(i64Ty)}, false);
                printvFunc = llvm::Function::Create(printvTy, llvm::Function::ExternalLinkage,
                    "jit_printv", module_.get());
            }
            builder_->CreateCall(printvFunc, {llvm::ConstantInt::get(i32Ty, argCount), basePtr});
            return llvm::ConstantInt::get(*context_, llvm::APInt(64, 0));
        }

        // tick() / tock() → jit_tick（已在 ORC JIT 中注册的可解析符号）
        if (funcName == "tick" || funcName == "tock") {
            if (pureMath_) {
                return llvm::ConstantInt::get(*context_, llvm::APInt(64, 0));
            }
            llvm::Function* tickFunc = module_->getFunction("jit_tick");
            if (!tickFunc) {
                llvm::FunctionType* tickTy = llvm::FunctionType::get(
                    llvm::Type::getInt64Ty(*context_), {}, false);
                tickFunc = llvm::Function::Create(tickTy, llvm::Function::ExternalLinkage,
                    "jit_tick", module_.get());
            }
            return builder_->CreateCall(tickFunc, {}, "tick");
        }
        
// 普通函数调用
        std::string safeFuncName = sanitizeName(funcName);
        
        // 纯数学模式下：跳过 print/打印 的普通函数调用（防御性检查，防止 BOM 导致的手写检查遗漏）
        if (pureMath_ && (safeFuncName.find("__u6253____u5370__") != std::string::npos || 
                          safeFuncName.find("print") != std::string::npos)) {
            std::cerr << "[LLVMCodegen] 警告: pure-math 模式下跳过 " << funcName << "() 调用（fallback 检测）";
            return llvm::ConstantInt::get(*context_, llvm::APInt(64, 0));
        }
        
        // 已知 standalone 函数（直接在 jit_runtime / aot_vm_bridge 中实现）：直连不经过桥接
        static const char* standaloneFuncs[] = {
            "len", "toString", "min", "max", "push", "pop", "insert",
            "remove", "substr", "find", "lower", "upper", "concat",
            "arrlen", "startsWith", "endsWith", "trim", "replace",
            "slice", "jit_abs", "clamp",
            "jit_strcat", "jit_printv", "jit_tick",
            "jit_table_create", "jit_table_get", "jit_table_set",
            "jit_len", "jit_toString", // JIT 原生实现
        };
        bool isStandalone = false;
        if (!pureMath_) {
            for (auto& fn : standaloneFuncs) {
                if (safeFuncName == fn) { isStandalone = true; break; }
            }
            // 中文函数名映射到 JIT 运行时独立函数
            if (!isStandalone) {
                if (safeFuncName == "__u957F____u5EA6__") {       // 长度 → jit_len
                    isStandalone = true; safeFuncName = "jit_len";
                } else if (safeFuncName == "__u8F6C____u5B57____u7B26____u4E32__") { // 转字符串 → jit_toString
                    isStandalone = true; safeFuncName = "jit_toString";
                }
            }
        }
        
        // 对不在 LLVM module 中的函数：通过桥接调用
        bool useBridge = (!pureMath_ && !isStandalone);
        llvm::Function* calleeFunc = module_->getFunction(safeFuncName);
        
        if (!calleeFunc && useBridge) {
            llvm::Type* i64Ty = llvm::Type::getInt64Ty(*context_);
            llvm::Type* i32Ty = llvm::Type::getInt32Ty(*context_);
            llvm::Type* i8PtrTy = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(*context_));
            
            // 创建函数名字符串常量
            llvm::Constant* nameStr = llvm::ConstantDataArray::getString(*context_, funcName);
            llvm::GlobalVariable* nameGlobal = new llvm::GlobalVariable(
                *module_, nameStr->getType(), true,
                llvm::GlobalValue::InternalLinkage, nameStr, ".native_name");
            llvm::Value* zero = llvm::ConstantInt::get(i32Ty, 0);
            llvm::Value* namePtr = builder_->CreateInBoundsGEP(nameStr->getType(), nameGlobal,
                {zero, zero}, "native_name_ptr");
            
            // 生成参数数组
            int argCount = (int)call->arguments.size();
            llvm::ArrayType* arrTy = llvm::ArrayType::get(i64Ty, argCount ? (unsigned)argCount : 1);
            llvm::Value* argsAlloca = builder_->CreateAlloca(arrTy, nullptr, "bridge_args");
            for (int i = 0; i < argCount; i++) {
                llvm::Value* argVal = generateExpression(func, call->arguments[i]);
                llvm::Value* gep = builder_->CreateInBoundsGEP(arrTy, argsAlloca,
                    {zero, llvm::ConstantInt::get(i32Ty, i)}, "bridge_arg");
                builder_->CreateStore(argVal, gep);
            }
            
            // 根据模式选择桥接函数：JIT 用 jit_call_native，AOT 用 aot_call_native
            const char* bridgeName = skipNativeCallRemoval_ ? "aot_call_native" : "jit_call_native";
            llvm::Function* bridgeFunc = module_->getFunction(bridgeName);
            if (!bridgeFunc) {
                llvm::FunctionType* ft = llvm::FunctionType::get(i64Ty,
                    {i8PtrTy, i32Ty, llvm::PointerType::getUnqual(i64Ty)}, false);
                bridgeFunc = llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                    bridgeName, module_.get());
            }
            
            llvm::Value* argsBase = builder_->CreateInBoundsGEP(arrTy, argsAlloca,
                {zero, zero}, "bridge_args_base");
            return builder_->CreateCall(bridgeFunc,
                {namePtr, llvm::ConstantInt::get(i32Ty, argCount), argsBase}, "bridge");
        }
        
        if (!calleeFunc) {
            // 函数可能还未定义，先创建一个声明
            std::vector<llvm::Type*> paramTypes;
            for (auto& arg : call->arguments) {
                paramTypes.push_back(llvm::Type::getInt64Ty(*context_));
            }
            llvm::FunctionType* funcType = llvm::FunctionType::get(llvm::Type::getInt64Ty(*context_), paramTypes, false);
            calleeFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, safeFuncName, module_.get());
        }
        
        // 生成参数
        std::vector<llvm::Value*> args;
        for (auto& arg : call->arguments) {
            args.push_back(generateExpression(func, arg));
        }
        
        llvm::Value* result = builder_->CreateCall(calleeFunc, args, "call");
        return result;
    }
    
    // 数组索引
    if (auto indexExpr = std::dynamic_pointer_cast<IndexExpr>(expr)) {
        llvm::Value* arrayVal = generateExpression(func, indexExpr->array);
        llvm::Value* indexVal = generateExpression(func, indexExpr->index);
        llvm::Type* i64Ty = llvm::Type::getInt64Ty(*context_);
        
        // 运行时派发：表（nan-boxed hi16=0xFFFF）走 jit_table_get，栈数组走 GEP
        llvm::Value* hi48 = builder_->CreateLShr(arrayVal, 48, "hi48_r");
        llvm::Value* isTable = builder_->CreateICmpEQ(hi48, llvm::ConstantInt::get(i64Ty, 0xFFFF), "istable_r");
        llvm::Function* currFn = builder_->GetInsertBlock()->getParent();
        llvm::BasicBlock* tblBlock = llvm::BasicBlock::Create(*context_, "tbl_read", currFn);
        llvm::BasicBlock* arrBlock = llvm::BasicBlock::Create(*context_, "arr_read", currFn);
        llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(*context_, "idx_read_end", currFn);
        builder_->CreateCondBr(isTable, tblBlock, arrBlock);
        
        // 表路径: jit_table_get
        builder_->SetInsertPoint(tblBlock);
        llvm::Function* getFunc = module_->getFunction("jit_table_get");
        if (!getFunc) {
            llvm::FunctionType* ft = llvm::FunctionType::get(i64Ty, {i64Ty, i64Ty}, false);
            getFunc = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "jit_table_get", module_.get());
        }
        llvm::Value* tblResult = builder_->CreateCall(getFunc, {arrayVal, indexVal}, "tbl_get");
        builder_->CreateBr(mergeBlock);
        
        // 栈数组路径: GEP + load
        builder_->SetInsertPoint(arrBlock);
        llvm::Value* basePtr = builder_->CreateIntToPtr(arrayVal, llvm::PointerType::getUnqual(*context_), "arrbase");
        llvm::Value* elementPtr = builder_->CreateGEP(i64Ty, basePtr, indexVal, "arrayidx");
        llvm::Value* arrResult = builder_->CreateLoad(i64Ty, elementPtr, "arrayload");
        builder_->CreateBr(mergeBlock);
        
        // 合并: phi 选择结果
        builder_->SetInsertPoint(mergeBlock);
        llvm::PHINode* phi = builder_->CreatePHI(i64Ty, 2, "idx_result");
        phi->addIncoming(tblResult, tblBlock);
        phi->addIncoming(arrResult, arrBlock);
        return phi;
    }
    
    // 成员访问（通过表运行时函数）
    if (auto memberExpr = std::dynamic_pointer_cast<MemberExpr>(expr)) {
        llvm::Type* i64Ty = llvm::Type::getInt64Ty(*context_);
        
        // 编译期折叠：.length 属性
        Shared<Expr> objExpr = memberExpr->object;
        if (memberExpr->member == "length") {
            // 直接数组字面量: [1,2,3].length → 3
            auto arrLit = std::dynamic_pointer_cast<ArrayExpr>(objExpr);
            if (arrLit) {
                return llvm::ConstantInt::get(i64Ty, (uint64_t)arrLit->elements.size());
            }
            // 直接字符串字面量: "hello".length → 5
            auto strLit = std::dynamic_pointer_cast<LiteralExpr>(objExpr);
            if (strLit) {
                if (auto* s = std::get_if<String>(&strLit->value)) {
                    return llvm::ConstantInt::get(i64Ty, (uint64_t)s->length());
                }
            }
            // 变量引用: arr.length → 查 literalLenByVar_
            auto ident = std::dynamic_pointer_cast<IdentifierExpr>(objExpr);
            if (ident) {
                auto it = literalLenByVar_.find(ident->name);
                if (it != literalLenByVar_.end()) {
                    return llvm::ConstantInt::get(i64Ty, (uint64_t)it->second);
                }
            }
        }
        
        // 运行时路径：表属性访问
        llvm::Value* tableVal = generateExpression(func, memberExpr->object);
        llvm::Value* keyVal = registerStringConstant(memberExpr->member);
        
        llvm::Function* getFunc = module_->getFunction("jit_table_get");
        if (!getFunc) {
            llvm::FunctionType* ft = llvm::FunctionType::get(i64Ty, {i64Ty, i64Ty}, false);
            getFunc = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "jit_table_get", module_.get());
        }
        return builder_->CreateCall(getFunc, {tableVal, keyVal}, "tbl_get");
    }
    
    // 数组字面量: [e1, e2, ...]
    if (auto arrLit = std::dynamic_pointer_cast<ArrayExpr>(expr)) {
        size_t count = arrLit->elements.size();
        if (count == 0) {
            // 空数组 [] → 使用表运行时函数
            llvm::Type* i64Ty = llvm::Type::getInt64Ty(*context_);
            llvm::Function* createFunc = module_->getFunction("jit_table_create");
            if (!createFunc) {
                llvm::FunctionType* ft = llvm::FunctionType::get(i64Ty, {}, false);
                createFunc = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "jit_table_create", module_.get());
            }
            return builder_->CreateCall(createFunc, {}, "table");
        }
        
        // 非空数组 → 使用表运行时函数（AOT 兼容：返回 NaN-boxed 表指针）
        llvm::Type* i64Ty = llvm::Type::getInt64Ty(*context_);
        llvm::Function* createFunc = module_->getFunction("jit_table_create");
        if (!createFunc) {
            llvm::FunctionType* ft = llvm::FunctionType::get(i64Ty, {}, false);
            createFunc = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "jit_table_create", module_.get());
        }
        llvm::Value* table = builder_->CreateCall(createFunc, {}, "table");
        
        // 声明 jit_table_set(i64 table, i64 key, i64 val) -> i64
        llvm::Function* setFunc = module_->getFunction("jit_table_set");
        if (!setFunc) {
            llvm::FunctionType* ft = llvm::FunctionType::get(i64Ty, {i64Ty, i64Ty, i64Ty}, false);
            setFunc = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "jit_table_set", module_.get());
        }
        
        // 逐个设置元素
        for (size_t i = 0; i < count; i++) {
            llvm::Value* elemVal = generateExpression(func, arrLit->elements[i]);
            llvm::Value* idxVal = llvm::ConstantInt::get(*context_, llvm::APInt(64, (uint64_t)i));
            builder_->CreateCall(setFunc, {table, idxVal, elemVal}, "set");
        }
        
        return table;
    }
    
    // 结构体/表字面量: {}、{a:1, b:2}、Point{x:42, y:10}
    if (auto structLit = std::dynamic_pointer_cast<StructLiteralExpr>(expr)) {
        llvm::Type* i64Ty = llvm::Type::getInt64Ty(*context_);
        
        // 创建表
        llvm::Function* createFunc = module_->getFunction("jit_table_create");
        if (!createFunc) {
            llvm::FunctionType* ft = llvm::FunctionType::get(i64Ty, {}, false);
            createFunc = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "jit_table_create", module_.get());
        }
        llvm::Value* table = builder_->CreateCall(createFunc, {}, "struct");
        
        // 声明 jit_table_set(i64 table, i64 key, i64 val) -> i64
        llvm::Function* setFunc = module_->getFunction("jit_table_set");
        if (!setFunc) {
            llvm::FunctionType* ft = llvm::FunctionType::get(i64Ty, {i64Ty, i64Ty, i64Ty}, false);
            setFunc = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "jit_table_set", module_.get());
        }
        
        // 逐个设置字段
        for (auto& field : structLit->fields) {
            llvm::Value* keyVal = registerStringConstant(field.first);
            llvm::Value* fieldVal = generateExpression(func, field.second);
            builder_->CreateCall(setFunc, {table, keyVal, fieldVal}, "setfield");
        }
        
        return table;
    }
    
    return llvm::ConstantInt::get(*context_, llvm::APInt(64, 0));
}

// === 优化 pass ===

void LLVMCodegen::runOptimizationPasses() {
    if (!module_) return;
    
    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;

    // LLVM 18: PGOOpt passed through PassBuilder constructor
    std::optional<llvm::PGOOptions> pgoOpt;
    if (enablePGOUse_) {
        try {
            pgoOpt = llvm::PGOOptions(pgoProfilePath_, "", "", "", nullptr, llvm::PGOOptions::IRUse);
        } catch (...) {
            std::cerr << "[LLVMCodegen] PGO profile配置失败，将跳过PGO优化\n";
            enablePGOUse_ = false;
        }
    }

    llvm::PassInstrumentationCallbacks PIC;
    llvm::PassBuilder PB(nullptr, llvm::PipelineTuningOptions(), std::move(pgoOpt), &PIC);
    // DebugLogging=false 避免输出冗长的 pass 日志到 stderr
    llvm::StandardInstrumentations SI(*context_, false);
    SI.registerCallbacks(PIC);

    // 注册所有标准分析Pass
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    llvm::ModulePassManager MPM;

    // PGO 插桩模式：生成带profile采集的代码
    if (enablePGOGenerate_) {
        MPM.addPass(llvm::PGOInstrumentationGen());
        MPM.addPass(llvm::InstrProfilingLoweringPass());
    }

    // PGO 优化模式：PGOOpt已在PassBuilder构造函数中设置

    // 根据优化级别配置Pass流水线
    if (optLevel_ == OptLevel::O2) {
        // O2级优化

        // 全局优化
        MPM.addPass(llvm::GlobalOptPass());
        MPM.addPass(llvm::ConstantMergePass());

        // 函数级优化流水线
        llvm::FunctionPassManager FPM;
        FPM.addPass(llvm::InstCombinePass());
        FPM.addPass(llvm::ReassociatePass());
        FPM.addPass(llvm::GVNPass());
        FPM.addPass(llvm::SimplifyCFGPass());
        FPM.addPass(llvm::LoopVectorizePass());
        FPM.addPass(llvm::SLPVectorizerPass());
        FPM.addPass(llvm::PromotePass());
        FPM.addPass(llvm::DCEPass());

        MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM)));
        
        // 内联优化
        MPM.addPass(llvm::ModuleInlinerPass());
    } else if (optLevel_ == OptLevel::O3) {
        // O3级优化（更激进）
        llvm::OptimizationLevel OL = enablePGOUse_ ? llvm::OptimizationLevel::O3 : llvm::OptimizationLevel::O3;
        PB.buildPerModuleDefaultPipeline(OL).run(*module_, MAM);
        return;
    } else if (optLevel_ == OptLevel::O1) {
        // O1级优化
        llvm::OptimizationLevel OL = enablePGOUse_ ? llvm::OptimizationLevel::O1 : llvm::OptimizationLevel::O1;
        PB.buildPerModuleDefaultPipeline(OL).run(*module_, MAM);
        return;
    }

    MPM.run(*module_, MAM);
}

void LLVMCodegen::foldConstants() {
    // 已集成到runOptimizationPasses
}

void LLVMCodegen::eliminateDeadCode() {
    // 已集成到runOptimizationPasses
}

void LLVMCodegen::inlineFunctions() {
    // 已集成到runOptimizationPasses
}

// === 生成 LLVM IR 字符串 ===
std::string LLVMCodegen::generateIRString(Shared<Program> program, OptLevel opt) {
    // 先生成 Module
    llvm::Module* module = generate(program, opt);
    if (!module) {
        return "";
    }
    
    // 转换为字符串
    std::string irStr;
    llvm::raw_string_ostream os(irStr);
    module->print(os, nullptr);
    os.flush();
    
    return irStr;
}

// === 生成单个函数的 LLVM IR 字符串 ===
std::string LLVMCodegen::generateSingleFunctionIRString(Shared<Program> program, const std::string& funcName, OptLevel opt) {
    // 先生成 Module
    llvm::Module* module = generateSingleFunction(program, funcName, opt);
    if (!module) {
        return "";
    }
    
    // 转换为字符串
    std::string irStr;
    llvm::raw_string_ostream os(irStr);
    module->print(os, nullptr);
    os.flush();
    
    return irStr;
}

} // namespace cplang
#endif