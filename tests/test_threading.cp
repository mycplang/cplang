打印("=== 互斥锁 ===");
变量 m = mutexCreate();
mutexLock(m);
打印(mutexTryLock(m));
mutexUnlock(m);
打印(mutexTryLock(m));
mutexUnlock(m);

打印("=== 原子整数 ===");
变量 a = atomicInt(42);
打印(atomicLoad(a));
atomicAdd(a, 8);
打印(atomicLoad(a));
打印(atomicCAS(a, 50, 99));
打印(atomicLoad(a));
打印(atomicCAS(a, 50, 77));
atomicStore(a, 0);
打印(atomicExchange(a, 1));
打印(atomicLoad(a));

打印("=== 信号量 ===");
变量 s = semCreate(0);
打印(semTryWait(s));
semPost(s);
打印(semTryWait(s));

打印("=== 屏障 ===");
变量 b = barrierCreate(1);
barrierWait(b);
打印(1);

打印("=== 基本工具 ===");
打印(threadHw());
打印(threadId());
mutexLock(m);
mutexUnlock(m);

打印("=== 条件变量 ===");
变量 cv = condCreate();
打印(1);

打印("THREADING_OK");
