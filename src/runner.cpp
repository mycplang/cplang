// CP Runner — 独立可执行文件引导 (v0.9.3)
// 读取自身 exe 尾部的字节码并执行
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace cplang { class VM; }

// 前向声明
extern "C" {
    using VMFunc = void*(*)(void*, const char*, int, const void*, int, void**, int*);
    // 简化：直接内联核心 VM 逻辑
}

// 魔数标记：字节码数据开始
static const char MAGIC[] = "CPBC\0\0\0\0";

struct BundleHeader {
    char magic[8];       // "CPBC\0\0\0\0"
    uint32_t codeSize;   // 字节码大小
    uint32_t constSize;  // 常量池大小
    uint32_t entryPoint; // 入口函数名长度
    // followed by: entryPoint name, code bytes, constants
};

// 获取自身 exe 路径
std::string getExePath() {
#ifdef _WIN32
    char buf[MAX_PATH];
    GetModuleFileNameA(NULL, buf, MAX_PATH);
    return buf;
#else
    char buf[4096];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf)-1);
    if (len != -1) buf[len] = '\0';
    else return "";
    return buf;
#endif
}

int main(int argc, char* argv[]) {
    std::string exePath = getExePath();
    
    // 读取自身尾部
    std::ifstream exeFile(exePath, std::ios::binary | std::ios::ate);
    if (!exeFile) {
        std::cerr << "FATAL: cannot open self: " << exePath << std::endl;
        return 1;
    }
    
    // 从尾部搜索 MAGIC
    const size_t fileSize = exeFile.tellg();
    const size_t searchStart = (fileSize > 65536) ? fileSize - 65536 : 0;
    
    exeFile.seekg(searchStart);
    std::vector<char> tail(fileSize - searchStart);
    exeFile.read(tail.data(), tail.size());
    exeFile.close();
    
    // 搜索 MAGIC
    const char* magicPos = nullptr;
    for (size_t i = 0; i + 8 <= tail.size(); i++) {
        if (std::memcmp(tail.data() + i, MAGIC, 4) == 0) {
            magicPos = tail.data() + i;
            break;
        }
    }
    
    if (!magicPos) {
        std::cerr << "FATAL: no bytecode bundle found in " << exePath << std::endl;
        return 1;
    }
    
    // 解析 bundle header
    const BundleHeader* hdr = reinterpret_cast<const BundleHeader*>(magicPos);
    const char* dataPtr = reinterpret_cast<const char*>(hdr + 1);
    
    // 读取入口函数名
    std::string entryName(dataPtr, hdr->entryPoint);
    dataPtr += hdr->entryPoint;
    
    // 读取字节码
    std::vector<uint8_t> code(hdr->codeSize);
    std::memcpy(code.data(), dataPtr, hdr->codeSize);
    dataPtr += hdr->codeSize;
    
    std::cerr << "[runner] bundle loaded: " << hdr->codeSize << "B code, "
              << hdr->constSize << "B consts" << std::endl;
    
    // TODO: Initialize VM, load bytecode, execute
    // 此处需要链接 cplang VM 库
    std::cerr << "[runner] VM execution not yet implemented" << std::endl;
    
    return 0;
}
