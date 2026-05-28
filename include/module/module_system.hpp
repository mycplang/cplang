#pragma once

#include "common/types.hpp"
#include "vm/vm.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  模块表示
// ═══════════════════════════════════════════════════════════════════

class Module : public std::enable_shared_from_this<Module> {
public:
    explicit Module(const String& name);

    String name() const { return name_; }
    bool isLoaded() const { return loaded_; }
    void setLoaded(bool v) { loaded_ = v; }

    // 依赖管理
    bool addDependency(const String& moduleName);
    bool hasDependency(const String& moduleName) const;
    const std::vector<String>& dependencies() const { return dependencies_; }
    bool hasCircularDependency(const String& moduleName,
                                 std::vector<String>& path) const;

private:
    String name_;
    bool loaded_ = false;
    std::vector<String> dependencies_;
};

// ═══════════════════════════════════════════════════════════════════
//  模块加载器
// ═══════════════════════════════════════════════════════════════════

class ModuleLoader {
public:
    ModuleLoader();

    void addSearchPath(const String& path);
    Shared<Module> loadModule(const String& moduleName, VM* vm = nullptr);
    
    // 只编译模块，不执行（用于OP_IMPORT指令）
    VMFunction* compileModule(const String& moduleName, String& errorMsg);
    String resolveModulePath(const String& moduleName);

    const String& lastError() const { return lastError_; }
    const std::unordered_map<String, Shared<Module>>& loadedModules() const {
        return modules_;
    }

private:
    std::vector<String> searchPaths_;
    std::unordered_map<String, Shared<Module>> modules_;
    std::vector<String> loadingStack_;
    String lastError_;

    Shared<Module> loadModuleInternal(const String& moduleName, VM* vm);
    bool isModuleLoading(const String& moduleName) const;
};

// ═══════════════════════════════════════════════════════════════════
//  模块管理器
// ═══════════════════════════════════════════════════════════════════

class ModuleManager {
public:
    explicit ModuleManager(VM* vm);

    bool import(const String& moduleName, const String& alias = "");
    Shared<Module> getModule(const String& name) const;
    bool isImported(const String& moduleName) const;

    const String& lastError() const { return lastError_; }
    ModuleLoader* loader() const { return loader_.get(); }

private:
    VM* vm_;
    std::unique_ptr<ModuleLoader> loader_;
    std::unordered_map<String, Shared<Module>> imported_;
    String lastError_;
};

// ═══════════════════════════════════════════════════════════════════
//  便捷函数
// ═══════════════════════════════════════════════════════════════════

bool importModule(VM* vm, const String& moduleName);

} // namespace cplang
