// HTTP 客户端测试
打印("=== HTTP ===");

// httpGet - 测试编译不测试实际网络
打印("httpGet: ", httpGet("http://httpbin.org/get"));

// 如果需要实际测试，取消注释
// 打印("download: ", httpDownload("http://httpbin.org/ip"));
// 打印("post: ", httpPost("http://httpbin.org/post", "data=test"));

打印("HTTP_OK");