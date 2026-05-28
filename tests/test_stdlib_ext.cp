打印("=== 算法扩展 ===");
arr = [1, 2, 3, 4, 5];
打印("accumulate:", accumulate(arr, 0));
打印("product:", product(arr));
打印("anyOf:", anyOf([0, 0, 1]));
打印("allOf:", allOf([1, 2, 3]));
打印("noneOf:", noneOf([0, 0, 0]));

打印("\n=== 数学 ===");
打印("erf(1):", erf(1));
打印("tgamma(5):", tgamma(5));

打印("\n=== 字符串 ===");
打印("strCompareIC:", strCompareIC("Hello", "hello"));
打印("strIsBlank:", strIsBlank("   "));

打印("\n=== 数组 ===");
打印("arrTake:", arrTake(arr, 3));
打印("arrDrop:", arrDrop(arr, 2));

打印("\n测试完成");