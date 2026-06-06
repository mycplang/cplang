#pragma once
#include <memory>
#include <string>
#include "vm/value.hpp"
#include "jit/jit_dispatch.hpp"

namespace cplang {

struct VMFunction;
struct Program;
class OrcJIT;
class JITCompiler;

class HybridJIT {
public:
    HybridJIT();
    ~HybridJIT();

    bool initialize();  // 初始化，返回是否成功

    // 编译（自动选择最佳模式）
    void* compile(std::shared_ptr<Program> program, const std::string& funcName);

    bool shouldCompile(VMFunction*) const;
    void recordCall(VMFunction*);
    void* compileHotFunction(VMFunction*);
    void compileAll(std::shared_ptr<Program>);
    void storeProgram(std::shared_ptr<Program>);
    void setHotThreshold(int);
    void dumpStats() const;

    // JIT 信息注册表（外部映射，不污染 VMFunction）
    JITRegistry& getJITRegistry() { return jitReg_; }
    const JITRegistry& getJITRegistry() const { return jitReg_; }

private:
    enum class Mode { None, Orc, External };

    class ExternalJIT;
    std::unique_ptr<OrcJIT> orcJit_;
    std::unique_ptr<ExternalJIT> externalJit_;
    Mode mode_ = Mode::None;
    JITRegistry jitReg_;
};

}