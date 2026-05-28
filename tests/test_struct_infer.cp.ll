; CP语言生成的LLVM IR
target triple = "x86_64-pc-windows-msvc"

; 外部函数声明
declare i32 @printf(i8*, ...)
declare i8* @malloc(i64)
declare void @free(i8*)

; 结构体定义
%struct.Point1 = type { i64, i64 }
%struct.Point2 = type { i64, i64 }
%struct.Point3 = type { i64, i64 }
%struct.Data = type { i64, i64, i64, i64 }

; 函数定义
define i64 @main() {
entry:
  %p1 = alloca %struct.Point1
  %t0 = getelementptr inbounds %struct.Point1, %struct.Point1* %p1, i32 0, i32 0
  store i64 10, i64* %t0
  %t1 = getelementptr inbounds %struct.Point1, %struct.Point1* %p1, i32 0, i32 1
  store i64 20, i64* %t1
  %t2 = getelementptr inbounds %struct.Point1, %struct.Point1* %p1, i32 0, i32 0  ; p1.x
  %t3 = load i64, i64* %t2
  %t4 = call i64 @print(i64 %t3)
  %t5 = getelementptr inbounds %struct.Point1, %struct.Point1* %p1, i32 0, i32 1  ; p1.y
  %t6 = load i64, i64* %t5
  %t7 = call i64 @print(i64 %t6)
  %p2 = alloca %struct.Point2
  %t8 = getelementptr inbounds %struct.Point2, %struct.Point2* %p2, i32 0, i32 0
  store i64 100, i64* %t8
  %t9 = getelementptr inbounds %struct.Point2, %struct.Point2* %p2, i32 0, i32 1
  store i64 200, i64* %t9
  %t10 = getelementptr inbounds %struct.Point2, %struct.Point2* %p2, i32 0, i32 0  ; p2.x
  %t11 = load i64, i64* %t10
  %t12 = call i64 @print(i64 %t11)
  %t13 = getelementptr inbounds %struct.Point2, %struct.Point2* %p2, i32 0, i32 1  ; p2.y
  %t14 = load i64, i64* %t13
  %t15 = call i64 @print(i64 %t14)
  %p3 = alloca %struct.Point3
  %t16 = getelementptr inbounds %struct.Point3, %struct.Point3* %p3, i32 0, i32 0
  store i64 5, i64* %t16
  %t17 = getelementptr inbounds %struct.Point3, %struct.Point3* %p3, i32 0, i32 1
  store i64 15, i64* %t17
  %t18 = getelementptr inbounds %struct.Point3, %struct.Point3* %p3, i32 0, i32 0  ; p3.x
  %t19 = load i64, i64* %t18
  %t20 = call i64 @print(i64 %t19)
  %t21 = getelementptr inbounds %struct.Point3, %struct.Point3* %p3, i32 0, i32 1  ; p3.y
  %t22 = load i64, i64* %t21
  %t23 = call i64 @print(i64 %t22)
  %d = alloca %struct.Data
  %t24 = getelementptr inbounds %struct.Data, %struct.Data* %d, i32 0, i32 0
  store i64 1, i64* %t24
  %t25 = getelementptr inbounds %struct.Data, %struct.Data* %d, i32 0, i32 1
  store i64 19, i64* %t25
  %t26 = ptrtoint i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str, i64 0, i64 0) to i64
  %t27 = getelementptr inbounds %struct.Data, %struct.Data* %d, i32 0, i32 2
  store i64 %t26, i64* %t27
  %t28 = getelementptr inbounds %struct.Data, %struct.Data* %d, i32 0, i32 3
  store i64 1, i64* %t28
  %t29 = getelementptr inbounds %struct.Data, %struct.Data* %d, i32 0, i32 0  ; d.count
  %t30 = load i64, i64* %t29
  %t31 = call i64 @print(i64 %t30)
  %t32 = getelementptr inbounds %struct.Data, %struct.Data* %d, i32 0, i32 1  ; d.price
  %t33 = load i64, i64* %t32
  %t34 = call i64 @print(i64 %t33)
  %t35 = getelementptr inbounds %struct.Data, %struct.Data* %d, i32 0, i32 2  ; d.name
  %t36 = load i64, i64* %t35
  %t37 = call i64 @print(i64 %t36)
  %t38 = getelementptr inbounds %struct.Data, %struct.Data* %d, i32 0, i32 3  ; d.flag
  %t39 = load i64, i64* %t38
  %t40 = call i64 @print(i64 %t39)
  %t41 = ptrtoint i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str, i64 0, i64 0) to i64
  %t42 = call i64 @print(i64 %t41)
  ret i64 0
}

