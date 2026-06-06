// ═══════════════════════════════════════════════════════════════════
//  CP 语言数据库统一抽象接口
//  所有数据库驱动（内置 + 插件）必须实现此接口
// ═══════════════════════════════════════════════════════════════════
#pragma once
#include "vm/value.hpp"
#include <string>
#include <vector>

namespace cplang {

// ─── 结果集 ───
struct DriverResult {
    std::vector<std::string> columnNames;
    std::vector<std::vector<Value>> rows;  // 行优先
    int64_t affectedRows = 0;
    int64_t lastInsertId = 0;
    std::string errorMsg;
    bool success = false;
};

// ─── 数据库驱动抽象接口 ───
class IDatabaseDriver {
public:
    virtual ~IDatabaseDriver() = default;

    // 元信息
    virtual const char* name() const = 0;     // "sqlite", "mysql", "pg", "redis"

    // 连接管理
    virtual void* connect(const char* connStr, std::string& errMsg) = 0;
    virtual bool close(void* conn) = 0;
    virtual bool ping(void* conn) = 0;

    // 数据操作
    virtual DriverResult query(void* conn, const char* sql) = 0;
    virtual DriverResult exec(void* conn, const char* sql) = 0;

    // 事务
    virtual bool begin(void* conn) = 0;
    virtual bool commit(void* conn) = 0;
    virtual bool rollback(void* conn) = 0;

    // 安全
    virtual std::string escape(void* conn, const char* input) = 0;
};

// ─── 驱动注册表 ───
class DriverRegistry {
public:
    static DriverRegistry& instance();

    // 注册驱动（内置或插件）
    bool registerDriver(const char* name, IDatabaseDriver* driver);
    
    // 按名字查找
    IDatabaseDriver* getDriver(const char* name);
    
    // 列出所有已注册驱动
    std::vector<const char*> listDrivers();
    
    // 注册时是否接管生命周期
    void setOwnership(const char* name, bool owned);

private:
    DriverRegistry() = default;
    ~DriverRegistry();
    DriverRegistry(const DriverRegistry&) = delete;
    DriverRegistry& operator=(const DriverRegistry&) = delete;

    struct Entry {
        IDatabaseDriver* driver = nullptr;
        bool owned = false;  // 析构时是否 delete
    };
    std::unordered_map<std::string, Entry> drivers_;
};

// ─── 驱动注册辅助宏 ───
#define REGISTER_DRIVER(name, driverClass) \
    namespace { \
        struct AutoRegister_##driverClass { \
            AutoRegister_##driverClass() { \
                auto* d = new driverClass(); \
                DriverRegistry::instance().registerDriver(name, d); \
                DriverRegistry::instance().setOwnership(name, true); \
            } \
        } _autoReg_##driverClass; \
    }

} // namespace cplang
