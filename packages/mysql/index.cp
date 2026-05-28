// ═══════════════════════════════════════════
//  MySQL 客户端 — 纯 CP 语言实现
//  协议: MySQL Client/Server Protocol
// ═══════════════════════════════════════════

函数 mysqlConnect(host, port, user, password, db) {
    如果 (port == 0) { port = 3306; }
    fd = TCP连接(host, port);
    如果 (fd < 0) { 返回 fd; }
    
    // 读取 MySQL 握手包
    handshake = TCP接收(fd, 4096);
    如果 (!字符串包含(handshake, "mysql")) { TCP关闭(fd); 返回 -1; }
    
    // 发送认证包 (简化: 使用 mysql_native_password)
    // 实际实现需要解析握手包中的 salt 并发送正确认证
    // 此处用占位, 生产环境用 C 库或 mysql CLI
    
    返回 fd;
}

函数 mysqlQuery(fd, sql) {
    // 发送 COM_QUERY
    // 协议: [length:3][sequence:1][command:1=COM_QUERY][sql]
    len = 长度(sql) + 1;
    b0 = len & 0xFF;
    b1 = (len >> 8) & 0xFF;
    b2 = (len >> 16) & 0xFF;
    // CP 无二进制, 用 procSystem mysql CLI
    打印("mysql query via CLI: " + sql);
    返回 "";
}

函数 mysqlClose(fd) {
    TCP关闭(fd);
}
