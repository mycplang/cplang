// JIT 编译器实现 - 外部进程编译模式
// 使用 LLVM 工具链（llc + clang）进行 AOT 编译

#include "jit/jit_compiler.hpp"
#include "core/verbose.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "codegen/codegen.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>
#include <filesystem>

// Windows 特定头文件 — 必须在 namespace 外，避免类型命名空间污染
#ifdef _WIN32
#include <windows.h>
#define dllexport __declspec(dllexport)
#else
#include <dlfcn.h>
#endif

namespace cplang {

namespace fs = std::filesystem;

using namespace std::chrono;

// ═══════════════════════════════════════════════════════════════════
//  工具函数
// ═══════════════════════════════════════════════════════════════════

static std::string execCmd(const char* cmd) {
    char buffer[128];
    std::string result;
    FILE* pipe = _popen(cmd, "r");
    if (!pipe) return "";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    _pclose(pipe);
    return result;
}

static bool fileExists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

static bool writeFile(const std::string& path, const std::string& content) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f << content;
    return true;
}

[[maybe_unused]] static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) return "";
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static std::string getEnv(const char* name) {
    char* val = std::getenv(name);
    return val ? val : "";
}

static int64_t currentTimeMs() {
    return duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()
    ).count();
}

// ═══════════════════════════════════════════════════════════════════
//  JITCompiler 实现
// ═══════════════════════════════════════════════════════════════════

JITCompiler::JITCompiler()
    : hotThreshold_(100), enableProfiling_(true), optLevel_(3) {
    llvmCodegen_ = std::make_unique<LLVMCodegen>();
    tempDir_ = getTempDir();
}

JITCompiler::~JITCompiler() {
    // 清理临时文件
    if (!tempDir_.empty() && fs::exists(tempDir_)) {
        try {
            fs::remove_all(tempDir_);
        } catch (...) {}
    }
}

bool JITCompiler::initialize() {
    if (initialized_) return isAvailable_;

    VERBOSE(std::cout << "[JIT] 初始化 JIT 编译器...\n");

    // 检查 LLVM 工具链
    isAvailable_ = checkLLVM();

    if (isAvailable_) {
        VERBOSE(std::cout << "[JIT] LLVM 工具链就绪\n");
        VERBOSE(std::cout << "[JIT] 优化级别: O" << optLevel_ << "\n");
        VERBOSE(std::cout << "[JIT] 热点阈值: " << hotThreshold_ << " 次\n");
    } else {
        VERBOSE(std::cout << "[JIT] LLVM 未安装，JIT 将在 LLVM 可用后启用\n");
        VERBOSE(std::cout << "[JIT] 提示: 运行 compile_optimized.bat 会自动使用 LLVM\n");
    }

    initialized_ = true;
    return isAvailable_;
}

bool JITCompiler::checkLLVM() {
    // 查找 LLVM 安装目录
    if (llvmDir_.empty()) {
        // 检查 PATH
        llvmDir_ = getEnv("LLVM_DIR");
        if (llvmDir_.empty()) {
            llvmDir_ = getEnv("LLVM_PATH");
        }
        if (llvmDir_.empty()) {
            // 检查常见安装位置
            const char* commonPaths[] = {
                "C:/Program Files/LLVM",
                "C:/Program Files (x86)/LLVM",
                "C:/LLVM",
            };
            for (const auto& p : commonPaths) {
                std::string binPath = std::string(p) + "/bin/clang.exe";
                if (fileExists(binPath)) {
                    llvmDir_ = p;
                    break;
                }
            }
        }
    }

    if (llvmDir_.empty()) {
        // 检查 PATH 中的 clang
        std::string result = execCmd("where clang 2>nul");
        if (!result.empty()) {
            // 提取目录
            size_t pos = result.find('\n');
            if (pos != std::string::npos) result = result.substr(0, pos);
            size_t lastSlash = result.find_last_of("/\\");
            if (lastSlash != std::string::npos) {
                llvmDir_ = result.substr(0, lastSlash);
                // 去掉 "/bin" 后缀
                size_t binPos = llvmDir_.rfind("/bin");
                if (binPos != std::string::npos) {
                    llvmDir_ = llvmDir_.substr(0, binPos);
                }
            }
        }
    }

    if (llvmDir_.empty()) {
        VERBOSE(std::cout << "[JIT] 警告: 无法找到 LLVM\n");
        return false;
    }

    // 检查必需工具 (clang 同时用于编译 IR 和链接)
    std::string clangPath = llvmDir_ + "/bin/clang.exe";

    if (!fileExists(clangPath)) {
        VERBOSE(std::cout << "[JIT] 警告: clang.exe 未找到 (" << clangPath << ")\n");
        return false;
    }

    // 获取版本
    std::string version = execCmd(("\"" + clangPath + "\" --version 2>nul").c_str());
    if (!version.empty()) {
        size_t pos = version.find("version");
        if (pos != std::string::npos) {
            std::string ver = version.substr(pos, 50);
            VERBOSE(std::cout << "[JIT] LLVM (clang): " << ver << "\n");
        }
    }

    return true;
}

bool JITCompiler::shouldCompile(VMFunction* func) const {
    if (!func || !isAvailable_) return false;

    int calls = getCallCount(func);
    return calls >= hotThreshold_ && !isCompiled(func);
}

void* JITCompiler::compile(VMFunction* func) {
    if (!func || !isAvailable_) return nullptr;

    // 检查是否已编译
    if (isCompiled(func)) {
        auto it = compiledFunctions_.find(func);
        if (it != compiledFunctions_.end()) {
            return it->second.machineCode;
        }
    }

    // 检查是否达到热点阈值
    if (!shouldCompile(func)) {
        return nullptr;
    }

    VERBOSE(std::cout << "[JIT] 编译热点函数: " << getFuncName(func)
              << " (调用次数: " << getCallCount(func) << ")\n");

    stats_.totalCompileRequests++;

    // 记录热点函数
    hotFunctions_.insert(func);

    // 生成 LLVM IR
    int64_t startTime = currentTimeMs();

    String ir = generateLLVMIR(func);
    if (ir.empty()) {
        std::cout << "[JIT] 错误: LLVM IR 生成失败\n";
        stats_.failedCompiles++;
        return nullptr;
    }

    // 保存 IR 到临时文件
    std::string funcName = getFuncName(func);
    std::string irFile = tempDir_ + "/" + funcName + ".ll";
    std::string objFile = tempDir_ + "/" + funcName + ".obj";
    std::string dllFile = tempDir_ + "/" + funcName + ".dll";

    if (!writeFile(irFile, ir)) {
        std::cout << "[JIT] 错误: 无法写入 IR 文件\n";
        stats_.failedCompiles++;
        return nullptr;
    }

    // 编译到目标文件
    if (!compileToObjectFile(irFile, objFile)) {
        std::cout << "[JIT] 错误: 编译到目标文件失败\n";
        stats_.failedCompiles++;
        return nullptr;
    }

    // 编译到 DLL (使用 lld-link)
    std::string linkCmd = "\"" + llvmDir_ + "/bin/lld-link.exe\" \"" + objFile +
        "\" /DLL /OUT:\"" + dllFile + "\" /SUBSYSTEM:WINDOWS /NOENTRY";
    std::string linkResult = execCmd(linkCmd.c_str());

    if (!fileExists(dllFile)) {
        std::cout << "[JIT] 错误: 链接失败\n";
        stats_.failedCompiles++;
        return nullptr;
    }

    // 加载 DLL
    void* dll = LoadLibraryA(dllFile.c_str());
    if (!dll) {
        std::cout << "[JIT] 错误: 无法加载 DLL: " << dllFile << "\n";
        stats_.failedCompiles++;
        return nullptr;
    }

    // 获取函数指针
    void* funcPtr = (void*)GetProcAddress((HMODULE)dll, funcName.c_str());
    if (!funcPtr) {
        std::cout << "[JIT] 错误: 无法获取函数地址\n";
        FreeLibrary((HMODULE)dll);
        stats_.failedCompiles++;
        return nullptr;
    }

    // 保存编译结果
    CompiledFunction cf;
    cf.machineCode = funcPtr;
    cf.llFile = irFile;
    cf.objFile = objFile;
    cf.compileTimeMs = static_cast<double>(currentTimeMs() - startTime);
    cf.isHot = true;
    compiledFunctions_[func] = cf;

    stats_.successfulCompiles++;
    stats_.totalCompileTimeMs += cf.compileTimeMs;
    stats_.avgCompileTimeMs = stats_.totalCompileTimeMs / stats_.successfulCompiles;

    VERBOSE(std::cout << "[JIT] 编译成功: " << funcName
              << " (耗时: " << std::fixed << std::setprecision(2)
              << cf.compileTimeMs << "ms)\n");

    return funcPtr;
}

String JITCompiler::generateLLVMIR(VMFunction* func) {
    // 从字节码函数提取 AST 程序
    Shared<Program> program = extractProgram(func);
    if (!program) {
        std::cerr << "[JIT] 错误: 无法从函数提取程序\n";
        return "";
    }

    // 生成 LLVM IR
    OptLevel opt = static_cast<OptLevel>(optLevel_);
    llvmCodegen_->setOptLevel(opt);
    String ir = llvmCodegen_->generateIRString(program, opt);

    return ir;
}

bool JITCompiler::compileToObjectFile(const String& irFile, const String& objFile) {
    std::string clangPath = llvmDir_ + "/bin/clang.exe";

    // 使用 clang 编译 LLVM IR -> 目标文件
    // -c: 只编译不链接
    // -O3: 最高优化
    std::string cmd = "\"" + clangPath + "\" -c \"" + irFile + "\""
        + " -o \"" + objFile + "\""
        + " -O" + std::to_string(optLevel_);

    std::string result = execCmd(cmd.c_str());

    if (!fileExists(objFile)) {
        std::cerr << "[JIT] clang 编译失败: " << result << "\n";
        return false;
    }

    return true;
}

Shared<Program> JITCompiler::extractProgram(VMFunction* /*func*/) {
    // 从字节码函数重建 AST
    // 由于字节码丢失了 AST 信息，这里返回空
    // 实际使用时应该直接从编译器获取 AST
    // TODO: 在 Compiler 中保存编译时的 AST
    return nullptr;
}

String JITCompiler::getTempDir() {
    std::string temp = getEnv("TEMP");
    if (temp.empty()) temp = getEnv("TMP");
    if (temp.empty()) temp = "C:/Temp";
    temp += "/cplang_jit_" + std::to_string(getCurrentTimeMs());

    // 创建目录
    try {
        fs::create_directories(temp);
    } catch (...) {
        return "";
    }

    return temp;
}

String JITCompiler::getFuncName(VMFunction* func) {
    if (func && func->name) {
        return func->name->data;
    }
    return "anonymous_func";
}

int64_t JITCompiler::getCurrentTimeMs() {
    return currentTimeMs();
}

// ═══════════════════════════════════════════════════════════════════
//  公共接口实现
// ═══════════════════════════════════════════════════════════════════

void* JITCompiler::compileFromAST(Shared<Program> program, const String& funcName) {
    if (!program || !isAvailable_) return nullptr;

    stats_.totalCompileRequests++;

    int64_t startTime = currentTimeMs();

    // 生成 LLVM IR
    OptLevel opt = static_cast<OptLevel>(optLevel_);
    llvmCodegen_->setOptLevel(opt);
    String ir = llvmCodegen_->generateIRString(program, opt);

    if (ir.empty()) {
        std::cout << "[JIT] 错误: LLVM IR 生成失败\n";
        stats_.failedCompiles++;
        return nullptr;
    }

    // 保存 IR 到临时文件
    std::string irFile = tempDir_ + "/" + funcName + ".ll";
    std::string objFile = tempDir_ + "/" + funcName + ".obj";
    std::string dllFile = tempDir_ + "/" + funcName + ".dll";

    if (!writeFile(irFile, ir)) {
        std::cout << "[JIT] 错误: 无法写入 IR 文件\n";
        stats_.failedCompiles++;
        return nullptr;
    }

    // 编译到目标文件
    if (!compileToObjectFile(irFile, objFile)) {
        std::cout << "[JIT] 错误: 编译到目标文件失败\n";
        stats_.failedCompiles++;
        return nullptr;
    }

    // 编译到 DLL (使用 lld-link)
    std::string linkCmd = "\"" + llvmDir_ + "/bin/lld-link.exe\" \"" + objFile + "\""
        + " /DLL /OUT:\"" + dllFile + "\" /SUBSYSTEM:WINDOWS /NOENTRY";
    execCmd(linkCmd.c_str());

    if (!fileExists(dllFile)) {
        std::cout << "[JIT] 错误: 链接失败\n";
        stats_.failedCompiles++;
        return nullptr;
    }

    // 加载 DLL
    void* dll = LoadLibraryA(dllFile.c_str());
    if (!dll) {
        std::cout << "[JIT] 错误: 无法加载 DLL\n";
        stats_.failedCompiles++;
        return nullptr;
    }

    // 获取函数指针
    void* funcPtr = (void*)GetProcAddress((HMODULE)dll, funcName.c_str());
    if (!funcPtr) {
        std::cout << "[JIT] 错误: 无法获取函数地址\n";
        FreeLibrary((HMODULE)dll);
        stats_.failedCompiles++;
        return nullptr;
    }

    stats_.successfulCompiles++;
    stats_.totalCompileTimeMs += currentTimeMs() - startTime;

    return funcPtr;
}

void JITCompiler::setHotThreshold(int threshold) {
    hotThreshold_ = threshold > 0 ? threshold : 1;
}

void JITCompiler::setEnableProfiling(bool enable) {
    enableProfiling_ = enable;
}

void JITCompiler::setOptLevel(int level) {
    optLevel_ = (level >= 0 && level <= 3) ? level : 2;
}

void JITCompiler::setLLVMDir(const String& path) {
    llvmDir_ = path;
    if (initialized_) {
        isAvailable_ = checkLLVM();
    }
}

void JITCompiler::recordCall(VMFunction* func) {
    if (!func || !enableProfiling_) return;
    callCounts_[func]++;

    // 检查是否达到热点阈值
    if (callCounts_[func] == hotThreshold_) {
        VERBOSE(std::cout << "[JIT] 函数 " << getFuncName(func)
                  << " 达到热点阈值 (" << hotThreshold_ << ")\n");
    }
}

int JITCompiler::getCallCount(VMFunction* func) const {
    if (!func) return 0;
    auto it = callCounts_.find(func);
    return it != callCounts_.end() ? it->second : 0;
}

bool JITCompiler::isCompiled(VMFunction* func) const {
    if (!func) return false;
    return compiledFunctions_.find(func) != compiledFunctions_.end();
}

void JITCompiler::clearCache() {
    // 释放所有编译的函数
    for ([[maybe_unused]] auto& kv : compiledFunctions_) {
        // 注意：这里只是清空记录，不释放实际的 DLL
        // 因为 DLL 可能被其他代码引用
    }
    compiledFunctions_.clear();
    callCounts_.clear();
    hotFunctions_.clear();

    // 重置统计
    stats_ = Stats();

    VERBOSE(std::cout << "[JIT] 缓存已清空\n");
}

void JITCompiler::dumpStats() const {
    if (!cplang::verboseEnabled()) return;
    std::cout << "\n╔══════════════════════════════════════════════╗\n";
    std::cout << "║           JIT 编译统计                      ║\n";
    std::cout << "╠══════════════════════════════════════════════╣\n";
    std::cout << "║  LLVM 可用:    " << (isAvailable_ ? "是" : "否") << "                         ║\n";
    std::cout << "║  热点阈值:     " << hotThreshold_ << " 次                      ║\n";
    std::cout << "║  优化级别:     O" << optLevel_ << "                            ║\n";
    std::cout << "╠══════════════════════════════════════════════╣\n";
    std::cout << "║  编译请求:     " << stats_.totalCompileRequests << "                        ║\n";
    std::cout << "║  成功编译:     " << stats_.successfulCompiles << "                        ║\n";
    std::cout << "║  失败编译:     " << stats_.failedCompiles << "                        ║\n";
    std::cout << "║  已缓存函数:   " << compiledFunctions_.size() << "                        ║\n";
    std::cout << "║  热点函数:     " << hotFunctions_.size() << "                        ║\n";
    if (stats_.successfulCompiles > 0) {
        std::cout << "║  平均编译耗时: " << std::fixed << std::setprecision(2)
                  << stats_.avgCompileTimeMs << " ms                ║\n";
    }
    std::cout << "╚══════════════════════════════════════════════╝\n\n";
}

} // namespace cplang