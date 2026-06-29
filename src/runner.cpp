// cplang-runner: minimal bundle executor (no compiler/JIT/debugger)
#define NOMINMAX
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#endif

#include "vm/vm_class.hpp"
#include "vm/vm_value_helpers.hpp"
#include "stdlib/stdlib.hpp"

using namespace cplang;

// Stubs for features not needed in slim runtime
namespace cplang {
    bool jitTryCallDispatch(VM*, VMFunction*, int, Value*, Value&) { return false; }
    bool jitTryCallDispatch(VM*, VMFunction*, int, Value*, int, Value*) { return false; }
    class DebugServer {
    public:
        bool shouldPause(const std::string&, int) { return false; }
        void waitForCommand() {}
        void poll() {}
        bool start(int) { return false; }
        void stop() {}
    };
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    { HINSTANCE hK32 = LoadLibraryA("kernel32.dll");
      if (hK32) { typedef BOOL (WINAPI *SetCP)(UINT); SetCP scp = (SetCP)GetProcAddress(hK32, "SetConsoleOutputCP"); if (scp) scp(65001); FreeLibrary(hK32); } }
#endif

    // Read self and find CPBC bundle at tail
    char selfPath[MAX_PATH];
    GetModuleFileNameA(NULL, selfPath, MAX_PATH);
    std::ifstream self(selfPath, std::ios::binary | std::ios::ate);
    if (!self) return 1;
    
    size_t total = self.tellg();
    size_t tailSz = total > 65536 ? 65536 : total;
    self.seekg(total - tailSz);
    std::string tail(tailSz, 0);
    self.read(&tail[0], tailSz); self.close();

    const char* mag = "CPBC\x00\x00\x00\x00";
    size_t pos = tail.rfind(mag);
    if (pos == std::string::npos) {
        std::cerr << "No bundle found\n";
        return 1;
    }

    const char* p = tail.data() + pos + 8;
    uint32_t cs = *(uint32_t*)p; p += 4;
    uint32_t ks = *(uint32_t*)p; p += 4;
    uint32_t el = *(uint32_t*)p; p += 4; p += el;

    if (p + cs > tail.data() + tail.size()) {
        std::cerr << "Bundle truncated\n";
        return 1;
    }

    // Create VM and load bundle
    VM vm;
    auto* func = new VMFunction();
    func->code.assign(p, p + cs);
    
    // Deserialize tagged constants (tag=1: string, tag=0: raw Value)
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

    func->maxStack = 4096;
    StdLib::registerAll(&vm);
    vm.refreshGlobalSlots();

    std::vector<Value> args;
    Value funcVal = makeFunctionVal(func);
    vm.callFunction(funcVal, args);

    return vm.hasError() ? 1 : 0;
}