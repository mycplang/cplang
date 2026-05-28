// CP 语言构建系统

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>

namespace cplang {
namespace build {

// ═══════════════════════════════════════════════════════════════════
//  构建配置
// ═══════════════════════════════════════════════════════════════════

struct BuildConfig {
    std::string name;           // 项目名称
    std::string version;        // 版本
    std::string entry;          // 入口文件
    std::string output;         // 输出文件
    std::string buildDir;       // 构建目录
    
    // 编译选项
    bool optimize = true;       // 启用优化
    int optLevel = 2;           // 优化级别
    bool debug = false;         // 调试信息
    bool strip = false;         // 去除符号
    
    // 源文件
    std::vector<std::string> sources;
    std::vector<std::string> includePaths;
    
    // 依赖
    std::vector<std::string> libraries;
    std::vector<std::string> packages;
};

// ═══════════════════════════════════════════════════════════════════
//  构建任务
// ═══════════════════════════════════════════════════════════════════

enum class TaskType {
    Compile,        // 编译
    Link,           // 链接
    Copy,           // 复制
    Clean,          // 清理
    Test,           // 测试
    Install         // 安装
};

struct BuildTask {
    TaskType type;
    std::string name;
    std::vector<std::string> inputs;
    std::string output;
    std::vector<BuildTask*> dependencies;
    bool completed = false;
};

// ═══════════════════════════════════════════════════════════════════
//  构建系统
// ═══════════════════════════════════════════════════════════════════

class BuildSystem {
public:
    BuildSystem() = default;
    
    // 加载配置文件
    bool loadConfig(const std::string& configFile = "build.json") {
        if (!std::filesystem::exists(configFile)) {
            // 使用默认配置
            config_.name = "project";
            config_.entry = "main.cp";
            config_.output = "main.exe";
            config_.buildDir = "build";
            return true;
        }
        
        // TODO: 解析 JSON 配置
        std::cout << "加载配置: " << configFile << std::endl;
        return true;
    }
    
    // 初始化项目
    bool init(const std::string& projectName) {
        std::cout << "初始化项目: " << projectName << std::endl;
        
        // 创建目录结构
        std::filesystem::create_directories(projectName + "/src");
        std::filesystem::create_directories(projectName + "/tests");
        std::filesystem::create_directories(projectName + "/docs");
        
        // 创建 build.json
        std::ofstream config(projectName + "/build.json");
        config << "{\n";
        config << "  \"name\": \"" << projectName << "\",\n";
        config << "  \"version\": \"1.0.0\",\n";
        config << "  \"entry\": \"src/main.cp\",\n";
        config << "  \"output\": \"" << projectName << "\",\n";
        config << "  \"buildDir\": \"build\",\n";
        config << "  \"optimize\": true,\n";
        config << "  \"optLevel\": 2,\n";
        config << "  \"sources\": [\"src/**/*.cp\"],\n";
        config << "  \"packages\": []\n";
        config << "}\n";
        config.close();
        
        // 创建入口文件
        std::ofstream main(projectName + "/src/main.cp");
        main << "// " << projectName << " 入口文件\n";
        main << "\n";
        main << "函数 main() {\n";
        main << "    打印(\"Hello, " << projectName << "!\");\n";
        main << "    返回 0;\n";
        main << "}\n";
        main.close();
        
        std::cout << "项目已创建: " << projectName << std::endl;
        return true;
    }
    
    // 构建项目
    bool build() {
        auto start = std::chrono::high_resolution_clock::now();
        
        std::cout << "构建项目: " << config_.name << std::endl;
        std::cout << "优化级别: O" << config_.optLevel << std::endl;
        
        // 创建构建目录
        std::filesystem::create_directories(config_.buildDir);
        
        // 收集源文件
        collectSources();
        
        // 编译每个源文件
        for (const auto& source : config_.sources) {
            if (!compile(source)) {
                std::cerr << "编译失败: " << source << std::endl;
                return false;
            }
        }
        
        // 链接
        if (!link()) {
            std::cerr << "链接失败" << std::endl;
            return false;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        std::cout << "构建成功! (" << duration << "ms)" << std::endl;
        std::cout << "输出: " << config_.output << std::endl;
        
        return true;
    }
    
    // 清理构建
    bool clean() {
        std::cout << "清理构建目录..." << std::endl;
        
        if (std::filesystem::exists(config_.buildDir)) {
            std::filesystem::remove_all(config_.buildDir);
        }
        
        std::cout << "清理完成" << std::endl;
        return true;
    }
    
    // 运行测试
    bool test() {
        std::cout << "运行测试..." << std::endl;
        
        // 查找测试文件
        for (const auto& entry : std::filesystem::directory_iterator("tests")) {
            if (entry.is_regular_file() && entry.path().extension() == ".cp") {
                std::cout << "运行测试: " << entry.path() << std::endl;
                // TODO: 执行测试
            }
        }
        
        std::cout << "测试完成" << std::endl;
        return true;
    }
    
    // 安装依赖
    bool install() {
        std::cout << "安装依赖..." << std::endl;
        
        for (const auto& pkg : config_.packages) {
            std::cout << "安装: " << pkg << std::endl;
            // TODO: 调用包管理器
        }
        
        std::cout << "依赖安装完成" << std::endl;
        return true;
    }
    
    // 运行项目
    bool run(const std::vector<std::string>& args) {
        if (!std::filesystem::exists(config_.output)) {
            std::cerr << "可执行文件不存在，请先构建" << std::endl;
            return false;
        }
        
        std::string cmd = config_.output;
        for (const auto& arg : args) {
            cmd += " " + arg;
        }
        
        std::cout << "运行: " << cmd << std::endl;
        return std::system(cmd.c_str()) == 0;
    }
    
    // 设置配置
    void setConfig(const BuildConfig& config) { config_ = config; }
    const BuildConfig& getConfig() const { return config_; }

private:
    BuildConfig config_;
    std::vector<std::string> objectFiles_;
    
    void collectSources() {
        config_.sources.clear();
        
        // 递归查找 .cp 文件
        for (const auto& entry : std::filesystem::recursive_directory_iterator("src")) {
            if (entry.is_regular_file() && entry.path().extension() == ".cp") {
                config_.sources.push_back(entry.path().string());
            }
        }
        
        std::cout << "找到 " << config_.sources.size() << " 个源文件" << std::endl;
    }
    
    bool compile(const std::string& source) {
        std::cout << "编译: " << source << std::endl;
        
        // 生成目标文件名
        std::filesystem::path srcPath(source);
        std::string objName = config_.buildDir + "/" + srcPath.stem().string() + ".o";
        objectFiles_.push_back(objName);
        
        // TODO: 调用编译器
        // 这里简化处理，实际应该调用 Compiler 类
        
        return true;
    }
    
    bool link() {
        std::cout << "链接: " << config_.output << std::endl;
        
        // TODO: 链接目标文件
        
        return true;
    }
};

} // namespace build
} // namespace cplang

// ═══════════════════════════════════════════════════════════════════
//  CLI 入口
// ═══════════════════════════════════════════════════════════════════

void printUsage(const char* program) {
    std::cout << "用法: " << program << " <command> [options]\n\n";
    std::cout << "命令:\n";
    std::cout << "  init <name>    初始化新项目\n";
    std::cout << "  build          构建项目\n";
    std::cout << "  clean          清理构建\n";
    std::cout << "  test           运行测试\n";
    std::cout << "  install        安装依赖\n";
    std::cout << "  run [args...]  运行项目\n";
    std::cout << "\n选项:\n";
    std::cout << "  -O0, -O1, -O2, -O3  优化级别\n";
    std::cout << "  -g                  调试信息\n";
    std::cout << "  -v                  详细输出\n";
}

int main(int argc, char* argv[]) {
    using namespace cplang::build;
    
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string command = argv[1];
    BuildSystem builder;
    
    if (command == "init") {
        if (argc < 3) {
            std::cerr << "错误: 请指定项目名称" << std::endl;
            return 1;
        }
        return builder.init(argv[2]) ? 0 : 1;
    }
    
    // 加载配置
    builder.loadConfig();
    
    if (command == "build") {
        // 解析优化选项
        BuildConfig config = builder.getConfig();
        for (int i = 2; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "-O0") config.optLevel = 0;
            else if (arg == "-O1") config.optLevel = 1;
            else if (arg == "-O2") config.optLevel = 2;
            else if (arg == "-O3") config.optLevel = 3;
            else if (arg == "-g") config.debug = true;
        }
        builder.setConfig(config);
        return builder.build() ? 0 : 1;
    }
    
    if (command == "clean") {
        return builder.clean() ? 0 : 1;
    }
    
    if (command == "test") {
        return builder.test() ? 0 : 1;
    }
    
    if (command == "install") {
        return builder.install() ? 0 : 1;
    }
    
    if (command == "run") {
        std::vector<std::string> args;
        for (int i = 2; i < argc; i++) {
            args.push_back(argv[i]);
        }
        return builder.run(args) ? 0 : 1;
    }
    
    std::cerr << "未知命令: " << command << std::endl;
    printUsage(argv[0]);
    return 1;
}
