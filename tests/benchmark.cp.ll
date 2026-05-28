; CP语言生成的LLVM IR
target triple = "x86_64-pc-windows-msvc"

; 外部函数声明
declare i32 @printf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)

; 结构体定义

; 函数定义
define i64 @fib(i64 %n) {
entry:
  %n.addr = alloca i64
  store i64 %n, i64* %n.addr
  %t0 = load i64, i64* %n
  %t1 = icmp sle i64 %t0, 1
  %t1 = zext i1 %t1 to i64
  %t2 = icmp ne i64 %t1, 0
  br i1 %t2, label %then0, label %endif2
then0:
  %t3 = load i64, i64* %n
  ret i64 %t3
endif2:
  %t4 = load i64, i64* %n
  %t5 = sub i64 %t4, 1
  %t6 = call i64 @fib(i64 %t5)
  %t7 = load i64, i64* %n
  %t8 = sub i64 %t7, 2
  %t9 = call i64 @fib(i64 %t8)
  %t10 = add i64 %t6, %t9
  ret i64 %t10
}

define i64 @arraySum(i64 %n) {
entry:
  %n.addr = alloca i64
  store i64 %n, i64* %n.addr
  %sum = alloca i64
  store i64 0, i64* %sum
  %i = alloca i64
  store i64 0, i64* %i
  br label %while.cond3
while.cond3:
  %t11 = load i64, i64* %i
  %t12 = load i64, i64* %n
  %t13 = icmp slt i64 %t11, %t12
  %t13 = zext i1 %t13 to i64
  %t14 = icmp ne i64 %t13, 0
  br i1 %t14, label %while.body4, label %while.end5
while.body4:
  %t15 = load i64, i64* %sum
  %t16 = load i64, i64* %i
  %t17 = add i64 %t15, %t16
  store i64 %t17, i64* %sum
  %t18 = load i64, i64* %i
  %t19 = add i64 %t18, 1
  store i64 %t19, i64* %i
  br label %while.cond3
while.end5:
  %t20 = load i64, i64* %sum
  ret i64 %t20
}

define i64 @factorial(i64 %n) {
entry:
  %n.addr = alloca i64
  store i64 %n, i64* %n.addr
  %t21 = load i64, i64* %n
  %t22 = icmp sle i64 %t21, 1
  %t22 = zext i1 %t22 to i64
  %t23 = icmp ne i64 %t22, 0
  br i1 %t23, label %then6, label %endif8
then6:
  ret i64 1
  br label %endif8
endif8:
  %t24 = load i64, i64* %n
  %t25 = load i64, i64* %n
  %t26 = sub i64 %t25, 1
  %t27 = call i64 @factorial(i64 %t26)
  %t28 = mul i64 %t24, %t27
  ret i64 %t28
}

define i64 @isPrime(i64 %n) {
entry:
  %n.addr = alloca i64
  store i64 %n, i64* %n.addr
  %t29 = load i64, i64* %n
  %t30 = icmp slt i64 %t29, 2
  %t30 = zext i1 %t30 to i64
  %t31 = icmp ne i64 %t30, 0
  br i1 %t31, label %then9, label %endif11
then9:
  ret i64 0
  br label %endif11
endif11:
  %i = alloca i64
  store i64 2, i64* %i
  br label %while.cond12
while.cond12:
  %t32 = load i64, i64* %i
  %t33 = load i64, i64* %i
  %t34 = mul i64 %t32, %t33
  %t35 = load i64, i64* %n
  %t36 = icmp sle i64 %t34, %t35
  %t36 = zext i1 %t36 to i64
  %t37 = icmp ne i64 %t36, 0
  br i1 %t37, label %while.body13, label %while.end14
while.body13:
  %t38 = load i64, i64* %n
  %t39 = load i64, i64* %i
  %t40 = srem i64 %t38, %t39
  %t41 = icmp eq i64 %t40, 0
  %t41 = zext i1 %t41 to i64
  %t42 = icmp ne i64 %t41, 0
  br i1 %t42, label %then15, label %endif17
then15:
  ret i64 0
  br label %endif17
endif17:
  %t43 = load i64, i64* %i
  %t44 = add i64 %t43, 1
  store i64 %t44, i64* %i
  br label %while.cond12
while.end14:
  ret i64 1
}

define i64 @primeCount(i64 %n) {
entry:
  %n.addr = alloca i64
  store i64 %n, i64* %n.addr
  %count = alloca i64
  store i64 0, i64* %count
  %i = alloca i64
  store i64 2, i64* %i
  br label %while.cond18
while.cond18:
  %t45 = load i64, i64* %i
  %t46 = load i64, i64* %n
  %t47 = icmp sle i64 %t45, %t46
  %t47 = zext i1 %t47 to i64
  %t48 = icmp ne i64 %t47, 0
  br i1 %t48, label %while.body19, label %while.end20
while.body19:
  %t49 = load i64, i64* %i
  %t50 = call i64 @isPrime(i64 %t49)
  %t51 = icmp eq i64 %t50, 1
  %t51 = zext i1 %t51 to i64
  %t52 = icmp ne i64 %t51, 0
  br i1 %t52, label %then21, label %endif23
then21:
  %t53 = load i64, i64* %count
  %t54 = add i64 %t53, 1
  store i64 %t54, i64* %count
  br label %endif23
endif23:
  %t55 = load i64, i64* %i
  %t56 = add i64 %t55, 1
  store i64 %t56, i64* %i
  br label %while.cond18
while.end20:
  %t57 = load i64, i64* %count
  ret i64 %t57
}

define i64 @main() {
entry:
  %t58 = ptrtoint i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str, i64 0, i64 0) to i64
  %t59 = call i64 @打印(i64 %t58)
  %t60 = ptrtoint i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str, i64 0, i64 0) to i64
  %t61 = call i64 @打印(i64 %t60)
  %t62 = ptrtoint i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str, i64 0, i64 0) to i64
  %t63 = call i64 @打印(i64 %t62)
  %t64 = call i64 @fib(i64 30)
  %t65 = call i64 @打印(i64 %t64)
  %t66 = ptrtoint i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str, i64 0, i64 0) to i64
  %t67 = call i64 @打印(i64 %t66)
  %t68 = call i64 @arraySum(i64 100000)
  %t69 = call i64 @打印(i64 %t68)
  %t70 = ptrtoint i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str, i64 0, i64 0) to i64
  %t71 = call i64 @打印(i64 %t70)
  %t72 = call i64 @factorial(i64 15)
  %t73 = call i64 @打印(i64 %t72)
  %t74 = ptrtoint i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str, i64 0, i64 0) to i64
  %t75 = call i64 @打印(i64 %t74)
  %t76 = call i64 @primeCount(i64 1000)
  %t77 = call i64 @打印(i64 %t76)
  %t78 = ptrtoint i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str, i64 0, i64 0) to i64
  %t79 = call i64 @打印(i64 %t78)
  %t80 = ptrtoint i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str, i64 0, i64 0) to i64
  %t81 = call i64 @打印(i64 %t80)
  ret i64 0
}

