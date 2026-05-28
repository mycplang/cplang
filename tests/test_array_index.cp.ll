; CP语言生成的LLVM IR
target triple = "x86_64-pc-windows-msvc"

; 外部函数声明
declare i32 @printf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)

; 结构体定义

; 函数定义
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
  br label %while.cond0
while.cond0:
  %t0 = load i64, i64* %i
  %t1 = load i64, i64* %n
  %t2 = icmp slt i64 %t0, %t1
  %t2 = zext i1 %t2 to i64
  %t3 = icmp ne i64 %t2, 0
  br i1 %t3, label %while.body1, label %while.end2
while.body1:
  %t4 = load i64, i64* %sum
  %t5 = load i64, i64* %i
  %t6 = getelementptr i64, i64* %arr.addr, i64 %t5
  %t7 = load i64, i64* %t6
  %t8 = add i64 %t4, %t7
  store i64 %t8, i64* %sum
  %t9 = load i64, i64* %i
  %t10 = add i64 %t9, 1
  store i64 %t10, i64* %i
  br label %while.cond0
while.end2:
  %t11 = load i64, i64* %sum
  ret i64 %t11
}

define i64 @数组最大值(i64 %arr, i64 %n) {
entry:
  %arr.addr = alloca i64
  store i64 %arr, i64* %arr.addr
  %n.addr = alloca i64
  store i64 %n, i64* %n.addr
  %t12 = getelementptr i64, i64* %arr.addr, i64 0
  %t13 = load i64, i64* %t12
  %max = alloca i64
  store i64 %t13, i64* %max
  %i = alloca i64
  store i64 1, i64* %i
  br label %while.cond3
while.cond3:
  %t14 = load i64, i64* %i
  %t15 = load i64, i64* %n
  %t16 = icmp slt i64 %t14, %t15
  %t16 = zext i1 %t16 to i64
  %t17 = icmp ne i64 %t16, 0
  br i1 %t17, label %while.body4, label %while.end5
while.body4:
  %t18 = load i64, i64* %i
  %t19 = getelementptr i64, i64* %arr.addr, i64 %t18
  %t20 = load i64, i64* %t19
  %t21 = load i64, i64* %max
  %t22 = icmp sgt i64 %t20, %t21
  %t22 = zext i1 %t22 to i64
  %t23 = icmp ne i64 %t22, 0
  br i1 %t23, label %then6, label %endif8
then6:
  %t24 = load i64, i64* %i
  %t25 = getelementptr i64, i64* %arr.addr, i64 %t24
  %t26 = load i64, i64* %t25
  store i64 %t26, i64* %max
  br label %endif8
endif8:
  %t27 = load i64, i64* %i
  %t28 = add i64 %t27, 1
  store i64 %t28, i64* %i
  br label %while.cond3
while.end5:
  %t29 = load i64, i64* %max
  ret i64 %t29
}

define i64 @主() {
entry:
  ret i64 0
}

