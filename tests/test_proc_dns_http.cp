// test_proc_dns_http.cp - test new subprocess pipes, DNS, HTTP
// Note: uses simple print calls to avoid register allocation bug
// (string concatenation after native calls can corrupt registers)

打印("=== DNS ===")

打印(dnsResolve("127.0.0.1"))
打印(dnsReverse("::1"))
打印(dnsResolve("invalid.host.xyz.test"))
打印(dnsResolveAll("127.0.0.1"))

打印("=== Process ===")

打印(procExec("echo hello"))
打印(procStderr("echo err 1>&2"))
打印(procRun("echo runtest"))
打印(procSystem("echo x"))

打印("ALL_PASS")
