打印("--- 范围迭代 ---");
变量 ri = iterRange(0, 10);
变量 sum = 0;
变量 cnt = 0;
当 (iterHasNext(ri)) {
    sum = sum + iterNext(ri);
    cnt = cnt + 1;
}
打印(sum);
打印(cnt);

// 带步长
变量 rs = iterRange(0, 10, 2);
变量 cnt2 = 0;
当 (iterHasNext(rs)) {
    cnt2 = cnt2 + 1;
    iterNext(rs);
}
打印(cnt2);

// 递减
变量 rd = iterRange(9, -1, -1);
变量 cnt3 = 0;
当 (iterHasNext(rd)) {
    cnt3 = cnt3 + 1;
    iterNext(rd);
}
打印(cnt3);

打印("--- 反向迭代 ---");
变量 arr = [10, 20, 30, 40];
变量 rix = iterReverse(arr);
// 应输出 40, 30, 20, 10
打印(iterNext(rix));
打印(iterNext(rix));
打印(iterNext(rix));
打印(iterNext(rix));

打印("--- 位置/剩余 ---");
变量 it = iter(arr);
打印(iterPos(it));      // 0
打印(iterRemaining(it)); // 4
iterNext(it);
打印(iterPos(it));      // 1
打印(iterRemaining(it)); // 3

打印("--- 跳过 ---");
变量 it2 = iter(arr);
iterSkip(it2, 2);
打印(iterNext(it2));    // 30

打印("--- 预览 ---");
变量 it3 = iter(arr);
打印(iterPeek(it3));    // 10
打印(iterPeek(it3));    // 10 (不改位置)
iterNext(it3);
打印(iterPeek(it3));    // 20

打印("ITER_EXT_OK");
