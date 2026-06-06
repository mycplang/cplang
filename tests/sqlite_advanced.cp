func main() {
    打印("=== SQLite 深度功能测试 ===");

    db = 数据库打开(":memory:");
    数据库执行(db, "CREATE TABLE t (id INTEGER PRIMARY KEY, name TEXT, score INTEGER)");

    // 事务 + 批量插入
    数据库开始事务(db);
    for (i = 0; i < 1000; i++) {
        数据库执行(db, "INSERT INTO t VALUES(" + 转字符串(i+1) + ", '用户" + 转字符串(i+1) + "', " + 转字符串((i % 100) + 1) + ")");
    }
    数据库提交(db);
    打印("✅ 事务插入 1000 条");

    // 预编译参数化查询
    stmt = 数据库准备(db, "SELECT count(*) AS c FROM t WHERE score = ?");
    数据库绑定整数(stmt, 1, 42);
    if (数据库步进(stmt)) {
        r = 数据库读取行(stmt);
        打印("✅ score=42: " + 转字符串(r["c"]) + " 条");
    }
    数据库结束(stmt);

    // 按索引取列
    stmt2 = 数据库准备(db, "SELECT name, score FROM t WHERE id = ?");
    数据库绑定整数(stmt2, 1, 1);
    if (数据库步进(stmt2)) {
        name = 数据库取列(stmt2, 0);
        score = 数据库取列(stmt2, 1);
        打印("✅ 按索引取列: id=1 → " + name + ", " + 转字符串(score));
    }
    数据库结束(stmt2);

    // 多参数预编译
    stmt3 = 数据库准备(db, "SELECT count(*) AS c FROM t WHERE score >= ? AND score <= ?");
    数据库绑定整数(stmt3, 1, 90);
    数据库绑定整数(stmt3, 2, 95);
    if (数据库步进(stmt3)) {
        r = 数据库读取行(stmt3);
        打印("✅ score 90-95: " + 转字符串(r["c"]) + " 条");
    }
    数据库结束(stmt3);

    // 表结构
    info = 数据库表信息(db, "t");
    打印("✅ 列数: " + 转字符串(arrlen(info)));
    for (i = 0; i < arrlen(info); i++) {
        pk = info[i]["pk"];
        if (pk == 1) { pkStr = " PK"; } else { pkStr = ""; }
        打印("   " + info[i]["name"] + " (" + info[i]["type"] + ")" + pkStr);
    }

    // 总数
    rows3 = 数据库查询(db, "SELECT COUNT(*) AS cnt FROM t");
    打印("✅ 总数: " + 转字符串(rows3[0]["cnt"]));

    数据库关闭(db);
    打印("=== 全部通过 ===");
}
