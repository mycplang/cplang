// 矩阵与向量运算测试
打印("=== Matrix ===");

// 单位矩阵
变量 i2 = 单位矩阵(2);
打印("identity 2x2: ", jsonStringify(i2));

// 2x2 矩阵
变量 a = [[1, 2], [3, 4]];
变量 b = [[5, 6], [7, 8]];
变量 c = 矩阵加(a, b);
打印("add: ", jsonStringify(c));

// 矩阵乘法
变量 d = [[1, 2], [3, 4]];
变量 e = [[2, 0], [1, 2]];
变量 f = 矩阵乘(d, e);
打印("mul: ", jsonStringify(f));

// 矩阵转置
变量 t = 矩阵转置(a);
打印("transpose: ", jsonStringify(t));

// 行列式 (2x2: 1*4 - 2*3 = -2)
变量 det = 矩阵行列式(a);
打印("det 2x2: ", det);

// 向量点积
变量 v1 = [1, 2, 3];
变量 v2 = [4, 5, 6];
变量 dot = 向量点积(v1, v2);
打印("dot: ", dot);

// 向量叉积 (3D)
变量 cr = 向量叉积(v1, v2);
打印("cross: ", jsonStringify(cr));

// 向量模
变量 nm = 向量模([3, 4]);
打印("norm [3,4]: ", nm);

// 向量归一化
变量 un = 向量归一化([3, 4, 0]);
打印("normalize: ", jsonStringify(un));

打印("MATRIX_OK");