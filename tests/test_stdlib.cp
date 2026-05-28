// 标准库确定性测试用例

// ===== 数学函数 =====
打印("abs(-10) = ", abs(-10));
打印("abs(-3.14) = ", abs(-3.14));
打印("sqrt(16) = ", sqrt(16));
打印("pow(2, 10) = ", pow(2, 10));
打印("floor(3.7) = ", floor(3.7));
打印("ceil(3.2) = ", ceil(3.2));
打印("round(3.5) = ", round(3.5));
打印("round(3.4) = ", round(3.4));

// ===== 字符串函数 =====
变量 s = "Hello, World!";
打印("len = ", len(s));
打印("substr = ", substr(s, 7, 5));
打印("concat = ", concat(s, " CP!"));
打印("find World = ", find(s, "World"));
打印("find XYZ = ", find(s, "XYZ"));
打印("lower = ", lower(s));
打印("upper = ", upper(s));

// ===== 数组函数 =====
变量 arr = [10, 20, 30];
打印("len = ", len(arr));
push(arr, 40);
打印("push 40 = ", arr);
变量 v = pop(arr);
打印("pop = ", v, " arr = ", arr);
insert(arr, 1, 15);
打印("insert 15 at 1 = ", arr);
变量 r = remove(arr, 2);
打印("remove idx 2 = ", r, " arr = ", arr);
变量 sub = slice(arr, 1, 3);
打印("slice [1,3) = ", sub);
