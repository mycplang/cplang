// CP语言编译器
#define NOMINMAX
#undef min
#undef max
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include "core/verbose.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "codegen/codegen.hpp"
#include "vm/vm.hpp"
#include "stdlib/stdlib.hpp"
#include "module/module_system.hpp"
#include "jit/jit_compiler.hpp"
#include "jit/orc_jit.hpp"
#include "jit/hybrid_jit.hpp"
#include "debug/debugger.hpp"
#include "repl/repl.hpp"
#include "platform/platform.hpp"
#include "codegen/aot_compiler.hpp"

#ifdef _WIN32
#include <windows.h>
#include <urlmon.h>
#endif
using namespace cplang;

// ═══════════════════════════════════════════════════════════════════
//  SFX 自解压嵌入 — 将 CP 源码打包进 .exe
//  格式: [exe内容][源码][魔数16字节][源码长度4字节小端序]
// ═══════════════════════════════════════════════════════════════════

static const char  EMBED_MAGIC[]   = "\n---CPEMBED---\n";
static const int   EMBED_MAGIC_LEN = 16;

static std::string getOwnExePath() {
    return cplang::platform::proc_exe_path();
}

// 检测自身 .exe 末尾是否包含嵌入的 CP 源码，有则返回源码内容
static std::string checkEmbeddedSource() {
    std::string exePath = getOwnExePath();
    if (exePath.empty()) return "";
    std::ifstream file(exePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return "";
    std::streamsize fs = file.tellg();
    if (fs < EMBED_MAGIC_LEN + 4) return "";

    // 读末尾4字节 → 源码长度 (uint32_t LE)
    file.seekg(-4, std::ios::end);
    uint8_t lb[4]; file.read((char*)lb, 4);
    uint32_t srclen = lb[0] | ((uint32_t)lb[1]<<8) | ((uint32_t)lb[2]<<16) | ((uint32_t)lb[3]<<24);
    if (srclen == 0 || srclen > 100*1024*1024) return "";
    size_t hdr = EMBED_MAGIC_LEN + 4;
    if ((std::streamoff)(hdr + srclen) > fs) return "";

    // 验证魔数
    file.seekg(-((std::streamoff)hdr), std::ios::end);
    std::vector<char> mb(EMBED_MAGIC_LEN);
    file.read(mb.data(), EMBED_MAGIC_LEN);
    if (std::memcmp(mb.data(), EMBED_MAGIC, EMBED_MAGIC_LEN) != 0) return "";

    // 读源码
    file.seekg(-((std::streamoff)(hdr + srclen)), std::ios::end);
    std::vector<char> sb(srclen);
    file.read(sb.data(), srclen);
    return std::string(sb.data(), srclen);
}

// 将 CP 源码打包到独立 .exe
static bool packSource(const char* srcFile, const char* outFile) {
    // 读取源码
    std::ifstream ifs(srcFile, std::ios::binary);
    if (!ifs.is_open()) {
        std::cerr << "打包错误: 无法打开源文件 '" << srcFile << "'\n";
        return false;
    }
    std::stringstream ss; ss << ifs.rdbuf();
    std::string src = ss.str();
    ifs.close();
    if (src.empty()) { std::cerr << "打包错误: 源文件为空\n"; return false; }
    // 去 BOM
    if (src.size()>=3 && (uint8_t)src[0]==0xEF && (uint8_t)src[1]==0xBB && (uint8_t)src[2]==0xBF)
        src.erase(0, 3);

    std::cout << "打包: " << srcFile << " -> " << outFile << "\n";
    std::cout << "  源码: " << src.size() << " 字节\n";

    // 复制自身到目标
    std::string self = getOwnExePath();
    if (self.empty()) { std::cerr << "打包错误: 无法获取自身路径\n"; return false; }
    std::ifstream sself(self, std::ios::binary);
    std::ofstream out(outFile, std::ios::binary | std::ios::trunc);
    if (!sself.is_open() || !out.is_open()) {
        std::cerr << "打包错误: 文件读写失败\n"; return false;
    }
    out << sself.rdbuf(); sself.close();

    // 追加：源码 + 魔数 + 长度
    out.write(src.data(), src.size());
    out.write(EMBED_MAGIC, EMBED_MAGIC_LEN);
    uint32_t sl = (uint32_t)src.size();
    uint8_t lb2[4] = {(uint8_t)sl, (uint8_t)(sl>>8), (uint8_t)(sl>>16), (uint8_t)(sl>>24)};
    out.write((const char*)lb2, 4);
    out.close();

    std::cout << "  输出: " << (std::ifstream(outFile,std::ios::binary|std::ios::ate).tellg()/1024) << " KB\n";
    std::cout << "打包成功!\n";
    return true;
}


void printUsage(const char* program) {
    std::cout << "CP语言编译器 v0.2.0-beta\n\n";
    std::cout << "用法: " << program << " [选项] <文件>\n\n";
    std::cout << "选项:\n";
    std::cout << "  -l, --lex      仅词法分析\n";
    std::cout << "  -p, --parse    仅语法分析\n";
    std::cout << "  -c, --compile  完整编译并执行（字节码 VM）\n";
    std::cout << "  -c --hotspot   字节码 VM + 热点 JIT\n";
    std::cout << "       --hotspot-threshold=N  热点阈值 (默认 100)\n";
    std::cout << "  -j, --jit      完整编译并执行（JIT 模式）\n";
    std::cout << "  -a, --aot      AOT 编译为原生 .exe（LLVM，需模块包）\n";
    std::cout << "  -k, --pack     打包 CP 源码为独立 .exe（嵌入 VM）\n";
    std::cout << "  -o <file>      指定输出文件（配合 --aot/--pack 使用）\n";
    std::cout << "  -O0/-O1/-O2/-O3 优化级别\n";
    std::cout << "  --no-bytecode-opt 禁用字节码优化\n";
    std::cout << "  -v, --verbose  详细输出（优化统计等调试信息）\n";
    std::cout << "  -r, --repl     交互式编程环境（REPL）\n";
    std::cout << "  --debug-server <port> 启动调试服务器（TCP），等待 IDE 连接\n";
    std::cout << "  -h, --help     显示帮助信息\n";
    std::cout << "\n打包示例:\n";
    std::cout << "  cplang -k 图书管理.cp -o 图书管理.exe\n";
    std::cout << "  图书管理.exe          # 直接运行，无需安装 cplang\n";
}

bool readFile(const char* filename, String& content) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "错误: 无法打开文件 '" << filename << "'\n";
        return false;
    }
    content = String((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
    // 去掉 UTF-8 BOM (0xEF 0xBB 0xBF)
    if (content.size() >= 3 && 
        (unsigned char)content[0] == 0xEF && 
        (unsigned char)content[1] == 0xBB && 
        (unsigned char)content[2] == 0xBF) {
        content.erase(0, 3);
    }
    return true;
}

bool runLexer(const String& source) {
    Lexer lexer(source);
    std::cout << "词法分析结果:\n";
    int tokenCount = 0;
    while (true) {
        Token token = lexer.nextToken();
        std::cout << "  [" << tokenCount++ << "] " 
                  << "[" << static_cast<int>(token.type) << "] '" << token.text << "'"
                  << " (行 " << token.line << ", 列 " << token.column << ")\n";
        if (token.type == TokenType::END_OF_FILE) break;
        if (token.type == TokenType::INVALID) {
            std::cerr << "词法错误: " << lexer.errorMessage() << "\n";
            return false;
        }
    }
    return true;
}

bool runParser(const String& source) {
    std::cout << "语法分析...\n";
    auto program = parseString(source);
    if (!program) {
        std::cerr << "语法错误\n";
        return false;
    }
    std::cout << "语法分析成功，语句数: " << program->statements.size() << "\n";
    return true;
}

bool runFullCompile(const String& source, const char* filepath, bool useJit, bool useHotspot = false, int hotspotThreshold = 100, OptLevel optLevel = OptLevel::O2, bool useBytecodeOpt = true, int debugPort = 0) {
    std::cout << "编译中...\n";
    
    Compiler compiler;
    compiler.setOptLevel(optLevel);
    compiler.setEnableBytecodeOpt(useBytecodeOpt);
    String srcFile = (filepath && filepath[0] != '\0') ? String(filepath) : String("<string>");
    VMFunction* func = compiler.compile(source, srcFile);
    if (!func) {
        std::cerr << "编译失败: " << compiler.errorMessage() << "\n";
        return false;
    }

    std::cout << "编译成功\n";
    
    // 打印字节码优化统计（仅 verbose 模式）
    if (useBytecodeOpt && cplang::verboseEnabled()) {
        const BytecodeOptStats* stats = compiler.getBytecodeOptStats();
        if (stats && (stats->peepholesApplied > 0 || stats->deadInstructionsRemoved > 0 || stats->totalBytesSaved > 0)) {
            std::cout << "\n=== 字节码优化统计 ===\n";
            std::cout << "窥孔优化: " << stats->peepholesApplied << " 次\n";
            std::cout << "死指令移除: " << stats->deadInstructionsRemoved << " 条\n";
            std::cout << "常量传播: " << stats->constantPropagations << " 次\n";
            std::cout << "寄存器分配: " << stats->registersAllocated << " 个\n";
            std::cout << "节省字节: " << stats->totalBytesSaved << " B\n";
            std::cout << "========================\n\n";
        }
    }
    
    // 使用 Compiler 内部的 VM，确保全局槽位一致
    VM* vm = compiler.vm();
    if (!vm) {
        std::cerr << "错误: VM 未初始化\n";
        return false;
    }
    
    // 注册标准库（如果尚未注册）
    // StdLib::registerAll 在 Compiler 构造函数中已调用
    
    ModuleLoader loader;
    loader.addSearchPath(".");
    // 记录源文件所在目录（用于运行时导入解析）
    std::string sourceDir;
    if (filepath && filepath[0] != '\0') {
        String fp(filepath);
        size_t pos = fp.find_last_of("\\/");
        if (pos != String::npos) {
            std::string dir = fp.substr(0, pos);
            loader.addSearchPath(dir);
            sourceDir = dir;
        }
    }
    vm->importCallback = [&compiler, vm, sourceDir](const std::string& moduleName) -> bool {
        VMFunction* moduleFunc = nullptr;
        
        // 1. 源文件所在目录优先（解决跨目录运行时模块查找失败）
        if (!sourceDir.empty()) {
            moduleFunc = compiler.compileFile(sourceDir + "/" + moduleName + ".cp");
        }
        // 2. 本地同名文件 (cwd)
        if (!moduleFunc) {
            moduleFunc = compiler.compileFile(moduleName + ".cp");
        }
        
        // 3. 本地 packages/ 注册表
        if (!moduleFunc) {
            std::string pkgPath = "packages/" + moduleName + "/index.cp";
            moduleFunc = compiler.compileFile(pkgPath);
        }
        
        // 4. 用户安装目录 ~/.cpkg/packages/
        if (!moduleFunc) {
            const char* home = getenv("HOME");
            if (home) {
                std::string installedPath = std::string(home) + "/.cpkg/packages/" + moduleName + "/index.cp";
                moduleFunc = compiler.compileFile(installedPath);
            }
        }
        
        // 5. tests/ 目录
        if (!moduleFunc) {
            std::string altPath = "tests/" + moduleName + ".cp";
            moduleFunc = compiler.compileFile(altPath);
        }
        
        // 6. URL 远程导入（使用安全的 API 调用，避免命令注入）
        if (!moduleFunc && (moduleName.rfind("http://", 0) == 0 || moduleName.rfind("https://", 0) == 0)) {
            // 验证 URL 只包含合法字符（防止命令注入）
            bool valid = true;
            for (char c : moduleName) {
                // 允许 URL 合法字符：字母、数字、:/?#[]@!$&'()*+,;=-._~
                if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                      c == ':' || c == '/' || c == '?' || c == '#' || c == '[' || c == ']' ||
                      c == '@' || c == '!' || c == '$' || c == '&' || c == '\'' || c == '(' || c == ')' ||
                      c == '*' || c == '+' || c == ',' || c == ';' || c == '=' || c == '-' || c == '.' ||
                      c == '_' || c == '~' || c == '%')) {
                    valid = false;
                    break;
                }
            }
            if (valid) {
                std::string importPath = cplang::platform::file_temp_dir()
                    + "/_cp_import_" + std::to_string(cplang::platform::proc_getpid()) + ".cp";
#ifdef _WIN32
                // 使用 URLDownloadToFileA（urlmon.dll 无需额外链接，由 Windows 提供）
                HRESULT hr = URLDownloadToFileA(NULL, moduleName.c_str(), importPath.c_str(), 0, NULL);
                if (SUCCEEDED(hr)) {
                    moduleFunc = compiler.compileFile(importPath);
                }
#else
                // 非 Windows：使用系统 curl，但输入已经过验证
                std::string cmd = "curl -sL '" + moduleName + "' -o '" + importPath + "' 2>/dev/null";
                int ret = std::system(cmd.c_str());
                if (ret == 0) {
                    moduleFunc = compiler.compileFile(importPath);
                }
#endif
                // 清理临时文件
                std::remove(importPath.c_str());
            }
        }
        
        if (!moduleFunc) {
            std::cerr << "模块加载失败: " << moduleName << "\n";
            return false;
        }
        
        // 直接注册模块中的函数到全局槽，并JIT预热
        for (const auto& c : moduleFunc->constants) {
            if (c.isFunction()) {
                auto* funcObj = c.asFunction();
                if (funcObj && funcObj->name) {
                    std::string fname(funcObj->name->data, funcObj->name->length);
                    vm->registerGlobal(fname.c_str(), c);
                }
            }
        }
        
        // JIT 预热：解析模块源码并编译所有函数
        if (auto* jit = vm->getJIT()) {
            // 读取模块源码文件（用于JIT编译）
            std::string modFilePath;
            if (!sourceDir.empty() && std::ifstream(sourceDir + "/" + moduleName + ".cp").good())
                modFilePath = sourceDir + "/" + moduleName + ".cp";
            else if (std::ifstream(moduleName + ".cp").good())
                modFilePath = moduleName + ".cp";
            else if (std::ifstream("tests/" + moduleName + ".cp").good())
                modFilePath = "tests/" + moduleName + ".cp";
            else
                modFilePath = "packages/" + moduleName + "/index.cp";
            
            std::ifstream ifs(modFilePath);
            if (ifs.is_open()) {
                std::string src((std::istreambuf_iterator<char>(ifs)),
                                std::istreambuf_iterator<char>());
                if (!src.empty()) {
                    Lexer lexer(src);
                    Parser parser(&lexer);
                    auto moduleAST = parser.parse();
                    if (moduleAST && !parser.hasError()) {
                        for (const auto& c : moduleFunc->constants) {
                            if (c.isFunction()) {
                                auto* funcObj = c.asFunction();
                                if (funcObj && funcObj->name) {
                                    std::string fname(funcObj->name->data, funcObj->name->length);
                                    void* entry = jit->compile(moduleAST, fname);
                                    if (entry) {
                                        funcObj->jitEntry = entry;
                                        funcObj->jitCompiled = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // 标记导入成功（OP_IMPORT handler 不再执行模块字节码）
        vm->setLastImportedFunc(nullptr);
        return true;
    };
    
    // 初始化 HybridJIT（全局，需在 VM 生命周期内有效）
    HybridJIT jit;
    
    // JIT 模式：全量预编译 + VM 驱动（threshold=0）
    // JIT 模式：全量预编译 + VM 驱动（threshold=0）
    if (useJit) {
        VERBOSE(std::cout << "JIT 模式启动（全量预编译）...\n");

        auto program = parseString(source);
        if (program && jit.initialize()) {
            jit.compileAll(program);  // 全量预编译所有函数到同一模块
            jit.storeProgram(program);
            vm->setJIT(&jit);
            jit.setHotThreshold(0);
            VERBOSE(std::cout << "[HybridJIT] 全 JIT 模式\n");
        } else {
            std::cout << "[JIT] 初始化失败，回退到字节码执行\n";
        }
    }

    // VM 字节码模式（可选热点检测）
    if (useHotspot) {
        if (jit.initialize()) {
            auto program = parseString(source);
            if (program) {
                jit.storeProgram(program);
                vm->setJIT(&jit);
                jit.setHotThreshold(hotspotThreshold);
                VERBOSE(std::cout << "[HybridJIT] 热点检测已启用，阈值: " << hotspotThreshold << "\n");
            }
        }
    }
    
    // 调试服务器（TCP，供 IDE 连接）
    Debugger* debugger = nullptr;
    DebugServer* debugServer = nullptr;
    if (debugPort > 0) {
        debugger = new Debugger(vm);
        debugServer = new DebugServer(debugger, vm);
        if (debugServer->start(debugPort)) {
            vm->setDebugServer(debugServer);
            std::cout << "[调试] 服务器已启动，端口 " << debugPort
                      << "，等待 IDE 连接..." << std::endl;
        } else {
            std::cerr << "[调试] 服务器启动失败" << std::endl;
            delete debugServer; debugServer = nullptr;
            delete debugger; debugger = nullptr;
        }
    }

    if (!vm->loadModule(func)) {
        // 清理调试器
        if (debugServer) { debugServer->stop(); delete debugServer; }
        if (debugger) delete debugger;
        std::cerr << "执行失败: " << vm->error() << "\n";
        return false;
    }

    // 执行完成后清理调试器
    if (debugServer) { debugServer->stop(); delete debugServer; }
    if (debugger) delete debugger;

    std::cout << "\n执行完成，总指令数: " << vm->totalInstructions() << "\n";
    
    // 如果有热点检测，输出统计（仅 verbose 模式）
    if (useHotspot && cplang::verboseEnabled()) {
        jit.dumpStats();
    }
    
    return true;
}

int main(int argc, char* argv[]) {
    cplang::platform::console_set_utf8();
    // ── SFX 检测：无参数时检查自身是否包含嵌入的 CP 源码 ──
    if (argc <= 1) {
        std::string embedded = checkEmbeddedSource();
        if (!embedded.empty()) {
            std::cout << "[SFX] 检测到嵌入源码 (" << embedded.size() << " 字节)\n\n";
            // 将自身 exe 路径作为文件路径传递，用于模块导入解析
            std::string exePath = getOwnExePath();
            const char* path = exePath.empty() ? "embedded.cp" : exePath.c_str();
            return runFullCompile(String(embedded), path, false, false, 100, OptLevel::O2, true, 0) ? 0 : 1;
        }
        printUsage(argv[0]);
        return 1;
    }
    
    String mode = argv[1];
    
    if (mode == "-h" || mode == "--help") {
        printUsage(argv[0]);
        return 0;
    }
    
    // REPL 模式
    if (mode == "-r" || mode == "--repl") {
        ReplEngine repl(true);
        repl.run();
        return 0;
    }

    if (argc < 3 && (mode == "-l" || mode == "--lex" || 
                     mode == "-p" || mode == "--parse" || 
                     mode == "-c" || mode == "--compile" ||
                     mode == "-j" || mode == "--jit" ||
                     mode == "-a" || mode == "--aot")) {
        std::cerr << "错误: 缺少输入文件\n";
        printUsage(argv[0]);
        return 1;
    }
    
    String source;
    const char* filename = nullptr;
    bool useHotspot = false;
    bool useBytecodeOpt = true;
    int  hotspotThreshold = 100;
    OptLevel optLevel = OptLevel::O2;
    const char* outputFile = nullptr;
    int  debugPort = 0;  // 0 = 禁用调试服务器
    
    // 检查主模式
    // ── 打包模式 ──
    if (mode == "-k" || mode == "--pack") {
        const char* pf = nullptr;
        const char* po = nullptr;
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-o")==0 && i+1<argc) po = argv[++i];
            else if (!pf) pf = argv[i];
        }
        if (!pf) { std::cerr << "错误: --pack 需要指定源文件\n"; return 1; }
        if (!po) {
            static std::string defout;
            defout = std::string(pf);
            size_t dot = defout.rfind('.');
            if (dot != std::string::npos) defout = defout.substr(0, dot);
            defout += ".exe";
            po = defout.c_str();
        }
        return packSource(pf, po) ? 0 : 1;
    }

    // 扫描参数（从第2个开始，跳过模式）
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--hotspot") == 0) {
            useHotspot = true;
        } else if (strncmp(argv[i], "--hotspot-threshold=", 20) == 0) {
            hotspotThreshold = atoi(argv[i] + 20);
        } else if (strcmp(argv[i], "--no-bytecode-opt") == 0) {
            useBytecodeOpt = false;
        } else if (strcmp(argv[i], "-O0") == 0) {
            optLevel = OptLevel::None;
        } else if (strcmp(argv[i], "-O1") == 0) {
            optLevel = OptLevel::O1;
        } else if (strcmp(argv[i], "-O2") == 0) {
            optLevel = OptLevel::O2;
        } else if (strcmp(argv[i], "-O3") == 0) {
            optLevel = OptLevel::O3;
        } else if (strcmp(argv[i], "-o") == 0 && i+1 < argc) {
            outputFile = argv[++i];
        } else if (strcmp(argv[i], "--debug-server") == 0 && i+1 < argc) {
            debugPort = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            cplang::setVerbose(true);
        } else if (!filename) {
            filename = argv[i];
        }
    }
    
    if (filename) {
        if (!readFile(filename, source)) {
            return 1;
        }
    }
    
    bool success = false;
    
    if (mode == "-l" || mode == "--lex") {
        success = runLexer(source);
    } else if (mode == "-p" || mode == "--parse") {
        success = runParser(source);
    } else if (mode == "-c" || mode == "--compile") {
        success = runFullCompile(source, filename, false, useHotspot, hotspotThreshold, optLevel, useBytecodeOpt, debugPort);
    } else if (mode == "-j" || mode == "--jit") {
        success = runFullCompile(source, filename, true, useHotspot, hotspotThreshold, optLevel, useBytecodeOpt, debugPort);
    } else if (mode == "-a" || mode == "--aot") {
        AOTConfig aotCfg;
        aotCfg.optLevel = optLevel;
        if (outputFile) aotCfg.outputFile = outputFile;
        AOTCompiler aot;
        AOTResult res = aot.compileFile(filename, aotCfg);
        if (res.success) {
            VERBOSE(std::cout << "AOT 编译成功: " << res.outputFile << " (" << res.codeSize << " 字节 IR)" << std::endl);
            success = true;
        } else {
            std::cerr << "AOT 编译失败: " << res.errorMessage << std::endl;
            success = false;
        }
    } else {
        std::cerr << "错误: 未知选项 '" << mode << "'\n";
        printUsage(argv[0]);
        return 1;
    }
    
    return success ? 0 : 1;
}