打印("=== 数学特殊函数 ===");
打印("erf(1):", erf(1));
打印("tgamma(5):", tgamma(5));

打印("=== 有序Map ===");
变量 mp = mapNew();
mapInsert(mp, "a", 1);
mapInsert(mp, "b", 2);
打印(mapSize(mp));
打印(mapFind(mp, "a"));
打印(mapContains(mp, "c"));
打印(mapKeys(mp));
打印(mapValues(mp));

打印("=== 缺失算法 ===");
变量 arr = [5, 2, 8, 1, 9];
stableSort(arr);
打印(arr);

函数 大于三(x) { 返回 x > 3; }
打印("countIf:", countIf(arr, 大于三));

函数 翻倍(x) { 返回 x * 2; }
打印("arrFilter:", arrFilter(arr, 大于三));

打印("=== result monad ===");
变量 r = resOk(42);
r2 = resMap(r, 翻倍);
打印(resUnwrap(r2));

打印("ALL_P0P1_OK");
