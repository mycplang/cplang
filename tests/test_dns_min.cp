// minimal DNS test
打印("=== DNS Minimal ===")
结果 = dnsResolve("localhost")
打印("resolve: " + 转字符串(结果))

全 = dnsResolveAll("127.0.0.1")
打印("resolveAll: " + 转字符串(全))

打印("DNS_OK")
