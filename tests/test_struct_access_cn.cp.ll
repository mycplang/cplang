; CP语言生成的LLVM IR
target triple = "x86_64-pc-windows-msvc"

; 外部函数声明
declare i32 @printf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)

; 结构体定义
%struct.点 = type { i64 }

; 函数定义
define i64 @主() {
entry:
  %p = alloca i64
  store i64 0, i64* %p
  %t0 = getelementptr i64, i64* %p, i64 0  ; 访问成员: x
  %t1 = load i64, i64* %t0
  %x = alloca i64
  store i64 %t1, i64* %x
  %t2 = load i64, i64* %x
  %t3 = call i64 @打印(i64 %t2)
  ret i64 0
}

