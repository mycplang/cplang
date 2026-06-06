; ═══════════════════════════════════════════════════════════════════
; cb_dispatcher.asm — 回调分发器（x64 MASM）
;
; 从 thunk 接收: RCX=slotId, RDX=a1(arg1), R8=a2, R9=a3
; 影子空间: [RSP+0]=RCX, [RSP+8]=RDX (由 thunk 写入)
;
; 问题: MSVC 编译器在 C++ 函数中使用 RDX 做 s_slots[id] 索引计算,
;       覆盖 a1 的值。本 ASM 文件通过栈保存避免此问题。
;
; 方案: 立即将所有参数压栈，然后安全地重新装入并调用 C++ helper
; ═══════════════════════════════════════════════════════════════════

.CODE

; extern "C" int64_t cbDispatcher(int64_t id, int64_t a1, int64_t a2, int64_t a3, int64_t a4)
; 被 thunk 调用，由 ASM 直接处理所有寄存器安全问题
cbDispatcher PROC

    ; ── 入口状态 ──
    ; RCX = slotId
    ; RDX = a1 (将被 C++ 函数内部使用而覆盖)
    ; R8  = a2
    ; R9  = a3
    ; [RSP+8] = shadow slot 0 = RCX (thunk 已写入)
    ; [RSP+16] = shadow slot 1 = RDX (thunk 已写入)
    ; [RSP+24] = shadow slot 2 = R8 (thunk 未写入，是原值)
    ; [RSP+32] = shadow slot 3 = R9 (thunk 未写入，是原值)

    ; 保存易失寄存器
    push    r9
    push    r8
    push    rdx
    push    rcx
    ; RSP 现在比 entry 低 32 字节

    ; ── 从保存的寄存器中安全加载参数 ──
    ; [RSP+0] = RCX (slotId)
    ; [RSP+8] = RDX (a1)
    ; [RSP+16] = R8 (a2)
    ; [RSP+24] = R9 (a3)

    mov     rcx, [rsp + 0]      ; rcx = slotId
    mov     rdx, [rsp + 8]      ; rdx = a1（从内存读，安全！）
    mov     r8,  [rsp + 16]     ; r8  = a2
    mov     r9,  [rsp + 24]     ; r9  = a3
    ; a4 从原影子空间读取: 在 push 前的 [RSP+32], push 后 [RSP+32+32] = [RSP+64]
    ; 但实际上我们需要从函数入口的 [RSP+32] 读取
    ; 在 push 前，[RSP+8] = slot RCX, [RSP+16] = slot RDX, [RSP+24] = R8, [RSP+32] = R9
    ; a4 本应是第5个参数，位于调用者栈上 [entry_RSP+shadow] = [entry_RSP+32]
    ; 但 thunk 只传了 4 个参数给 dispatcher (RCX/RDX/R8/R9)
    ; 所以 a4 没有值，传入 0

    ; 分配影子空间给 helper 函数
    sub     rsp, 32

    ; 调用 C++ helper: cbDispatcherHelper(slotId, a1, a2, a3, a4)
    call    cbDispatcherHelper

    ; 回收影子空间
    add     rsp, 32

    ; 恢复栈 (4 个 push)
    add     rsp, 32

    ret

cbDispatcher ENDP

END
