变量 r = tcpConnect("120.48.128.250", 6379);
打印("tcpConnect=" + r);
如果 (r >= 0) { tcpClose(r); }

变量 r2 = redisConnect("127.0.0.1", 6379);
打印("redisConnect=" + r2);

变量 r3 = mysqlConnect("120.48.128.250", "root", "ClawMall_2026!", "claw_mall_test", 3306);
打印("mysqlConnect=" + r3);

变量 r4 = pgConnect("host=120.48.128.250 port=5432 dbname=claw_mall user=claw_mall password=ClawMall_2026!");
打印("pgConnect=" + r4);
