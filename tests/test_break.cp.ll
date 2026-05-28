; CP语言生成的LLVM IR
target triple = "x86_64-pc-windows-msvc"

; 外部函数声明
declare i32 @printf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)

; 结构体定义

; 函数定义
define i64 @测试跳出() {
entry:
  %i = alloca i64
  store i64 0, i64* %i
  br label %while.cond0
while.cond0:
  %t0 = load i64, i64* %i
  %t1 = icmp slt i64 %t0, 100
  %t1 = zext i1 %t1 to i64
  %t2 = icmp ne i64 %t1, 0
  br i1 %t2, label %while.body1, label %while.end2
while.body1:
  %t3 = load i64, i64* %i
  %t4 = icmp eq i64 %t3, 50
  %t4 = zext i1 %t4 to i64
  %t5 = icmp ne i64 %t4, 0
  br i1 %t5, label %then3, label %endif5
then3:
  br label %while.end2
  br label %endif5
endif5:
  %t6 = load i64, i64* %i
  %t7 = add i64 %t6, 1
  store i64 %t7, i64* %i
  br label %while.cond0
while.end2:
  %t8 = load i64, i64* %i
  ret i64 %t8
}

define i64 @测试继续() {
entry:
  %sum = alloca i64
  store i64 0, i64* %sum
  %i = alloca i64
  store i64 0, i64* %i
  br label %while.cond6
while.cond6:
  %t9 = load i64, i64* %i
  %t10 = icmp slt i64 %t9, 10
  %t10 = zext i1 %t10 to i64
  %t11 = icmp ne i64 %t10, 0
  br i1 %t11, label %while.body7, label %while.end8
while.body7:
  %t12 = load i64, i64* %i
  %t13 = add i64 %t12, 1
  store i64 %t13, i64* %i
  %t14 = load i64, i64* %i
  %t15 = srem i64 %t14, 2
  %t16 = icmp eq i64 %t15, 0
  %t16 = zext i1 %t16 to i64
  %t17 = icmp ne i64 %t16, 0
  br i1 %t17, label %then9, label %endif11
then9:
  br label %while.cond6
  br label %endif11
endif11:
  %t18 = load i64, i64* %sum
  %t19 = load i64, i64* %i
  %t20 = add i64 %t18, %t19
  store i64 %t20, i64* %sum
  br label %while.cond6
while.end8:
  %t21 = load i64, i64* %sum
  ret i64 %t21
}

define i64 @主() {
entry:
  %t22 = call i64 @测试跳出()
  %t23 = call i64 @测试继续()
  %t24 = add i64 %t22, %t23
  ret i64 %t24
}

