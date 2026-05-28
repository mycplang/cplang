// test_sqlite.cp - SQLite debug
db = sqliteOpen(":memory:")
打印("open nil=")
打印(isNil(db))
打印("err=")
打印(sqliteErrMsg(db))

ok = sqliteExec(db, "CREATE TABLE t(x)")
打印("create=")
打印(ok)
打印("err2=")
打印(sqliteErrMsg(db))

ok = sqliteExec(db, "INSERT INTO t VALUES(1)")
打印("insert=")
打印(ok)
打印("lastId=")
打印(sqliteLastInsertId(db))

rows = sqliteQuery(db, "SELECT * FROM t")
打印("rows=")
打印(arrlen(rows))
打印("err3=")
打印(sqliteErrMsg(db))

sqliteClose(db)
打印("DONE")
