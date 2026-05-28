打印("before sqlite")
db = sqliteOpen(":memory:")
打印("after sqlite")
打印(isNil(db))
