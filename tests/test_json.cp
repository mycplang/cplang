// JSON 解析/序列化测试
打印("=== JSON ===");

// jsonParse
变量 obj = jsonParse("{\"name\":\"CP\",\"version\":1}");
打印("parse obj: ", type(obj));
打印("name: ", obj["name"]);
打印("version: ", obj["version"]);

// 数组
变量 arr = jsonParse("[1, 2, 3, 4]");
打印("parse arr: ", type(arr));
打印("arr[0]: ", arr[0]);
打印("arr[3]: ", arr[3]);

// 嵌套
变量 nested = jsonParse("{\"items\":[1,2],\"meta\":{\"ok\":true}}");
打印("nested items[1]: ", nested["items"][1]);
打印("nested meta.ok: ", nested["meta"]["ok"]);

// jsonStringify
变量 simple = {a: 1, b: "hello"};
变量 str = jsonStringify(simple);
打印("stringify: ", str);

// jsonPretty
变量 pretty = jsonPretty(simple);
打印("pretty: ", pretty);

// jsonValidate
打印("validate valid: ", jsonValidate("{\"x\":1}"));
打印("validate invalid: ", jsonValidate("{x:1}"));

打印("JSON_OK");