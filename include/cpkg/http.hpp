// CP Language — 跨平台 HTTP 客户端（轻量封装）
// Windows: WinHTTP  |  Linux/macOS: POSIX sockets + SSL
#pragma once
#include <string>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <winhttp.h>
  #include <urlmon.h>
  #include <shlobj.h>
  #pragma comment(lib, "winhttp.lib")
  #pragma comment(lib, "urlmon.lib")
#else
  #include <sys/socket.h>
  #include <netdb.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <sys/stat.h>
  #include <errno.h>
#endif

namespace cpkg {

// ═══════════════════════════════════════════════════════════════════
//  跨平台 UTF-8 字符串转换（Windows 需要，Linux 原生 UTF-8）
// ═══════════════════════════════════════════════════════════════════

inline std::string narrow(const std::string& s) { return s; }

#ifdef _WIN32
inline std::string wstrToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string result((size_t)len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), result.data(), len, nullptr, nullptr);
    return result;
}
inline std::wstring utf8ToWstr(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    if (len <= 0) return {};
    std::wstring r((size_t)len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), r.data(), len);
    return r;
}
#endif

// ═══════════════════════════════════════════════════════════════════
//  跨平台环境变量
// ═══════════════════════════════════════════════════════════════════

inline std::string getEnv(const std::string& key, const std::string& def = "") {
#ifdef _WIN32
    std::wstring wkey = utf8ToWstr(key);
    DWORD len = GetEnvironmentVariableW(wkey.c_str(), nullptr, 0);
    if (len == 0) return def;
    std::wstring wval(len, L'\0');
    GetEnvironmentVariableW(wkey.c_str(), wval.data(), len);
    return wstrToUtf8(wval);
#else
    const char* val = ::getenv(key.c_str());
    return val ? std::string(val) : def;
#endif
}

// ═══════════════════════════════════════════════════════════════════
//  跨平台用户主目录
// ═══════════════════════════════════════════════════════════════════

inline std::string getHomeDir() {
#ifdef _WIN32
    wchar_t buf[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, buf)))
        return wstrToUtf8(buf);
    return getEnv("USERPROFILE", "C:\\Users\\Default");
#else
    const char* home = ::getenv("HOME");
    return home ? std::string(home) : "/tmp";
#endif
}

// ═══════════════════════════════════════════════════════════════════
//  跨平台 HTTP GET（返回响应体字符串）
// ═══════════════════════════════════════════════════════════════════

inline std::string httpGet(const std::string& url, const std::string& token = "") {
#ifdef _WIN32
    // ── Windows: WinHTTP ──
    std::wstring wurl = utf8ToWstr(url);
    URL_COMPONENTS urlComp = { sizeof(URL_COMPONENTS) };
    wchar_t host[256] = {0}, path[1024] = {0};
    urlComp.lpszHostName = host;  urlComp.dwHostNameLength = 256;
    urlComp.lpszUrlPath = path;   urlComp.dwUrlPathLength = 1024;
    urlComp.dwSchemeLength = (DWORD)-1;
    WinHttpCrackUrl(wurl.c_str(), 0, 0, &urlComp);

    HINTERNET hSession = WinHttpOpen(L"cpkg/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                      WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return "";

    HINTERNET hConnect = WinHttpConnect(hSession, host, urlComp.nPort, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return ""; }

    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path, NULL,
                                             WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return ""; }

    if (!token.empty()) {
        std::string hdr = "Authorization: Bearer " + token;
        std::wstring whdr = utf8ToWstr(hdr);
        WinHttpAddRequestHeaders(hRequest, whdr.c_str(), (DWORD)whdr.size(),
                                  WINHTTP_ADDREQ_FLAG_ADD);
    }

    std::string result;
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                           WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hRequest, NULL)) {
        DWORD avail = 0;
        while (WinHttpQueryDataAvailable(hRequest, &avail) && avail > 0) {
            std::string chunk((size_t)avail, '\0');
            DWORD read = 0;
            if (WinHttpReadData(hRequest, chunk.data(), avail, &read))
                result.append(chunk.data(), read);
        }
    }
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return result;

#else
    // ── Linux/macOS: POSIX sockets ──
    // 解析 URL
    std::string host, path = "/";
    int port = 80;
    bool useTls = false;

    size_t schemeEnd = url.find("://");
    size_t hostStart = (schemeEnd != std::string::npos) ? schemeEnd + 3 : 0;
    if (url.compare(0, schemeEnd, "https") == 0) { useTls = true; port = 443; }

    size_t pathStart = url.find('/', hostStart);
    size_t portStart = url.find(':', hostStart);
    if (portStart != std::string::npos && (pathStart == std::string::npos || portStart < pathStart)) {
        host = url.substr(hostStart, portStart - hostStart);
        size_t portEnd = pathStart != std::string::npos ? pathStart : url.size();
        port = std::stoi(url.substr(portStart + 1, portEnd - portStart - 1));
    } else if (pathStart != std::string::npos) {
        host = url.substr(hostStart, pathStart - hostStart);
    } else {
        host = url.substr(hostStart);
    }
    if (pathStart != std::string::npos) path = url.substr(pathStart);

    // TLS 暂不实现，仅支持 HTTP
    if (useTls) {
        // 回退：尝试用系统命令 curl（简单可靠）
        std::string cmd = "curl -sL";
        if (!token.empty()) cmd += " -H 'Authorization: Bearer " + token + "'";
        cmd += " '" + url + "'";
        FILE* fp = popen(cmd.c_str(), "r");
        if (!fp) return "";
        std::string result;
        char buf[4096];
        while (fgets(buf, sizeof(buf), fp)) result += buf;
        pclose(fp);
        return result;
    }

    // 纯 HTTP：用 POSIX socket
    struct addrinfo hints = {}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    char portStr[16];
    snprintf(portStr, sizeof(portStr), "%d", port);

    if (getaddrinfo(host.c_str(), portStr, &hints, &res) != 0) return "";
    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) { freeaddrinfo(res); return ""; }

    struct timeval tv = {10, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
        close(sock); freeaddrinfo(res); return "";
    }
    freeaddrinfo(res);

    std::string req = "GET " + path + " HTTP/1.1\r\nHost: " + host +
                      "\r\nUser-Agent: cpkg/1.0\r\nConnection: close\r\n";
    if (!token.empty()) req += "Authorization: Bearer " + token + "\r\n";
    req += "\r\n";

    send(sock, req.c_str(), req.size(), 0);

    std::string result;
    char buf[4096];
    int n;
    while ((n = recv(sock, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        result += buf;
    }
    close(sock);

    // 剥离 HTTP 头
    size_t bodyPos = result.find("\r\n\r\n");
    if (bodyPos != std::string::npos) result = result.substr(bodyPos + 4);
    return result;
#endif
}

// ═══════════════════════════════════════════════════════════════════
//  跨平台 HTTP 下载文件
// ═══════════════════════════════════════════════════════════════════

inline bool httpDownload(const std::string& url, const std::string& dest,
                         const std::string& token = "") {
#ifdef _WIN32
    std::wstring wurl = utf8ToWstr(url);
    std::wstring wdest = utf8ToWstr(dest);
    HRESULT hr = URLDownloadToFileW(NULL, wurl.c_str(), wdest.c_str(), 0, NULL);
    return SUCCEEDED(hr);
#else
    std::string cmd = "curl -sL";
    if (!token.empty()) cmd += " -H 'Authorization: Bearer " + token + "'";
    cmd += " -o '" + dest + "' '" + url + "'";
    return system(cmd.c_str()) == 0;
#endif
}

} // namespace cpkg
