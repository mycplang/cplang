; CP语言生成的LLVM IR
target triple = "x86_64-pc-windows-msvc"

; 外部函数声明
declare i32 @printf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)

; 结构体定义

; 函数定义
define i64 @加(i64 %a, i64 %b) {
entry:
  %t0 = load i64, i64* %a
  %t1 = load i64, i64* %b
  %t2 = add i64 %t0, %t1
  ret i64 %t2
}

define i64 @主() {
entry:
  %x = alloca i64
  store i64 10, i64* %x
  %y = alloca i64
  store i64 20, i64* %y
  %t3 = load i64, i64* %x
  %t4 = load i64, i64* %y
  %t5 = call i64 @加(i64 %t3, i64 %t4)
  %z = alloca i64
  store i64 %t5, i64* %z
  %t6 = load i64, i64* %z
  ret i64 %t6
}

