// CP语言编译器
#define NOMINMAX
#undef min
#undef max
#include <iostream>
#include <fstream>
#include <vector>
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
#include "codegen/llvm_codegen.hpp"
#include "debug/debugger.hpp"
#include "repl/repl.hpp"

#ifdef _WIN32
#include <urlmon.h>
#endif

using namespace cplang;

void printUsage(const char* program) {
    std::cout << "CP语言编译器 v0.9.3\n\n";
    std::cout << "用法: " << program << " [选项] <文件>\n\n";
    std::cout << "选项:\n";
    std::cout << "  -l, --lex      仅词法分析\n";
    std::cout << "  -p, --parse    仅语法分析\n";
    std::cout << "  -c, --compile  完整编译并执行（字节码 VM）\n";
    std::cout << "  -c --hotspot   字节码 VM + 热点 JIT\n";
    std::cout << "       --hotspot-threshold=N  热点阈值 (默认 100)\n";
    std::cout << "  -j, --jit      完整编译并执行（JIT 模式）\n";
    std::cout << "  -k, --pack     SFX 自解压打包（源码嵌入 cplang.exe）\n";
    std::cout << "  -o <file>      指定输出文件\n";
    std::cout << "  -O0/-O1/-O2/-O3 优化级别\n";
    std::cout << "  --no-bytecode-opt 禁用字节码优化\n";
    std::cout << "  -v, --verbose  详细输出（优化统计等调试信息）\n";
    std::cout << "  -e, --exec <代码>  直接执行代码字符串（WebIDE模式）\n";
    std::cout << "  -r, --repl     交互式编程环境（REPL）\n";
    std::cout << "  --debug-server <port> 启动调试服务器（TCP），等待 IDE 连接\n";
    std::cout << "  --headless      无头模式：禁用GUI，仅文本输出\n";
    std::cout << "  -h, --help     显示帮助信息\n";
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
    
    // 注册标准库
    StdLib::registerAll(vm);
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
    // v0.9.0
    std::string projectRoot;
    if (!sourceDir.empty()) {
        std::string probe = sourceDir;
        while (true) {
            std::ifstream f(probe + "/CMakeLists.txt");
            if (f.good()) { projectRoot = probe; break; }
            size_t s = probe.find_last_of("/\\");
            if (s == size_t(-1) || s == 0) break;
            probe = probe.substr(0, s);
        }
    }
    // P0.4：循环依赖检测 - 维护当前正在加载的模块栈
    static std::vector<std::string> loadingStack;
    
    vm->importCallback = [&compiler, vm, &projectRoot, sourceDir](const std::string& moduleName) -> bool {
        // 循环依赖检测：检查模块是否正在加载中
        for (const auto& m : loadingStack) {
            if (m == moduleName) {
                std::string errorMsg = "循环依赖检测: ";
                for (size_t i = 0; i < loadingStack.size(); i++) {
                    if (i > 0) errorMsg += " -> ";
                    errorMsg += loadingStack[i];
                }
                errorMsg += " -> " + moduleName;
                std::cerr << errorMsg << "\n";
                vm->raiseError(errorMsg.c_str());
                return false;
            }
        }
        
        // 入栈：标记模块正在加载
        loadingStack.push_back(moduleName);
        
        VMFunction* moduleFunc = nullptr;
        
        // 1. @前缀 = 明确指定 cpkg 包导入（不搜索本地）
        if (moduleName.size() > 1 && moduleName[0] == '@') {
            std::string pkgName = moduleName.substr(1);
            const char* home = getenv("HOME");
            if (!home) home = getenv("USERPROFILE");
            if (home) {
                moduleFunc = compiler.compileFile(std::string(home) + "/.cpkg/packages/" + pkgName + "/index.cp");
            }
            // @包找不到就不回退 —— 直接跳到报错
            if (!moduleFunc) {
                std::cerr << "包未找到: " << pkgName << "\n";
                loadingStack.pop_back();
                return false;
            }
        } else {
        
        // 2. 源文件所在目录（本地文件）
        if (!sourceDir.empty()) {
            moduleFunc = compiler.compileFile(sourceDir + "/" + moduleName + ".cp");
            if (!moduleFunc) moduleFunc = compiler.compileFile(sourceDir + "/" + moduleName + "/index.cp");
        }
        // 2. 本地同名文件 (cwd)
        if (!moduleFunc) {
            moduleFunc = compiler.compileFile(moduleName + ".cp");
        }
        
        // 3. 项目内 packages/ 目录
        if (!moduleFunc) {
            std::string pkgPath = "packages/" + moduleName + "/index.cp";
            moduleFunc = compiler.compileFile(pkgPath);
        if (!moduleFunc && !projectRoot.empty()) {
            moduleFunc = compiler.compileFile(projectRoot + "/packages/" + moduleName + "/index.cp");
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
                std::string importPath = std::string(std::getenv("TEMP") ? std::getenv("TEMP") : "C:/Temp")
                    + "/_cp_import_" + std::to_string(GetCurrentProcessId()) + ".cp";
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
        }

        if (!moduleFunc) {
            std::cerr << "模块加载失败: " << moduleName << "\n";
            loadingStack.pop_back();  // 出栈
            return false;
        }

        // === P0.1：模块命名空间隔离 ===
        // 创建一个 table 作为模块对象，容纳模块的函数和变量
        // 同时保留旧行为：函数也直接注册到全局（向后兼容）
        VMTable* moduleTable = VMTable::create();

        // 1. 把模块中的函数注册到：
        //    a) 模块 table（新方式：模块.函数() 调用）
        //    b) 全局槽（旧方式：直接调用函数名）
        for (const auto& c : moduleFunc->constants) {
            if (c.isFunction()) {
                auto* funcObj = c.asFunction();
                if (funcObj && funcObj->name) {
                    std::string fname(funcObj->name->data, funcObj->name->length);
                    // a) 注册到模块 table
                    VMString* keyStr = VMString::create(fname.c_str(),
                                                         static_cast<UInt32>(fname.length()));
                    moduleTable->set(makeStringVal(keyStr), c);
                    // b) 同时注册到全局（向后兼容）
                    vm->registerGlobal(fname.c_str(), c);
                }
            }
        }

        // 2. 记录执行前的全局槽值快照（用于检测模块代码设置的全局变量）
        //    注意：不能用名称检测，因为变量名在 codegen 阶段就已经注册到全局槽了
        std::unordered_map<std::string, Value> globalValuesBefore;
        auto slotNames = vm->getGlobalSlotNames();
        for (const auto& name : slotNames) {
            Int32 slot = vm->getGlobalSlot(name.c_str());
            if (slot >= 0) {
                Value* vPtr = vm->getGlobalBySlot(static_cast<UInt16>(slot));
                if (vPtr) {
                    globalValuesBefore[name] = *vPtr;
                }
            }
        }

        // 3. 执行被导入模块的顶层代码（执行全局变量赋值、模块初始化）
        {
            std::vector<Value> emptyArgs;
            Value moduleFuncVal = makeFunctionVal(moduleFunc);
            vm->callFunction(moduleFuncVal, emptyArgs);
        }

        // 4. 把模块顶层代码设置的全局变量也注册到模块 table 中
        //    通过比较执行前后的值变化来检测（值不同的就是模块设置的变量）
        slotNames = vm->getGlobalSlotNames();
        for (const auto& name : slotNames) {
            Int32 slot = vm->getGlobalSlot(name.c_str());
            if (slot < 0) continue;
            Value* vPtr = vm->getGlobalBySlot(static_cast<UInt16>(slot));
            if (!vPtr) continue;

            Value currentVal = *vPtr;
            auto it = globalValuesBefore.find(name);
            bool valueChanged = false;
            if (it == globalValuesBefore.end()) {
                // 新名称（理论上不会发生，因为 codegen 已注册所有名称）
                valueChanged = !currentVal.isNil();
            } else {
                // 值发生了变化（不是同一个值）
                valueChanged = !it->second.equals(currentVal);
            }

            if (valueChanged) {
                // 检查是否已经作为函数注册（避免重复）
                VMString* keyStr = VMString::create(name.c_str(),
                                                     static_cast<UInt32>(name.length()));
                Value existing = moduleTable->get(makeStringVal(keyStr));
                if (existing.isNil()) {
                    moduleTable->set(makeStringVal(keyStr), currentVal);
                }
            }
        }

        // 5. 以模块名把模块对象注册到全局
        // 提取纯模块名（不含路径），如 "utils/math" → "math"（但这里优先用完整模块名）
        std::string moduleKey = moduleName;
        size_t slashPos = moduleName.find_last_of("/\\");
        if (slashPos != std::string::npos) {
            // 如果有路径，使用最后一段作为模块变量名
            // 同时用全名注册，方便 import "path/module" 后 path.module.xxx()
            std::string shortName = moduleName.substr(slashPos + 1);
            Value tableVal = makeTableVal(moduleTable);
            vm->registerGlobal(shortName.c_str(), tableVal);
        }
        // 无论如何都用原始模块名注册
        {
            Value tableVal = makeTableVal(moduleTable);
            vm->registerGlobal(moduleName.c_str(), tableVal);
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
        vm->setLastImportedFunc(moduleFunc);
        loadingStack.pop_back();  // 出栈
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
#ifdef _WIN32
    // 设置控制台输出为 UTF-8，否则中文乱码
    { HINSTANCE hK32 = LoadLibraryA("kernel32.dll"); if (hK32) { typedef BOOL (WINAPI *SetCP)(UINT); SetCP scp = (SetCP)GetProcAddress(hK32, "SetConsoleOutputCP"); if (scp) scp(65001); FreeLibrary(hK32); } }
#endif
    if (argc < 2) {
        std::ifstream self(argv[0], std::ios::binary | std::ios::ate);
        if (self) {
            size_t total = self.tellg();
            size_t tailSz = total > 65536 ? 65536 : total;
            self.seekg(total - tailSz);
            std::string tail(tailSz, 0);
            self.read(&tail[0], tailSz); self.close();
            // CPBC: bytecode bundle (fallback)
            const char* mag = "CPBC\x00\x00\x00\x00";
            size_t pos = tail.rfind(mag);
            if (pos != std::string::npos) {
                const char* p = tail.data() + pos + 8;
                uint32_t cs = *(uint32_t*)p; p+=4;
                uint32_t ks = *(uint32_t*)p; p+=4;
                uint32_t el = *(uint32_t*)p; p+=4; p+=el;
                if (p+cs <= tail.data()+tail.size()) {
                    VM vm;
                    auto* func = new VMFunction();
                    func->code.assign(p, p + cs);
                    const uint8_t* cp = (const uint8_t*)p + cs;
                    for (uint32_t i = 0; i < ks; i++) {
                        uint8_t tag = *cp; cp++;
                        if (tag == 1) {
                            uint32_t len = *(uint32_t*)cp; cp += 4;
                            auto* s = VMString::create(std::string((const char*)cp, len).c_str());
                            cp += len;
                            func->constants.push_back(makeStringVal(s));
                        } else {
                            uint64_t raw; memcpy(&raw, cp, 8); cp += 8;
                            func->constants.push_back(Value(raw));
                        }
                    }
                    func->maxStack = 256;
                    // hasSlots=false: name-based LOADGLOBAL (no slot table needed)
                    StdLib::registerAll(&vm);
                    vm.refreshGlobalSlots();
                    std::vector<Value> args;
                    Value funcVal = makeFunctionVal(func);
                    std::cerr << "[CPBC] calling..." << std::endl;
                    vm.callFunction(funcVal, args);
                    return vm.hasError() ? 1 : 0;
                }
            }
        }
        printUsage(argv[0]);
        return 1;
    }
    
    String mode = argv[1];
    
    if (mode == "build" && argc >= 3) {
        const char* outFile = argc > 4 ? argv[4] : nullptr;
        // BYTECODE AOT: compile, rewrite LOADGLOBAL slot->name, serialize
        Compiler comp;
        comp.setOptLevel(OptLevel::O2);
        comp.setEnableBytecodeOpt(true);
        auto* modFunc = comp.compileFile(argv[2]);
        if (comp.hasError()) { std::cerr << comp.errorMessage() << "\n"; return 1; }
        VM* cvm = comp.vm();
        // Rewrite LOADGLOBAL: slot index -> constant pool name index
        modFunc->hasSlots = false;  // use name-based LOADGLOBAL
        for (size_t pc = 0; pc + 3 < modFunc->code.size(); ) {
            UInt8 op = modFunc->code[pc];
            UInt8 a  = modFunc->code[pc+1];
            if (op == OP_LOADGLOBAL || op == OP_STOREGLOBAL) {
                UInt16 slot = ((UInt16)modFunc->code[pc+2] << 8) | modFunc->code[pc+3];
                // Find name for this slot
                std::string fname;
                for (auto& kv : cvm->getSlotMap()) {
                    if (kv.second == slot) { fname = kv.first; break; }
                }
                if (!fname.empty()) {
                    UInt16 poolIdx = (UInt16)modFunc->constants.size();
                    modFunc->constants.push_back(makeStringVal(VMString::create(fname.c_str())));
                    // Replace slot index with pool index (big-endian)
                    modFunc->code[pc+2] = (UInt8)(poolIdx >> 8);
                    modFunc->code[pc+3] = (UInt8)(poolIdx & 0xFF);
                }
            }
            pc += 4;
        }
        // Serialize
        char selfPath[MAX_PATH]; GetModuleFileNameA(NULL, selfPath, MAX_PATH);
        std::string out = outFile ? outFile : "a.exe";
        std::ifstream src(selfPath, std::ios::binary);
        std::ofstream dst(out, std::ios::binary);
        dst << src.rdbuf(); src.close();
        const char MAGIC[] = "CPBC\x00\x00\x00\x00";
        dst.write(MAGIC, 8);
        uint32_t cs = (uint32_t)modFunc->code.size();
        uint32_t ks = (uint32_t)modFunc->constants.size();
        uint32_t el = 0;
        dst.write((char*)&cs, 4); dst.write((char*)&ks, 4); dst.write((char*)&el, 4);
        dst.write((char*)modFunc->code.data(), cs);
        // Serialize constants: tag=1 for string (len+data), tag=0 for raw Value
        for (auto& v : modFunc->constants) {
            if (v.isString()) {
                uint8_t tag = 1; dst.write((char*)&tag, 1);
                auto* s = v.asString();
                uint32_t len = (uint32_t)s->length;
                dst.write((char*)&len, 4);
                dst.write(s->data, len);
            } else {
                uint8_t tag = 0; dst.write((char*)&tag, 1);
                uint64_t raw = v.raw(); dst.write((char*)&raw, 8);
            }
        }
        dst.close();
        delete modFunc;
        std::cout << "Built: " << out << " (code=" << cs << "B consts=" << ks << ")\n";
        return 0;
    }

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

    // -e/--exec 模式：直接执行代码字符串（WebIDE）
    if (mode == "-e" || mode == "--exec") {
        if (argc < 3) {
            std::cerr << "错误: 缺少代码参数\n";
            printUsage(argv[0]);
            return 1;
        }
        String codeStr = argv[2];
#ifdef _WIN32
        // Windows 命令行参数使用系统代码页（GBK），需转换为 UTF-8
        // 以便 CP 语言的 UTF-8 词法分析器正确处理中文代码
        {
            int wlen = MultiByteToWideChar(CP_ACP, 0, codeStr.c_str(), -1, nullptr, 0);
            if (wlen > 0) {
                std::wstring wstr(wlen, L'\0');
                MultiByteToWideChar(CP_ACP, 0, codeStr.c_str(), -1, &wstr[0], wlen);
                int ulen = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
                if (ulen > 0) {
                    std::string ustr(ulen, '\0');
                    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &ustr[0], ulen, nullptr, nullptr);
                    // 去掉末尾 null terminator
                    if (!ustr.empty() && ustr.back() == '\0') ustr.pop_back();
                    codeStr = ustr;
                }
            }
        }
#endif
        bool headless = false;
        int  debugPort = 0;
        OptLevel optLevel = OptLevel::O2;
        bool useBytecodeOpt = true;

        // 扫描额外参数
        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "--headless") == 0) {
                headless = true;
            } else if (strcmp(argv[i], "-O0") == 0) {
                optLevel = OptLevel::None;
            } else if (strcmp(argv[i], "-O1") == 0) {
                optLevel = OptLevel::O1;
            } else if (strcmp(argv[i], "-O2") == 0) {
                optLevel = OptLevel::O2;
            } else if (strcmp(argv[i], "-O3") == 0) {
                optLevel = OptLevel::O3;
            } else if (strcmp(argv[i], "--no-bytecode-opt") == 0) {
                useBytecodeOpt = false;
            } else if (strcmp(argv[i], "--debug-server") == 0 && i + 1 < argc) {
                debugPort = atoi(argv[++i]);
            } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
                cplang::setVerbose(true);
            }
        }

        if (headless) {
            // 设置环境变量，让 stdlib 跳过图形初始化
#ifdef _WIN32
            _putenv("CPLANG_HEADLESS=1");
#else
            setenv("CPLANG_HEADLESS", "1", 1);
#endif
        }

        return runFullCompile(codeStr, "<stdin>", false, false, 100, optLevel, useBytecodeOpt, debugPort) ? 0 : 1;
    }

    if (argc < 3 && (mode == "-l" || mode == "--lex" || 
                     mode == "-p" || mode == "--parse" || 
                     mode == "-c" || mode == "--compile" ||
                     mode == "-j" || mode == "--jit")) {
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
    } else {
        std::cerr << "错误: 未知选项 '" << mode << "'\n";
        printUsage(argv[0]);
        return 1;
    }
    
    return success ? 0 : 1;
}