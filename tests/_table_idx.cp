函数 主() {
    变量 t = [["name", "CP"], ["version", 1]];
    打印(t);
    变量 v = t["name"];
    打印(v);
    t["newkey"] = 42;
    打印(t);
    打印(t["newkey"]);
    返回 0;
 }
主();
