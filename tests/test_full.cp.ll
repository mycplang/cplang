; CP语言生成的LLVM IR
target triple = "x86_64-pc-windows-msvc"

; 外部函数声明
declare i32 @printf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)

; 结构体定义

; 函数定义
define i64 @阶乘(i64 %n) {
entry:
  %n.addr = alloca i64
  store i64 %n, i64* %n.addr
  %t0 = load i64, i64* %n
  %t1 = icmp sle i64 %t0, 1
  %t1 = zext i1 %t1 to i64
  %t2 = icmp ne i64 %t1, 0
  br i1 %t2, label %then0, label %endif2
then0:
  ret i64 1
  br label %endif2
endif2:
  %t3 = load i64, i64* %n
  %t4 = load i64, i64* %n
  %t5 = sub i64 %t4, 1
  %t6 = call i64 @阶乘(i64 %t5)
  %t7 = mul i64 %t3, %t6
  ret i64 %t7
}

define i64 @数组求和(i64 %arr, i64 %n) {
entry:
  %arr.addr = alloca i64
  store i64 %arr, i64* %arr.addr
  %n.addr = alloca i64
  store i64 %n, i64* %n.addr
  %sum = alloca i64
  store i64 0, i64* %sum
  %i = alloca i64
  store i64 0, i64* %i
  br label %while.cond3
while.cond3:
  %t8 = load i64, i64* %i
  %t9 = load i64, i64* %n
  %t10 = icmp slt i64 %t8, %t9
  %t10 = zext i1 %t10 to i64
  %t11 = icmp ne i64 %t10, 0
  br i1 %t11, label %while.body4, label %while.end5
while.body4:
  %t12 = load i64, i64* %sum
  %t13 = load i64, i64* %i
  %t14 = getelementptr i64, i64* %arr.addr, i64 %t13
  %t15 = load i64, i64* %t14
  %t16 = add i64 %t12, %t15
  store i64 %t16, i64* %sum
  %t17 = load i64, i64* %i
  %t18 = add i64 %t17, 1
  store i64 %t18, i64* %i
  br label %while.cond3
while.end5:
  %t19 = load i64, i64* %sum
  ret i64 %t19
}

define i64 @查找元素(i64 %arr, i64 %n, i64 %target) {
entry:
  %arr.addr = alloca i64
  store i64 %arr, i64* %arr.addr
  %n.addr = alloca i64
  store i64 %n, i64* %n.addr
  %target.addr = alloca i64
  store i64 %target, i64* %target.addr
  %i = alloca i64
  store i64 0, i64* %i
  br label %while.cond6
while.cond6:
  %t20 = load i64, i64* %i
  %t21 = load i64, i64* %n
  %t22 = icmp slt i64 %t20, %t21
  %t22 = zext i1 %t22 to i64
  %t23 = icmp ne i64 %t22, 0
  br i1 %t23, label %while.body7, label %while.end8
while.body7:
  %t24 = load i64, i64* %i
  %t25 = getelementptr i64, i64* %arr.addr, i64 %t24
  %t26 = load i64, i64* %t25
  %t27 = load i64, i64* %target
  %t28 = icmp eq i64 %t26, %t27
  %t28 = zext i1 %t28 to i64
  %t29 = icmp ne i64 %t28, 0
  br i1 %t29, label %then9, label %endif11
then9:
  %t30 = load i64, i64* %i
  ret i64 %t30
  br label %endif11
endif11:
  %t31 = load i64, i64* %i
  %t32 = add i64 %t31, 1
  store i64 %t32, i64* %i
  br label %while.cond6
while.end8:
  %t33 = sub i64 0, 1
  ret i64 %t33
}

define i64 @斐波那契(i64 %n) {
entry:
  %n.addr = alloca i64
  store i64 %n, i64* %n.addr
  %t34 = load i64, i64* %n
  %t35 = icmp sle i64 %t34, 0
  %t35 = zext i1 %t35 to i64
  %t36 = icmp ne i64 %t35, 0
  br i1 %t36, label %then12, label %endif14
then12:
  ret i64 0
  br label %endif14
endif14:
  %t37 = load i64, i64* %n
  %t38 = icmp eq i64 %t37, 1
  %t38 = zext i1 %t38 to i64
  %t39 = icmp ne i64 %t38, 0
  br i1 %t39, label %then15, label %endif17
then15:
  ret i64 1
  br label %endif17
endif17:
  %a = alloca i64
  store i64 0, i64* %a
  %b = alloca i64
  store i64 1, i64* %b
  %i = alloca i64
  store i64 2, i64* %i
  br label %while.cond18
while.cond18:
  %t40 = load i64, i64* %i
  %t41 = load i64, i64* %n
  %t42 = icmp sle i64 %t40, %t41
  %t42 = zext i1 %t42 to i64
  %t43 = icmp ne i64 %t42, 0
  br i1 %t43, label %while.body19, label %while.end20
while.body19:
  %t44 = load i64, i64* %a
  %t45 = load i64, i64* %b
  %t46 = add i64 %t44, %t45
  %temp = alloca i64
  store i64 %t46, i64* %temp
  %t47 = load i64, i64* %b
  store i64 %t47, i64* %a
  %t48 = load i64, i64* %temp
  store i64 %t48, i64* %b
  %t49 = load i64, i64* %i
  %t50 = add i64 %t49, 1
  store i64 %t50, i64* %i
  br label %while.cond18
while.end20:
  %t51 = load i64, i64* %b
  ret i64 %t51
}

define i64 @最大值(i64 %a, i64 %b, i64 %c) {
entry:
  %a.addr = alloca i64
  store i64 %a, i64* %a.addr
  %b.addr = alloca i64
  store i64 %b, i64* %b.addr
  %c.addr = alloca i64
  store i64 %c, i64* %c.addr
  %t52 = load i64, i64* %a
  %max = alloca i64
  store i64 %t52, i64* %max
  %t53 = load i64, i64* %b
  %t54 = load i64, i64* %max
  %t55 = icmp sgt i64 %t53, %t54
  %t55 = zext i1 %t55 to i64
  %t56 = icmp ne i64 %t55, 0
  br i1 %t56, label %then21, label %endif23
then21:
  %t57 = load i64, i64* %b
  store i64 %t57, i64* %max
  br label %endif23
endif23:
  %t58 = load i64, i64* %c
  %t59 = load i64, i64* %max
  %t60 = icmp sgt i64 %t58, %t59
  %t60 = zext i1 %t60 to i64
  %t61 = icmp ne i64 %t60, 0
  br i1 %t61, label %then24, label %endif26
then24:
  %t62 = load i64, i64* %c
  store i64 %t62, i64* %max
  br label %endif26
endif26:
  %t63 = load i64, i64* %max
  ret i64 %t63
}

define i64 @主() {
entry:
  %t64 = call i64 @阶乘(i64 5)
  %f = alloca i64
  store i64 %t64, i64* %f
  %t65 = call i64 @斐波那契(i64 10)
  %fib = alloca i64
  store i64 %t65, i64* %fib
  %t66 = call i64 @最大值(i64 10, i64 25, i64 15)
  %m = alloca i64
  store i64 %t66, i64* %m
  %t67 = load i64, i64* %f
  %t68 = load i64, i64* %fib
  %t69 = add i64 %t67, %t68
  %t70 = load i64, i64* %m
  %t71 = add i64 %t69, %t70
  ret i64 %t71
}

