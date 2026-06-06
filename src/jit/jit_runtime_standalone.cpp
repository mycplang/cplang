// JIT 运行时辅助函数实现 — AOT 独立版（纯 C，无 C++ 标准库依赖）
//
// 此文件用于 AOT 编译的链接阶段，不依赖任何 cplang 运行时头文件或 C++ 标准库。
// 所有依赖仅限于 C 标准库（libcmt.lib + libucrt.lib）。
//
// 在 AOT 模式下，NaN-boxed 字符串指针总是指向 LLVM 全局字符串常量
// (null-terminated C 字符串)，而非 CP 运行时的 VMString 对象。
// 因此可以直接用 raw pointer 操作。
//
// Table (哈希表) 使用 magic 字段区分纳秒盒装指针类型：
//   0xFFFF + 指针 → 若首 8 字节为 TABLE_MAGIC → 表；否则 → C 字符串

// 屏蔽 C 标准库的 remove() 以避免与 CP 标准库函数冲突
#define remove __cplang_remove_c_hidden_
#include <stdio.h>
#undef remove
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

// ===== 分配追踪器（用于 jit_cleanup 释放所有堆分配） =====
// AOT 模式下程序退出时由 OS 回收，但长时间运行的 JIT 程序需要在
// 每次 JIT 调用完成后清理临时字符串，否则会累积内存。
// 以下 alloc/free 包装器记录所有 malloc 分配，jit_cleanup 统一释放。

typedef struct AllocNode {
    struct AllocNode* next;
    void* ptr;
} AllocNode;

static AllocNode* allocList = NULL;

static void* tracked_malloc(size_t sz) {
    void* p = malloc(sz);
    if (!p) return NULL;
    AllocNode* node = (AllocNode*)malloc(sizeof(AllocNode));
    if (node) {
        node->ptr = p;
        node->next = allocList;
        allocList = node;
    }
    return p;
}

// 注意: 不要单独 free 被追踪的指针（会导致 tracker 中悬空指针）。
// 所有清理统一由 jit_cleanup 完成。

// NaN-boxing 编码常量（与 cplang 运行时保持一致）
#define BIT47_MASK  0x0000800000000000ULL
#define NAN_TAG     0xFFFF000000000000ULL
#define PTR_MASK    0x0000FFFFFFFFFFFFULL

// Table magic — 位于 TableData 首字段，用于在 len() 中区分表与字符串
#define TABLE_MAGIC 0x43504C5441424C45ULL  // "CPLTABLE"

// 判断是否为 NaN-boxed 非整数（高 16 位 = 0xFFFF，bit 47 = 0 表示指针）
static int isNanBoxedPtr(uint64_t v) {
    return (v >> 48) == 0xFFFF && (v & BIT47_MASK) == 0;
}

// 从 NaN-boxed 值中提取指针
static const void* getNanBoxedPtr(uint64_t v) {
    return (const void*)(uintptr_t)(v & PTR_MASK);
}

// 从 NaN-boxed 值中提取字符串指针
static const char* getNanBoxedStringPtr(uint64_t v) {
    return (const char*)(uintptr_t)(v & PTR_MASK);
}

// 将指针编码为 NaN-boxed 值
static uint64_t makeNanBoxedStringPtr(const void* ptr) {
    return NAN_TAG | ((uint64_t)(uintptr_t)ptr & PTR_MASK);
}

// 判断 NaN-boxed 值是否指向 Table
static int isTable(uint64_t v) {
    if (!isNanBoxedPtr(v)) return 0;
    const void* ptr = getNanBoxedPtr(v);
    if (!ptr) return 0;
    return *(const uint64_t*)ptr == TABLE_MAGIC;
}

// 将 NaN-boxed 值转换为字符串（返回静态缓冲区，不可重入；buf 必须 >= 24 字节）
static const char* valueToString(uint64_t v, char* buf, size_t bufSize) {
    if (isNanBoxedPtr(v)) {
        const void* ptr = getNanBoxedPtr(v);
        if (ptr && *(const uint64_t*)ptr == TABLE_MAGIC) {
            // 表/数组：无法直接显示，返回 <table>
            snprintf(buf, bufSize, "<table>");
            return buf;
        }
        return (const char*)ptr;
    }
    snprintf(buf, bufSize, "%lld", (long long)(int64_t)v);
    return buf;
}

// ---- 辅助 alloc（malloc + 拷贝 NaN-boxed C 字符串） ----
static char* strdup_nanobox(uint64_t v) {
    char tmp[24];
    const char* s = valueToString(v, tmp, sizeof(tmp));
    size_t len = strlen(s);
    char* r = (char*)tracked_malloc(len + 1);
    if (r) memcpy(r, s, len + 1);
    return r;
}

extern "C" {

// ===== 字符串拼接 =====
uint64_t jit_strcat(uint64_t a, uint64_t b) {
    char bufa[24], bufb[24];
    const char* sa = valueToString(a, bufa, sizeof(bufa));
    const char* sb = valueToString(b, bufb, sizeof(bufb));
    size_t la = strlen(sa), lb = strlen(sb);
    char* r = (char*)tracked_malloc(la + lb + 1);
    if (!r) return 0;
    memcpy(r, sa, la);
    memcpy(r + la, sb, lb);
    r[la + lb] = '\0';
    return makeNanBoxedStringPtr(r);
}

// ===== 多值打印 =====
void jit_printv(int32_t count, uint64_t* args) {
    for (int32_t i = 0; i < count; i++) {
        if (i > 0) putchar(' ');
        char buf[24];
        const char* s = valueToString(args[i], buf, sizeof(buf));
        fputs(s, stdout);
    }
    putchar('\n');
    fflush(stdout);
}

void jit_cleanup(void) {
    // 释放所有通过 tracked_malloc 分配的堆内存
    // AOT 短程序可以依赖 OS 回收，但长时间运行的 JIT 程序
    // 需要在每次 JIT 调用完成后调用此函数避免内存累积。
    AllocNode* node = allocList;
    while (node) {
        AllocNode* next = node->next;
        free(node->ptr);
        free(node);
        node = next;
    }
    allocList = NULL;
}

// ===== Math 常量 =====
uint64_t tau(void)         { return 0x401921FB54442D18ULL; }
uint64_t sqrt2(void)       { return 0x3FF6A09E667F3BCDULL; }
uint64_t goldenRatio(void) { return 0x3FF9E3779B97F4A8ULL; }

// ===== 串(x) — 数值→字符串 =====
uint64_t __u4E32__(uint64_t v) {
    char buf[24];
    const char* s = valueToString(v, buf, sizeof(buf));
    size_t len = strlen(s);
    char* r = (char*)tracked_malloc(len + 1);
    if (r) memcpy(r, s, len + 1);
    return makeNanBoxedStringPtr(r ? r : "");
}

// ===== Table（字典/数组）操作 =====
#define TABLE_INIT_CAP 8
#define TABLE_EMPTY    0x8000000000000001ULL  // 空槽哨兵

typedef struct {
    uint64_t key;
    uint64_t value;
} TableEntry;

typedef struct {
    uint64_t magic;         // = TABLE_MAGIC
    TableEntry* entries;
    int32_t count;
    int32_t capacity;
} TableData;

static void tableInitEntries(TableEntry* entries, int32_t n) {
    for (int32_t i = 0; i < n; i++) {
        entries[i].key = TABLE_EMPTY;
        entries[i].value = 0;
    }
}

static uint32_t hashKey(uint64_t key) {
    if (key == TABLE_EMPTY) return 0;
    if (isNanBoxedPtr(key)) {
        const char* s = getNanBoxedStringPtr(key);
        if (!s) return (uint32_t)(key ^ (key >> 32));
        uint32_t h = 5381;
        int c;
        while ((c = *s++)) h = ((h << 5) + h) + (unsigned char)c;
        return h;
    }
    return (uint32_t)(key ^ (key >> 32));
}

static void tableGrow(TableData* tbl) {
    int32_t oldCap = tbl->capacity;
    int32_t newCap = oldCap == 0 ? TABLE_INIT_CAP : oldCap * 2;
    TableEntry* newEnt = (TableEntry*)tracked_malloc((size_t)newCap * sizeof(TableEntry));
    tableInitEntries(newEnt, newCap);
    for (int32_t i = 0; i < oldCap; i++) {
        if (tbl->entries[i].key != TABLE_EMPTY) {
            uint32_t idx = hashKey(tbl->entries[i].key) % (uint32_t)newCap;
            while (newEnt[idx].key != TABLE_EMPTY) idx = (idx + 1) % (uint32_t)newCap;
            newEnt[idx] = tbl->entries[i];
        }
    }
    free(tbl->entries);
    tbl->entries = newEnt;
    tbl->capacity = newCap;
}

uint64_t jit_table_create(void) {
    TableData* tbl = (TableData*)tracked_malloc(sizeof(TableData));
    tbl->magic = TABLE_MAGIC;
    tbl->entries = (TableEntry*)tracked_malloc((size_t)TABLE_INIT_CAP * sizeof(TableEntry));
    tableInitEntries(tbl->entries, TABLE_INIT_CAP);
    tbl->count = 0;
    tbl->capacity = TABLE_INIT_CAP;
    return makeNanBoxedStringPtr(tbl);
}

uint64_t jit_table_get(uint64_t tableVal, uint64_t key) {
    if (tableVal == 0 || key == TABLE_EMPTY) return 0;
    const void* ptr = getNanBoxedPtr(tableVal);
    if (!ptr) return 0;
    // 安全：先检查 magic 再转型
    if (*(const uint64_t*)ptr != TABLE_MAGIC) return 0;
    TableData* tbl = (TableData*)ptr;
    if (!tbl->entries) return 0;
    uint32_t cap = (uint32_t)tbl->capacity;
    uint32_t idx = hashKey(key) % cap;
    for (int32_t p = 0; p < tbl->capacity; p++) {
        if (tbl->entries[idx].key == TABLE_EMPTY) return 0;
        if (tbl->entries[idx].key == key) return tbl->entries[idx].value;
        idx = (idx + 1) % cap;
    }
    return 0;
}

uint64_t jit_table_set(uint64_t tableVal, uint64_t key, uint64_t value) {
    if (tableVal == 0) return 0;
    if (key == TABLE_EMPTY) return value;
    const void* ptr = getNanBoxedPtr(tableVal);
    if (!ptr || *(const uint64_t*)ptr != TABLE_MAGIC) return 0;
    TableData* tbl = (TableData*)ptr;
    if (!tbl->entries) return 0;
    if (tbl->count * 10 >= tbl->capacity * 7) tableGrow(tbl);
    uint32_t cap = (uint32_t)tbl->capacity;
    uint32_t idx = hashKey(key) % cap;
    for (int32_t p = 0; p < tbl->capacity; p++) {
        if (tbl->entries[idx].key == TABLE_EMPTY) {
            tbl->entries[idx].key = key;
            tbl->entries[idx].value = value;
            tbl->count++;
            return value;
        }
        if (tbl->entries[idx].key == key) {
            tbl->entries[idx].value = value;
            return value;
        }
        idx = (idx + 1) % cap;
    }
    return value;
}

// ===== CP 标准库函数 =====

// ----- 长度 -----
// len(v)：字符串 → strlen；表/数组 → count；整数 → v 本身
uint64_t len(uint64_t v) {
    if (!isNanBoxedPtr(v)) return v;                          // 整数
    const void* ptr = getNanBoxedPtr(v);
    if (!ptr) return 0;
    if (*(const uint64_t*)ptr == TABLE_MAGIC)                 // 表
        return (uint64_t)((const TableData*)ptr)->count;
    return (uint64_t)strlen((const char*)ptr);                // 字符串
}

uint64_t arrlen(uint64_t v) {
    if (!isNanBoxedPtr(v)) return 0;
    const void* ptr = getNanBoxedPtr(v);
    if (!ptr || *(const uint64_t*)ptr != TABLE_MAGIC) return 0;
    return (uint64_t)((const TableData*)ptr)->count;
}

// ----- 数组操作 -----

// push(arr, val) — 追加到末尾
uint64_t push(uint64_t arr, uint64_t val) {
    if (!arr || !isTable(arr)) return arr;
    TableData* tbl = (TableData*)getNanBoxedPtr(arr);
    jit_table_set(arr, (uint64_t)(int64_t)tbl->count, val);
    return arr;
}

// pop(arr) — 移除并返回末尾元素
uint64_t pop(uint64_t arr) {
    if (!arr || !isTable(arr)) return 0;
    TableData* tbl = (TableData*)getNanBoxedPtr(arr);
    if (tbl->count <= 0) return 0;
    int32_t lastKey = tbl->count - 1;
    // 在哈希表中找到 key=lastKey 的条目
    uint32_t cap = (uint32_t)tbl->capacity;
    uint32_t idx = hashKey((uint64_t)(int64_t)lastKey) % cap;
    for (int32_t p = 0; p < tbl->capacity; p++) {
        if (tbl->entries[idx].key == (uint64_t)(int64_t)lastKey) {
            uint64_t val = tbl->entries[idx].value;
            tbl->entries[idx].key = TABLE_EMPTY;
            tbl->count--;
            return val;
        }
        idx = (idx + 1) % cap;
    }
    // 未找到（健壮性兜底）
    tbl->count--;
    return 0;
}

// insert(arr, idx, val) — 在 idx 处插入，后续元素右移
uint64_t insert(uint64_t arr, uint64_t idxVal, uint64_t val) {
    if (!arr || !isTable(arr)) return arr;
    TableData* tbl = (TableData*)getNanBoxedPtr(arr);
    int32_t idx = (int32_t)(int64_t)idxVal;
    if (idx < 0) idx = 0;
    if (idx >= tbl->count) { push(arr, val); return arr; }
    // 从后往前移位：k 从 count-1 到 idx
    for (int32_t k = tbl->count - 1; k >= idx; k--) {
        uint64_t oldKey = (uint64_t)(int64_t)k;
        uint64_t newKey = (uint64_t)(int64_t)(k + 1);
        // 在哈希表中找出 key=k 的条目
        uint32_t cap = (uint32_t)tbl->capacity;
        uint32_t h = hashKey(oldKey) % cap;
        uint64_t v = 0;
        int found = 0;
        for (int32_t p = 0; p < tbl->capacity; p++) {
            if (tbl->entries[h].key == oldKey) {
                v = tbl->entries[h].value;
                tbl->entries[h].key = TABLE_EMPTY;   // 移除旧条目
                tbl->count--;                         // 临时减 count
                found = 1;
                break;
            }
            h = (h + 1) % cap;
        }
        if (found) {
            // 在新 key 处插入（table_set 会再增加 count）
            jit_table_set(arr, newKey, v);
        }
    }
    // 在 idx 处插入新值
    jit_table_set(arr, (uint64_t)(int64_t)idx, val);
    return arr;
}

// slice(arr, start, end) — 提取子数组
uint64_t slice(uint64_t arr, uint64_t startVal, uint64_t endVal) {
    if (!arr || !isTable(arr)) return 0;
    TableData* tbl = (TableData*)getNanBoxedPtr(arr);
    int32_t start = (int32_t)(int64_t)startVal;
    int32_t end   = (int32_t)(int64_t)endVal;
    if (start < 0) start = 0;
    if (end > tbl->count) end = tbl->count;
    if (start >= end) return 0;  // 返回空表？但需要给调用者一个有效值

    // 创建新表
    uint64_t newArr = jit_table_create();
    TableData* newTbl = (TableData*)getNanBoxedPtr(newArr);
    for (int32_t k = start; k < end; k++) {
        // 在旧表中查找 key=k
        uint32_t cap = (uint32_t)tbl->capacity;
        uint32_t h = hashKey((uint64_t)(int64_t)k) % cap;
        uint64_t v = 0;
        for (int32_t p = 0; p < tbl->capacity; p++) {
            if (tbl->entries[h].key == (uint64_t)(int64_t)k) {
                v = tbl->entries[h].value;
                break;
            }
            h = (h + 1) % cap;
        }
        // 直接写入新表（避免 table_set 触发 grow）
        uint32_t nc = (uint32_t)newTbl->capacity;
        uint32_t ni = hashKey((uint64_t)(int64_t)(k - start)) % nc;
        while (newTbl->entries[ni].key != TABLE_EMPTY)
            ni = (ni + 1) % nc;
        newTbl->entries[ni].key = (uint64_t)(int64_t)(k - start);
        newTbl->entries[ni].value = v;
        newTbl->count++;
    }
    return newArr;
}

// ----- 字符串操作 -----

// substr(str, start, len) — 子串（第三个参数为长度，非结束索引）
uint64_t substr(uint64_t str, uint64_t startVal, uint64_t lenVal) {
    char tmp[24];
    const char* s = valueToString(str, tmp, sizeof(tmp));
    size_t slen = strlen(s);
    int32_t start = (int32_t)(int64_t)startVal;
    int32_t length = (int32_t)(int64_t)lenVal;
    if (start < 0) start = 0;
    if (length < 0) length = 0;
    if ((size_t)start >= slen) return makeNanBoxedStringPtr("");
    int32_t end = start + length;
    if ((size_t)end > slen) end = (int32_t)slen;
    if (start >= end) return makeNanBoxedStringPtr("");

    size_t outLen = (size_t)(end - start);
    char* r = (char*)tracked_malloc(outLen + 1);
    if (!r) return makeNanBoxedStringPtr("");
    memcpy(r, s + start, outLen);
    r[outLen] = '\0';
    return makeNanBoxedStringPtr(r);
}

// concat(a, b) — 字符串连接
uint64_t concat(uint64_t a, uint64_t b) {
    return jit_strcat(a, b);
}

// find(str, pat) — 查找子串位置（返回索引或 -1）
uint64_t find(uint64_t str, uint64_t pat) {
    char bufs[24], bufp[24];
    const char* s = valueToString(str, bufs, sizeof(bufs));
    const char* p = valueToString(pat, bufp, sizeof(bufp));
    const char* found = strstr(s, p);
    if (found) return (uint64_t)(int64_t)(found - s);
    return (uint64_t)(int64_t)-1;
}

// lower(str) — 转小写
uint64_t lower(uint64_t v) {
    char* s = strdup_nanobox(v);
    if (!s) return makeNanBoxedStringPtr("");
    for (char* p = s; *p; p++) *p = (char)tolower((unsigned char)*p);
    uint64_t r = makeNanBoxedStringPtr(s);
    free(s);
    return r;
}

// upper(str) — 转大写
uint64_t upper(uint64_t v) {
    char* s = strdup_nanobox(v);
    if (!s) return makeNanBoxedStringPtr("");
    for (char* p = s; *p; p++) *p = (char)toupper((unsigned char)*p);
    uint64_t r = makeNanBoxedStringPtr(s);
    free(s);
    return r;
}

// remove(arr, idx) — 移除指定索引的元素，后续左移，返回移除的值
uint64_t remove(uint64_t arr, uint64_t idxVal) {
    if (!arr || !isTable(arr)) return 0;
    TableData* tbl = (TableData*)getNanBoxedPtr(arr);
    int32_t idx = (int32_t)(int64_t)idxVal;
    if (idx < 0 || idx >= tbl->count) return 0;

    // 找出 key=idx 条目的值
    uint64_t result = 0;
    int foundResult = 0;
    {
        uint32_t cap = (uint32_t)tbl->capacity;
        uint32_t h = hashKey((uint64_t)(int64_t)idx) % cap;
        for (int32_t p = 0; p < tbl->capacity; p++) {
            if (tbl->entries[h].key == (uint64_t)(int64_t)idx) {
                result = tbl->entries[h].value;
                tbl->entries[h].key = TABLE_EMPTY;
                tbl->count--;
                foundResult = 1;
                break;
            }
            h = (h + 1) % cap;
        }
    }
    if (!foundResult) return 0;

    // 将 idx 之后的元素左移
    for (int32_t k = idx + 1; k <= tbl->count; k++) {
        uint64_t oldKey = (uint64_t)(int64_t)k;
        uint64_t newKey = (uint64_t)(int64_t)(k - 1);
        uint32_t cap = (uint32_t)tbl->capacity;
        uint32_t h = hashKey(oldKey) % cap;
        int found = 0;
        uint64_t v = 0;
        for (int32_t p = 0; p < tbl->capacity; p++) {
            if (tbl->entries[h].key == oldKey) {
                v = tbl->entries[h].value;
                tbl->entries[h].key = TABLE_EMPTY;
                found = 1;
                break;
            }
            h = (h + 1) % cap;
        }
        if (found) {
            jit_table_set(arr, newKey, v);
        }
    }

    return result;
}

// 长度 — 中文 alias for len
uint64_t __u957F____u5EA6__(uint64_t v) {
    return len(v);
}

// toString(v) — 将任意值转为字符串
uint64_t toString(uint64_t v) {
    char buf[48];
    const char* s = valueToString(v, buf, sizeof(buf));
    size_t slen = strlen(s);
    if (s == buf) {
        // s 是栈缓冲区，必须复制
        char* r = (char*)tracked_malloc(slen + 1);
        if (!r) return makeNanBoxedStringPtr("");
        memcpy(r, s, slen + 1);
        return makeNanBoxedStringPtr(r);
    }
    // s 是常量字符串或堆字符串，可直接返回
    return makeNanBoxedStringPtr(s);
}

// 转字符串 — Chinese alias for toString
uint64_t __u8F6C____u5B57____u7B26____u4E32__(uint64_t v) {
    return toString(v);
}

// min(a, b) — 返回最小值（仅整数）
uint64_t min(uint64_t a, uint64_t b) {
    int64_t sa = (int64_t)a, sb = (int64_t)b;
    return (sa < sb) ? a : b;
}

// max(a, b) — 返回最大值（仅整数）
uint64_t max(uint64_t a, uint64_t b) {
    int64_t sa = (int64_t)a, sb = (int64_t)b;
    return (sa > sb) ? a : b;
}

// abs(v) — 整数绝对值
uint64_t jit_abs(uint64_t v) {
    int64_t s = (int64_t)v;
    return (uint64_t)(s < 0 ? -s : s);
}

// clamp(v, lo, hi) — 数值限幅
uint64_t clamp(uint64_t v, uint64_t lo, uint64_t hi) {
    int64_t sv = (int64_t)v, slo = (int64_t)lo, shi = (int64_t)hi;
    if (sv < slo) return lo;
    if (sv > shi) return hi;
    return v;
}

// 取整函数（使用 math.h）
#include <math.h>

// floor(v) — 向下取整（NaN-boxed：整数直接返回）
uint64_t floor_val(uint64_t v) {
    int64_t sv = (int64_t)v;
    // 整数直接返回；实际浮点走桥接
    return (uint64_t)sv;
}

// ceil(v) — 向上取整
uint64_t ceil_val(uint64_t v) {
    int64_t sv = (int64_t)v;
    return (uint64_t)sv;
}

// round(v) — 四舍五入
uint64_t round_val(uint64_t v) {
    int64_t sv = (int64_t)v;
    return (uint64_t)sv;
}

// startsWith(str, prefix) — 判断字符串是否以指定前缀开头
uint64_t startsWith(uint64_t str, uint64_t prefix) {
    const char* s = (const char*)(uintptr_t)(str & PTR_MASK);
    const char* p = (const char*)(uintptr_t)(prefix & PTR_MASK);
    if (!s || !p) return 0;
    while (*p) {
        if (*s != *p) return 0;
        s++; p++;
    }
    return 1;
}

// endsWith(str, suffix) — 判断字符串是否以指定后缀结尾
uint64_t endsWith(uint64_t str, uint64_t suffix) {
    const char* s = (const char*)(uintptr_t)(str & PTR_MASK);
    const char* suf = (const char*)(uintptr_t)(suffix & PTR_MASK);
    if (!s || !suf) return 0;
    size_t slen = strlen(s), suflen = strlen(suf);
    if (suflen > slen) return 0;
    return memcmp(s + slen - suflen, suf, suflen) == 0 ? 1 : 0;
}

// trim(str) — 去除首尾空白
uint64_t trim(uint64_t str) {
    const char* s = (const char*)(uintptr_t)(str & PTR_MASK);
    if (!s) return makeNanBoxedStringPtr("");
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
    if (!*s) return makeNanBoxedStringPtr("");
    const char* end = s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) end--;
    size_t len = (size_t)(end - s + 1);
    char* r = (char*)tracked_malloc(len + 1);
    if (!r) return makeNanBoxedStringPtr("");
    memcpy(r, s, len);
    r[len] = '\0';
    return makeNanBoxedStringPtr(r);
}

// replace(str, from, to) — 替换子串（单次替换）
uint64_t replace(uint64_t str, uint64_t from, uint64_t to) {
    const char* s = (const char*)(uintptr_t)(str & PTR_MASK);
    const char* f = (const char*)(uintptr_t)(from & PTR_MASK);
    const char* t = (const char*)(uintptr_t)(to & PTR_MASK);
    if (!s || !f || !t) return str;
    const char* pos = strstr(s, f);
    if (!pos) return str;
    size_t slen = strlen(s), flen = strlen(f), tlen = strlen(t);
    size_t newLen = slen - flen + tlen;
    char* r = (char*)tracked_malloc(newLen + 1);
    if (!r) return str;
    memcpy(r, s, pos - s);
    memcpy(r + (pos - s), t, tlen);
    memcpy(r + (pos - s) + tlen, pos + flen, slen - (pos - s) - flen + 1);
    return makeNanBoxedStringPtr(r);
}

// writef(fmt, ...) — 格式化字符串输出（简化实现：拼接后打印）
// 参数: args[0]=格式字符串, args[1..N]=要格式化的值
// 格式说明符: {} 表示插入下一个参数
uint64_t writef(uint64_t* args, int32_t count) {
    if (count < 1) return 0;
    char fmtBuf[24];
    const char* fmt = valueToString(args[0], fmtBuf, sizeof(fmtBuf));
    if (!fmt) return 0;

    // 构建输出：遇到 {} 时替换为对应参数
    size_t outCap = strlen(fmt) + 256;
    char* out = (char*)tracked_malloc(outCap);
    if (!out) return 0;
    size_t outLen = 0;
    int argIdx = 1;

    const char* p = fmt;
    while (*p) {
        if (*p == '{' && *(p+1) == '}' && argIdx < count) {
            // 替换 {} 为参数值
            char valBuf[48];
            const char* val = valueToString(args[argIdx], valBuf, sizeof(valBuf));
            size_t valLen = strlen(val);
            if (outLen + valLen >= outCap) {
                outCap = outCap * 2 + valLen;
                char* newOut = (char*)tracked_malloc(outCap);
                if (!newOut) { return 0; }
                memcpy(newOut, out, outLen);
                out = newOut;
            }
            memcpy(out + outLen, val, valLen);
            outLen += valLen;
            p += 2;
            argIdx++;
        } else {
            out[outLen++] = *p++;
            if (outLen >= outCap) {
                outCap *= 2;
                char* newOut = (char*)tracked_malloc(outCap);
                if (!newOut) { return 0; }
                memcpy(newOut, out, outLen);
                out = newOut;
            }
        }
    }
    out[outLen] = '\0';
    fputs(out, stdout);
    fflush(stdout);
    return 1;
}

} // extern "C"
