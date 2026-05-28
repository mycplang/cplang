// ═══════════════════════════════════════════
//  Redis 客户端 — 纯 CP 语言实现
//  协议: RESP (Redis Serialization Protocol)
// ═══════════════════════════════════════════

// 发送 Redis 命令
函数 发送命令(fd, args) {
    // RESP: *<argc>\r\n$<len>\r\n<arg>\r\n...
    argc = 长度(args);
    cmd = "*" + argc + "\r\n";
    i = 0;
    当 (i < argc) {
        arg = args[i];
        cmd = cmd + "$" + 长度(arg) + "\r\n" + arg + "\r\n";
        i = i + 1;
    }
    TCP发送(fd, cmd);
}

// 读取 RESP 响应
函数 读取响应(fd) {
    data = TCP接收(fd, 4096);
    如果 (data == "") { 返回 "连接超时"; }
    // 简化：直接返回原始响应
    返回 data;
}

// ── Redis API ──
函数 redisConnect(host, port) {
    如果 (port == 0) { port = 6379; }
    return TCP连接(host, port);
}

函数 redisPing(fd) {
    发送命令(fd, ["PING"]);
    return 读取响应(fd);
}

函数 redisSet(fd, key, value) {
    发送命令(fd, ["SET", key, value]);
    return 读取响应(fd);
}

函数 redisGet(fd, key) {
    发送命令(fd, ["GET", key]);
    resp = 读取响应(fd);
    // RESP bulk string: $<len>\r\n<data>\r\n
    // 简化返回
    返回 resp;
}

函数 redisDel(fd, key) {
    发送命令(fd, ["DEL", key]);
    return 读取响应(fd);
}

函数 redisKeys(fd, pattern) {
    如果 (pattern == "") { pattern = "*"; }
    发送命令(fd, ["KEYS", pattern]);
    return 读取响应(fd);
}

函数 redisClose(fd) {
    TCP关闭(fd);
}
