// 模块系统实现

#include "module/module_system.hpp"
#include "codegen/codegen.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <iostream>

namespace cplang {

namespace fs = std::filesystem;

// ═══════════════════════════════════════════════════════════════════
//  Module 实现
// ═══════════════════════════════════════════════════════════════════

Module::Module(const String& name)
    : name_(name), loaded_(false) {}

bool Module::addDependency(const String& moduleName) {
    if (hasDependency(moduleName)) return false;
    dependencies_.push_back(moduleName);
    return true;
}

bool Module::hasDependency(const String& moduleName) const {
    return std::find(dependencies_.begin(), dependencies_.end(), moduleName)
           != dependencies_.end();
}

bool Module::hasCircularDependency(const String& moduleName,
                                    std::vector<String>& path) const {
    if (name_ == moduleName) return true;
    if (std::find(path.begin(), path.end(), name_) != path.end()) return false;
    path.push_back(name_);
    // 这里需要检查依赖模块，简化实现
    return false;
}

// ═══════════════════════════════════════════════════════════════════
//  ModuleLoader 实现
// ═══════════════════════════════════════════════════════════════════

ModuleLoader::ModuleLoader() {
    addSearchPath(".");
}

void ModuleLoader::addSearchPath(const String& path) {
    if (std::find(searchPaths_.begin(), searchPaths_.end(), path) == searchPaths_.end()) {
        searchPaths_.push_back(path);
    }
}

Shared<Module> ModuleLoader::loadModule(const String& moduleName, VM* vm) {
    auto it = modules_.find(moduleName);
    if (it != modules_.end()) {
        return it->second;
    }
    return loadModuleInternal(moduleName, vm);
}

Shared<Module> ModuleLoader::loadModuleInternal(const String& moduleName, VM* vm) {
    // 检查循环依赖
    if (isModuleLoading(moduleName)) {
        lastError_ = "Circular dependency detected: " + moduleName;
        for (const auto& mod : loadingStack_) {
            lastError_ += " <- " + mod;
        }
        lastError_ += " <- " + moduleName;
        return nullptr;
    }

    // 查找模块文件
    String filename = resolveModulePath(moduleName);
    if (filename.empty()) {
        lastError_ = "找不到模块: " + moduleName;
        return nullptr;
    }

    // 读取源代码
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        lastError_ = "Cannot open module file: " + filename;
        return nullptr;
    }
    String source((std::istreambuf_iterator<char>(ifs)),
                  std::istreambuf_iterator<char>());
    ifs.close();

    // 创建模块
    auto module = std::make_shared<Module>(moduleName);
    modules_[moduleName] = module;
    loadingStack_.push_back(moduleName);

    // 编译并执行模块（非slot模式：通过常数池存储，确保与主VM slot不冲突）
    Compiler compiler(false);
    VMFunction* func = compiler.compile(source);
    if (!func) {
        lastError_ = "Module compilation failed: " + compiler.errorMessage();
        loadingStack_.pop_back();
        return nullptr;
    }

    // 加载到 VM
    if (vm && !vm->loadModule(func)) {
        lastError_ = "Module execution failed: " + vm->error();
        loadingStack_.pop_back();
        return nullptr;
    }

    module->setLoaded(true);
    loadingStack_.pop_back();
    return module;
}

String ModuleLoader::resolveModulePath(const String& moduleName) {
    // 尝试直接路径
    String directPath = moduleName + ".cp";
    if (fs::exists(directPath)) {
        return directPath;
    }

    // 搜索路径（使用 / 和 \ 都尝试）
    for (const auto& searchPath : searchPaths_) {
        String path1 = searchPath + "/" + moduleName + ".cp";
        String path2 = searchPath + "\\" + moduleName + ".cp";
        if (fs::exists(path1)) {
            std::cout << "[IMPORT] Found module at: " << path1 << "\n";
            return path1;
        }
        if (fs::exists(path2)) {
            std::cout << "[IMPORT] Found module at: " << path2 << "\n";
            return path2;
        }
    }

    return "";
}

bool ModuleLoader::isModuleLoading(const String& moduleName) const {
    return std::find(loadingStack_.begin(), loadingStack_.end(), moduleName)
           != loadingStack_.end();
}

// ═══════════════════════════════════════════════════════════════════
//  ModuleManager 实现
// ═══════════════════════════════════════════════════════════════════

ModuleManager::ModuleManager(VM* vm) : vm_(vm) {
    loader_ = std::make_unique<ModuleLoader>();
}

bool ModuleManager::import(const String& moduleName, const String& alias) {
    auto module = loader_->loadModule(moduleName, vm_);
    if (!module) {
        lastError_ = loader_->lastError();
        return false;
    }

    String effectiveName = alias.empty() ? moduleName : alias;
    imported_[effectiveName] = module;
    return true;
}

Shared<Module> ModuleManager::getModule(const String& name) const {
    auto it = imported_.find(name);
    return it != imported_.end() ? it->second : nullptr;
}

bool ModuleManager::isImported(const String& moduleName) const {
    return imported_.count(moduleName) > 0;
}

// ═══════════════════════════════════════════════════════════════════
//  只编译模块，不执行（用于OP_IMPORT指令）
// ═══════════════════════════════════════════════════════════════════

VMFunction* ModuleLoader::compileModule(const String& moduleName, String& errorMsg) {
    // 检查是否已加载
    auto it = modules_.find(moduleName);
    if (it != modules_.end()) {
        // 已加载，返回nullptr表示无需重新编译
        return nullptr;
    }
    
    // 查找模块文件
    String filename = resolveModulePath(moduleName);
    if (filename.empty()) {
        errorMsg = "找不到模块: " + moduleName;
        return nullptr;
    }
    
    // 读取源代码
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        errorMsg = "Cannot open module file: " + filename;
        return nullptr;
    }
    String source((std::istreambuf_iterator<char>(ifs)),
                  std::istreambuf_iterator<char>());
    ifs.close();
    
    // 创建模块记录
    auto module = std::make_shared<Module>(moduleName);
    modules_[moduleName] = module;
    
    // 编译模块
    Compiler compiler(false);
    VMFunction* func = compiler.compile(source);
    if (!func) {
        errorMsg = "Module compilation failed: " + compiler.errorMessage();
        modules_.erase(moduleName);
        return nullptr;
    }
    
    module->setLoaded(true);
    return func;
}

// ═══════════════════════════════════════════════════════════════════
//  导出函数
// ═══════════════════════════════════════════════════════════════════

bool importModule(VM* vm, const String& moduleName) {
    static ModuleManager manager(vm);
    return manager.import(moduleName);
}

} // namespace cplang
