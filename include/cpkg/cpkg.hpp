// CP语言 包管理器头文件
#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace cpkg {

// ============================================================================
// 配置
// ============================================================================
struct Config {
    std::string packagesDir;   // 包安装目录
    std::string registryUrl;   // 注册表 URL
    std::string giteeToken;    // Gitee token（可选）
    std::string registryFile;  // 本地注册表 URL 缓存文件
};

Config loadConfig();

// ============================================================================
// HTTP
// ============================================================================
std::string httpGet(const std::string& url, const std::string& token = "");
bool httpDownload(const std::string& url, const std::string& dest, const std::string& token = "");

// ============================================================================
// JSON（轻量，仅用于解析 registry index）
// ============================================================================
using JsonValue = std::variant<std::string, std::vector<std::string>,
    std::unordered_map<std::string, std::string>>;
std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
    parseRegistryIndex(const std::string& json);

// ============================================================================
// 包管理操作
// ============================================================================
bool installPackage(const std::string& name, const Config& cfg);
bool removePackage(const std::string& name, const Config& cfg);
bool listPackages(const Config& cfg);
bool searchPackages(const std::string& query, const Config& cfg);
bool updatePackage(const std::string& name, const Config& cfg);
bool showPackageInfo(const std::string& name, const Config& cfg);
bool setRegistry(const std::string& url, Config& cfg);
bool getRegistry(const Config& cfg);

// ============================================================================
// CLI
// ============================================================================
int main(int argc, char* argv[]);

} // namespace cpkg
