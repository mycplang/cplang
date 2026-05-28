// Auto-generated stub implementations
#include "vm/vm.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <shared_mutex>
#include <thread>

namespace cplang {

void registerStubImpls(VM* vm) {
    // BOX
    vm->registerNative("boxNew", [](std::vector<Value>& a) -> Value {
        auto* t = VMTable::create();
        t->set(makeStringVal(VMString::create("v")), a.empty() ? Value::nil() : a[0]);
        return makeTableVal(t);
    });
    vm->registerNative("boxGet", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !isTableVal(a[0])) return Value::nil();
        return asTableVal(a[0])->get(makeStringVal(VMString::create("v")));
    });

    // SPAN
    vm->registerNative("spanNew", [](std::vector<Value>& a) -> Value {
        auto* t = VMTable::create();
        t->set(makeStringVal(VMString::create("start")), a.empty() ? Value::Int(0) : a[0]);
        t->set(makeStringVal(VMString::create("len")), a.size() > 1 ? a[1] : Value::Int(0));
        return makeTableVal(t);
    });
    vm->registerNative("spanGet", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !isTableVal(a[0])) return Value::nil();
        return asTableVal(a[0])->get(makeStringVal(VMString::create("start")));
    });
    vm->registerNative("spanLen", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !isTableVal(a[0])) return Value::Int(0);
        return asTableVal(a[0])->get(makeStringVal(VMString::create("len")));
    });

    // RC / TLS
    vm->registerNative("rcNew", [](std::vector<Value>& a) -> Value { return a.empty() ? Value::nil() : a[0]; });
    vm->registerNative("rcCount", [](std::vector<Value>&) -> Value { return Value::Int(1); });
    vm->registerNative("tlsSet", [](std::vector<Value>& a) -> Value { return a.empty() ? Value::nil() : a[0]; });

    // RESULT
    vm->registerNative("resIsOk", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !isTableVal(a[0])) return Value::Bool(false);
        return asTableVal(a[0])->get(makeStringVal(VMString::create("ok")));
    });
    vm->registerNative("resUnwrap", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !isTableVal(a[0])) return Value::nil();
        return asTableVal(a[0])->get(makeStringVal(VMString::create("v")));
    });
    vm->registerNative("resMap", [](std::vector<Value>& a) -> Value { return a.empty() ? Value::nil() : a[0]; });

    // LOG
    vm->registerNative("logDebug", [](std::vector<Value>& a) -> Value {
        if (!a.empty() && a[0].isString()) {
            std::string s(a[0].asString()->data, a[0].asString()->length);
            fprintf(stderr, "[DEBUG] %s\n", s.c_str());
        }
        return Value::nil();
    });
    vm->registerNative("logError", [](std::vector<Value>& a) -> Value {
        if (!a.empty() && a[0].isString()) {
            std::string s(a[0].asString()->data, a[0].asString()->length);
            fprintf(stderr, "[ERROR] %s\n", s.c_str());
        }
        return Value::nil();
    });
    vm->registerNative("logWarn", [](std::vector<Value>& a) -> Value {
        if (!a.empty() && a[0].isString()) {
            std::string s(a[0].asString()->data, a[0].asString()->length);
            fprintf(stderr, "[WARN] %s\n", s.c_str());
        }
        return Value::nil();
    });
    vm->registerNative("logFlush", [](std::vector<Value>&) -> Value { fflush(stderr); return Value::nil(); });
    vm->registerNative("logSetFile", [](std::vector<Value>&) -> Value { return Value::nil(); });

    // PROC
    vm->registerNative("procExec", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !a[0].isString()) return Value::Int(-1);
        std::string cmd(a[0].asString()->data, a[0].asString()->length);
        return Value::Int(std::system(cmd.c_str()));
    });
    vm->registerNative("procRun", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !a[0].isString()) return makeStringVal(VMString::create(""));
        std::string cmd(a[0].asString()->data, a[0].asString()->length);
        cmd += " 2>&1";
        FILE* fp = popen(cmd.c_str(), "r");
        if (!fp) return makeStringVal(VMString::create(""));
        std::string r;
        char buf[4096];
        while (fgets(buf, sizeof(buf), fp)) r += buf;
        pclose(fp);
        return makeStringVal(VMString::create(r));
    });
    vm->registerNative("procStderr", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !a[0].isString()) return makeStringVal(VMString::create(""));
        std::string cmd(a[0].asString()->data, a[0].asString()->length);
        cmd += " 2>&1 1>/dev/null";
        FILE* fp = popen(cmd.c_str(), "r");
        if (!fp) return makeStringVal(VMString::create(""));
        std::string r;
        char buf[4096];
        while (fgets(buf, sizeof(buf), fp)) r += buf;
        pclose(fp);
        return makeStringVal(VMString::create(r));
    });

    // FILE
    vm->registerNative("fileClose", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !a[0].isPtr()) return Value::Bool(false);
        fclose(reinterpret_cast<FILE*>(a[0].asPtr()));
        return Value::Bool(true);
    });
    vm->registerNative("fileOpenRead", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !a[0].isString()) return Value::nil();
        std::string p(a[0].asString()->data, a[0].asString()->length);
        FILE* f = fopen(p.c_str(), "r");
        if (!f) return Value::nil();
        return Value::Ptr(reinterpret_cast<VMObject*>(f));
    });
    vm->registerNative("fileReadChunk", [](std::vector<Value>& a) -> Value {
        if (a.size() < 2 || !a[0].isPtr()) return makeStringVal(VMString::create(""));
        FILE* f = reinterpret_cast<FILE*>(a[0].asPtr());
        int n = a[1].asInt();
        if (n <= 0) n = 4096;
        std::string s(n, 0);
        size_t rd = fread(&s[0], 1, n, f);
        s.resize(rd);
        return makeStringVal(VMString::create(s));
    });
    vm->registerNative("fileReadLine", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !a[0].isPtr()) return makeStringVal(VMString::create(""));
        FILE* f = reinterpret_cast<FILE*>(a[0].asPtr());
        char buf[4096];
        if (fgets(buf, sizeof(buf), f)) {
            std::string s(buf);
            while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
            return makeStringVal(VMString::create(s));
        }
        return makeStringVal(VMString::create(""));
    });
    vm->registerNative("fileSeek", [](std::vector<Value>& a) -> Value {
        if (a.size() < 2 || !a[0].isPtr()) return Value::Bool(false);
        fseek(reinterpret_cast<FILE*>(a[0].asPtr()), a[1].asInt(), a.size() > 2 ? a[2].asInt() : 0);
        return Value::Bool(true);
    });
    vm->registerNative("fileWalk", [](std::vector<Value>& a) -> Value {
        auto* arr = VMArray::create();
        std::string dir = a.empty() ? "." : std::string(a[0].asString()->data, a[0].asString()->length);
        DIR* d = opendir(dir.c_str());
        if (d) {
            struct dirent* e;
            while ((e = readdir(d))) {
                arr->data.push_back(makeStringVal(VMString::create(e->d_name)));
            }
            closedir(d);
        }
        return makeArrayVal(arr);
    });
    vm->registerNative("listDir", [](std::vector<Value>& a) -> Value {
        std::string p = a.empty() ? "." : std::string(a[0].asString()->data, a[0].asString()->length);
        auto* arr = VMArray::create();
        DIR* d = opendir(p.c_str());
        if (d) {
            struct dirent* e;
            while ((e = readdir(d))) {
                arr->data.push_back(makeStringVal(VMString::create(e->d_name)));
            }
            closedir(d);
        }
        return makeArrayVal(arr);
    });

    // MAP
    vm->registerNative("mapNew", [](std::vector<Value>&) -> Value {
        auto* t = VMTable::create();
        return makeTableVal(t);
    });
    vm->registerNative("mapInsert", [](std::vector<Value>& a) -> Value {
        if (a.size() < 3 || !isTableVal(a[0]) || !a[1].isString()) return Value::Bool(false);
        asTableVal(a[0])->set(a[1], a[2]);
        return Value::Bool(true);
    });
    vm->registerNative("mapFind", [](std::vector<Value>& a) -> Value {
        if (a.size() < 2 || !isTableVal(a[0]) || !a[1].isString()) return Value::nil();
        return asTableVal(a[0])->get(a[1]);
    });
    vm->registerNative("mapContains", [](std::vector<Value>& a) -> Value {
        if (a.size() < 2 || !isTableVal(a[0]) || !a[1].isString()) return Value::Bool(false);
        return Value::Bool(asTableVal(a[0])->has(a[1]));
    });
    vm->registerNative("mapKeys", [](std::vector<Value>& a) -> Value {
        auto* arr = VMArray::create();
        if (!a.empty() && isTableVal(a[0]))
            for (auto& p : asTableVal(a[0])->data) arr->data.push_back(p.first);
        return makeArrayVal(arr);
    });
    vm->registerNative("mapValues", [](std::vector<Value>& a) -> Value {
        auto* arr = VMArray::create();
        if (!a.empty() && isTableVal(a[0]))
            for (auto& p : asTableVal(a[0])->data) arr->data.push_back(p.second);
        return makeArrayVal(arr);
    });
    vm->registerNative("mapSize", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !isTableVal(a[0])) return Value::Int(0);
        return Value::Int((int)asTableVal(a[0])->data.size());
    });

    // CHANNEL / CONCURRENCY
    vm->registerNative("channelCreate", [](std::vector<Value>&) -> Value {
        auto* ch = new std::vector<Value>();
        auto* mtx = new std::mutex();
        auto* t = VMTable::create();
        t->set(makeStringVal(VMString::create("q")), Value::Ptr(reinterpret_cast<VMObject*>(ch)));
        t->set(makeStringVal(VMString::create("m")), Value::Ptr(reinterpret_cast<VMObject*>(mtx)));
        return makeTableVal(t);
    });
    vm->registerNative("channelClose", [](std::vector<Value>&) -> Value { return Value::Bool(true); });
    vm->registerNative("condCreate", [](std::vector<Value>&) -> Value {
        auto* c = new std::condition_variable();
        return Value::Ptr(reinterpret_cast<VMObject*>(c));
    });
    vm->registerNative("barrierCreate", [](std::vector<Value>& a) -> Value {
        int n = a.empty() ? 2 : a[0].asInt();
        auto* b = new std::pair<int, int>(n, 0);
        auto* m = new std::mutex();
        auto* cv = new std::condition_variable();
        auto* t = VMTable::create();
        t->set(makeStringVal(VMString::create("b")), Value::Ptr(reinterpret_cast<VMObject*>(b)));
        t->set(makeStringVal(VMString::create("m")), Value::Ptr(reinterpret_cast<VMObject*>(m)));
        t->set(makeStringVal(VMString::create("c")), Value::Ptr(reinterpret_cast<VMObject*>(cv)));
        return makeTableVal(t);
    });
    vm->registerNative("barrierWait", [](std::vector<Value>&) -> Value { return Value::Bool(true); });
    vm->registerNative("rwLockCreate", [](std::vector<Value>&) -> Value {
        auto* l = new std::shared_timed_mutex();
        return Value::Ptr(reinterpret_cast<VMObject*>(l));
    });
    vm->registerNative("rwLockWrite", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !a[0].isPtr()) return Value::nil();
        auto* l = reinterpret_cast<std::shared_timed_mutex*>(a[0].asPtr());
        l->lock();
        return Value::nil();
    });
    vm->registerNative("futureGo", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !a[0].isFunction()) return Value::nil();
        VMFunction* fn = a[0].asFunction();
        std::thread([fn]() {
            std::vector<Value> args;
            Value::Ptr(fn);
        }).detach();
        return Value::Ptr(reinterpret_cast<VMObject*>(new std::thread()));
    });
    vm->registerNative("futureIsReady", [](std::vector<Value>&) -> Value { return Value::Bool(false); });
    vm->registerNative("futureGet", [](std::vector<Value>&) -> Value { return Value::nil(); });

    // ARRAY ALGO
    vm->registerNative("filter", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !a[0].isArray()) return makeArrayVal(VMArray::create());
        auto* src = asArrayVal(a[0]);
        auto* dst = VMArray::create();
        for (auto& v : src->data)
            if (!v.isNil() && (!v.isBool() || v.asBool())) dst->data.push_back(v);
        return makeArrayVal(dst);
    });
    vm->registerNative("map", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !a[0].isArray()) return makeArrayVal(VMArray::create());
        auto* src = asArrayVal(a[0]);
        auto* dst = VMArray::create();
        for (auto& v : src->data) dst->data.push_back(v);
        return makeArrayVal(dst);
    });
    vm->registerNative("reduce", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !a[0].isArray()) return Value::Float(0);
        auto* src = asArrayVal(a[0]);
        double sum = 0;
        for (auto& v : src->data) sum += v.asFloat();
        return Value::Float(sum);
    });
    vm->registerNative("difference", [](std::vector<Value>& a) -> Value {
        if (a.size() < 2 || !a[0].isArray() || !a[1].isArray()) return a.empty() ? Value::nil() : a[0];
        auto* s1 = asArrayVal(a[0]);
        auto* s2 = asArrayVal(a[1]);
        auto* dst = VMArray::create();
        for (auto& v : s1->data) {
            bool found = false;
            for (auto& w : s2->data)
                if (v.asFloat() == w.asFloat()) { found = true; break; }
            if (!found) dst->data.push_back(v);
        }
        return makeArrayVal(dst);
    });
    vm->registerNative("intersection", [](std::vector<Value>& a) -> Value {
        if (a.size() < 2 || !a[0].isArray() || !a[1].isArray()) return makeArrayVal(VMArray::create());
        auto* s1 = asArrayVal(a[0]);
        auto* s2 = asArrayVal(a[1]);
        auto* dst = VMArray::create();
        for (auto& v : s1->data)
            for (auto& w : s2->data)
                if (v.asFloat() == w.asFloat()) { dst->data.push_back(v); break; }
        return makeArrayVal(dst);
    });
    vm->registerNative("guards", [](std::vector<Value>& a) -> Value { return a.empty() ? Value::nil() : a[0]; });
    vm->registerNative("isMouseButtonDown", [](std::vector<Value>&) -> Value { return Value::Bool(false); });
    vm->registerNative("pressed", [](std::vector<Value>&) -> Value { return Value::Bool(false); });
    vm->registerNative("fmt", [](std::vector<Value>& a) -> Value {
        std::string r;
        for (size_t i = 0; i < a.size(); i++) {
            if (i > 0) r += " ";
            if (a[i].isString())
                r += std::string(a[i].asString()->data, a[i].asString()->length);
            else
                r += a[i].toString();
        }
        return makeStringVal(VMString::create(r));
    });

    // MATH
    vm->registerNative("\xe5\xa4\x8d\xe6\x95\xb0\xe8\xbe\x90\xe8\xa7\x92", [](std::vector<Value>& a) -> Value {
        double x = a.size() > 0 ? a[0].asFloat() : 0;
        double y = a.size() > 1 ? a[1].asFloat() : 0;
        return Value::Float(atan2(y, x));
    });
    vm->registerNative("\xe7\x9f\xa9\xe9\x98\xb5\xe4\xb9\x98", [](std::vector<Value>& a) -> Value { return a.empty() ? Value::nil() : a[0]; });
    vm->registerNative("\xe7\x9f\xa9\xe9\x98\xb5\xe8\xa1\x8c\xe5\x88\x97\xe5\xbc\x8f", [](std::vector<Value>&) -> Value { return Value::Float(0); });
    vm->registerNative("\xe7\x9f\xa9\xe9\x98\xb5\xe5\x8a\xa0", [](std::vector<Value>& a) -> Value { return a.empty() ? Value::nil() : a[0]; });
    vm->registerNative("\xe7\x9f\xa9\xe9\x98\xb5\xe8\xbd\xac\xe7\xbd\xae", [](std::vector<Value>& a) -> Value { return a.empty() ? Value::nil() : a[0]; });
    vm->registerNative("\xe5\x90\x91\xe9\x87\x8f\xe5\x8f\x89\xe7\xa7\xaf", [](std::vector<Value>& a) -> Value {
        double x1 = a.size() > 0 ? a[0].asFloat() : 0, y1 = a.size() > 1 ? a[1].asFloat() : 0, z1 = a.size() > 2 ? a[2].asFloat() : 0;
        double x2 = a.size() > 3 ? a[3].asFloat() : 0, y2 = a.size() > 4 ? a[4].asFloat() : 0, z2 = a.size() > 5 ? a[5].asFloat() : 0;
        auto* arr = VMArray::create();
        arr->data.push_back(Value::Float(y1 * z2 - z1 * y2));
        arr->data.push_back(Value::Float(z1 * x2 - x1 * z2));
        arr->data.push_back(Value::Float(x1 * y2 - y1 * x2));
        return makeArrayVal(arr);
    });
    vm->registerNative("bernoulliDist", [](std::vector<Value>& a) -> Value {
        double p = a.empty() ? 0.5 : a[0].asFloat();
        return Value::Float((rand() / (double)RAND_MAX) < p ? 1 : 0);
    });
    vm->registerNative("exponentialDist", [](std::vector<Value>& a) -> Value {
        double lambda = a.empty() ? 1.0 : a[0].asFloat();
        double u = (rand() + 1.0) / (RAND_MAX + 2.0);
        return Value::Float(-log(u) / lambda);
    });
    vm->registerNative("normalDist", [](std::vector<Value>& a) -> Value {
        (void)a;
        double u1 = (rand() + 1.0) / (RAND_MAX + 2.0);
        double u2 = (rand() + 1.0) / (RAND_MAX + 2.0);
        return Value::Float(sqrt(-2 * log(u1)) * cos(2 * M_PI * u2));
    });
    vm->registerNative("poissonDist", [](std::vector<Value>& a) -> Value {
        double lambda = a.empty() ? 1.0 : a[0].asFloat();
        double L = exp(-lambda);
        int k = 0;
        double p = 1;
        while (p > L) {
            k++;
            p *= (rand() + 1.0) / (RAND_MAX + 2.0);
        }
        return Value::Int(k - 1);
    });

    // BASE32
    vm->registerNative("base32", [](std::vector<Value>& a) -> Value { return a.empty() ? makeStringVal(VMString::create("")) : a[0]; });
    vm->registerNative("base32Encode", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !a[0].isString()) return makeStringVal(VMString::create(""));
        std::string s(a[0].asString()->data, a[0].asString()->length);
        const char* tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
        std::string r;
        int bits = 0, val = 0;
        for (char ch : s) {
            val = (val << 8) | (unsigned char)ch;
            bits += 8;
            while (bits >= 5) {
                r += tbl[(val >> (bits - 5)) & 31];
                bits -= 5;
            }
        }
        if (bits > 0) r += tbl[(val << (5 - bits)) & 31];
        return makeStringVal(VMString::create(r));
    });
    vm->registerNative("base32Decode", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !a[0].isString()) return makeStringVal(VMString::create(""));
        std::string s(a[0].asString()->data, a[0].asString()->length);
        const char* tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
        int val = 0, bits = 0;
        std::string r;
        for (char ch : s) {
            const char* p = (const char*)memchr(tbl, ch, 32);
            if (!p) continue;
            val = (val << 5) | (int)(p - tbl);
            bits += 5;
            if (bits >= 8) {
                r += (char)((val >> (bits - 8)) & 0xFF);
                bits -= 8;
            }
        }
        return makeStringVal(VMString::create(r));
    });

    // CSV
    vm->registerNative("csvWrite", [](std::vector<Value>& a) -> Value {
        if (a.size() < 2 || !a[0].isString() || !a[1].isArray()) return Value::Bool(false);
        std::string p(a[0].asString()->data, a[0].asString()->length);
        auto* rows = asArrayVal(a[1]);
        std::ofstream f(p);
        if (!f) return Value::Bool(false);
        for (auto& rv : rows->data) {
            if (rv.isArray()) {
                auto* row = asArrayVal(rv);
                for (size_t i = 0; i < row->data.size(); i++) {
                    if (i > 0) f << ",";
                    f << row->data[i].toString();
                }
            }
            f << "\n";
        }
        return Value::Bool(true);
    });

    // SQLITE
    vm->registerNative("sqliteOpen", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !a[0].isString()) return Value::nil();
        std::string p(a[0].asString()->data, a[0].asString()->length);
        std::string cmd = "sqlite3 " + p + " \"CREATE TABLE IF NOT EXISTS cp_kv(k TEXT PRIMARY KEY,v TEXT);\" 2>/dev/null";
        pclose(popen(cmd.c_str(), "r"));
        return a[0];
    });
    vm->registerNative("sqliteClose", [](std::vector<Value>&) -> Value { return Value::nil(); });
    vm->registerNative("sqliteExec", [](std::vector<Value>& a) -> Value {
        if (a.size() < 2 || !a[0].isString() || !a[1].isString()) return Value::Bool(true);
        std::string db(a[0].asString()->data, a[0].asString()->length);
        std::string sql(a[1].asString()->data, a[1].asString()->length);
        std::string cmd = "sqlite3 " + db + " \x22" + sql + "\x22 2>/dev/null";
        pclose(popen(cmd.c_str(), "r"));
        return Value::Bool(true);
    });
    vm->registerNative("sqliteLastInsertId", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !a[0].isString()) return Value::Int(0);
        std::string db(a[0].asString()->data, a[0].asString()->length);
        std::string cmd = "sqlite3 " + db + " \"SELECT last_insert_rowid();\" 2>/dev/null";
        FILE* fp = popen(cmd.c_str(), "r");
        if (!fp) return Value::Int(0);
        char buf[64];
        std::string r;
        if (fgets(buf, sizeof(buf), fp)) r = buf;
        pclose(fp);
        while (!r.empty() && (r.back() == '\n' || r.back() == '\r')) r.pop_back();
        try { return Value::Int(std::stoll(r)); }
        catch (...) { return Value::Int(0); }
    });

    // WEBSOCKET
    vm->registerNative("wsConnect", [](std::vector<Value>&) -> Value { return Value::nil(); });
    vm->registerNative("wsClose", [](std::vector<Value>&) -> Value { return Value::nil(); });
    vm->registerNative("wsIsOpen", [](std::vector<Value>&) -> Value { return Value::Bool(false); });
    vm->registerNative("wsSend", [](std::vector<Value>&) -> Value { return Value::Bool(false); });
    vm->registerNative("wsRecv", [](std::vector<Value>&) -> Value { return makeStringVal(VMString::create("")); });
}

} // namespace cplang
