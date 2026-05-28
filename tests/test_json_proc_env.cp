打印("=== JSON ===");
变量 j1 = jsonParse("42");
打印(j1);
变量 j2 = jsonParse("3.14");
打印(j2);
变量 j3 = jsonParse("\"hello\\nworld\"");
打印(j3);
变量 j4 = jsonParse("true");
打印(j4);
变量 j5 = jsonParse("null");
打印(j5);
变量 j6 = jsonParse("[1, 2, 3]");
打印(j6);
变量 j7 = jsonParse("{\"a\":1,\"b\":\"x\"}");
打印(j7);

打印("=== JSON stringify ===");
打印(jsonStringify(42));
打印(jsonStringify(true));
打印(jsonStringify("hello"));
变量 arr = [1, 2, 3];
打印(jsonStringify(arr));
变量 obj = [["a", 1], ["b", 2]];
打印(jsonStringify(obj));

打印("=== Process ===");
变量 out = procExec("echo OK");
打印(out);
变量 rc = procSystem("exit 0");
打印(rc);

打印("=== Environment ===");
变量 path = envGet("PATH");
打印(path);
打印(envSet("CP_TEST_VAR", "hello123"));
变量 val = envGet("CP_TEST_VAR");
打印(val);

打印("ALL_THREE_OK");
