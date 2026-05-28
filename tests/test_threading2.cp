打印("=== Future ===");
变量 f = futureGo(abs, -42);
循环 (变量 i = 0; i < 100; i = i + 1) {
    如果 (futureIsReady(f)) { 跳出; }
    threadYield();
}
打印(futureGet(f));

打印("=== RWLock ===");
变量 rwl = rwLockCreate();
rwLockRead(rwl);
rwLockUnlock(rwl);
rwLockWrite(rwl);
rwLockUnlock(rwl);
// Multiple readers
rwLockRead(rwl);
rwLockRead(rwl);
rwLockUnlock(rwl);
rwLockUnlock(rwl);
打印("RWLock_OK");

打印("=== TLS ===");
tlsSet("key1", 123);
tlsSet("key2", "hello");
打印(tlsGet("key1"));
打印(tlsGet("key2"));
打印(tlsGet("noexist"));

打印("=== Channel ===");
变量 ch = channelCreate(3);
打印(channelSend(ch, 10));
打印(channelSend(ch, 20));
打印(channelSend(ch, 30));
打印(channelRecv(ch));
打印(channelRecv(ch));
打印(channelRecv(ch));
// Close + behavior
打印(channelSend(ch, 40));
channelClose(ch);
打印(channelSend(ch, 50));
打印(channelRecv(ch));
打印(channelRecv(ch));

打印("THREADING2_OK");
