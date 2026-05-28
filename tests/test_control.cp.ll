; CP语言生成的LLVM IR
target triple = "x86_64-pc-windows-msvc"

; 外部函数声明
declare i32 @printf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)

; 结构体定义

; 函数定义
define i64 @最大值(i64 %a, i64 %b) {
entry:
  %a.addr = alloca i64
  store i64 %a, i64* %a.addr
  %b.addr = alloca i64
  store i64 %b, i64* %b.addr
  %t0 = load i64, i64* %a
  %t1 = load i64, i64* %b
  %t2 = icmp sgt i64 %t0, %t1
  %t2 = zext i1 %t2 to i64
  %t3 = icmp ne i64 %t2, 0
  br i1 %t3, label %then0, label %else1
then0:
  %t4 = load i64, i64* %a
  ret i64 %t4
  br label %endif2
else1:
  %t5 = load i64, i64* %b
  ret i64 %t5
  br label %endif2
endif2:
}

define i64 @阶乘(i64 %n) {
entry:
  %n.addr = alloca i64
  store i64 %n, i64* %n.addr
  %t6 = load i64, i64* %n
  %t7 = icmp sle i64 %t6, 1
  %t7 = zext i1 %t7 to i64
  %t8 = icmp ne i64 %t7, 0
  br i1 %t8, label %then3, label %endif5
then3:
  ret i64 1
  br label %endif5
endif5:
  %t9 = load i64, i64* %n
  %t10 = load i64, i64* %n
  %t11 = sub i64 %t10, 1
  %t12 = call i64 @阶乘(i64 %t11)
  %t13 = mul i64 %t9, %t12
  ret i64 %t13
}

define i64 @求和(i64 %n) {
entry:
  %n.addr = alloca i64
  store i64 %n, i64* %n.addr
  %sum = alloca i64
  store i64 0, i64* %sum
  %i = alloca i64
  store i64 1, i64* %i
  br label %while.cond6
while.cond6:
  %t14 = load i64, i64* %i
  %t15 = load i64, i64* %n
  %t16 = icmp sle i64 %t14, %t15
  %t16 = zext i1 %t16 to i64
  %t17 = icmp ne i64 %t16, 0
  br i1 %t17, label %while.body7, label %while.end8
while.body7:
  %t18 = load i64, i64* %sum
  %t19 = load i64, i64* %i
  %t20 = add i64 %t18, %t19
  store i64 %t20, i64* %sum
  %t21 = load i64, i64* %i
  %t22 = add i64 %t21, 1
  store i64 %t22, i64* %i
  br label %while.cond6
while.end8:
  %t23 = load i64, i64* %sum
  ret i64 %t23
}

define i64 @主() {
entry:
  %a = alloca i64
  store i64 10, i64* %a
  %b = alloca i64
  store i64 20, i64* %b
  %t24 = load i64, i64* %a
  %t25 = load i64, i64* %b
  %t26 = call i64 @最大值(i64 %t24, i64 %t25)
  %max = alloca i64
  store i64 %t26, i64* %max
  %t27 = call i64 @阶乘(i64 5)
  %fact = alloca i64
  store i64 %t27, i64* %fact
  %t28 = call i64 @求和(i64 10)
  %sum = alloca i64
  store i64 %t28, i64* %sum
  %t29 = load i64, i64* %sum
  ret i64 %t29
}

