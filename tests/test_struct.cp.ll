; CP语言生成的LLVM IR
target triple = "x86_64-pc-windows-msvc"

; 外部函数声明
declare i32 @printf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)

; 结构体定义
%struct.Point = type { i64, i64 }
%struct.Rect = type { i64, i64 }

; 函数定义
define i64 @main() {
entry:
  %p = alloca %struct.Point
  %t0 = getelementptr inbounds %struct.Point, %struct.Point* %p, i32 0, i32 0
  store i64 10, i64* %t0
  %t1 = getelementptr inbounds %struct.Point, %struct.Point* %p, i32 0, i32 1
  store i64 20, i64* %t1
  %t2 = getelementptr inbounds %struct.Point, %struct.Point* %p, i32 0, i32 0  ; p.x
  %t3 = load i64, i64* %t2
  %t4 = call i64 @print(i64 %t3)
  %t5 = getelementptr inbounds %struct.Point, %struct.Point* %p, i32 0, i32 1  ; p.y
  %t6 = load i64, i64* %t5
  %t7 = call i64 @print(i64 %t6)
  %t8 = getelementptr inbounds %struct.Point, %struct.Point* %p, i32 0, i32 0  ; p.x = ...
  store i64 100, i64* %t8
  %t9 = getelementptr inbounds %struct.Point, %struct.Point* %p, i32 0, i32 1  ; p.y = ...
  store i64 200, i64* %t9
  %t10 = getelementptr inbounds %struct.Point, %struct.Point* %p, i32 0, i32 0  ; p.x
  %t11 = load i64, i64* %t10
  %t12 = call i64 @print(i64 %t11)
  %t13 = getelementptr inbounds %struct.Point, %struct.Point* %p, i32 0, i32 1  ; p.y
  %t14 = load i64, i64* %t13
  %t15 = call i64 @print(i64 %t14)
  %r = alloca %struct.Rect
  %t16 = getelementptr inbounds %struct.Rect, %struct.Rect* %r, i32 0, i32 0
  store i64 0, i64* %t16
  %t17 = getelementptr inbounds %struct.Rect, %struct.Rect* %r, i32 0, i32 1
  store i64 0, i64* %t17
  %t18 = call i64 @print(i64 0)
  %t19 = call i64 @print(i64 0)
  ret i64 0
}

