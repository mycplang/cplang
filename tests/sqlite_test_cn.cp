// CP 语言 SQLite 数据库功能测试
// 使用中文别名 API
func main() {
    打印("=== SQLite 数据库测试 ===");

    // 打开内存数据库
    db = 数据库打开(":memory:");
    如果 (isNil(db)) {
        打印("打开数据库失败: " + 数据库错误(db));
        返回;
    }
    打印("✅ 数据库已打开");

    // 建表
    ok = 数据库执行(db, "CREATE TABLE 用户 (id INTEGER PRIMARY KEY, 姓名 TEXT, 年龄 INTEGER)");
    打印("✅ 建表: " + 转字符串(ok));

    // 插入数据
    数据库执行(db, "INSERT INTO 用户 VALUES(1, '张三', 28)");
    数据库执行(db, "INSERT INTO 用户 VALUES(2, '李四', 35)");
    数据库执行(db, "INSERT INTO 用户 VALUES(3, '王五', 22)");
    打印("✅ 插入 3 条数据");
    打印("   最后插入ID: " + 转字符串(数据库最后插入ID(db)));

    // 查询
    rows = 数据库查询(db, "SELECT * FROM 用户");
    打印("✅ 查询到 " + 转字符串(arrlen(rows)) + " 条记录");

    // 遍历结果
    for (i = 0; i < arrlen(rows); i++) {
        row = rows[i];
        打印("   第" + 转字符串(i+1) + "行: " + 转字符串(row));
    }

    // 条件查询
    rows2 = 数据库查询(db, "SELECT 姓名, 年龄 FROM 用户 WHERE 年龄 > 25");
    打印("✅ 年龄>25: " + 转字符串(arrlen(rows2)) + " 人");
    for (i = 0; i < arrlen(rows2); i++) {
        打印("   " + rows2[i]["姓名"] + " (" + 转字符串(rows2[i]["年龄"]) + "岁)");
    }

    // 关闭
    数据库关闭(db);
    打印("✅ 数据库已关闭");
    打印("=== 全部通过 ===");
}
