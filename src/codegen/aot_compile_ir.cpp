// AOT IR编译：使用 llc.exe（自动从 llvm-dev/bin/ 检测）
#include "codegen/aot_compiler.hpp"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace cplang {

static std::string findLLVMTool2(const std::string& tool) {
    const char* env = std::getenv("LLVM_DIR");
    if (env) {
        std::string p = std::string(env) + "/bin/" + tool;
        std::ifstream f(p); if (f.good()) return p;
    }
    char own[MAX_PATH];
    GetModuleFileNameA(NULL, own, MAX_PATH);
    std::string dir(own);
    auto pos = dir.find_last_of("\\/");
    if (pos != std::string::npos) dir = dir.substr(0, pos);
    std::string p = dir + "\\..\\..\\llvm-dev\\bin\\" + tool;
    std::ifstream f(p); if (f.good()) return p;
    return tool;
}

bool llvmIRToObject(const std::string& irContent, const std::string& objPath, int optLevel) {
    std::string llcExe = findLLVMTool2("llc.exe");
    
    


    std::string irFile = objPath + ".ll";
    {
        std::ofstream ofs(irFile);
        if (!ofs) { std::cerr << "[AOT] cannot write IR\n"; return false; }
        // Strip BOM if present
    size_t start = 0;
    if (irContent.size() >= 3 && (unsigned char)irContent[0] == 0xEF && (unsigned char)irContent[1] == 0xBB && (unsigned char)irContent[2] == 0xBF) start = 3;
    ofs.write(irContent.data() + start, irContent.size() - start);
    }
    // 将 llc 所在目录加入 PATH（需 LLVM DLL）
    std::string llcDir = llcExe.substr(0, llcExe.find_last_of("\\/"));
    std::string oldPath = std::getenv("PATH") ? std::getenv("PATH") : "";
    std::string newPath = llcDir + ";" + oldPath;
    _putenv_s("PATH", newPath.c_str());

    std::string cmd = "set \"PATH=" + llcDir + ";%PATH%\" && \"" + llcExe + "\" -filetype=obj";
    if (optLevel <= 0) cmd += " -O0";
    else if (optLevel == 1) cmd += " -O1";
    else if (optLevel >= 3) cmd += " -O3";
    else cmd += " -O2";
    cmd += " \"" + irFile + "\" -o \"" + objPath + "\"";
    
    int ret = std::system(cmd.c_str());
    if (ret != 0) { std::cerr << "[AOT] llc failed (exit=" << ret << ") IR=" << irFile << "\n"; return false; }
    std::remove(irFile.c_str());
    return true;
}

} // namespace cplang
