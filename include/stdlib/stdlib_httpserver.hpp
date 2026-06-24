#pragma once
#include "vm/value.hpp"
#include <vector>
#include <string>

namespace cplang {

// ═══════════════════════════════════════════════════════════
//  HTTP 服务端（基于 Mongoose）— v0.5.0 回调桥接
// ═══════════════════════════════════════════════════════════

namespace httpserver {
    // 启动静态文件服务器：端口, 根目录
    Value startFileServer(std::vector<Value>& args);

    // 启动简单API服务器：端口
    Value startAPIServer(std::vector<Value>& args);

    // v0.5.0: 启动回调服务器 — CP 函数处理每个 HTTP 请求
    //  回调签名: function(请求表) → 响应表
    //  请求表: { method, path, query, headers, body }
    //  响应表: { status, content_type, body }
    Value startCallbackServer(std::vector<Value>& args);

    // 停止服务器
    Value stop(std::vector<Value>& args);
}

} // namespace cplang