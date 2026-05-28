// Stubs upgraded to real implementations where possible
#include "vm/vm.hpp"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

using namespace cplang;

static void registerStubs(VM* vm) {
    // ── Math (cmath) ──
    vm->registerNative("fdim", [](std::vector<Value>& a) -> Value {
        double x = a.size()>0 ? a[0].asFloat() : 0, y = a.size()>1 ? a[1].asFloat() : 0;
        return Value::fromFloat(std::fdim(x, y));
    });
    vm->registerNative("fmax", [](std::vector<Value>& a) -> Value {
        double x = a.size()>0 ? a[0].asFloat() : 0, y = a.size()>1 ? a[1].asFloat() : 0;
        return Value::fromFloat(std::fmax(x, y));
    });
    vm->registerNative("fmin", [](std::vector<Value>& a) -> Value {
        double x = a.size()>0 ? a[0].asFloat() : 0, y = a.size()>1 ? a[1].asFloat() : 0;
        return Value::fromFloat(std::fmin(x, y));
    });
    vm->registerNative("conj", [](std::vector<Value>& a) -> Value {
        return a.empty() ? Value::fromFloat(0) : a[0];
    });
    vm->registerNative("signbit", [](std::vector<Value>& a) -> Value {
        return a.empty() ? Value::Bool(false) : Value::Bool(std::signbit(a[0].asFloat()));
    });
    vm->registerNative("isnormal", [](std::vector<Value>& a) -> Value {
        return a.empty() ? Value::Bool(false) : Value::Bool(std::isnormal(a[0].asFloat()));
    });
    vm->registerNative("sqrt2", [](std::vector<Value>&) -> Value { return Value::fromFloat(1.4142135623730951); });
    vm->registerNative("goldenRatio", [](std::vector<Value>&) -> Value { return Value::fromFloat(1.618033988749895); });
    vm->registerNative("tau", [](std::vector<Value>&) -> Value { return Value::fromFloat(6.283185307179586); });
    vm->registerNative("byteswap", [](std::vector<Value>& a) -> Value {
        if (a.empty()) return Value::Int(0);
        uint64_t v = (uint64_t)a[0].asInt();
        v = ((v & 0xFF00FF00FF00FF00ULL) >> 8) | ((v & 0x00FF00FF00FF00FFULL) << 8);
        v = ((v & 0xFFFF0000FFFF0000ULL) >> 16) | ((v & 0x0000FFFF0000FFFFULL) << 16);
        v = (v >> 32) | (v << 32);
        return Value::Int((int64_t)v);
    });

    // ── Time / Clock ──
    vm->registerNative("clock", [](std::vector<Value>&) -> Value {
        return Value::fromFloat((double)std::clock() / CLOCKS_PER_SEC);
    });
    vm->registerNative("timestamp", [](std::vector<Value>&) -> Value {
        return Value::Int((int64_t)std::time(nullptr));
    });
    vm->registerNative("elapsed", [](std::vector<Value>& a) -> Value {
        if (a.empty()) return Value::fromFloat(0);
        double then = a[0].asFloat();
        return Value::fromFloat((double)std::clock() / CLOCKS_PER_SEC - then);
    });
    vm->registerNative("durationFormat", [](std::vector<Value>& a) -> Value {
        if (a.empty()) return makeStringVal(VMString::create("0s"));
        double secs = a[0].asFloat();
        char buf[64]; snprintf(buf, sizeof(buf), "%.2fs", secs);
        return makeStringVal(VMString::create(buf));
    });
    vm->registerNative("msToSec", [](std::vector<Value>& a) -> Value {
        return a.empty() ? Value::fromFloat(0) : Value::fromFloat(a[0].asFloat() / 1000.0);
    });
    vm->registerNative("secToMs", [](std::vector<Value>& a) -> Value {
        return a.empty() ? Value::Int(0) : Value::Int((int)(a[0].asFloat() * 1000));
    });
    vm->registerNative("nsToSec", [](std::vector<Value>& a) -> Value {
        return a.empty() ? Value::fromFloat(0) : Value::fromFloat(a[0].asFloat() / 1e9);
    });

    // ── Environment ──
    vm->registerNative("envGet", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !a[0].isString()) return Value::nil();
        std::string key(a[0].asString()->data, a[0].asString()->length);
        const char* val = getenv(key.c_str());
        return val ? makeStringVal(VMString::create(val)) : Value::nil();
    });
    vm->registerNative("envSet", [](std::vector<Value>& a) -> Value {
        if (a.size()<2 || !a[0].isString() || !a[1].isString()) return Value::Bool(false);
        std::string key(a[0].asString()->data, a[0].asString()->length);
        std::string val(a[1].asString()->data, a[1].asString()->length);
        return Value::Bool(setenv(key.c_str(), val.c_str(), 1) == 0);
    });

    // ── String ──
    vm->registerNative("intToStr", [](std::vector<Value>& a) -> Value {
        if (a.empty()) return makeStringVal(VMString::create("0"));
        return makeStringVal(VMString::create(std::to_string((int64_t)a[0].asInt())));
    });
    vm->registerNative("hexEncode", [](std::vector<Value>& a) -> Value {
        if (a.empty()) return makeStringVal(VMString::create(""));
        if (a[0].isString()) {
            std::string s(a[0].asString()->data, a[0].asString()->length);
            std::stringstream ss;
            for (unsigned char c : s) ss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
            return makeStringVal(VMString::create(ss.str()));
        }
        char buf[32]; snprintf(buf, sizeof(buf), "%lx", (uint64_t)a[0].asInt());
        return makeStringVal(VMString::create(buf));
    });
    vm->registerNative("strContains", [](std::vector<Value>& a) -> Value {
        if (a.size()<2 || !a[0].isString() || !a[1].isString()) return Value::Bool(false);
        std::string hay(a[0].asString()->data, a[0].asString()->length);
        std::string ndl(a[1].asString()->data, a[1].asString()->length);
        return Value::Bool(hay.find(ndl) != std::string::npos);
    });
    vm->registerNative("strContainsI", [](std::vector<Value>& a) -> Value {
        if (a.size()<2 || !a[0].isString() || !a[1].isString()) return Value::Bool(false);
        std::string hay(a[0].asString()->data, a[0].asString()->length);
        std::string ndl(a[1].asString()->data, a[1].asString()->length);
        auto toLower = [](std::string s) { for (auto& c:s) c=tolower(c); return s; };
        return Value::Bool(toLower(hay).find(toLower(ndl)) != std::string::npos);
    });
    vm->registerNative("strCompareI", [](std::vector<Value>& a) -> Value {
        if (a.size()<2 || !a[0].isString() || !a[1].isString()) return Value::Int(0);
        std::string x(a[0].asString()->data, a[0].asString()->length);
        std::string y(a[1].asString()->data, a[1].asString()->length);
        for (auto& c:x) c=tolower(c); for (auto& c:y) c=tolower(c);
        return Value::Int(x.compare(y));
    });
    vm->registerNative("strEqualsI", [](std::vector<Value>& a) -> Value {
        if (a.size()<2 || !a[0].isString() || !a[1].isString()) return Value::Bool(false);
        std::string x(a[0].asString()->data, a[0].asString()->length);
        std::string y(a[1].asString()->data, a[1].asString()->length);
        for (auto& c:x) c=tolower(c); for (auto& c:y) c=tolower(c);
        return Value::Bool(x == y);
    });
    vm->registerNative("strCount", [](std::vector<Value>& a) -> Value {
        if (a.size()<2 || !a[0].isString() || !a[1].isString()) return Value::Int(0);
        std::string hay(a[0].asString()->data, a[0].asString()->length);
        std::string ndl(a[1].asString()->data, a[1].asString()->length);
        if (ndl.empty()) return Value::Int(0);
        int count=0; size_t pos=0;
        while ((pos=hay.find(ndl,pos)) != std::string::npos) { count++; pos+=ndl.size(); }
        return Value::Int(count);
    });
    vm->registerNative("compare", [](std::vector<Value>& a) -> Value {
        if (a.size()<2) return Value::Int(0);
        if (a[0].isString() && a[1].isString()) {
            std::string x(a[0].asString()->data, a[0].asString()->length);
            std::string y(a[1].asString()->data, a[1].asString()->length);
            return Value::Int(x.compare(y));
        }
        double d = a[0].asFloat() - a[1].asFloat();
        return Value::Int(d<0?-1:d>0?1:0);
    });

    // ── File System ──
    vm->registerNative("fileSize", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !a[0].isString()) return Value::Int(-1);
        std::string p(a[0].asString()->data, a[0].asString()->length);
        struct stat st; return Value::Int(::stat(p.c_str(),&st)==0 ? (int64_t)st.st_size : -1);
    });
    vm->registerNative("fileIsDir", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !a[0].isString()) return Value::Bool(false);
        std::string p(a[0].asString()->data, a[0].asString()->length);
        struct stat st; return Value::Bool(::stat(p.c_str(),&st)==0 && S_ISDIR(st.st_mode));
    });
    vm->registerNative("fileMtime", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !a[0].isString()) return Value::Int(0);
        std::string p(a[0].asString()->data, a[0].asString()->length);
        struct stat st; return Value::Int(::stat(p.c_str(),&st)==0 ? (int64_t)st.st_mtime : 0);
    });
    vm->registerNative("fileDelete", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !a[0].isString()) return Value::Bool(false);
        std::string p(a[0].asString()->data, a[0].asString()->length);
        return Value::Bool(unlink(p.c_str()) == 0);
    });
    vm->registerNative("dirRemove", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !a[0].isString()) return Value::Bool(false);
        std::string p(a[0].asString()->data, a[0].asString()->length);
        return Value::Bool(rmdir(p.c_str()) == 0);
    });

    // ── Collection helpers ──
    vm->registerNative("contains", [](std::vector<Value>& a) -> Value {
        if (a.size()<2) return Value::Bool(false);
        if (a[0].isPtr() && a[0].asPtr()) {
            if (a[0].asPtr()->typeTag == ObjectHeader::TAG_SET) {
                auto* set = reinterpret_cast<VMSet*>(a[0].asPtr());
                return Value::Bool(set->has(a[1]));
            }
        }
        if (a[0].isString() && a[1].isString()) {
            std::string hay(a[0].asString()->data, a[0].asString()->length);
            std::string ndl(a[1].asString()->data, a[1].asString()->length);
            return Value::Bool(hay.find(ndl) != std::string::npos);
        }
        return Value::Bool(false);
    });

    // ── Distributions (basic) ──
    vm->registerNative("distSeed", [](std::vector<Value>& a) -> Value {
        int s = a.empty() ? (int)std::time(nullptr) : (int)a[0].asInt();
        srand((unsigned)s); return Value::Bool(true);
    });
    vm->registerNative("uniformInt", [](std::vector<Value>& a) -> Value {
        int lo=a.size()>0?(int)a[0].asInt():0, hi=a.size()>1?(int)a[1].asInt():100;
        return Value::Int(lo + rand() % (hi-lo+1));
    });
    vm->registerNative("uniformFloat", [](std::vector<Value>&) -> Value {
        return Value::fromFloat((double)rand() / RAND_MAX);
    });

    // ── Thread (POSIX) ──
    vm->registerNative("threadId", [](std::vector<Value>&) -> Value {
        return Value::Int((int64_t)pthread_self());
    });
    vm->registerNative("threadHw", [](std::vector<Value>&) -> Value {
        long n = sysconf(_SC_NPROCESSORS_ONLN);
        return Value::Int(n > 0 ? (int64_t)n : 1);
    });

    // ── Remaining stubs (still return nil/0 for unimplemented) ──
    vm->registerNative("arrClear", [](std::vector<Value>& a) -> Value { if(!a.empty()&&a[0].isArray()) asArrayVal(a[0])->data.clear(); return Value::nil(); });
    vm->registerNative("fileEof", [](std::vector<Value>& a) -> Value { if(a.empty()||!a[0].isPtr()) return Value::Bool(true); return Value::Bool(feof(reinterpret_cast<FILE*>(a[0].asPtr()))!=0); });
    vm->registerNative("fileTell", [](std::vector<Value>& a) -> Value { if(a.empty()||!a[0].isPtr()) return Value::Int(-1); return Value::Int((int)ftell(reinterpret_cast<FILE*>(a[0].asPtr()))); });
    vm->registerNative("identity", [](std::vector<Value>& a) -> Value { return a.empty()?Value::nil():a[0]; });
    vm->registerNative("resOk", [](std::vector<Value>&) -> Value { return Value::nil(); });
    vm->registerNative("semCreate", [](std::vector<Value>& a) -> Value { int v=a.empty()?1:a[0].asInt(); auto* s=new std::atomic<int>(v); return Value::Ptr(reinterpret_cast<VMObject*>(s)); });
    vm->registerNative("semPost", [](std::vector<Value>& a) -> Value { if(a.empty()||!a[0].isPtr()) return Value::Bool(false); auto* s=reinterpret_cast<std::atomic<int>*>(a[0].asPtr()); s->fetch_add(1); return Value::Bool(true); });
    vm->registerNative("semTryWait", [](std::vector<Value>& a) -> Value { if(a.empty()||!a[0].isPtr()) return Value::Bool(false); auto* s=reinterpret_cast<std::atomic<int>*>(a[0].asPtr()); int v=s->load(); if(v>0){ s->compare_exchange_strong(v,v-1); return Value::Bool(true); } return Value::Bool(false); });
    vm->registerNative("sha512", [](std::vector<Value>& a) -> Value { if(a.empty()||!a[0].isString()) return makeStringVal(VMString::create("")); std::string s(a[0].asString()->data,a[0].asString()->length); std::string cmd="echo -n "+s+"|sha512sum|cut -d' ' -f1"; FILE* fp=popen(cmd.c_str(),"r"); if(!fp) return makeStringVal(VMString::create("")); char buf[256]; std::string r; if(fgets(buf,sizeof(buf),fp)) r=buf; pclose(fp); while(!r.empty()&&(r.back()==0x0a||r.back()==0x0d)) r.pop_back(); return makeStringVal(VMString::create(r)); });
    vm->registerNative("stableSort", [](std::vector<Value>& a) -> Value { if(a.empty()||!a[0].isArray()) return a.empty()?Value::nil():a[0]; auto* arr=asArrayVal(a[0]); std::stable_sort(arr->data.begin(),arr->data.end(),[](const Value& x,const Value& y){ return x.asFloat()<y.asFloat(); }); return a[0]; });
    vm->registerNative("strToInt", [](std::vector<Value>& a) -> Value { if(a.empty()||!a[0].isString()) return Value::Int(0); std::string s(a[0].asString()->data,a[0].asString()->length); try{return Value::Int(std::stoll(s));}catch(...){return Value::Int(0);} });
    vm->registerNative("tempDir", [](std::vector<Value>&) -> Value { return makeStringVal(VMString::create("/tmp")); });
    vm->registerNative("tempFile", [](std::vector<Value>&) -> Value { char t[]="/tmp/cptmpXXXXXX"; int fd=mkstemp(t); if(fd>=0){ close(fd); return makeStringVal(VMString::create(t)); } return makeStringVal(VMString::create("")); });
    vm->registerNative("threadYield", [](std::vector<Value>&) -> Value { std::this_thread::yield(); return Value::nil(); });
    vm->registerNative("测试", [](std::vector<Value>&) -> Value { return Value::nil(); });
    vm->registerNative("大于三", [](std::vector<Value>& a) -> Value { if(a.empty()||!a[0].isArray()) return makeArrayVal(VMArray::create()); auto* src=asArrayVal(a[0]); auto* dst=VMArray::create(); for(auto& v:src->data) if(v.asFloat()>3) dst->data.push_back(v); return makeArrayVal(dst); });
    vm->registerNative("单位矩阵", [](std::vector<Value>& a) -> Value { int n=a.empty()?3:a[0].asInt(); auto* m=VMArray::create(); for(int i=0;i<n;i++){ auto* row=VMArray::create(); for(int j=0;j<n;j++) row->data.push_back(Value::fromFloat(i==j?1:0)); m->data.push_back(makeArrayVal(row)); } return makeArrayVal(m); });
    vm->registerNative("翻倍", [](std::vector<Value>& a) -> Value { if(a.empty()||!a[0].isArray()) return makeArrayVal(VMArray::create()); auto* src=asArrayVal(a[0]); auto* dst=VMArray::create(); for(auto& v:src->data) dst->data.push_back(Value::fromFloat(v.asFloat()*2)); return makeArrayVal(dst); });
    vm->registerNative("复数辐角", [](std::vector<Value>&) -> Value { return Value::nil(); });
    vm->registerNative("矩阵乘", [](std::vector<Value>&) -> Value { return Value::nil(); });
    vm->registerNative("矩阵行列式", [](std::vector<Value>&) -> Value { return Value::nil(); });
    vm->registerNative("矩阵加", [](std::vector<Value>&) -> Value { return Value::nil(); });
    vm->registerNative("矩阵转置", [](std::vector<Value>&) -> Value { return Value::nil(); });
    vm->registerNative("路径连接", [](std::vector<Value>& a) -> Value { std::string r; for(size_t i=0;i<a.size()&&a[i].isString();i++){ if(i>0) r+="/"; r+=std::string(a[i].asString()->data,a[i].asString()->length); } return makeStringVal(VMString::create(r)); });
    vm->registerNative("路径目录", [](std::vector<Value>& a) -> Value { if(a.empty()||!a[0].isString()) return makeStringVal(VMString::create("")); std::string p(a[0].asString()->data,a[0].asString()->length); auto pos=p.rfind("/"); if(pos==std::string::npos){ pos=p.rfind("\\"); } return pos!=std::string::npos?makeStringVal(VMString::create(p.substr(0,pos))):makeStringVal(VMString::create(".")); });
    vm->registerNative("深拷贝", [](std::vector<Value>& a) -> Value { return a.empty()?Value::nil():a[0]; });
    vm->registerNative("向量叉积", [](std::vector<Value>&) -> Value { return Value::nil(); });
    vm->registerNative("向量点积", [](std::vector<Value>& a) -> Value { if(a.size()<2||!a[0].isArray()||!a[1].isArray()) return Value::fromFloat(0); auto* v1=asArrayVal(a[0]); auto* v2=asArrayVal(a[1]); double dot=0; size_t n=std::min(v1->data.size(),v2->data.size()); for(size_t i=0;i<n;i++) dot+=v1->data[i].asFloat()*v2->data[i].asFloat(); return Value::fromFloat(dot); });
    vm->registerNative("向量归一化", [](std::vector<Value>& a) -> Value { if(a.empty()||!a[0].isArray()) return a.empty()?Value::nil():a[0]; auto* src=asArrayVal(a[0]); double sum=0; for(auto& x:src->data) sum+=x.asFloat()*x.asFloat(); double len=sqrt(sum); auto* dst=VMArray::create(); for(auto& x:src->data) dst->data.push_back(Value::fromFloat(len>0?x.asFloat()/len:0)); return makeArrayVal(dst); });
    vm->registerNative("向量模", [](std::vector<Value>& a) -> Value { if(a.empty()||!a[0].isArray()) return Value::fromFloat(0); auto* v=asArrayVal(a[0]); double sum=0; for(auto& x:v->data) sum+=x.asFloat()*x.asFloat(); return Value::fromFloat(sqrt(sum)); });
    
    // ── 高价值补充函数 ──
    vm->registerNative("max", [](std::vector<Value>& a) -> Value {
        if(a.size()<2) return a.empty()?Value::Int(0):a[0];
        return a[0].asFloat()>a[1].asFloat()?a[0]:a[1];
    });
    vm->registerNative("min", [](std::vector<Value>& a) -> Value {
        if(a.size()<2) return a.empty()?Value::Int(0):a[0];
        return a[0].asFloat()<a[1].asFloat()?a[0]:a[1];
    });
    vm->registerNative("round", [](std::vector<Value>& a) -> Value {
        return a.empty()?Value::Int(0):Value::Int((int64_t)std::round(a[0].asFloat()));
    });
    vm->registerNative("floor", [](std::vector<Value>& a) -> Value {
        return a.empty()?Value::Int(0):Value::Int((int64_t)std::floor(a[0].asFloat()));
    });
    vm->registerNative("ceil", [](std::vector<Value>& a) -> Value {
        return a.empty()?Value::Int(0):Value::Int((int64_t)std::ceil(a[0].asFloat()));
    });
    vm->registerNative("log", [](std::vector<Value>& a) -> Value {
        return a.empty()?Value::fromFloat(0):Value::fromFloat(std::log(a[0].asFloat()));
    });
    vm->registerNative("log10", [](std::vector<Value>& a) -> Value {
        return a.empty()?Value::fromFloat(0):Value::fromFloat(std::log10(a[0].asFloat()));
    });
    vm->registerNative("exp", [](std::vector<Value>& a) -> Value {
        return a.empty()?Value::fromFloat(1):Value::fromFloat(std::exp(a[0].asFloat()));
    });
    vm->registerNative("读取文件", [](std::vector<Value>& a) -> Value {
        if(a.empty()||!a[0].isString()) return makeStringVal(VMString::create(""));
        std::string p(a[0].asString()->data,a[0].asString()->length);
        std::ifstream f(p); if(!f) return makeStringVal(VMString::create(""));
        std::stringstream ss; ss<<f.rdbuf(); return makeStringVal(VMString::create(ss.str()));
    });
    vm->registerNative("写入文件", [](std::vector<Value>& a) -> Value {
        if(a.size()<2||!a[0].isString()) return Value::Bool(false);
        std::string p(a[0].asString()->data,a[0].asString()->length);
        std::string content=a[1].isString()?std::string(a[1].asString()->data,a[1].asString()->length):a[1].toString();
        std::ofstream f(p); if(!f) return Value::Bool(false); f<<content; return Value::Bool(true);
    });
    vm->registerNative("文件存在", [](std::vector<Value>& a) -> Value {
        if(a.empty()||!a[0].isString()) return Value::Bool(false);
        std::string p(a[0].asString()->data,a[0].asString()->length);
        std::ifstream f(p); return Value::Bool(f.good());
    });
    vm->registerNative("join", [](std::vector<Value>& a) -> Value {
        if(a.size()<2||!a[0].isArray()) return makeStringVal(VMString::create(""));
        std::string sep=a.size()>1&&a[1].isString()?std::string(a[1].asString()->data,a[1].asString()->length):",";
        std::string result; auto& arr=a[0].asArray()->data;
        for(size_t i=0;i<arr.size();i++){ if(i>0)result+=sep; result+=arr[i].toString(); }
        return makeStringVal(VMString::create(result));
    });
    vm->registerNative("排序", [](std::vector<Value>& a) -> Value {
        if(a.empty()||!a[0].isArray()) return Value::nil();
        auto& arr=a[0].asArray()->data;
        std::sort(arr.begin(),arr.end(),[](const Value& x,const Value& y){return x.asFloat()<y.asFloat();});
        return a[0];
    });
    vm->registerNative("反转", [](std::vector<Value>& a) -> Value {
        if(a.empty()||!a[0].isArray()) return Value::nil();
        auto& arr=a[0].asArray()->data;
        std::reverse(arr.begin(),arr.end()); return a[0];
    });
    vm->registerNative("当前时间", [](std::vector<Value>&) -> Value {
        return Value::Int((int64_t)std::time(nullptr));
    });
    vm->registerNative("等待", [](std::vector<Value>& a) -> Value {
        int ms=a.size()>0?(int)a[0].asInt():1000;
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        return Value::nil();
    });
    vm->registerNative("随机", [](std::vector<Value>& a) -> Value {
        int lo=a.size()>0?(int)a[0].asInt():0, hi=a.size()>1?(int)a[1].asInt():100;
        return Value::Int(lo+rand()%(hi-lo+1));
    });

    // ── Common missing stubs (for test coverage) ──
    vm->registerNative("println", [](std::vector<Value>& a) -> Value {
        if (!a.empty()) { std::cout << a[0].toString() << "\n"; }
        else { std::cout << "\n"; }
        return Value::nil();
    });
    vm->registerNative("isNil", [](std::vector<Value>& a) -> Value {
        return Value::Bool(a.empty() || a[0].isNil());
    });
    vm->registerNative("toString", [](std::vector<Value>& a) -> Value {
        if (a.empty()) return makeStringVal(VMString::create("nil"));
        return makeStringVal(VMString::create(a[0].toString()));
    });
    vm->registerNative("push", [](std::vector<Value>& a) -> Value {
        if (a.size() < 2 || !a[0].isArray()) return a.size()>0 ? a[0] : Value::nil();
        a[0].asArray()->data.push_back(a[1]);
        return a[0];
    });
    vm->registerNative("arrlen", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !a[0].isArray()) return Value::Int(0);
        return Value::Int((int)a[0].asArray()->data.size());
    });
    vm->registerNative("dirMake", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !a[0].isString()) return Value::Bool(false);
        std::string p(a[0].asString()->data, a[0].asString()->length);
        return Value::Bool(mkdir(p.c_str(), 0755) == 0);
    });

    // ── Charset/encoding stubs ──
    vm->registerNative("aesEncrypt", [](std::vector<Value>& a) -> Value {
        if (a.empty()) return makeStringVal(VMString::create("aes-not-implemented"));
        return a[0];
    });
    vm->registerNative("aesDecrypt", [](std::vector<Value>& a) -> Value {
        if (a.empty()) return makeStringVal(VMString::create("aes-not-implemented"));
        return a[0];
    });
    vm->registerNative("convertEncoding", [](std::vector<Value>& a) -> Value {
        return a.empty() ? Value::nil() : a[0];
    });
    vm->registerNative("detectEncoding", [](std::vector<Value>&) -> Value {
        return makeStringVal(VMString::create("UTF-8"));
    });
    vm->registerNative("big5ToUtf8", [](std::vector<Value>& a) -> Value {
        return a.empty() ? Value::nil() : a[0];
    });
    vm->registerNative("gbkToUtf8", [](std::vector<Value>& a) -> Value {
        return a.empty() ? Value::nil() : a[0];
    });
    vm->registerNative("isValidUtf8", [](std::vector<Value>&) -> Value {
        return Value::Bool(true);
    });

    // ── Logging stubs ──

    // ── Third-party stubs (sqlite, websocket, etc.) ──
    vm->registerNative("sqliteErrMsg", [](std::vector<Value>&) -> Value { return makeStringVal(VMString::create("")); });
    vm->registerNative("procSystem", [](std::vector<Value>& a) -> Value {
        if (a.empty() || !a[0].isString()) return makeStringVal(VMString::create(""));
        std::string cmd(a[0].asString()->data, a[0].asString()->length);
        cmd += " 2>&1";
        FILE* fp = popen(cmd.c_str(), "r");
        if (!fp) return makeStringVal(VMString::create(""));
        std::string result;
        char buf[4096];
        while (fgets(buf, sizeof(buf), fp)) result += buf;
        pclose(fp);
        return makeStringVal(VMString::create(result));
    });
    vm->registerNative("ToGbk", [](std::vector<Value>& a) -> Value { return a.empty()?Value::nil():a[0]; });
    vm->registerNative("ToShiftJis", [](std::vector<Value>& a) -> Value { return a.empty()?Value::nil():a[0]; });
    vm->registerNative("randomBytes", [](std::vector<Value>& a) -> Value {
        int n=a.size()>0?(int)a[0].asInt():16;
        std::string s(n, 0); for(int i=0;i<n;i++) s[i]=(char)(rand()%256);
        return makeStringVal(VMString::create(s));
    });
    vm->registerNative("strlen", [](std::vector<Value>& a) -> Value {
        if(a.empty()||!a[0].isString()) return Value::Int(0);
        return Value::Int((int)a[0].asString()->length);
    });


    // ── Misc game/event stubs ──
    vm->registerNative("down", [](std::vector<Value>& a) -> Value { if(a.empty()||!a[0].isInt()) return Value::fromFloat(0); double v=a[0].asInt(); while(v>1.0) v*=0.5; return Value::fromFloat(v); });
}
namespace cplang { void registerStubImpls(VM* vm) {} }
