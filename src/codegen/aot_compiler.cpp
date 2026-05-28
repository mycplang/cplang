// CP语言 AOT 编译器实现
#include "codegen/aot_compiler.hpp"
#include "codegen/llvm_codegen.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "core/verbose.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace cplang {

AOTCompiler::AOTCompiler() {}
AOTCompiler::~AOTCompiler() {
    cleanupTempFiles();
}

AOTResult AOTCompiler::compileFile(const String& filename, const AOTConfig& config) {
    AOTResult result;
    
    // 读取源文件
    std::ifstream file(filename);
    if (!file.is_open()) {
        result.errorMessage = "无法打开文件: " + filename;
        return result;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    String source = buffer.str();
    
    return compileSource(source, config);
}

AOTResult AOTCompiler::compileSource(const String& source, const AOTConfig& config) {
    // 词法分析 + 语法分析（合并：Parser 从 Lexer 逐个读取 token）
    Lexer lexer(source);
    Parser parser(&lexer);
    auto ast = parser.parse();
    if (parser.hasError()) {
        AOTResult result;
        result.errorMessage = "语法分析错误: " + parser.errorMessage();
        return result;
    }
    
    return compileAST(std::move(ast), config);
}

AOTResult AOTCompiler::compileAST(Shared<Program> ast, const AOTConfig& config) {
    AOTResult result;
    
    // 语义分析
    SemanticAnalyzer analyzer;
    if (!analyzer.analyze(ast)) {
        result.errorMessage = "语义分析错误: " + analyzer.errorMessage();
        return result;
    }
    
    // 生成 LLVM IR
    String ir;
    if (!generateLLVMIR(ast, ir, config)) {
        result.errorMessage = "LLVM IR 生成失败（请检查代码是否包含不支持的特性，如数组、类或多文件模块）";
        return result;
    }
    
    // 如果只需要输出 LLVM IR
    if (config.emitLLVM) {
        String irFile = config.outputFile;
        if (irFile.empty()) {
            // 无 -o 参数：写至默认文件 output.ll
            irFile = "output.ll";
            std::ofstream ofs(irFile);
            if (ofs) {
                ofs << ir;
                ofs.close();
                result.success = true;
                result.outputFile = irFile;
                result.codeSize = ir.size();
            } else {
                result.errorMessage = "无法写入 LLVM IR 文件";
            }
        } else {
            std::ofstream ofs(irFile);
            if (ofs) {
                ofs << ir;
                ofs.close();
                result.success = true;
                result.outputFile = irFile;
                result.codeSize = ir.size();
            } else {
                result.errorMessage = "无法写入 LLVM IR 文件";
            }
        }
        return result;
    }
    
    // 编译 IR 到目标文件
    String objFile = getTempFile(".obj");
    if (!compileIRToObject(ir, objFile, config)) {
        return result;
    }
    
    // 生成 main 包装器
    String wrapperObj = getTempFile("_main.obj");
    String wrapperLl = getTempFile("_main.ll");
    if (!generateMainWrapper(wrapperLl, wrapperObj, ir, config)) {
        result.errorMessage = "无法生成 main 包装器";
        return result;
    }
    
    // 链接到可执行文件
    String exeFile = config.outputFile;
    if (exeFile.empty()) {
        exeFile = "a" + getDefaultOutputExtension();
    }
    
    if (!linkToExecutable({objFile, wrapperObj}, exeFile, config)) {
        return result;
    }
    
    result.success = true;
    result.outputFile = exeFile;
    result.codeSize = ir.size();
    
    // 清理临时文件
    cleanupTempFiles();
    
    return result;
}

bool AOTCompiler::generateLLVMIR(Shared<Program> ast, String& ir, const AOTConfig& config) {
    try {
        LLVMCodegen codegen;
        codegen.setOptLevel(config.optLevel);
        codegen.setSkipNativeCallRemoval(true);
        if (config.pureMath) {
            codegen.setPureMath(true);
        }
        ir = codegen.generateIRString(ast, config.optLevel);
        return !ir.empty();
    } catch (const std::exception& e) {
        std::cerr << "[AOT] LLVM IR 生成异常: " << e.what() << "\n";
        ir = "";
        return false;
    }
}

// ==================== 工具进程执行（CreateProcess，绕过 cmd.exe 引号问题） ====================

#ifdef _WIN32
static bool runTool(const std::string& cmdLine) {
    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    PROCESS_INFORMATION pi;
    // CreateProcessA 需要可写缓冲区
    std::string mutableCmd = cmdLine;
    if (!CreateProcessA(NULL, &mutableCmd[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        std::cerr << "[AOT] 错误: 进程启动失败 (error=" << GetLastError() << ")\n";
        return false;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return exitCode == 0;
}
#endif
// ==================== LLVM 工具路径检测 ====================

static String findLLVMTool(const String& tool, const AOTConfig& config) {
    // 1. 如果配置了目录，优先使用
    if (!config.llvmToolsDir.empty()) {
        String path = config.llvmToolsDir + "/" + tool;
        std::ifstream test(path.c_str());
        if (test.good()) return path;
        test.close();
    }
    
    // 2. 尝试相对于 cplang.exe 的路径
#ifdef _WIN32
    // 获取当前可执行文件路径
    char ownPath[MAX_PATH];
    GetModuleFileNameA(NULL, ownPath, MAX_PATH);
    String ownDir(ownPath);
    auto pos = ownDir.find_last_of("\\/");
    if (pos != String::npos) ownDir = ownDir.substr(0, pos);
    
    // 检查 ..\llvm-dev\bin\ 
    String candidate = ownDir + "\\..\\llvm-dev\\bin\\" + tool;
    std::ifstream test(candidate.c_str());
    if (test.good()) { test.close(); return candidate; }
    test.close();
    
    // 检查 ..\..\llvm-dev\bin\ 
    candidate = ownDir + "\\..\\..\\llvm-dev\\bin\\" + tool;
    test.open(candidate.c_str());
    if (test.good()) { test.close(); return candidate; }
    test.close();
#endif
    
    // 3. 在 PATH 中查找
    FILE* pipe = _popen(("where " + tool + " 2>nul").c_str(), "r");
    if (pipe) {
        char buffer[MAX_PATH];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            String result(buffer);
            // 去掉末尾换行
            while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
                result.pop_back();
            _pclose(pipe);
            return result;
        }
        _pclose(pipe);
    }
    
    return "";
}

// ==================== MSVC 路径检测 ====================

AOTCompiler::MSVCPaths AOTCompiler::detectMSVCPaths() {
    MSVCPaths paths;
    
#ifdef _WIN32
    // 1. 检查环境变量 LIB（如果已运行 vcvars）
    const char* libEnv = getenv("LIB");
    if (libEnv && strlen(libEnv) > 0) {
        String libStr(libEnv);
        auto parts = libStr;
        size_t start = 0;
        size_t end;
        std::vector<String> libDirs;
        while ((end = parts.find(';', start)) != String::npos) {
            libDirs.push_back(parts.substr(start, end - start));
            start = end + 1;
        }
        if (start < parts.length()) libDirs.push_back(parts.substr(start));
        
        for (auto& d : libDirs) {
            // 清理首尾空格和引号
            while (!d.empty() && (d[0] == ' ' || d[0] == '"')) d.erase(0, 1);
            while (!d.empty() && (d.back() == ' ' || d.back() == '"')) d.pop_back();
            
            if (d.find("ucrt") != String::npos) paths.ucrtLib = d;
            else if (d.find("um\\x64") != String::npos || d.find("um/x64") != String::npos) paths.umLib = d;
            else if (d.find("MSVC") != String::npos || d.find("14.") != String::npos) paths.msvcLib = d;
        }
    }
    
    // 2. 尝试 vswhere 检测
    if (paths.msvcLib.empty()) {
        FILE* pipe = _popen("\"%ProgramFiles(x86)%\\Microsoft Visual Studio\\Installer\\vswhere.exe\" -latest -property installationPath 2>nul", "r");
        if (!pipe) {
            pipe = _popen("\"%ProgramFiles%\\Microsoft Visual Studio\\Installer\\vswhere.exe\" -latest -property installationPath 2>nul", "r");
        }
        if (pipe) {
            char buffer[MAX_PATH];
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                String vsPath(buffer);
                while (!vsPath.empty() && (vsPath.back() == '\n' || vsPath.back() == '\r'))
                    vsPath.pop_back();
                _pclose(pipe);
                
                // 检测 MSVC 版本
                String msvcBase = vsPath + "\\VC\\Tools\\MSVC\\";
                WIN32_FIND_DATAA ffd;
                HANDLE hFind = FindFirstFileA((msvcBase + "*").c_str(), &ffd);
                if (hFind != INVALID_HANDLE_VALUE) {
                    String msvcVer = ffd.cFileName;
                    FindClose(hFind);
                    
                    paths.msvcLib = msvcBase + msvcVer + "\\lib\\x64";
                    paths.msvcBin = msvcBase + msvcVer + "\\bin\\Hostx64\\x64";
                    paths.msvcInclude = msvcBase + msvcVer + "\\include";
                }
                
                // 检测 Windows Kits 版本
                String kitsBase = "C:\\Program Files (x86)\\Windows Kits\\10\\";
                hFind = FindFirstFileA((kitsBase + "Lib\\*").c_str(), &ffd);
                if (hFind != INVALID_HANDLE_VALUE) {
                    String kitVer = ffd.cFileName;
                    FindClose(hFind);
                    
                    paths.ucrtLib = kitsBase + "Lib\\" + kitVer + "\\ucrt\\x64";
                    paths.umLib = kitsBase + "Lib\\" + kitVer + "\\um\\x64";
                    paths.ucrtInclude = kitsBase + "Include\\" + kitVer + "\\ucrt";
                    paths.umInclude = kitsBase + "Include\\" + kitVer + "\\um";
                    paths.sharedInclude = kitsBase + "Include\\" + kitVer + "\\shared";
                }
            } else {
                _pclose(pipe);
            }
        }
    }
    
    // 3. 如果 vswhere 失败，尝试已知路径
    if (paths.msvcLib.empty()) {
        String knownMsvc = "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.44.35207";
        paths.msvcLib = knownMsvc + "\\lib\\x64";
        paths.msvcBin = knownMsvc + "\\bin\\Hostx64\\x64";
        paths.msvcInclude = knownMsvc + "\\include";
    }
    if (paths.ucrtLib.empty()) {
        String knownKit = "C:\\Program Files (x86)\\Windows Kits\\10\\Lib\\10.0.26100.0";
        String knownKitInc = "C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.26100.0";
        paths.ucrtLib = knownKit + "\\ucrt\\x64";
        paths.umLib = knownKit + "\\um\\x64";
        paths.ucrtInclude = knownKitInc + "\\ucrt";
        paths.umInclude = knownKitInc + "\\um";
        paths.sharedInclude = knownKitInc + "\\shared";
    }
#endif
    
    return paths;
}

// jit_runtime.lib 由 build_msvc.bat 自动生成，aot_compiler 直接使用预编译版本

// ==================== 编译 IR 到目标文件 ====================

bool AOTCompiler::compileIRToObject(const String& ir, const String& objFile, const AOTConfig& config) {
    // 步骤 1：将 IR 写入临时文件
    String irFile = getTempFile(".ll");
    std::ofstream ofs(irFile);
    if (!ofs) {
        std::cerr << "[AOT] 错误: 无法写入临时 IR 文件\n";
        return false;
    }
    ofs << ir;
    ofs.close();
    
    // 步骤 2：查找 llc.exe
    String llcExe;
    if (!config.llvmToolsDir.empty()) {
        llcExe = config.llvmToolsDir + "/llc.exe";
    } else {
        // 默认搜索路径
        llcExe = findLLVMTool("llc.exe", config);
    }
    
    if (llcExe.empty()) {
        // 尝试验证已知路径是否有 llc.exe
        std::ifstream test("C:\\cplang\\llvm-dev\\bin\\llc.exe");
        if (test.good()) {
            test.close();
            llcExe = "C:\\cplang\\llvm-dev\\bin\\llc.exe";
        } else {
            std::cerr << "[AOT] 错误: 未找到 llc.exe，请设置 llvmToolsDir 或安装 LLVM\n";
            return false;
        }
    }
    
    // 步骤 3：构建并执行 llc 命令
    std::string cmd = "\"" + llcExe + "\" -filetype=obj";
    switch (config.optLevel) {
        case OptLevel::None: cmd += " -O0"; break;
        case OptLevel::O1: cmd += " -O1"; break;
        case OptLevel::O2: cmd += " -O2"; break;
        case OptLevel::O3: cmd += " -O3"; break;
    }
    cmd += " \"" + irFile + "\" -o \"" + objFile + "\"";
    
    if (!runTool(cmd)) {
        std::cerr << "[AOT] 错误: llc 编译失败\n";
        return false;
    }
    return true;
}

// ==================== 生成 main 包装器 ====================

bool AOTCompiler::generateMainWrapper(const String& wrapperLl, const String& wrapperObj,
                                       const String& irContent, const AOTConfig& config) {
    bool hasEntry = (irContent.find("@__cplang_entry") != String::npos);
    
    std::ofstream ofs(wrapperLl);
    if (!ofs) return false;
    
    if (hasEntry) {
        // 有 __cplang_entry：生成 main 包装器调用它
        ofs << "declare i64 @__cplang_entry()\n"
               "\n"
               "define i32 @main(i32 %argc, ptr %argv) {\n"
               "entry:\n"
               "  %result = call i64 @__cplang_entry()\n"
               "  %exit_code = trunc i64 %result to i32\n"
               "  ret i32 %exit_code\n"
               "}\n";
    } else {
        // 无 __cplang_entry：生成简单 main（正确的 CRT 兼容签名）
        ofs << "define i32 @main(i32 %argc, ptr %argv) {\n"
               "entry:\n"
               "  ret i32 0\n"
               "}\n";
    }
    ofs.close();
    
    // 编译包装器 .ll -> .obj
    String llcExe;
    if (!config.llvmToolsDir.empty()) {
        llcExe = config.llvmToolsDir + "/llc.exe";
    } else {
        llcExe = "C:\\cplang\\llvm-dev\\bin\\llc.exe";
    }
    
    std::string cmd = "\"" + llcExe + "\" -filetype=obj \"" + wrapperLl + "\" -o \"" + wrapperObj + "\"";
    return runTool(cmd);
}

// ==================== 链接到可执行文件 ====================

bool AOTCompiler::linkToExecutable(const std::vector<String>& objFiles, const String& exeFile, const AOTConfig& config) {
#ifdef _WIN32
    // 1. 检测 MSVC 路径
    MSVCPaths msvc = detectMSVCPaths();
    
    // 2. 收集所有 .obj 文件
    std::vector<String> allObjs = objFiles;
    
    // 3. 非纯数学模式：链接预编译的 jit_runtime.lib
    if (!config.pureMath) {
        String runtimeLib = "C:\\cplang\\build\\jit_runtime.lib";
        std::ifstream testLib(runtimeLib);
        if (testLib.good()) {
            testLib.close();
            allObjs.push_back(runtimeLib);
        } else {
            std::cerr << "[AOT] 警告: 未找到 " << runtimeLib << "，将仅链接纯数学模式\n";
            std::cerr << "[AOT] 提示: 请运行 build_msvc.bat 以生成 jit_runtime.lib\n";
        }
    }
    
    // 4. 选择链接器
    // 纯数学模式：无需 CRT → 用 lld-link（轻量，仅链接 LLVM .obj）
    // 非纯数学模式：需要 MSVC CRT → 用 link.exe（处理 MSVC 运行时内部符号）
    String linker;
    if (config.pureMath) {
        if (!config.llvmToolsDir.empty()) {
            linker = config.llvmToolsDir + "/lld-link.exe";
        } else {
            linker = findLLVMTool("lld-link.exe", config);
        }
        if (linker.empty()) {
            std::ifstream test("C:\\cplang\\llvm-dev\\bin\\lld-link.exe");
            if (test.good()) {
                linker = "C:\\cplang\\llvm-dev\\bin\\lld-link.exe";
            } else {
                std::cerr << "[AOT] 错误: 未找到 lld-link.exe\n";
                return false;
            }
        }
    } else {
        linker = msvc.msvcBin + "\\link.exe";
    }
    
    // 5. 构建链接命令
    std::string cmd = "\"" + linker + "\" ";
    for (auto& obj : allObjs) {
        cmd += "\"" + obj + "\" ";
    }
    cmd += "/subsystem:console /nologo /out:\"" + exeFile + "\"";
    
    // 非纯数学模式：需要 entry:mainCRTStartup 以初始化 C++ 运行时
    if (config.pureMath) {
        cmd += " /entry:main";
    }
    
    // 添加 MSVC lib 路径
    if (!msvc.msvcLib.empty()) {
        cmd += " /libpath:\"" + msvc.msvcLib + "\"";
    }
    if (!msvc.ucrtLib.empty()) {
        cmd += " /libpath:\"" + msvc.ucrtLib + "\"";
    }
    if (!msvc.umLib.empty()) {
        cmd += " /libpath:\"" + msvc.umLib + "\"";
    }
    
    // LLVM 生成的 .obj 不含 #pragma comment(lib)，需手动指定 CRT 静态库
    // 注意: MSVC 的 link.exe 对 llc 生成的 .obj（无 pragma 指令）不自动链接 CRT，
    // 必须显式指定 /defaultlib。使用静态库以避免 VCRUNTIME140.dll 运行时依赖:
    cmd += " /defaultlib:libcmt /defaultlib:libucrt /defaultlib:libvcruntime";
    
    VERBOSE(
        if (config.pureMath) std::cout << "[AOT] 链接中: lld-link...\n";
        else std::cout << "[AOT] 链接中: MSVC link.exe...\n";
    );
    if (!runTool(cmd)) {
        std::cerr << "[AOT] 错误: 链接失败\n";
        return false;
    }
    VERBOSE(std::cout << "[AOT] 链接成功!\n");
    return true;
#else
    // 非 Windows 平台暂不支持
    (void)objFiles;
    (void)exeFile;
    (void)config;
    std::cerr << "[AOT] AOT 编译目前仅支持 Windows 平台\n";
    return false;
#endif
}

// ==================== 平台函数 ====================

String AOTCompiler::getDefaultOutputExtension() {
#ifdef _WIN32
    return ".exe";
#else
    return "";
#endif
}

String AOTCompiler::getDefaultTargetTriple() {
#ifdef _WIN32
    return "x86_64-pc-windows-msvc";
#elif __APPLE__
    return "x86_64-apple-darwin";
#else
    return "x86_64-pc-linux-gnu";
#endif
}

std::vector<String> AOTCompiler::getDefaultLinkLibraries() {
    std::vector<String> libs;
#ifdef _WIN32
    libs.push_back("user32");
    libs.push_back("kernel32");
#else
    libs.push_back("m");
    libs.push_back("c");
#endif
    return libs;
}

String AOTCompiler::getTempFile(const String& suffix) {
    static int counter = 0;
    String filename;
    
#ifdef _WIN32
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    filename = tempPath;
    filename += "cplang_aot_";
    filename += std::to_string(++counter);
    filename += suffix;
#else
    filename = "/tmp/cplang_aot_";
    filename += std::to_string(++counter);
    filename += suffix;
#endif
    
    tempFiles_.push_back(filename);
    return filename;
}

void AOTCompiler::cleanupTempFiles() {
    for (const auto& file : tempFiles_) {
#ifdef _WIN32
        DeleteFileA(file.c_str());
#else
        unlink(file.c_str());
#endif
    }
    tempFiles_.clear();
}

} // namespace cplang

// DIAGNOSTIC PLACEHOLDER
