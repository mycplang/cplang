#include <string>
#include <memory>

namespace cplang {

class Program;
struct AOTConfig {};

class AOTCompiler {
public:
    AOTCompiler();
    ~AOTCompiler();
    bool compileFile(const std::string&, const AOTConfig&);
    bool compileSource(const std::string&, const AOTConfig&);
};

AOTCompiler::AOTCompiler() {}
AOTCompiler::~AOTCompiler() {}
bool AOTCompiler::compileFile(const std::string&, const AOTConfig&) { return false; }
bool AOTCompiler::compileSource(const std::string&, const AOTConfig&) { return false; }

}