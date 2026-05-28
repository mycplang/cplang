; LLVM IR 示例 - 斐波那契函数
; 用于演示 LLVM 优化效果

; 原始代码:
; 函数 fib(整数 n) {
;     如果 n <= 1 则 返回 n;
;     返回 fib(n - 1) + fib(n - 2);
; }

define i32 @fib(i32 %n) {
entry:
    %cmp = icmp sle i32 %n, 1
    br i1 %cmp, label %base, label %recurse

base:
    ret i32 %n

recurse:
    %n1 = sub i32 %n, 1
    %n2 = sub i32 %n, 2
    %fib1 = call i32 @fib(i32 %n1)
    %fib2 = call i32 @fib(i32 %n2)
    %result = add i32 %fib1, %fib2
    ret i32 %result
}

; 尾递归版本
; 函数 fib_tail(整数 n, 整数 a, 整数 b) {
;     如果 n == 0 则 返回 a;
;     如果 n == 1 则 返回 b;
;     返回 fib_tail(n - 1, b, a + b);
; }

define i32 @fib_tail(i32 %n, i32 %a, i32 %b) {
entry:
    %cmp0 = icmp eq i32 %n, 0
    br i1 %cmp0, label %ret_a, label %check_one

ret_a:
    ret i32 %a

check_one:
    %cmp1 = icmp eq i32 %n, 1
    br i1 %cmp1, label %ret_b, label %recurse

ret_b:
    ret i32 %b

recurse:
    %n1 = sub i32 %n, 1
    %sum = add i32 %a, %b
    %result = call i32 @fib_tail(i32 %n1, i32 %b, i32 %sum)
    ret i32 %result
}

; 循环版本
; 函数 fib_loop(整数 n) {
;     整数 a = 0;
;     整数 b = 1;
;     对于 整数 i = 0; i < n; i = i + 1 则 {
;         整数 temp = a + b;
;         a = b;
;         b = temp;
;     }
;     返回 a;
; }

define i32 @fib_loop(i32 %n) {
entry:
    br label %loop

loop:
    %i = phi i32 [ 0, %entry ], [ %i1, %loop ]
    %a = phi i32 [ 0, %entry ], [ %b, %loop ]
    %b = phi i32 [ 1, %entry ], [ %sum, %loop ]
    
    %cmp = icmp slt i32 %i, %n
    br i1 %cmp, label %body, label %exit

body:
    %sum = add i32 %a, %b
    %i1 = add i32 %i, 1
    br label %loop

exit:
    ret i32 %a
}

; 数组求和
; 函数 sum_array(整数数组 arr, 整数 n) {
;     整数 sum = 0;
;     对于 整数 i = 0; i < n; i = i + 1 则 {
;         sum = sum + arr[i];
;     }
;     返回 sum;
; }

define i32 @sum_array(i32* %arr, i32 %n) {
entry:
    br label %loop

loop:
    %i = phi i32 [ 0, %entry ], [ %i1, %loop ]
    %sum = phi i32 [ 0, %entry ], [ %sum1, %loop ]
    
    %cmp = icmp slt i32 %i, %n
    br i1 %cmp, label %body, label %exit

body:
    %ptr = getelementptr i32, i32* %arr, i32 %i
    %val = load i32, i32* %ptr
    %sum1 = add i32 %sum, %val
    %i1 = add i32 %i, 1
    br label %loop

exit:
    ret i32 %sum
}

; 矩阵乘法
define void @matmul(i32* %A, i32* %B, i32* %C, i32 %n) {
entry:
    br label %loop_i

loop_i:
    %i = phi i32 [ 0, %entry ], [ %i1, %loop_i_end ]
    %cmp_i = icmp slt i32 %i, %n
    br i1 %cmp_i, label %loop_j, label %exit

loop_j:
    %j = phi i32 [ 0, %loop_i ], [ %j1, %loop_j_end ]
    %cmp_j = icmp slt i32 %j, %n
    br i1 %cmp_j, label %loop_k, label %loop_i_end

loop_k:
    %k = phi i32 [ 0, %loop_j ], [ %k1, %loop_k_end ]
    %sum = phi i32 [ 0, %loop_j ], [ %sum1, %loop_k_end ]
    
    %cmp_k = icmp slt i32 %k, %n
    br i1 %cmp_k, label %body, label %loop_j_end

body:
    ; C[i][j] += A[i][k] * B[k][j]
    %ptr_ik = getelementptr i32, i32* %A, i32 %k
    %val_a = load i32, i32* %ptr_ik
    
    %ptr_kj = getelementptr i32, i32* %B, i32 %j
    %val_b = load i32, i32* %ptr_kj
    
    %prod = mul i32 %val_a, %val_b
    %sum1 = add i32 %sum, %prod
    
    %k1 = add i32 %k, 1
    br label %loop_k_end

loop_k_end:
    br label %loop_k

loop_j_end:
    ; 存储 C[i][j]
    %ptr_ij = getelementptr i32, i32* %C, i32 %j
    store i32 %sum, i32* %ptr_ij
    
    %j1 = add i32 %j, 1
    br label %loop_j

loop_i_end:
    %i1 = add i32 %i, 1
    br label %loop_i

exit:
    ret void
}

; 主函数
define i32 @main() {
entry:
    ; 测试 fib(20)
    %fib_result = call i32 @fib(i32 20)
    call void @print_int(i32 %fib_result)
    
    ; 测试 fib_tail(20, 0, 1)
    %fib_tail_result = call i32 @fib_tail(i32 20, i32 0, i32 1)
    call void @print_int(i32 %fib_tail_result)
    
    ; 测试 fib_loop(20)
    %fib_loop_result = call i32 @fib_loop(i32 20)
    call void @print_int(i32 %fib_loop_result)
    
    ret i32 0
}

; 外部函数
declare void @print_int(i32)
