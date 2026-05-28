函数 主() {
    变量 t = jsonParse("{\"name\":\"CP\",\"version\":1}");
    打印("jsonParse结果: " + jsonStringify(t));
    打印("表长: " + toString(表长(t)));
    打印("有name: " + toString(表有(t, "name")));
    打印("取name: " + 表取(t, "name"));
    打印("取version: " + toString(表取(t, "version")));
    表设(t, "newkey", 42);
    打印("取newkey: " + toString(表取(t, "newkey")));
    打印("键: " + jsonStringify(表键(t)));
    打印("OK");
    返回 0;
 }
主();
