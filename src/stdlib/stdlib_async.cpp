// CP语言 原生异步标准库（P9.3）
// 提供 Promise/承诺 原生对象和协程调度支持
#include "stdlib/stdlib.hpp"
#include <thread>
#include <chrono>
#ifdef _WIN32
#include <windows.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#endif

namespace cplang {

namespace async_ns {

// 创建新承诺
Value promiseNew(std::vector<Value>& args) {
    VM* vm = VM::current();
    VMPromise* p = vm->createPromise();
    
    // 如果传入了执行器函数 (executor)，调用它
    if (args.size() > 0 && (args[0].isFunction() || args[0].isClosure())) {
        // 创建 resolve 和 reject 回调
        // 简化实现：这里先返回空承诺，executor 支持稍后添加
        (void)args[0];
    }
    
    return makePromiseVal(p);
}

// 解决承诺
Value promiseResolve(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    
    // 如果第一个参数已经是承诺，直接返回
    if (args[0].isPromise()) {
        return args[0];
    }
    
    // 否则创建一个已解决的承诺
    VM* vm = VM::current();
    VMPromise* p = vm->createPromise();
    vm->resolvePromise(p, args[0]);
    return makePromiseVal(p);
}

// 拒绝承诺
Value promiseReject(std::vector<Value>& args) {
    Value reason = args.empty() ? Value::nil() : args[0];
    VM* vm = VM::current();
    VMPromise* p = vm->createPromise();
    vm->rejectPromise(p, reason);
    return makePromiseVal(p);
}

// Promise.then
Value promiseThen(std::vector<Value>& args) {
    if (args.empty() || !args[0].isPromise()) return Value::nil();
    VMPromise* promise = args[0].asPromise();
    Value onFulfilled = args.size() > 1 ? args[1] : Value::nil();
    
    VM* vm = VM::current();
    
    // 如果承诺已经完成，立即调度回调
    if (promise->state == VMPromise::FULFILLED) {
        if (!onFulfilled.isNil()) {
            vm->enqueueMicrotask(onFulfilled, promise->result);
            vm->runMicrotasks();
        }
        return args[0];  // 返回原承诺
    }
    
    // 否则注册回调
    if (!onFulfilled.isNil()) {
        promise->thenCallbacks.push_back(onFulfilled);
    }
    
    return args[0];  // 返回原承诺（简化实现，链式调用返回新承诺）
}

// Promise.catch
Value promiseCatch(std::vector<Value>& args) {
    if (args.empty() || !args[0].isPromise()) return Value::nil();
    VMPromise* promise = args[0].asPromise();
    Value onRejected = args.size() > 1 ? args[1] : Value::nil();
    
    VM* vm = VM::current();
    
    if (promise->state == VMPromise::REJECTED) {
        if (!onRejected.isNil()) {
            vm->enqueueMicrotask(onRejected, promise->result);
            vm->runMicrotasks();
        }
        return args[0];
    }
    
    if (!onRejected.isNil()) {
        promise->catchCallbacks.push_back(onRejected);
    }
    
    return args[0];
}

// 手动解决承诺
Value promiseDoResolve(std::vector<Value>& args) {
    if (args.empty() || !args[0].isPromise()) return Value::Bool(false);
    VMPromise* promise = args[0].asPromise();
    Value value = args.size() > 1 ? args[1] : Value::nil();
    VM* vm = VM::current();
    vm->resolvePromise(promise, value);
    return Value::Bool(true);
}

// 手动拒绝承诺
Value promiseDoReject(std::vector<Value>& args) {
    if (args.empty() || !args[0].isPromise()) return Value::Bool(false);
    VMPromise* promise = args[0].asPromise();
    Value reason = args.size() > 1 ? args[1] : Value::nil();
    VM* vm = VM::current();
    vm->rejectPromise(promise, reason);
    return Value::Bool(true);
}

// 检查承诺是否已完成
Value promiseIsDone(std::vector<Value>& args) {
    if (args.empty() || !args[0].isPromise()) return Value::Bool(false);
    VMPromise* promise = args[0].asPromise();
    return Value::Bool(promise->state != VMPromise::PENDING);
}

// 获取承诺结果
Value promiseResult(std::vector<Value>& args) {
    if (args.empty() || !args[0].isPromise()) return Value::nil();
    VMPromise* promise = args[0].asPromise();
    return promise->result;
}

// 同步等待承诺完成（简化：忙等待，不推荐）
Value promiseWait(std::vector<Value>& args) {
    if (args.empty() || !args[0].isPromise()) return Value::nil();
    VMPromise* promise = args[0].asPromise();
    VM* vm = VM::current();
    
    // 简单的忙等待：运行微任务直到承诺完成
    int maxIterations = 10000;
    while (promise->state == VMPromise::PENDING && maxIterations > 0) {
        vm->runMicrotasks();
        maxIterations--;
    }
    
    return promise->result;
}

// 延迟承诺（延迟指定毫秒后解决）
Value promiseDelay(std::vector<Value>& args) {
    // 简化实现：立即解决，不真正延迟
    // 真正的延迟需要事件循环和定时器支持（P9.4）
    Value value = args.size() > 1 ? args[1] : Value::nil();
    VM* vm = VM::current();
    VMPromise* p = vm->createPromise();
    vm->resolvePromise(p, value);
    return makePromiseVal(p);
}

// Promise.all：等待所有承诺完成
Value promiseAll(std::vector<Value>& args) {
    VM* vm = VM::current();
    VMPromise* result = vm->createPromise();
    
    // 如果没有参数，立即解决为空数组
    if (args.empty()) {
        VMArray* arr = VMArray::create();
        vm->resolvePromise(result, makeArrayVal(arr));
        return makePromiseVal(result);
    }
    
    // 收集所有结果
    VMArray* arr = VMArray::create();
    Int64 total = args.size();
    Int64 completed = 0;
    bool rejected = false;
    
    for (Int64 i = 0; i < total; i++) {
        arr->data.push_back(Value::nil());
        
        if (args[i].isPromise()) {
            VMPromise* p = args[i].asPromise();
            if (p->state == VMPromise::FULFILLED) {
                arr->data[i] = p->result;
                completed++;
            } else if (p->state == VMPromise::REJECTED) {
                if (!rejected) {
                    rejected = true;
                    vm->rejectPromise(result, p->result);
                }
            } else {
                // 简化实现：同步等待承诺完成
                // 完整实现需要注册回调
                while (p->state == VMPromise::PENDING) {
                    vm->runMicrotasks();
                }
                if (p->state == VMPromise::FULFILLED) {
                    arr->data[i] = p->result;
                    completed++;
                } else {
                    if (!rejected) {
                        rejected = true;
                        vm->rejectPromise(result, p->result);
                    }
                }
            }
        } else {
            // 非承诺值，直接放入
            arr->data[i] = args[i];
            completed++;
        }
    }
    
    if (!rejected && completed == total) {
        vm->resolvePromise(result, makeArrayVal(arr));
    }
    
    return makePromiseVal(result);
}

// Promise.race：竞速，第一个完成的承诺获胜
Value promiseRace(std::vector<Value>& args) {
    VM* vm = VM::current();
    VMPromise* result = vm->createPromise();
    
    if (args.empty()) {
        // 空数组：永远不解决
        return makePromiseVal(result);
    }
    
    bool settled = false;
    
    // 简化实现：找到第一个已完成的承诺，或者第一个承诺
    for (auto& arg : args) {
        if (arg.isPromise()) {
            VMPromise* p = arg.asPromise();
            if (p->state == VMPromise::FULFILLED) {
                if (!settled) {
                    settled = true;
                    vm->resolvePromise(result, p->result);
                }
                break;
            } else if (p->state == VMPromise::REJECTED) {
                if (!settled) {
                    settled = true;
                    vm->rejectPromise(result, p->result);
                }
                break;
            }
        } else {
            // 非承诺值直接胜出
            if (!settled) {
                settled = true;
                vm->resolvePromise(result, arg);
            }
            break;
        }
    }
    
    return makePromiseVal(result);
}

// Promise.allSettled：所有承诺都完成（不论成功失败）
Value promiseAllSettled(std::vector<Value>& args) {
    VM* vm = VM::current();
    VMPromise* result = vm->createPromise();
    
    VMArray* arr = VMArray::create();
    
    for (auto& arg : args) {
        VMTable* item = VMTable::create();
        if (arg.isPromise()) {
            VMPromise* p = arg.asPromise();
            // 简化：同步等待
            while (p->state == VMPromise::PENDING) {
                vm->runMicrotasks();
            }
            if (p->state == VMPromise::FULFILLED) {
                item->set(makeStringVal(VMString::create("状态")), makeStringVal(VMString::create("已完成")));
                item->set(makeStringVal(VMString::create("值")), p->result);
            } else {
                item->set(makeStringVal(VMString::create("状态")), makeStringVal(VMString::create("已拒绝")));
                item->set(makeStringVal(VMString::create("原因")), p->result);
            }
        } else {
            item->set(makeStringVal(VMString::create("状态")), makeStringVal(VMString::create("已完成")));
            item->set(makeStringVal(VMString::create("值")), arg);
        }
        arr->data.push_back(makeTableVal(item));
    }
    
    vm->resolvePromise(result, makeArrayVal(arr));
    return makePromiseVal(result);
}

// Promise.any：任意一个成功就返回成功，全部失败才拒绝
Value promiseAny(std::vector<Value>& args) {
    VM* vm = VM::current();
    VMPromise* result = vm->createPromise();
    
    if (args.empty()) {
        vm->rejectPromise(result, makeStringVal(VMString::create("没有承诺")));
        return makePromiseVal(result);
    }
    
    bool fulfilled = false;
    Int64 rejectedCount = 0;
    VMArray* errors = VMArray::create();
    
    for (auto& arg : args) {
        if (fulfilled) break;
        
        if (arg.isPromise()) {
            VMPromise* p = arg.asPromise();
            // 简化：同步等待
            while (p->state == VMPromise::PENDING) {
                vm->runMicrotasks();
            }
            if (p->state == VMPromise::FULFILLED) {
                if (!fulfilled) {
                    fulfilled = true;
                    vm->resolvePromise(result, p->result);
                }
            } else {
                rejectedCount++;
                errors->data.push_back(p->result);
            }
        } else {
            if (!fulfilled) {
                fulfilled = true;
                vm->resolvePromise(result, arg);
            }
        }
    }
    
    if (!fulfilled && rejectedCount == (Int64)args.size()) {
        vm->rejectPromise(result, makeArrayVal(errors));
    }
    
    return makePromiseVal(result);
}

// Promise.finally：无论成功失败都执行
Value promiseFinally(std::vector<Value>& args) {
    if (args.empty() || !args[0].isPromise()) return Value::nil();
    VMPromise* promise = args[0].asPromise();
    Value onFinally = args.size() > 1 ? args[1] : Value::nil();
    
    VM* vm = VM::current();
    
    // 简化实现：同步等待承诺完成，然后执行 finally
    while (promise->state == VMPromise::PENDING) {
        vm->runMicrotasks();
    }
    
    if (!onFinally.isNil()) {
        std::vector<Value> cbArgs = {};
        vm->callFunction(onFinally, cbArgs);
    }
    
    // 返回原承诺
    return args[0];
}

// ========== 定时器 ==========

// setTimeout / 延迟执行：延迟指定毫秒后执行回调（非阻塞）
Value asyncSetTimeout(std::vector<Value>& args) {
    VM* vm = VM::current();
    if (args.empty()) return Value::nil();
    Value callback = args[0];
    Int64 delayMs = args.size() > 1 ? args[1].asInt() : 0;
    
    if (delayMs <= 0) {
        vm->enqueueMicrotask(callback, Value::nil());
        vm->runMicrotasks();
    } else {
        EventLoop* loop = vm->getEventLoop();
        loop->start();
        loop->setTimeout([vm, callback]() {
            std::vector<Value> cbArgs = {};
            vm->callFunction(callback, cbArgs);
        }, static_cast<uint64_t>(delayMs));
    }
    
    return Value::Bool(true);
}

// setInterval / 周期执行：每隔指定毫秒执行一次回调
// 简化实现：不支持（需要事件循环）
Value asyncSetInterval(std::vector<Value>& args) {
    // 简化实现：返回nil，表示不支持
    return Value::nil();
}

// clearTimeout / 清除延迟
Value asyncClearTimeout(std::vector<Value>& args) {
    // 简化实现：什么都不做
    return Value::Bool(true);
}

// clearInterval / 清除周期
Value asyncClearInterval(std::vector<Value>& args) {
    return Value::Bool(true);
}

// ========== 异步工具 ==========

// 休眠（异步版本，返回 Promise）
Value asyncSleep(std::vector<Value>& args) {
    VM* vm = VM::current();
    Int64 ms = args.empty() ? 0 : args[0].asInt();
    
    VMPromise* p = vm->createPromise();
    
    if (ms <= 0) {
        vm->resolvePromise(p, Value::nil());
        vm->runMicrotasks();
    } else {
        EventLoop* loop = vm->getEventLoop();
        loop->start();
        loop->setTimeout([p, vm]() {
            vm->resolvePromise(p, Value::nil());
            vm->runMicrotasks();
        }, static_cast<uint64_t>(ms));
    }
    
    return makePromiseVal(p);
}

// 异步延时执行（setTimeout替代实现）

Value asyncPromisify(std::vector<Value>& args) {
    if (args.empty()) return Value::nil();
    return args[0];
}

Value asyncNextTick(std::vector<Value>& args) {
    VM* vm = VM::current();
    if (!args.empty()) {
        vm->enqueueMicrotask(args[0], Value::nil());
        vm->runMicrotasks();
    }
    return Value::nil();
}

Value asyncRunLoop(std::vector<Value>& args) {
    VM* vm = VM::current();
    vm->runMicrotasks();
    return Value::Bool(true);
}

// ═══════════════════════════════════════════════════════════════
//  异步文件 I/O
// ═══════════════════════════════════════════════════════════════

Value asyncReadFile(std::vector<Value>& args) {
    VM* vm = VM::current();
    if (args.empty()) return Value::nil();
    std::string filepath = std::string(args[0].asString()->data, args[0].asString()->length);
    
    VMPromise* p = vm->createPromise();
    Value promiseVal = makePromiseVal(p);
    
    EventLoop* loop = vm->getEventLoop();
    loop->start();
    
    loop->runAsync(
        [filepath]() {
            std::ifstream ifs(filepath, std::ios::binary | std::ios::ate);
            if (!ifs) return;
            size_t size = ifs.tellg();
            ifs.seekg(0);
            std::vector<char> buf(size);
            ifs.read(buf.data(), size);
        },
        [p, vm, filepath]() {
            std::ifstream ifs(filepath, std::ios::binary | std::ios::ate);
            if (!ifs) {
                vm->rejectPromise(p, makeStringVal(VMString::create("file not found")));
                return;
            }
            size_t size = ifs.tellg();
            ifs.seekg(0);
            VMByteArray* ba = VMByteArray::create(static_cast<UInt32>(size));
            ifs.read(reinterpret_cast<char*>(ba->data), size);
            vm->resolvePromise(p, makeByteArrayVal(ba));
        }
    );
    
    return promiseVal;
}

Value asyncWriteFile(std::vector<Value>& args) {
    VM* vm = VM::current();
    if (args.size() < 2) return Value::nil();
    std::string filepath = std::string(args[0].asString()->data, args[0].asString()->length);
    Value data = args[1];
    
    VMPromise* p = vm->createPromise();
    
    EventLoop* loop = vm->getEventLoop();
    loop->start();
    
    loop->runAsync(
        [filepath, data]() {
            std::ofstream ofs(filepath, std::ios::binary);
            if (!ofs) return;
            if (isByteArrayVal(data)) {
                auto* ba = asByteArrayVal(data);
                ofs.write(reinterpret_cast<const char*>(ba->ptr()), ba->length);
            } else if (data.isString()) {
                auto* s = data.asString();
                ofs.write(s->data, s->length);
            }
        },
        [p, vm]() {
            vm->resolvePromise(p, Value::Bool(true));
        }
    );
    
    return makePromiseVal(p);
}

// ═══════════════════════════════════════════════════════════════
//  异步 HTTP 客户端（WinInet）
// ═══════════════════════════════════════════════════════════════

#ifdef _WIN32
#include <wininet.h>
#endif

Value asyncHttpGet(std::vector<Value>& args) {
    VM* vm = VM::current();
    if (args.empty()) return Value::nil();
    std::string url = std::string(args[0].asString()->data, args[0].asString()->length);
    
    VMPromise* p = vm->createPromise();
    
    EventLoop* loop = vm->getEventLoop();
    loop->start();
    
    loop->runAsync(
        [url]() {
#ifdef _WIN32
            HINTERNET hInternet = InternetOpenA("CPLang/0.9", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
            if (hInternet) {
                HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
                if (hUrl) { InternetCloseHandle(hUrl); }
                InternetCloseHandle(hInternet);
            }
#endif
        },
        [p, vm, url]() {
#ifdef _WIN32
            HINTERNET hInternet = InternetOpenA("CPLang/0.9", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
            if (!hInternet) {
                vm->rejectPromise(p, makeStringVal(VMString::create("network init failed")));
                return;
            }
            HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
            if (!hUrl) {
                InternetCloseHandle(hInternet);
                vm->rejectPromise(p, makeStringVal(VMString::create("request failed")));
                return;
            }
            std::string body;
            char buf[4096];
            DWORD bytesRead;
            while (InternetReadFile(hUrl, buf, sizeof(buf), &bytesRead) && bytesRead > 0)
                body.append(buf, bytesRead);
            InternetCloseHandle(hUrl);
            InternetCloseHandle(hInternet);
            vm->resolvePromise(p, makeStringVal(VMString::create(body.c_str())));
#else
            vm->rejectPromise(p, makeStringVal(VMString::create("platform not supported")));
#endif
        }
    );
    
    return makePromiseVal(p);
}

} // namespace async_ns

void StdLib::registerAsync(VM* vm) {
    using namespace async_ns;
    
    // 核心 Promise 函数
    registerFunction(vm, "promiseNew",       promiseNew);
    registerFunction(vm, "promiseResolve",   promiseResolve);
    registerFunction(vm, "promiseReject",    promiseReject);
    registerFunction(vm, "promiseThen",      promiseThen);
    registerFunction(vm, "promiseCatch",     promiseCatch);
    registerFunction(vm, "promiseFinally",   promiseFinally);
    registerFunction(vm, "promiseDoResolve", promiseDoResolve);
    registerFunction(vm, "promiseDoReject",  promiseDoReject);
    registerFunction(vm, "promiseIsDone",    promiseIsDone);
    registerFunction(vm, "promiseResult",    promiseResult);
    registerFunction(vm, "promiseWait",      promiseWait);
    registerFunction(vm, "promiseDelay",     promiseDelay);
    
    // Promise 组合子
    registerFunction(vm, "promiseAll",       promiseAll);
    registerFunction(vm, "promiseRace",      promiseRace);
    registerFunction(vm, "promiseAllSettled", promiseAllSettled);
    registerFunction(vm, "promiseAny",       promiseAny);
    
    // 定时器
    registerFunction(vm, "setTimeout",       asyncSetTimeout);
    registerFunction(vm, "setInterval",      asyncSetInterval);
    registerFunction(vm, "clearTimeout",     asyncClearTimeout);
    registerFunction(vm, "clearInterval",    asyncClearInterval);
    
    // 异步工具
    registerFunction(vm, "asyncSleep",       asyncSleep);
    registerFunction(vm, "promisify",        asyncPromisify);
    registerFunction(vm, "nextTick",         asyncNextTick);
    registerFunction(vm, "runEventLoop",     asyncRunLoop);
    
    // 中文别名 - 核心 Promise
    registerAlias(vm, "承诺新建",            "promiseNew");
    registerAlias(vm, "承诺解决",            "promiseResolve");
    registerAlias(vm, "承诺拒绝",            "promiseReject");
    registerAlias(vm, "承诺然后",            "promiseThen");
    registerAlias(vm, "承诺捕获",            "promiseCatch");
    registerAlias(vm, "承诺最终",            "promiseFinally");
    registerAlias(vm, "承诺执行解决",        "promiseDoResolve");
    registerAlias(vm, "承诺执行拒绝",        "promiseDoReject");
    registerAlias(vm, "承诺完成",            "promiseIsDone");
    registerAlias(vm, "承诺结果",            "promiseResult");
    registerAlias(vm, "承诺等待",            "promiseWait");
    registerAlias(vm, "承诺延迟",            "promiseDelay");
    
    // 中文别名 - 组合子
    registerAlias(vm, "承诺全部",            "promiseAll");
    registerAlias(vm, "承诺竞速",            "promiseRace");
    registerAlias(vm, "承诺全部完成",        "promiseAllSettled");
    registerAlias(vm, "承诺任一",            "promiseAny");
    
    // 中文别名 - 定时器
    registerAlias(vm, "延迟执行",            "setTimeout");
    registerAlias(vm, "周期执行",            "setInterval");
    registerAlias(vm, "清除延迟",            "clearTimeout");
    registerAlias(vm, "清除周期",            "clearInterval");
    
    // 中文别名 - 异步工具
    registerAlias(vm, "异步休眠",            "asyncSleep");
    registerAlias(vm, "承诺化",              "promisify");
    registerAlias(vm, "下一帧",              "nextTick");
    registerAlias(vm, "运行事件循环",        "runEventLoop");
    
    // 兼容旧代码：async.cp 库中使用的函数名
    registerAlias(vm, "已完成",              "promiseResolve");
    registerAlias(vm, "resolved",            "promiseResolve");
    registerAlias(vm, "同步等待",            "promiseWait");
    registerAlias(vm, "waitFor",             "promiseWait");
    
    // 异步 I/O (v0.9.3)
    registerFunction(vm, "asyncReadFile",    asyncReadFile);
    registerFunction(vm, "asyncWriteFile",   asyncWriteFile);
    registerFunction(vm, "asyncHttpGet",     asyncHttpGet);
    registerAlias(vm, "异步读取文件",          "asyncReadFile");
    registerAlias(vm, "异步写入文件",          "asyncWriteFile");
    registerAlias(vm, "异步HTTP获取",          "asyncHttpGet");
}

} // namespace cplang