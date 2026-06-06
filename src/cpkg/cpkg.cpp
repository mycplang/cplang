// ═══════════════════════════════════════════════════════════════════
// cpkg — CP语言 包管理器（C++ 原生实现，Windows WinHTTP）
// ═══════════════════════════════════════════════════════════════════
// 用法:
//   cpkg install <包名>      安装包
//   cpkg remove <包名>       卸载包
//   cpkg search [关键词]     搜索包
//   cpkg list                列出已安装
//   cpkg update [包名]       更新包
//   cpkg info <包名>         包信息
//   cpkg registry [URL]      查看/设置注册表
//
// 环境变量:
//   CPKG_GITHUB_TOKEN    GitHub token（访问私有仓库/提高限频）
//   CPKG_REGISTRY_URL    注册表地址（默认指向 mycpmlang/registry）
// ═══════════════════════════════════════════════════════════════════

#include "cpkg/http.hpp"
using cpkg::getEnv;
using cpkg::getHomeDir;
using cpkg::httpGet;
using cpkg::httpDownload;

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#include <urlmon.h>
#include <shellapi.h>
#include <shlobj.h>
#else
// Linux/macOS 兼容层：文件路径即 UTF-8，不需宽字符转换
#include <cstring>
static std::string wstrToUtf8(const std::string& s) { return s; }
static std::string utf8ToWstr(const std::string& s) { return s; }
inline void SetConsoleOutputCP(int) {}
#define CP_UTF8 65001
#endif

#ifndef _WIN32
static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" 	
");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" 	
");
    return s.substr(start, end - start + 1);
}
#endif

// ═══════════════════════════════════════════════════════════════════
// 工具函数
// ═══════════════════════════════════════════════════════════════════

#ifdef _WIN32
static std::string wstrToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string result((size_t)len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), result.data(), len, nullptr, nullptr);
    return result;
}

static std::wstring utf8ToWstr(const std::string& str) {
    if (str.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
    if (len <= 0) return {};
    std::wstring result((size_t)len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), result.data(), len);
    return result;
}

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::string getEnv(const std::string& key, const std::string& def = "") {
    std::wstring wkey = utf8ToWstr(key);
    DWORD len = GetEnvironmentVariableW(wkey.c_str(), nullptr, 0);
    if (len == 0) return def;
    std::wstring wval(len, L'\0');
    GetEnvironmentVariableW(wkey.c_str(), wval.data(), len);
    return wstrToUtf8(wval);
}

static std::string getHomeDir() {
    wchar_t buf[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, buf))) {
        return wstrToUtf8(buf);
    }
    return "C:\\Users\\Default";
}

static bool ensureDir(const std::string& path) {
    std::error_code ec;
    fs::create_directories(fs::path(utf8ToWstr(path)), ec);
    return !ec;
}

static std::string readFile(const std::string& path) {
    std::ifstream f(utf8ToWstr(path), std::ios::binary);
    if (!f) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static bool writeFile(const std::string& path, const std::string& content) {
    std::ofstream f(utf8ToWstr(path), std::ios::binary);
    if (!f) return false;
    f.write(content.data(), (std::streamsize)content.size());
    return f.good();
}

// ═══════════════════════════════════════════════════════════════════
// WinHTTP — HTTPS 请求（支持 Bearer token 认证）
// ═══════════════════════════════════════════════════════════════════

static std::string httpRequest(const std::string& url, const std::string& token = "") {
    // 解析 URL: protocol://host/path
    size_t pos = url.find("://");
    if (pos == std::string::npos) return "";
    std::string protocol = url.substr(0, pos);
    std::string rest = url.substr(pos + 3);
    pos = rest.find('/');
    std::string host = (pos == std::string::npos) ? rest : rest.substr(0, pos);
    std::string path = (pos == std::string::npos) ? "/" : rest.substr(pos);

    bool isHttps = (protocol == "https");

    HINTERNET hSession = nullptr;
    // 尝试多种代理配置：无代理 → 默认代理 → 自动代理
    DWORD modes[] = { WINHTTP_ACCESS_TYPE_NO_PROXY,
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY };
    for (int i = 0; i < 3 && !hSession; i++) {
        hSession = WinHttpOpen(L"cpkg/1.0", modes[i], NULL, NULL, 0);
    }
    if (!hSession) return "";

    // 启用 TLS 1.2
    DWORD tlsProtocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
    WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS,
        &tlsProtocols, sizeof(tlsProtocols));

    HINTERNET hConnect = WinHttpConnect(hSession, utf8ToWstr(host).c_str(),
        isHttps ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return ""; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET",
        utf8ToWstr(path).c_str(), NULL, NULL, NULL,
        isHttps ? WINHTTP_FLAG_SECURE : 0);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return ""; }

    // 添加认证头
    std::wstring headers;
    if (!token.empty()) {
        headers = L"Authorization: Bearer " + utf8ToWstr(token) + L"\r\n";
        headers += L"User-Agent: cpkg/1.0\r\n";
    }

    BOOL result = WinHttpSendRequest(hRequest,
        headers.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headers.c_str(),
        headers.empty() ? 0 : (DWORD)headers.length(),
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (!result) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return "";
    }

    result = WinHttpReceiveResponse(hRequest, NULL);
    if (!result) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return "";
    }

    // 检查 HTTP 状态码
    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        NULL, &statusCode, &statusSize, NULL);
    if (statusCode != 200 && statusCode != 301 && statusCode != 302) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return "";
    }

    std::string response;
    DWORD dwSize = 0;
    do {
        dwSize = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
        if (dwSize == 0) break;
        std::vector<char> buf((size_t)dwSize + 1);
        DWORD dwRead = 0;
        if (WinHttpReadData(hRequest, buf.data(), dwSize, &dwRead)) {
            response.append(buf.data(), (size_t)dwRead);
        }
    } while (dwSize > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return response;
}

// 使用 urlmon 下载（WinINET 栈，兼容性更好，支持系统代理）
static std::string httpRequestUrlmon(const std::string& url) {
    wchar_t tmpDir[MAX_PATH];
    wchar_t tmpFile[MAX_PATH];
    if (GetTempPathW(MAX_PATH, tmpDir) == 0) return "";
    if (GetTempFileNameW(tmpDir, L"cpkg", 0, tmpFile) == 0) return "";

    HRESULT hr = URLDownloadToFileW(NULL, utf8ToWstr(url).c_str(), tmpFile, 0, NULL);
    std::string result;
    if (SUCCEEDED(hr)) {
        result = readFile(wstrToUtf8(tmpFile));
    }
    DeleteFileW(tmpFile);
    return result;
}

static bool httpDownload(const std::string& url, const std::string& destPath,
    const std::string& token) {
    // 如果有 token，用 WinHTTP
    if (!token.empty()) {
        std::string data = httpRequest(url, token);
        if (!data.empty()) return writeFile(destPath, data);
    }
    // 无 token 或 WinHTTP 失败，用 urlmon
    return SUCCEEDED(URLDownloadToFileW(NULL, utf8ToWstr(url).c_str(),
        utf8ToWstr(destPath).c_str(), 0, NULL));
}

// ═══════════════════════════════════════════════════════════════════
// 轻量 JSON 解析器（仅解析 registry index 格式）
// ═══════════════════════════════════════════════════════════════════

static std::string jsonDecodeString(const std::string& s, size_t& pos) {
    if (pos >= s.size() || s[pos] != '"') return "";
    pos++;
    std::string result;
    while (pos < s.size() && s[pos] != '"') {
        if (s[pos] == '\\') {
            pos++;
            if (pos >= s.size()) break;
            switch (s[pos]) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case 'u': {
                    if (pos + 4 < s.size()) {
                        std::string hex = s.substr(pos + 1, 4);
                        char* end = nullptr;
                        unsigned long cp = std::strtoul(hex.c_str(), &end, 16);
                        result += (cp <= 0x7F) ? (char)cp : '?';
                        pos += 4;
                    }
                    break;
                }
                default: result += s[pos]; break;
            }
        } else {
            result += s[pos];
        }
        pos++;
    }
    if (pos < s.size()) pos++;
    return result;
}

// 解析扁平 JSON 对象 { key: value, ... } 所有值为字符串
static std::unordered_map<std::string, std::string> jsonParseFlatObject(
    const std::string& s, size_t& pos) {
    std::unordered_map<std::string, std::string> result;
    if (pos >= s.size() || s[pos] != '{') return result;
    pos++;

    while (pos < s.size()) {
        while (pos < s.size() && strchr(" \t\n\r", s[pos])) pos++;
        if (pos >= s.size() || s[pos] == '}') break;
        if (s[pos] != '"') { pos++; continue; }
        std::string key = jsonDecodeString(s, pos);
        while (pos < s.size() && strchr(" \t\n\r:", s[pos])) pos++;
        if (pos < s.size() && s[pos] == '"') {
            result[key] = jsonDecodeString(s, pos);
        } else {
            // 跳过非字符串值（嵌套对象、数字等）
            int depth = 0;
            while (pos < s.size() && !(s[pos] == ',' && depth == 0) && !(s[pos] == '}' && depth == 0)) {
                if (s[pos] == '{' || s[pos] == '[') depth++;
                if (s[pos] == '}' || s[pos] == ']') depth--;
                pos++;
            }
        }
        while (pos < s.size() && strchr(" \t\n\r,", s[pos])) pos++;
    }
    if (pos < s.size() && s[pos] == '}') pos++;
    return result;
}

// 解析注册表索引 JSON
static std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
    parseRegistryIndex(const std::string& json) {
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> result;
    size_t pos = 0;
    while (pos < json.size() && json[pos] != '{') pos++;
    if (pos >= json.size()) return result;
    pos++;

    while (pos < json.size()) {
        while (pos < json.size() && strchr(" \t\n\r", json[pos])) pos++;
        if (pos >= json.size() || json[pos] == '}') break;
        if (json[pos] != '"') { pos++; continue; }
        std::string pkgName = jsonDecodeString(json, pos);
        while (pos < json.size() && strchr(" \t\n\r:", json[pos])) pos++;
        auto info = jsonParseFlatObject(json, pos);
        if (!pkgName.empty()) result[pkgName] = info;
        while (pos < json.size() && strchr(" \t\n\r,", json[pos])) pos++;
    }
    return result;
}

// ═══════════════════════════════════════════════════════════════════
// 配置
// ═══════════════════════════════════════════════════════════════════

struct Config {
    std::string packagesDir;
    std::string registryUrl;
    std::string githubToken;
    std::string registryCacheFile;
};

static Config loadConfig() {
    Config cfg;
    std::string home = getHomeDir();
    cfg.registryCacheFile = home + "\\.cpkg\\registry_url";
    cfg.packagesDir = home + "\\.cpkg\\packages";
    cfg.registryUrl = getEnv("CPKG_REGISTRY_URL",
        "https://raw.githubusercontent.com/mycplang/registry/main/index.json");
    cfg.githubToken = getEnv("CPKG_GITHUB_TOKEN", "");

    // 从文件读取备用 token
    if (cfg.githubToken.empty()) {
        cfg.githubToken = trim(readFile(home + "\\.cpkg\\token"));
    }

    // 从文件读取缓存的 registry URL
    std::string cachedUrl = trim(readFile(cfg.registryCacheFile));
    if (!cachedUrl.empty()) {
        cfg.registryUrl = cachedUrl;
    }

    ensureDir(home + "\\.cpkg");
    ensureDir(cfg.packagesDir);
    return cfg;
}

// ═══════════════════════════════════════════════════════════════════
// 包管理命令
// ═══════════════════════════════════════════════════════════════════

static std::string fetchRegistry(const Config& cfg) {
    std::string result;
    if (!cfg.githubToken.empty()) {
        result = httpRequest(cfg.registryUrl, cfg.githubToken);
    }
    if (result.empty()) {
        result = httpRequestUrlmon(cfg.registryUrl);
    }
    return result;
}

static std::string getPackageUrl(const std::string& name, const Config& cfg) {
    std::string json = fetchRegistry(cfg);
    if (json.empty()) return "";
    auto index = parseRegistryIndex(json);
    auto it = index.find(name);
    if (it == index.end()) return "";
    auto urlIt = it->second.find("url");
    return (urlIt != it->second.end()) ? urlIt->second : "";
}

#endif

#ifndef _WIN32
static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" 	
");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" 	
");
    return s.substr(start, end - start + 1);
}
#endif
static bool installPackage(const std::string& name, const Config& cfg) {
    std::cout << "▸ 正在安装: " << name << " ..." << std::endl;
    std::string targetDir = cfg.packagesDir + "\\" + name;
    std::string targetFile = targetDir + "\\index.cp";

    // 1. 本地 .cp 文件
    if (name.size() >= 3 && name.substr(name.size() - 3) == ".cp" && fs::exists(utf8ToWstr(name))) {
        ensureDir(targetDir);
        fs::copy_file(utf8ToWstr(name), utf8ToWstr(targetFile), fs::copy_options::overwrite_existing);
        std::cout << "  ✓ " << name << " 安装完成" << std::endl;
        return true;
    }

    // 2. URL
    if (name.substr(0, 7) == "http://" || name.substr(0, 8) == "https://") {
        ensureDir(targetDir);
        if (!httpDownload(name, targetFile, cfg.githubToken)) {
            std::cerr << "  ✗ 下载失败: " << name << std::endl;
            return false;
        }
        std::cout << "  ✓ " << name << " 安装完成" << std::endl;
        return true;
    }

    // 3. 注册表查询
    std::string pkgUrl = getPackageUrl(name, cfg);
    if (!pkgUrl.empty()) {
        ensureDir(targetDir);
        if (!httpDownload(pkgUrl, targetFile, cfg.githubToken)) {
            std::cerr << "  ✗ 下载失败: " << pkgUrl << std::endl;
            return false;
        }
        std::string meta = "{ \"name\": \"" + name + "\", \"source\": \"" + pkgUrl + "\" }\n";
        writeFile(targetDir + "\\package.json", meta);
        std::cout << "  ✓ " << name << " 安装完成" << std::endl;
        return true;
    }

    std::cerr << "  ✗ 未找到包: " << name << std::endl;
    return false;
}

static bool removePackage(const std::string& name, const Config& cfg) {
    std::string targetDir = cfg.packagesDir + "\\" + name;
    std::error_code ec;
    if (fs::remove_all(utf8ToWstr(targetDir), ec) > 0) {
        std::cout << "  ✓ " << name << " 已卸载" << std::endl;
        return true;
    }
    std::cerr << "  ✗ " << name << " 未安装" << std::endl;
    return false;
}

static bool listPackages(const Config& cfg) {
    std::cout << "已安装的包:" << std::endl;
    std::error_code ec;
    bool found = false;
    if (fs::exists(utf8ToWstr(cfg.packagesDir))) {
        for (const auto& entry : fs::directory_iterator(utf8ToWstr(cfg.packagesDir), ec)) {
            if (entry.is_directory()) {
                std::string pkgName = wstrToUtf8(entry.path().filename().wstring());
                std::cout << "  " << pkgName << std::endl;
                found = true;
            }
        }
    }
    if (!found) std::cout << "  (无)" << std::endl;
    return true;
}

static bool searchPackages(const std::string& query, const Config& cfg) {
    std::string json = fetchRegistry(cfg);
    if (json.empty()) {
        std::cerr << "  ✗ 无法连接注册表" << std::endl;
        return false;
    }
    auto index = parseRegistryIndex(json);
    if (index.empty()) {
        std::cout << "  注册表中没有可用包" << std::endl;
        return true;
    }
    std::cout << "注册表中的包:" << std::endl;
    for (const auto& [name, info] : index) {
        if (!query.empty()) {
            std::string lowerName = name;
            std::string lowerQuery = query;
            auto to_lower = [](unsigned char c) { return (char)std::tolower(c); };
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), to_lower);
            std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), to_lower);
            auto dit = info.find("desc");
            std::string desc = (dit != info.end()) ? dit->second : "";
            std::string lowerDesc = desc;
            std::transform(lowerDesc.begin(), lowerDesc.end(), lowerDesc.begin(), to_lower);
            if (lowerName.find(lowerQuery) == std::string::npos &&
                lowerDesc.find(lowerQuery) == std::string::npos) {
                continue;
            }
        }
        auto dit = info.find("desc");
        std::string desc = (dit != info.end()) ? dit->second : "";
        std::cout << "  " << name;
        if (!desc.empty()) std::cout << "  — " << desc;
        std::cout << std::endl;
    }
    return true;
}

static bool updatePackage(const std::string& name, const Config& cfg) {
    if (!name.empty()) {
        std::cout << "▸ 更新 " << name << " ..." << std::endl;
        removePackage(name, cfg);
        return installPackage(name, cfg);
    }
    std::cout << "▸ 更新所有已安装包..." << std::endl;
    std::error_code ec;
    bool allOk = true;
    if (fs::exists(utf8ToWstr(cfg.packagesDir))) {
        for (const auto& entry : fs::directory_iterator(utf8ToWstr(cfg.packagesDir), ec)) {
            if (entry.is_directory()) {
                std::string pkgName = wstrToUtf8(entry.path().filename().wstring());
                std::cout << "  ▸ " << pkgName << std::endl;
                removePackage(pkgName, cfg);
                if (!installPackage(pkgName, cfg)) allOk = false;
            }
        }
    }
    return allOk;
}

static bool showPackageInfo(const std::string& name, const Config& cfg) {
    std::string json = fetchRegistry(cfg);
    if (!json.empty()) {
        auto index = parseRegistryIndex(json);
        auto it = index.find(name);
        if (it != index.end()) {
            std::cout << "包名: " << name << std::endl;
            auto dit = it->second.find("desc");
            if (dit != it->second.end()) std::cout << "描述: " << dit->second << std::endl;
            auto vit = it->second.find("version");
            if (vit != it->second.end()) std::cout << "版本: " << vit->second << std::endl;
            auto uit = it->second.find("url");
            if (uit != it->second.end()) std::cout << "URL:  " << uit->second << std::endl;
            return true;
        }
    }
    std::string metaFile = cfg.packagesDir + "\\" + name + "\\package.json";
    if (fs::exists(utf8ToWstr(metaFile))) {
        std::cout << "包名: " << name << " (已安装)" << std::endl;
        return true;
    }
    std::cerr << "  ✗ 未找到包: " << name << std::endl;
    return false;
}

// ═══════════════════════════════════════════════════════════════════
// 主入口
// ═══════════════════════════════════════════════════════════════════

static void printHelp() {
    std::cout << R"(cpkg / 包 — CP语言包管理器

用法:
  cpkg install <包名>       安装包（支持本地.cp / 仓库名 / URL）
  包   安装  <包名>

  cpkg remove <包名>        卸载包
  包   卸载  <包名>

  cpkg search [关键词]      搜索包（无关键词列出全部）
  包   搜索  [关键词]

  cpkg list                 列出已安装的包
  包   列表

  cpkg update [包名]        更新包（无包名则更新全部）
  包   更新  [包名]

  cpkg info <包名>          查看包信息
  包   信息  <包名>

  cpkg registry [URL]       查看/设置注册表地址
  包   注册表 [URL]

环境变量:
  CPKG_GITHUB_TOKEN    GitHub token（访问私有仓库/提高 API 限频）
  CPKG_REGISTRY_URL    注册表地址

配置文件:
  %USERPROFILE%\.cpkg\  包管理器配置目录
  %USERPROFILE%\.cpkg\token   GitHub token（可选，优先级低于环境变量）
)" << std::endl;
}

// 获取宽字符命令行参数（支持中文）
static std::vector<std::string> getWideArgs() {
    std::vector<std::string> args;
    int argc = 0;
    LPWSTR* argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argvW) {
        for (int i = 0; i < argc; i++) {
            args.push_back(wstrToUtf8(argvW[i]));
        }
        LocalFree(argvW);
    }
    return args;
}

int main() {
    // 设置控制台输出为 UTF-8
    SetConsoleOutputCP(CP_UTF8);

    auto args = getWideArgs();

    if (args.size() < 2) {
        printHelp();
        return 0;
    }

    Config cfg = loadConfig();
    std::string cmd = args[1];

    if (cmd == "install" || cmd == "i" || cmd == "安装") {
        if (args.size() < 3) { std::cerr << "用法: cpkg install <包名>  或  包 安装 <包名>" << std::endl; return 1; }
        return installPackage(args[2], cfg) ? 0 : 1;
    }
    else if (cmd == "remove" || cmd == "rm" || cmd == "uninstall" || cmd == "卸载") {
        if (args.size() < 3) { std::cerr << "用法: cpkg remove <包名>  或  包 卸载 <包名>" << std::endl; return 1; }
        return removePackage(args[2], cfg) ? 0 : 1;
    }
    else if (cmd == "list" || cmd == "ls" || cmd == "列表") {
        return listPackages(cfg) ? 0 : 1;
    }
    else if (cmd == "search" || cmd == "s" || cmd == "搜索") {
        std::string query = (args.size() >= 3) ? args[2] : "";
        return searchPackages(query, cfg) ? 0 : 1;
    }
    else if (cmd == "update" || cmd == "up" || cmd == "更新") {
        std::string name = (args.size() >= 3) ? args[2] : "";
        return updatePackage(name, cfg) ? 0 : 1;
    }
    else if (cmd == "info" || cmd == "信息") {
        if (args.size() < 3) { std::cerr << "用法: cpkg info <包名>  或  包 信息 <包名>" << std::endl; return 1; }
        return showPackageInfo(args[2], cfg) ? 0 : 1;
    }
    else if (cmd == "registry" || cmd == "注册表") {
        if (args.size() >= 3) {
            writeFile(cfg.registryCacheFile, std::string(args[2]) + "\n");
            std::cout << "  注册表地址已设为: " << args[2] << std::endl;
            return 0;
        } else {
            std::cout << "  当前注册表: " << cfg.registryUrl << std::endl;
            return 0;
        }
    }
    else if (cmd == "help" || cmd == "--help" || cmd == "-h" || cmd == "帮助") {
        printHelp();
        return 0;
    }
    else {
        std::cerr << "未知命令: " << cmd << std::endl;
        std::cerr << "试试: cpkg help" << std::endl;
        return 1;
    }
}
