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

define i64 @main() {
entry:
  %t11 = call i64 @fib(i64 35)
  %result = alloca i64
  store i64 %t11, i64* %result
  %t12 = load i64, i64* %result
  %t13 = call i64 @打印(i64 %t12)
  ret i64 0
}

