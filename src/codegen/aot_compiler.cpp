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
    
    // 检测是否需要图形库
    scanGraphicsUsage(ast, graphicsNeeded_);
    
    // 检测模块导入（AOT 模块化支持）
    scanModuleImports(ast, neededModules_);
    VERBOSE(
        if (!neededModules_.empty()) {
            std::cout << "[AOT] 检测到模块导入:";
            for (auto& m : neededModules_) std::cout << " " << m;
            std::cout << std::endl;
        }
    );
    
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
    
    // 生成模块引导文件（如果检测到模块导入）
    std::vector<String> linkObjects = {objFile, wrapperObj};
    String bootstrapObj;
    if (!neededModules_.empty() && !config.pureMath) {
        String bootstrapLl = generateModuleBootstrap(neededModules_);
        bootstrapObj = getTempFile("_bootstrap.obj");
        if (!compileBootstrapToObj(bootstrapLl, bootstrapObj, config)) {
            std::cerr << "[AOT] 警告: 模块引导编译失败，将跳过模块注册\n";
        } else {
            linkObjects.push_back(bootstrapObj);
        }
    }
    
    // 链接到可执行文件
    String exeFile = config.outputFile;
    if (exeFile.empty()) {
        exeFile = "a" + getDefaultOutputExtension();
    }
    
    if (!linkToExecutable(linkObjects, exeFile, config)) {
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
        // AOT 模式不允许 LLVM 优化器折叠 NaN-boxing 编码中的算术
        // 由于 NaN-boxed 值使用 trunc→op→zext→box 序列，
        // LLVM 可能将 trunc(zext(x)) 优化回原始 NaN-boxed 值，使算术直接作用在带标签的 i64 上
        codegen.setOptLevel(OptLevel::None);
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
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    HANDLE hRead, hWrite;
    if (!CreatePipe(&hRead, &hWrite, &sa, 4096)) {
        return false;
    }
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);
    
    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION pi;
    
    std::string mutableCmd = cmdLine;
    if (!CreateProcessA(NULL, &mutableCmd[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(hRead); CloseHandle(hWrite);
        std::cerr << "[AOT] 错误: 进程启动失败 (error=" << GetLastError() << ")\n";
        return false;
    }
    CloseHandle(hWrite);
    
    // 读取子进程输出
    char buf[4096];
    DWORD read;
    std::string output;
    while (ReadFile(hRead, buf, sizeof(buf) - 1, &read, NULL) && read > 0) {
        buf[read] = '\0';
        output += buf;
    }
    CloseHandle(hRead);
    
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    // 打印子进程输出（仅 verbose 模式，正常编译不显示链接器警告）
    if (!output.empty()) {
        VERBOSE(std::cerr << output);
    }
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
    
    // 检查 exe 同级目录（集中部署方式：所有工具放一起）
    candidate = ownDir + "\\" + tool;
    test.open(candidate.c_str());
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
        // 从 msvcLib 推导 msvcBin（LIB 环境变量中有 lib 路径但没有 bin 路径）
        if (!paths.msvcLib.empty() && paths.msvcBin.empty()) {
            String binDir = paths.msvcLib;
            auto pos = binDir.rfind("lib");
            if (pos != String::npos) {
                binDir = binDir.substr(0, pos);
                // 优先 Hostx64\x86（64 位原生 linker），其次 Hostx64\x64
                String candidate1 = binDir + "bin\\Hostx64\\x86\\link.exe";
                String candidate2 = binDir + "bin\\Hostx64\\x64\\link.exe";
                std::ifstream test1(candidate1);
                if (test1.good()) {
                    test1.close();
                    paths.msvcBin = binDir + "bin\\Hostx64\\x86";
                } else {
                    test1.close();
                    paths.msvcBin = binDir + "bin\\Hostx64\\x64";
                }
            }
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
    
    // 3. 如果 vswhere 失败，扫描最新可用版本（避免硬编码版本号）
    if (paths.msvcLib.empty()) {
        String msvcBase = "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\";
        // 扫描 MSVC 版本目录，取最新的
        WIN32_FIND_DATAA ffd;
        String latestMsvc;
        HANDLE hFind = FindFirstFileA((msvcBase + "*").c_str(), &ffd);
        if (hFind != INVALID_HANDLE_VALUE) {
            latestMsvc = ffd.cFileName;
            while (FindNextFileA(hFind, &ffd)) {
                if (strcmp(ffd.cFileName, ".") != 0 && strcmp(ffd.cFileName, "..") != 0) {
                    // 取字典序最大的版本号（14.xx.xxxxx 格式，可自然比较）
                    if (strcmp(ffd.cFileName, latestMsvc.c_str()) > 0) {
                        latestMsvc = ffd.cFileName;
                    }
                }
            }
            FindClose(hFind);
        }
        if (!latestMsvc.empty()) {
            String knownMsvc = msvcBase + latestMsvc;
            paths.msvcLib = knownMsvc + "\\lib\\x64";
            paths.msvcBin = knownMsvc + "\\bin\\Hostx64\\x64";
            paths.msvcInclude = knownMsvc + "\\include";
        }
    }
    if (paths.ucrtLib.empty()) {
        String kitsBase = "C:\\Program Files (x86)\\Windows Kits\\10\\Lib\\";
        String kitsIncBase = "C:\\Program Files (x86)\\Windows Kits\\10\\Include\\";
        // 扫描 Windows Kits 版本目录，取最新的
        WIN32_FIND_DATAA ffd;
        String latestKit;
        HANDLE hFind = FindFirstFileA((kitsBase + "*").c_str(), &ffd);
        if (hFind != INVALID_HANDLE_VALUE) {
            latestKit = ffd.cFileName;
            while (FindNextFileA(hFind, &ffd)) {
                if (strcmp(ffd.cFileName, ".") != 0 && strcmp(ffd.cFileName, "..") != 0) {
                    if (strcmp(ffd.cFileName, latestKit.c_str()) > 0) {
                        latestKit = ffd.cFileName;
                    }
                }
            }
            FindClose(hFind);
        }
        if (!latestKit.empty()) {
            String knownKit = kitsBase + latestKit;
            String knownKitInc = kitsIncBase + latestKit;
            paths.ucrtLib = knownKit + "\\ucrt\\x64";
            paths.umLib = knownKit + "\\um\\x64";
            paths.ucrtInclude = knownKitInc + "\\ucrt";
            paths.umInclude = knownKitInc + "\\um";
            paths.sharedInclude = knownKitInc + "\\shared";
        }
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
        std::cerr << "[AOT] 错误: 未找到 llc.exe，请设置 llvmToolsDir 或安装 LLVM\n";
        return false;
    }
    
    // 步骤 3：构建并执行 llc 命令
    std::string cmd = "\"" + llcExe + "\" -filetype=obj";
    // AOT 模式强制 -O0（防止 NaN-boxing 算术被优化器破坏）
    cmd += " -O0";
    // switch (config.optLevel) {
    //     case OptLevel::None: cmd += " -O0"; break;
    //     case OptLevel::O1: cmd += " -O1"; break;
    //     case OptLevel::O2: cmd += " -O2"; break;
    //     case OptLevel::O3: cmd += " -O3"; break;
    // }
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
        // 非纯数学模式：先初始化 AOT VM 桥接，再执行用户代码
        if (!config.pureMath) {
            ofs << "declare void @aot_init_runtime()\n";
        }
        ofs << "declare i64 @__cplang_entry()\n"
               "\n"
               "define i32 @main(i32 %argc, ptr %argv) {\n"
               "entry:\n";
        if (!config.pureMath) {
            ofs << "  call void @aot_init_runtime()\n";
        }
        ofs << "  %result = call i64 @__cplang_entry()\n"
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
        llcExe = findLLVMTool("llc.exe", config);
    }
    if (llcExe.empty()) {
        std::cerr << "[AOT] 错误: 未找到 llc.exe，请设置 llvmToolsDir 或安装 LLVM\n";
        return false;
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
    
    // 3. 非纯数学模式：链接预编译的运行时库
    if (!config.pureMath) {
        // 在 cplang.exe 同级目录及 CMake 构建输出目录查找运行时库文件
        char ownPath[MAX_PATH];
        GetModuleFileNameA(NULL, ownPath, MAX_PATH);
        String ownDir(ownPath);
        auto pos = ownDir.find_last_of("\\/");
        if (pos != String::npos) ownDir = ownDir.substr(0, pos);
        
        // CMake 构建输出目录（回退搜索路径）
        String buildDir = ownDir + "\\..";  // build_verify/
        
        // 3b. 链接 aot_vm_bridge.obj（运行时桥接入口）
        String bridgeObj = ownDir + "\\aot_vm_bridge.obj";
        std::ifstream testBridge(bridgeObj);
        if (!testBridge.good()) {
            testBridge.close();
            bridgeObj = buildDir + "\\CMakeFiles\\cplang_aot.dir\\src\\aot\\aot_vm_bridge.cpp.obj";
            testBridge.open(bridgeObj);
        }
        if (testBridge.good()) {
            testBridge.close();
            allObjs.push_back(bridgeObj);
        } else {
            std::cerr << "[AOT] 警告: 未找到 aot_vm_bridge.obj，将跳过桥接初始化\n";
        }
        
        // 3a. 链接核心运行时库（cplang_stdlib_core.lib + cplang_stdlib.lib + cplang_vm.lib）
        //     cplang_stdlib_core 提供 registerCore()，cplang_stdlib 提供各模块 .obj
        //     由于 registerAll() 仅存在于 stdlib.obj 且不再被引用，不会拖入全部 stdlib
        std::vector<String> runtimeLibs = {
            buildDir + "\\cplang_stdlib_core.lib",
            buildDir + "\\cplang_stdlib.lib",
            buildDir + "\\cplang_vm.lib",
            buildDir + "\\cplang_platform.lib",
            buildDir + "\\cplang_debug.lib"
        };
        for (auto& lib : runtimeLibs) {
            std::ifstream test(lib, std::ios::binary);
            if (test.good()) {
                test.close();
                allObjs.push_back(lib);
            }
        }
        // 回退：exe 同级目录
        if (allObjs.size() == objFiles.size()) {
            String fallback = ownDir + "\\cplang_aot.lib";
            std::ifstream test(fallback, std::ios::binary);
            if (test.good()) { test.close(); allObjs.push_back(fallback); }
        }
        
        // 3a2. 添加检测到的模块 .lib 文件（AOT 模块化）
        for (auto& modName : neededModules_) {
            String modLib = findModuleLib(modName);
            if (!modLib.empty()) {
                VERBOSE(std::cout << "[AOT] 链接模块: " << modName << " → " << modLib << std::endl);
                allObjs.push_back(modLib);
            } else {
                std::cerr << "[AOT] 警告: 未找到模块 " << modName << " 的 .lib 文件\n";
            }
        }
        
        // 3c. 链接所有 LLVM lib（自动扫描目录）
        {
            String llvmDir = ownDir + "\\..\\..\\llvm-dev\\lib";
            String testFile = llvmDir + "\\LLVMCore.lib";
            std::ifstream tf(testFile.c_str());
            if (tf.good()) { /* AOT: scan LLVM libs for codegen symbols */
                tf.close();
#ifdef _WIN32
                // 扫描 LLVM lib 目录，添加所有 .lib 文件
                WIN32_FIND_DATAA ffd;
                String searchPath = llvmDir + "\\*.lib";
                HANDLE hFind = FindFirstFileA(searchPath.c_str(), &ffd);
                if (hFind != INVALID_HANDLE_VALUE) {
                    do {
                        String fp = llvmDir + "\\" + ffd.cFileName;
                        allObjs.push_back(fp);
                    } while (FindNextFileA(hFind, &ffd));
                    FindClose(hFind);
                }
#endif
            }
        }
        
        // 3d. 链接 jit_runtime.lib
        String runtimeLib = ownDir + "\\jit_runtime.lib";
        std::ifstream testLib(runtimeLib);
        if (testLib.good()) {
            testLib.close();
            allObjs.push_back(runtimeLib);
        } else {
            std::cerr << "[AOT] 警告: 未找到 jit_runtime.lib\n";
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
            if (findLLVMTool("lld-link.exe", config).empty()) {
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
        // /WHOLEARCHIVE: 自身已含内层引号，不加外层；其余路径统一加引号应对空格
        if (obj.compare(0, 14, "/WHOLEARCHIVE:") == 0) {
            cmd += obj + " ";
        } else {
            cmd += "\"" + obj + "\" ";
        }
    }
    cmd += "/subsystem:console /nologo /out:\"" + exeFile + "\"";
    // 允许不同 MSVC 版本间的 PDB 不匹配（预编译 lib 使用不同工具链版本）
    cmd += " /ignore:4098 /ignore:4099";
    cmd += " /merge:.CRT=.rdata";
    // 统一 /MD 动态 CRT：排除 debug CRT，显式指定 release CRT 组件
    cmd += " /NODEFAULTLIB:msvcrtd.lib /NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:libcmt.lib";
    cmd += " msvcrt.lib libvcruntime.lib libucrt.lib";
    cmd += " /FORCE:MULTIPLE";  // 兜底：忽略残余重复符号
    // 添加系统库（Shell32 / WinHTTP / Winsock / OpenGL 等）
    cmd += " Shell32.lib Winhttp.lib Ws2_32.lib Cabinet.lib opengl32.lib";
    // raylib.lib（图形支持，可选）
    if (graphicsNeeded_) {
        char _own[MAX_PATH]; GetModuleFileNameA(NULL, _own, MAX_PATH);
        String _dir(_own); size_t _p = _dir.find_last_of("\\/");
        if (_p != String::npos) _dir = _dir.substr(0, _p);
        String rlLib = _dir + "\\raylib.lib";
        std::ifstream testRl(rlLib);
        if (!testRl.good()) {
            testRl.close();
            rlLib = _dir + "\\..\\third_party\\raylib\\build_vs\\raylib\\raylib.lib";
            testRl.open(rlLib);
        }
        if (testRl.good()) {
            testRl.close();
            cmd += " \"" + rlLib + "\"";
        }
    }
    cmd += " gdi32.lib winmm.lib ole32.lib comctl32.lib user32.lib urlmon.lib";
    
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
    

    
    VERBOSE(
        if (config.pureMath) std::cout << "[AOT] 链接中: lld-link...\n";
        else std::cout << "[AOT] 链接中: MSVC link.exe...\n";
        std::cerr << "[AOT] DEBUG 命令: " << cmd << "\n";
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

// 递归扫描 AST 检测是否需要图形库（raylib/ImGui 函数调用）
static void scanGraphicsNode(Shared<Expr> expr, bool& found) {
    if (!expr || found) return;
    if (auto call = std::dynamic_pointer_cast<CallExpr>(expr)) {
        if (auto ident = std::dynamic_pointer_cast<IdentifierExpr>(call->callee)) {
            String name = ident->name;
            // 检查 raylib 函数（中文/英文）
            if (name.find("raylib.") != String::npos ||
                name == "InitWindow" || name == "BeginDrawing" || name == "EndDrawing" ||
                name.find("Drawing") != String::npos ||
                name.find("Window") != String::npos ||
                name.find("Texture") != String::npos ||
                name.find("Shader") != String::npos ||
                name.find("Camera") != String::npos ||
                name.find("Model") != String::npos ||
                name.find("Mesh") != String::npos ||
                name.find("Font") != String::npos ||
                name.find("Audio") != String::npos ||
                name.find("Sound") != String::npos ||
                name.find("Music") != String::npos ||
                // 检查 ImGui 函数
                name.find("ImGui") != String::npos ||
                name.find("imgui.") != String::npos ||
                name.find("Push") != String::npos ||
                name.find("Pop") != String::npos ||
                name.find("Slider") != String::npos || name.find("Button") != String::npos ||
                name == "开启窗口" || name == "关闭窗口" || name == "窗口应关闭" ||
                name == "开始绘制" || name == "结束绘制" || name == "清空背景" ||
                name == "绘制圆形" || name == "绘制矩形" ||
                name == "检测按键" || name == "检测按键按下")
            {
                found = true; return;
            }
        }
    }
    if (auto bin = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        scanGraphicsNode(bin->left, found);
        scanGraphicsNode(bin->right, found);
    }
    if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        scanGraphicsNode(unary->operand, found);
    }
    if (auto idx = std::dynamic_pointer_cast<IndexExpr>(expr)) {
        scanGraphicsNode(idx->array, found);
        scanGraphicsNode(idx->index, found);
    }
    if (auto member = std::dynamic_pointer_cast<MemberExpr>(expr)) {
        scanGraphicsNode(member->object, found);
    }
}

static void scanGraphicsStmt(Shared<Stmt> stmt, bool& found) {
    if (!stmt || found) return;
    if (auto exprStmt = std::dynamic_pointer_cast<ExprStmt>(stmt)) {
        scanGraphicsNode(exprStmt->expr, found);
    } else if (auto varDecl = std::dynamic_pointer_cast<VarDeclStmt>(stmt)) {
        if (varDecl->init) scanGraphicsNode(varDecl->init, found);
    } else if (auto ret = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
        scanGraphicsNode(ret->value, found);
    } else if (auto ifStmt = std::dynamic_pointer_cast<IfStmt>(stmt)) {
        scanGraphicsNode(ifStmt->condition, found);
        scanGraphicsStmt(ifStmt->thenBranch, found);
        if (ifStmt->elseBranch) scanGraphicsStmt(ifStmt->elseBranch, found);
    } else if (auto whileStmt = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
        scanGraphicsNode(whileStmt->condition, found);
        scanGraphicsStmt(whileStmt->body, found);
    } else if (auto forStmt = std::dynamic_pointer_cast<ForStmt>(stmt)) {
        scanGraphicsStmt(forStmt->body, found);
    } else if (auto block = std::dynamic_pointer_cast<BlockStmt>(stmt)) {
        for (auto& s : block->statements) scanGraphicsStmt(s, found);
    }
}

void AOTCompiler::scanGraphicsUsage(Shared<Program> ast, bool& found) {
    for (auto& stmt : ast->statements) {
        scanGraphicsStmt(stmt, found);
        if (found) return;
    }
}

// ═══════════════════════════════════════════════════════════════════
//  模块导入扫描（AOT 模块化支持）
// ═══════════════════════════════════════════════════════════════════

void AOTCompiler::scanModuleImports(Shared<Program> ast, std::set<String>& modules) {
    if (!ast) return;
    
    for (auto& stmt : ast->statements) {
        // 查找 ImportStmt: 导入 "@cp/xxx"
        if (auto importStmt = std::dynamic_pointer_cast<ImportStmt>(stmt)) {
            std::string modulePath = importStmt->moduleName;
            // 只处理 @cp/ 前缀的官方模块
            if (modulePath.find("@cp/") == 0 || modulePath.find("cp/") == 0) {
                // 标准化模块名
                std::string normalized;
                if (modulePath.find("@cp/") == 0) {
                    normalized = modulePath;
                } else {
                    normalized = "@" + modulePath;
                }
                modules.insert(normalized);
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
//  模块 .lib 文件查找
// ═══════════════════════════════════════════════════════════════════

String AOTCompiler::findModuleLib(const String& moduleName) {
    // 提取模块短名 (如 @cp/graphics → graphics)
    std::string shortName = moduleName;
    auto pos = shortName.rfind('/');
    if (pos != std::string::npos) shortName = shortName.substr(pos + 1);
    
    // 搜索路径优先级:
    // 1. cplang.exe 同级的 modules/<name>/win-x64/<name>.lib
    // 2. 项目根目录的 modules/<name>/win-x64/<name>.lib
    // 3. ~/.cpkg/packages/@cp/<name>/win-x64/<name>.lib
    
    std::vector<std::string> searchPaths;
    
#ifdef _WIN32
    char ownPath[MAX_PATH];
    GetModuleFileNameA(NULL, ownPath, MAX_PATH);
    std::string ownDir(ownPath);
    auto slashPos = ownDir.find_last_of("\\/");
    if (slashPos != std::string::npos) ownDir = ownDir.substr(0, slashPos);
    
    searchPaths.push_back(ownDir + "\\modules\\" + shortName + "\\win-x64\\" + shortName + ".lib");
    searchPaths.push_back(ownDir + "\\..\\modules\\" + shortName + "\\win-x64\\" + shortName + ".lib");
    searchPaths.push_back(ownDir + "\\..\\build_modules\\" + shortName + "\\" + shortName + "\\Release\\" + shortName + ".lib");
    
    // ~/.cpkg/packages/
    std::string home;
    const char* homeEnv = getenv("USERPROFILE");
    if (!homeEnv) homeEnv = getenv("HOME");
    if (homeEnv) {
        home = homeEnv;
        searchPaths.push_back(home + "\\.cpkg\\packages\\@cp\\" + shortName + "\\win-x64\\" + shortName + ".lib");
        searchPaths.push_back(home + "\\.cpkg\\packages\\" + moduleName + "\\win-x64\\" + shortName + ".lib");
    }
    
    // 也尝试不带 @ 的路径
    if (moduleName.find("@cp/") == 0) {
        std::string barePath = moduleName.substr(1); // cp/graphics
        if (!home.empty()) {
            searchPaths.push_back(home + "\\.cpkg\\packages\\" + barePath + "\\win-x64\\" + shortName + ".lib");
        }
    }
#endif
    
    for (auto& path : searchPaths) {
        std::ifstream test(path, std::ios::binary);
        if (test.good()) {
            test.close();
            return path;
        }
        test.close();
    }
    
    return "";
}

// ═══════════════════════════════════════════════════════════════════
//  生成模块引导 LLVM IR
// ═══════════════════════════════════════════════════════════════════

// 模块名 → C 链接函数名映射
static const std::unordered_map<std::string, std::string> kModuleRegisterFuncs = {
    {"@cp/graphics",    "cplang_module_graphics_register"},
    {"@cp/database",    "cplang_module_database_register"},
    {"@cp/crypto",      "cplang_module_crypto_register"},
    {"@cp/ffi",         "cplang_module_ffi_register"},
    {"@cp/net",         "cplang_module_network_register"},
    {"@cp/container",   "cplang_module_container_register"},
    {"@cp/concurrent",  "cplang_module_concurrent_register"},
    {"@cp/string_ext",  "cplang_module_string_ext_register"},
    {"@cp/charset",     "cplang_module_charset_register"},
    {"@cp/algorithm",   "cplang_module_algorithm_register"},
};

String AOTCompiler::generateModuleBootstrap(const std::set<String>& modules) {
    std::stringstream ss;
    ss << "; _aot_modules.ll — AOT 模块引导（自动生成）\n";
    ss << "; 检测到的模块:";
    for (auto& m : modules) ss << " " << m;
    ss << "\n\n";
    
    // 声明所有需要的注册函数
    for (auto& m : modules) {
        auto it = kModuleRegisterFuncs.find(m);
        if (it != kModuleRegisterFuncs.end()) {
            ss << "declare void @" << it->second << "(ptr)\n";
        }
    }
    
    // 定义 aot_register_modules —— 桥接代码会调用此函数
    ss << "\ndefine void @aot_register_modules(ptr %vm) {\n";
    ss << "entry:\n";
    for (auto& m : modules) {
        auto it = kModuleRegisterFuncs.find(m);
        if (it != kModuleRegisterFuncs.end()) {
            ss << "  call void @" << it->second << "(ptr %vm)\n";
        }
    }
    ss << "  ret void\n";
    ss << "}\n";
    
    return ss.str();
}

// ═══════════════════════════════════════════════════════════════════
//  编译 LLVM IR 引导文件为 .obj
// ═══════════════════════════════════════════════════════════════════

bool AOTCompiler::compileBootstrapToObj(const String& bootstrapLl, const String& objFile, const AOTConfig& config) {
    // 写入临时 .ll 文件
    std::string llFile = getTempFile(".ll");
    std::ofstream ofs(llFile);
    if (!ofs) return false;
    ofs << bootstrapLl;
    ofs.close();
    
    // 用 llc 编译为 .obj
    std::string llcExe;
    if (!config.llvmToolsDir.empty()) {
        llcExe = config.llvmToolsDir + "/llc.exe";
    } else {
        llcExe = findLLVMTool("llc.exe", config);
    }
    if (llcExe.empty()) {
        std::cerr << "[AOT] 错误: 未找到 llc.exe\n";
        return false;
    }
    
    std::string cmd = "\"" + llcExe + "\" -filetype=obj \"" + llFile + "\" -o \"" + objFile + "\"";
    if (!runTool(cmd)) {
        std::cerr << "[AOT] 错误: 模块引导编译失败\n";
        return false;
    }
    return true;
}

} // namespace cplang

// DIAGNOSTIC PLACEHOLDER