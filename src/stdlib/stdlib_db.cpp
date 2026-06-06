#include "stdlib/stdlib.hpp"

namespace cplang {

// ╔═════════════════════════════════════════════════════════════════╗
//  ║  MySQL + PostgreSQL 数据库支持（运行时动态加载）              ║
//  ║  无需预装客户端库，DLL 存在则自动可用                         ║
//  ╚═════════════════════════════════════════════════════════════════╝

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

// ═══════════════════════════════════════════════════════════════════
//  通用动态库加载
// ═══════════════════════════════════════════════════════════════════
#ifdef _WIN32
static HMODULE loadLib(const char* name) { return LoadLibraryA(name); }
static void* getSym(HMODULE h, const char* name) { return (void*)GetProcAddress(h, name); }
using LibHandle = HMODULE;
static void freeLib(LibHandle h) { if (h) FreeLibrary(h); }
#else
static void* loadLib(const char* name) { return dlopen(name, RTLD_LAZY); }
static void* getSym(void* h, const char* name) { return dlsym(h, name); }
using LibHandle = void*;
static void freeLib(LibHandle h) { if (h) dlclose(h); }
#endif

// ══════════════════════════════════════════════════════════════════╗
//  MySQL 运行时绑定（跨平台 Win/Linux，兼容 MySQL 5.7+ / 8.0+ / MariaDB）
// ══════════════════════════════════════════════════════════════════╝
// MySQL MYSQL_FIELD 最小定义（name 在偏移为 0 处）
struct MysqlField { char* name; char* org_name; char* table; char* org_table; char* db; char* catalog; char* def; unsigned long length; unsigned long max_length; unsigned int name_length; unsigned int org_name_length; unsigned int table_length; unsigned int org_table_length; unsigned int db_length; unsigned int catalog_length; unsigned int def_length; unsigned int flags; unsigned int decimals; unsigned int charsetnr; int type; void* extension; };

// MySQL 客户端标志（mysql_com.h 常量，避免依赖 MySQL 头文件）
#define MYSQL_CP_CLIENT_LONG_PASSWORD              1UL
#define MYSQL_CP_CLIENT_PROTOCOL_41                512UL       // 0x200
#define MYSQL_CP_CLIENT_SECURE_CONNECTION          32768UL     // 0x8000
#define MYSQL_CP_CLIENT_MULTI_STATEMENTS           65536UL     // 0x10000
#define MYSQL_CP_CLIENT_MULTI_RESULTS              131072UL    // 0x20000
#define MYSQL_CP_CLIENT_PLUGIN_AUTH                524288UL    // 0x80000
#define MYSQL_CP_CLIENT_PLUGIN_AUTH_LENENC_DATA    2097152UL   // 0x200000
#define MYSQL_CP_CLIENT_DEPRECATE_EOF              16777216UL  // 0x1000000

// mysql_options 枚举常量
enum {
    MYSQL_CP_OPT_CONNECT_TIMEOUT   = 0,
    MYSQL_CP_OPT_COMPRESS          = 1,
    MYSQL_CP_OPT_NAMED_PIPE        = 2,
    MYSQL_CP_OPT_RECONNECT         = 20,
    MYSQL_CP_SET_CHARSET_NAME      = 7,
    MYSQL_CP_OPT_SSL_MODE          = 120,
    MYSQL_CP_OPT_GET_SERVER_PUBLIC_KEY = 123,
};

// mysql_ssl_mode 枚举
enum {
    MYSQL_CP_SSL_MODE_DISABLED      = 1,
    MYSQL_CP_SSL_MODE_PREFERRED     = 2,
    MYSQL_CP_SSL_MODE_REQUIRED      = 3,
    MYSQL_CP_SSL_MODE_VERIFY_CA     = 4,
    MYSQL_CP_SSL_MODE_VERIFY_IDENTITY = 5,
};

// 默认客户端标志组（兼容 MySQL 5.5 ~ 8.4 + MariaDB）
static const unsigned long MYSQL_CP_DEFAULT_CLIENT_FLAGS =
    MYSQL_CP_CLIENT_PLUGIN_AUTH
    | MYSQL_CP_CLIENT_PLUGIN_AUTH_LENENC_DATA
    | MYSQL_CP_CLIENT_SECURE_CONNECTION
    | MYSQL_CP_CLIENT_PROTOCOL_41
    | MYSQL_CP_CLIENT_MULTI_RESULTS
    | MYSQL_CP_CLIENT_MULTI_STATEMENTS
    | MYSQL_CP_CLIENT_DEPRECATE_EOF;

namespace mysql_ns {

// ─── 各连接的最近错误消息存储 ──────────────────────────────
static std::unordered_map<void*, std::string> g_mysqlLastErr;

static void setLastErr(void* conn, const char* msg) {
    if (conn && msg) g_mysqlLastErr[conn] = msg;
}
static const char* getLastErr(void* conn) {
    auto it = g_mysqlLastErr.find(conn);
    return (it != g_mysqlLastErr.end()) ? it->second.c_str() : "";
}
static void clearLastErr(void* conn) {
    g_mysqlLastErr.erase(conn);
}

struct MySQLAPI {
    LibHandle lib = nullptr;
    bool loaded = false;

    using mysql_init_t             = void* (*)(void*);
    using mysql_real_connect_t     = void* (*)(void*, const char*, const char*, const char*, const char*, unsigned int, const char*, unsigned long);
    using mysql_close_t            = void (*)(void*);
    using mysql_error_t            = const char* (*)(void*);
    using mysql_query_t            = int (*)(void*, const char*);
    using mysql_store_result_t     = void* (*)(void*);
    using mysql_free_result_t      = void (*)(void*);
    using mysql_fetch_row_t        = char** (*)(void*);
    using mysql_num_fields_t       = unsigned int (*)(void*);
    using mysql_fetch_fields_t     = void* (*)(void*);
    using mysql_fetch_lengths_t    = unsigned long* (*)(void*);
    using mysql_insert_id_t        = unsigned long long (*)(void*);
    using mysql_affected_rows_t    = unsigned long long (*)(void*);
    using mysql_ping_t             = int (*)(void*);
    using mysql_real_escape_string_t = unsigned long (*)(void*, char*, const char*, unsigned long);
    using mysql_select_db_t        = int (*)(void*, const char*);
    using mysql_list_dbs_t         = void* (*)(void*, const char*);
    using mysql_list_tables_t      = void* (*)(void*, const char*);
    using mysql_field_count_t      = unsigned int (*)(void*);
    using mysql_options_t          = int (*)(void*, int, const void*);
    using mysql_get_client_info_t  = const char* (*)();

    mysql_init_t          mysql_init = nullptr;
    mysql_real_connect_t  mysql_real_connect = nullptr;
    mysql_close_t         mysql_close = nullptr;
    mysql_error_t         mysql_error = nullptr;
    mysql_query_t         mysql_query = nullptr;
    mysql_store_result_t  mysql_store_result = nullptr;
    mysql_free_result_t   mysql_free_result = nullptr;
    mysql_fetch_row_t     mysql_fetch_row = nullptr;
    mysql_num_fields_t    mysql_num_fields = nullptr;
    mysql_fetch_fields_t  mysql_fetch_fields = nullptr;
    mysql_fetch_lengths_t mysql_fetch_lengths = nullptr;
    mysql_insert_id_t     mysql_insert_id = nullptr;
    mysql_affected_rows_t mysql_affected_rows = nullptr;
    mysql_ping_t          mysql_ping = nullptr;
    mysql_real_escape_string_t mysql_real_escape_string = nullptr;
    mysql_select_db_t     mysql_select_db = nullptr;
    mysql_list_dbs_t      mysql_list_dbs = nullptr;
    mysql_list_tables_t   mysql_list_tables = nullptr;
    mysql_field_count_t   mysql_field_count = nullptr;
    mysql_options_t       mysql_options = nullptr;
    mysql_get_client_info_t mysql_get_client_info = nullptr;

    bool tryLoad() {
        if (loaded) return lib != nullptr;
        loaded = true;

    #ifdef _WIN32
        static const char* candidates[] = {
            "libmysql.dll",        // MySQL 8.0 官方
            "libmariadb.dll",      // MariaDB Connector/C
            "libmysql-8.dll",      // 某些定制版本
            nullptr
        };
    #else
        static const char* candidates[] = {
            "libmysqlclient.so.21",    // MySQL 8.0
            "libmysqlclient.so.22",    // MySQL 8.4
            "libmariadb.so.3",         // MariaDB 10.x
            "libmysqlclient.so.20",    // MySQL 5.7
            "libmysqlclient.so",       // 通用符号链接
            "libmariadb.so",           // MariaDB 通用
            nullptr
        };
    #endif

        for (const char** p = candidates; *p; ++p) {
            lib = (LibHandle)loadLib(*p);
            if (lib) break;
        }
        if (!lib) return false;

        // 必须的函数（连接和查询的核心）
        #define L(sym) sym = (decltype(sym))getSym(lib, #sym); if (!sym) { freeLib(lib); lib = nullptr; return false; }
        L(mysql_init);      L(mysql_real_connect); L(mysql_close);   L(mysql_error);
        L(mysql_query);     L(mysql_store_result); L(mysql_free_result); L(mysql_fetch_row);
        L(mysql_num_fields); L(mysql_fetch_fields); L(mysql_fetch_lengths);
        L(mysql_insert_id); L(mysql_affected_rows); L(mysql_ping);
        L(mysql_real_escape_string); L(mysql_select_db);
        L(mysql_list_dbs);  L(mysql_list_tables);   L(mysql_field_count);
        #undef L

        // 可选函数（旧版库可能没有）
        #define O(sym) sym = (decltype(sym))getSym(lib, #sym)
        O(mysql_options);
        O(mysql_get_client_info);
        #undef O

        return true;
    }
};

static MySQLAPI& mysqlAPI() {
    static MySQLAPI api;
    return api;
}

static std::unordered_map<VMTable*,void*> mysqlDbMap;

static void* getDb(Value& v) {
    if (!v.isTable()) return nullptr;
    auto it = mysqlDbMap.find(v.asTable());
    return it != mysqlDbMap.end() ? it->second : nullptr;
}

// ─── 连接前配置 ────────────────────────────────────────────
static bool configureBeforeConnect(void* conn, const char* charset) {
    auto& api = mysqlAPI();

    // 1. 设置字符集 utf8mb4（必须在 mysql_real_connect 之前）
    if (api.mysql_options && charset && charset[0]) {
        api.mysql_options(conn, MYSQL_CP_SET_CHARSET_NAME, charset);
    }

    // 2. SSL mode = PREFERRED (尝试 SSL，不支持则回退明文)
    //    对云数据库兼容至关重要：阿里云RDS / 腾讯云CDB 默认开启SSL
    if (api.mysql_options) {
        int sslMode = MYSQL_CP_SSL_MODE_PREFERRED;
        api.mysql_options(conn, MYSQL_CP_OPT_SSL_MODE, &sslMode);
    }

    // 3. 自动重连（网络闪断恢复）
    if (api.mysql_options) {
        bool reconnect = true;
        api.mysql_options(conn, MYSQL_CP_OPT_RECONNECT, &reconnect);
    }

    return true;
}

// mysqlConnect(host, user, password, database, port=3306, charset="utf8mb4")
Value connect_(std::vector<Value>& args) {
    auto& api = mysqlAPI();
    if (!api.tryLoad()) return Value::nil();
    if (args.size() < 4) return Value::nil();

    const char* host = args[0].isString() ? args[0].asString()->data : "127.0.0.1";
    const char* user = args[1].isString() ? args[1].asString()->data : "root";
    const char* pass = args[2].isString() ? args[2].asString()->data : "";
    const char* db   = args[3].isString() ? args[3].asString()->data : "";
    unsigned int port = (args.size() >= 5 && args[4].isInt())
        ? static_cast<unsigned int>(args[4].asInt()) : 3306;
    const char* charset = (args.size() >= 6 && args[5].isString())
        ? args[5].asString()->data : "utf8mb4";

    void* conn = api.mysql_init(nullptr);
    if (!conn) return Value::nil();

    configureBeforeConnect(conn, charset);

    void* result = api.mysql_real_connect(
        conn, host, user, pass, db, port, nullptr,
        MYSQL_CP_DEFAULT_CLIENT_FLAGS);

    if (!result) {
        const char* err = api.mysql_error(conn);
        setLastErr(conn, err ? err : "连接被拒");
        api.mysql_close(conn);
        return Value::nil();
    }

    VMTable* tbl = VMTable::create();
    mysqlDbMap[tbl] = conn;
    return makeTableVal(tbl);
}

Value query_(std::vector<Value>& args) {
    auto& api = mysqlAPI();
    if (!api.tryLoad() || args.size() < 2) return Value::nil();
    void* db = getDb(args[0]);
    if (!db) return Value::nil();
    std::string sql(args[1].asString()->data, args[1].asString()->length);
    if (api.mysql_query(db, sql.c_str()) != 0) {
        setLastErr(db, api.mysql_error(db));
        return Value::nil();
    }

    void* result = api.mysql_store_result(db);
    if (!result) {
        if (api.mysql_field_count(db) == 0) return makeArrayVal(VMArray::create());
        setLastErr(db, api.mysql_error(db));
        return Value::nil();
    }

    unsigned int numFields = api.mysql_num_fields(result);
    auto* fields = (MysqlField*)api.mysql_fetch_fields(result);

    VMArray* rows = VMArray::create();
    char** row;
    while ((row = api.mysql_fetch_row(result))) {
        VMTable* tbl = VMTable::create();
        for (unsigned int i = 0; i < numFields; i++) {
            Value key = makeStringVal(VMString::create(fields[i].name));
            Value val = row[i] ? makeStringVal(VMString::create(row[i])) : Value::nil();
            tbl->set(key, val);
        }
        rows->data.push_back(makeTableVal(tbl));
    }
    api.mysql_free_result(result);
    return makeArrayVal(rows);
}

Value exec_(std::vector<Value>& args) {
    auto& api = mysqlAPI();
    if (!api.tryLoad() || args.size() < 2) return Value::Bool(false);
    void* db = getDb(args[0]);
    if (!db) return Value::Bool(false);
    std::string sql(args[1].asString()->data, args[1].asString()->length);
    bool ok = (api.mysql_query(db, sql.c_str()) == 0);
    if (!ok) setLastErr(db, api.mysql_error(db));
    return Value::Bool(ok);
}

Value close_(std::vector<Value>& args) {
    auto& api = mysqlAPI();
    if (!api.tryLoad() || args.empty()) return Value::Bool(false);
    void* db = getDb(args[0]);
    if (!db) return Value::Bool(false);
    api.mysql_close(db);
    clearLastErr(db);
    if (args[0].isTable()) mysqlDbMap.erase(args[0].asTable());
    return Value::Bool(true);
}

Value errMsg_(std::vector<Value>& args) {
    auto& api = mysqlAPI();
    if (!api.tryLoad()) return makeStringVal(VMString::create("MySQL 客户端库未安装"));
    if (args.empty()) return makeStringVal(VMString::create("缺少连接句柄参数"));
    void* db = getDb(args[0]);
    if (!db) return makeStringVal(VMString::create("invalid handle"));

    // 优先使用缓存的错误（连接失败时的错误）
    const char* cached = getLastErr(db);
    if (cached && cached[0]) return makeStringVal(VMString::create(cached));

    const char* err = api.mysql_error(db);
    return makeStringVal(VMString::create(err ? err : "未知错误"));
}

Value insertId_(std::vector<Value>& args) {
    auto& api = mysqlAPI();
    if (!api.tryLoad() || args.empty()) return Value::Int(0);
    void* db = getDb(args[0]);
    if (!db) return Value::Int(0);
    return Value::Int(static_cast<Int64>(api.mysql_insert_id(db)));
}

Value affectedRows_(std::vector<Value>& args) {
    auto& api = mysqlAPI();
    if (!api.tryLoad() || args.empty()) return Value::Int(0);
    void* db = getDb(args[0]);
    if (!db) return Value::Int(0);
    return Value::Int(static_cast<Int64>(api.mysql_affected_rows(db)));
}

Value isOpen_(std::vector<Value>& args) {
    auto& api = mysqlAPI();
    if (!api.tryLoad() || args.empty()) return Value::Bool(false);
    void* db = getDb(args[0]);
    if (!db) return Value::Bool(false);
    return Value::Bool(api.mysql_ping(db) == 0);
}

Value escape_(std::vector<Value>& args) {
    auto& api = mysqlAPI();
    if (!api.tryLoad() || args.size() < 2) return Value::nil();
    void* db = getDb(args[0]);
    if (!db) return Value::nil();
    std::string src(args[1].asString()->data, args[1].asString()->length);
    size_t bufLen = src.size() * 2 + 1;
    std::vector<char> buf(bufLen);
    unsigned long len = api.mysql_real_escape_string(db, buf.data(), src.c_str(), static_cast<unsigned long>(src.size()));
    return makeStringVal(VMString::create(std::string(buf.data(), len)));
}

Value selectDb_(std::vector<Value>& args) {
    auto& api = mysqlAPI();
    if (!api.tryLoad() || args.size() < 2) return Value::Bool(false);
    void* db = getDb(args[0]);
    if (!db) return Value::Bool(false);
    std::string name(args[1].asString()->data, args[1].asString()->length);
    return Value::Bool(api.mysql_select_db(db, name.c_str()) == 0);
}

Value listDbs_(std::vector<Value>& args) {
    auto& api = mysqlAPI();
    if (!api.tryLoad() || args.empty()) return Value::nil();
    void* db = getDb(args[0]);
    if (!db) return Value::nil();
    void* result = api.mysql_list_dbs(db, nullptr);
    if (!result) return Value::nil();
    VMArray* arr = VMArray::create();
    char** row;
    while ((row = api.mysql_fetch_row(result))) {
        arr->data.push_back(makeStringVal(VMString::create(row[0])));
    }
    api.mysql_free_result(result);
    return makeArrayVal(arr);
}

Value listTables_(std::vector<Value>& args) {
    auto& api = mysqlAPI();
    if (!api.tryLoad() || args.empty()) return Value::nil();
    void* db = getDb(args[0]);
    if (!db) return Value::nil();
    void* result = api.mysql_list_tables(db, nullptr);
    if (!result) return Value::nil();
    VMArray* arr = VMArray::create();
    char** row;
    while ((row = api.mysql_fetch_row(result))) {
        arr->data.push_back(makeStringVal(VMString::create(row[0])));
    }
    api.mysql_free_result(result);
    return makeArrayVal(arr);
}

Value clientInfo_(std::vector<Value>& args) {
    auto& api = mysqlAPI();
    if (!api.tryLoad()) return makeStringVal(VMString::create("客户端库未安装"));
    if (!api.mysql_get_client_info) return makeStringVal(VMString::create("unavailable"));
    const char* info = api.mysql_get_client_info();
    return makeStringVal(VMString::create(info ? info : "unknown"));
}

} // namespace mysql_ns

// ══════════════════════════════════════════════════════════════════╗
//  PostgreSQL 运行时绑定（跨平台(Win/Linux╚
// ══════════════════════════════════════════════════════════════════╝
namespace pg_ns {

struct PGAPI {
    LibHandle lib = nullptr;
    bool loaded = false;

    using PQconnectdb_t      = void* (*)(const char*);
    using PQstatus_t         = int (*)(void*);
    using PQfinish_t         = void (*)(void*);
    using PQerrorMessage_t   = const char* (*)(void*);
    using PQexec_t           = void* (*)(void*, const char*);
    using PQresultStatus_t   = int (*)(void*);
    using PQclear_t          = void (*)(void*);
    using PQntuples_t        = int (*)(void*);
    using PQnfields_t        = int (*)(void*);
    using PQfname_t          = const char* (*)(void*, int);
    using PQgetvalue_t       = const char* (*)(void*, int, int);
    using PQcmdTuples_t      = const char* (*)(void*);

    PQconnectdb_t    PQconnectdb = nullptr;
    PQstatus_t       PQstatus = nullptr;
    PQfinish_t       PQfinish = nullptr;
    PQerrorMessage_t PQerrorMessage = nullptr;
    PQexec_t         PQexec = nullptr;
    PQresultStatus_t PQresultStatus = nullptr;
    PQclear_t        PQclear = nullptr;
    PQntuples_t      PQntuples = nullptr;
    PQnfields_t      PQnfields = nullptr;
    PQfname_t        PQfname = nullptr;
    PQgetvalue_t     PQgetvalue = nullptr;
    PQcmdTuples_t    PQcmdTuples = nullptr;

    bool tryLoad() {
        if (loaded) return lib != nullptr;
        loaded = true;

    #ifdef _WIN32
        static const char* candidates[] = {
            "libpq.dll",
            "libpq-16.dll",
            "libpq-15.dll",
            "libpq-14.dll",
            nullptr
        };
    #else
        static const char* candidates[] = {
            "libpq.so.5",        // PostgreSQL 13+
            "libpq.so.5.16",     // PostgreSQL 16
            "libpq.so.5.15",     // PostgreSQL 15
            "libpq.so",          // 通用符号链接
            nullptr
        };
    #endif

        for (const char** p = candidates; *p; ++p) {
            lib = (LibHandle)loadLib(*p);
            if (lib) break;
        }
        if (!lib) return false;

        #define L(sym) sym = (decltype(sym))getSym(lib, #sym); if (!sym) { freeLib(lib); lib = nullptr; return false; }
        L(PQconnectdb);   L(PQstatus);      L(PQfinish);      L(PQerrorMessage);
        L(PQexec);        L(PQresultStatus); L(PQclear);       L(PQntuples);
        L(PQnfields);     L(PQfname);       L(PQgetvalue);    L(PQcmdTuples);
        #undef L
        return true;
    }
};

static PGAPI& pgAPI() {
    static PGAPI api;
    return api;
}

static std::unordered_map<VMTable*,void*> pgDbMap;

static void* getDb(Value& v) {
    if (!v.isTable()) return nullptr;
    auto it = pgDbMap.find(v.asTable());
    return it != pgDbMap.end() ? it->second : nullptr;
}

Value connect_(std::vector<Value>& args) {
    auto& api = pgAPI();
    if (!api.tryLoad() || args.empty() || !args[0].isString()) return Value::nil();
    std::string connStr(args[0].asString()->data, args[0].asString()->length);
    void* conn = api.PQconnectdb(connStr.c_str());
    if (api.PQstatus(conn) != 0 /* CONNECTION_OK */) {
        api.PQfinish(conn);
        return Value::nil();
    }
    VMTable* tbl = VMTable::create();
    pgDbMap[tbl] = conn;
    return makeTableVal(tbl);
}

Value query_(std::vector<Value>& args) {
    auto& api = pgAPI();
    if (!api.tryLoad() || args.size() < 2) return Value::nil();
    void* db = getDb(args[0]);
    if (!db) return Value::nil();
    std::string sql(args[1].asString()->data, args[1].asString()->length);
    void* res = api.PQexec(db, sql.c_str());
    int st = api.PQresultStatus(res);
    if (st != 2 /* PGRES_TUPLES_OK */ && st != 1 /* PGRES_COMMAND_OK */) {
        api.PQclear(res);
        return Value::nil();
    }
    int nRows = api.PQntuples(res);
    int nCols = api.PQnfields(res);
    VMArray* rows = VMArray::create();
    for (int r = 0; r < nRows; r++) {
        VMTable* tbl = VMTable::create();
        for (int c = 0; c < nCols; c++) {
            Value key = makeStringVal(VMString::create(api.PQfname(res, c)));
            const char* val = api.PQgetvalue(res, r, c);
            tbl->set(key, val ? makeStringVal(VMString::create(val)) : Value::nil());
        }
        rows->data.push_back(makeTableVal(tbl));
    }
    api.PQclear(res);
    return makeArrayVal(rows);
}

Value exec_(std::vector<Value>& args) {
    auto& api = pgAPI();
    if (!api.tryLoad() || args.size() < 2) return Value::Int(-1);
    void* db = getDb(args[0]);
    if (!db) return Value::Int(-1);
    std::string sql(args[1].asString()->data, args[1].asString()->length);
    void* res = api.PQexec(db, sql.c_str());
    int affected = -1;
    if (api.PQresultStatus(res) == 1 /* PGRES_COMMAND_OK */) {
        const char* tuples = api.PQcmdTuples(res);
        affected = tuples[0] ? atoi(tuples) : 0;
    }
    api.PQclear(res);
    return Value::Int(affected);
}

Value close_(std::vector<Value>& args) {
    auto& api = pgAPI();
    if (!api.tryLoad() || args.empty()) return Value::Bool(false);
    void* db = getDb(args[0]);
    if (!db) return Value::Bool(false);
    api.PQfinish(db);
    if (args[0].isTable()) pgDbMap.erase(args[0].asTable());
    return Value::Bool(true);
}

Value errMsg_(std::vector<Value>& args) {
    auto& api = pgAPI();
    if (!api.tryLoad() || args.empty()) return makeStringVal(VMString::create("PostgreSQL 客户端库未安装"));
    void* db = getDb(args[0]);
    if (!db) return makeStringVal(VMString::create("invalid handle"));
    return makeStringVal(VMString::create(api.PQerrorMessage(db)));
}

Value isOpen_(std::vector<Value>& args) {
    auto& api = pgAPI();
    if (!api.tryLoad() || args.empty()) return Value::Bool(false);
    void* db = getDb(args[0]);
    if (!db) return Value::Bool(false);
    return Value::Bool(api.PQstatus(db) == 0 /* CONNECTION_OK */);
}

Value listDbs_(std::vector<Value>& args) {
    auto& api = pgAPI();
    if (!api.tryLoad() || args.empty()) return Value::nil();
    void* db = getDb(args[0]);
    if (!db) return Value::nil();
    void* res = api.PQexec(db, "SELECT datname FROM pg_database ORDER BY datname");
    if (api.PQresultStatus(res) != 2) { api.PQclear(res); return Value::nil(); }
    VMArray* arr = VMArray::create();
    int n = api.PQntuples(res);
    for (int i = 0; i < n; i++) {
        arr->data.push_back(makeStringVal(VMString::create(api.PQgetvalue(res, i, 0))));
    }
    api.PQclear(res);
    return makeArrayVal(arr);
}

Value listTables_(std::vector<Value>& args) {
    auto& api = pgAPI();
    if (!api.tryLoad() || args.empty()) return Value::nil();
    void* db = getDb(args[0]);
    if (!db) return Value::nil();
    void* res = api.PQexec(db,
        "SELECT table_name FROM information_schema.tables "
        "WHERE table_schema='public' ORDER BY table_name");
    if (api.PQresultStatus(res) != 2) { api.PQclear(res); return Value::nil(); }
    VMArray* arr = VMArray::create();
    int n = api.PQntuples(res);
    for (int i = 0; i < n; i++) {
        arr->data.push_back(makeStringVal(VMString::create(api.PQgetvalue(res, i, 0))));
    }
    api.PQclear(res);
    return makeArrayVal(arr);
}

} // namespace pg_ns

// ══════════════════════════════════════════════════════════════════╗
//  注册函数（始终注册，DLL 存在时才可用）
// ════════════════════════════════════════════════════════════════════════
// ══════════════════════════════════════════════════════════════════╝
void StdLib::registerMysql(VM* vm) {
    registerFunction(vm, "mysqlConnect",      mysql_ns::connect_);
    registerFunction(vm, "mysqlQuery",        mysql_ns::query_);
    registerFunction(vm, "mysqlExec",         mysql_ns::exec_);
    registerFunction(vm, "mysqlClose",        mysql_ns::close_);
    registerFunction(vm, "mysqlErrMsg",       mysql_ns::errMsg_);
    registerFunction(vm, "mysqlInsertId",     mysql_ns::insertId_);
    registerFunction(vm, "mysqlAffectedRows", mysql_ns::affectedRows_);
    registerFunction(vm, "mysqlIsOpen",       mysql_ns::isOpen_);
    registerFunction(vm, "mysqlEscape",       mysql_ns::escape_);
    registerFunction(vm, "mysqlSelectDb",     mysql_ns::selectDb_);
    registerFunction(vm, "mysqlListDbs",      mysql_ns::listDbs_);
    registerFunction(vm, "mysqlListTables",   mysql_ns::listTables_);
    registerFunction(vm, "mysqlClientInfo",  mysql_ns::clientInfo_);
    registerAlias(vm, "MySQL连接",            "mysqlConnect");
    registerAlias(vm, "MySQL查询",            "mysqlQuery");
    registerAlias(vm, "MySQL执行",            "mysqlExec");
    registerAlias(vm, "MySQL关闭",            "mysqlClose");
    registerAlias(vm, "MySQL错误",            "mysqlErrMsg");
    registerAlias(vm, "MySQL插入标识",        "mysqlInsertId");
    registerAlias(vm, "MySQL影响行数",        "mysqlAffectedRows");
    registerAlias(vm, "MySQL是否打开",        "mysqlIsOpen");
    registerAlias(vm, "MySQL转义",            "mysqlEscape");
    registerAlias(vm, "MySQL选库",            "mysqlSelectDb");
    registerAlias(vm, "MySQL列出库",          "mysqlListDbs");
    registerAlias(vm, "MySQL列出表",          "mysqlListTables");
    registerAlias(vm, "MySQL客户端信息",      "mysqlClientInfo");
}

void StdLib::registerPg(VM* vm) {
    registerFunction(vm, "pgConnect",      pg_ns::connect_);
    registerFunction(vm, "pgQuery",        pg_ns::query_);
    registerFunction(vm, "pgExec",         pg_ns::exec_);
    registerFunction(vm, "pgClose",        pg_ns::close_);
    registerFunction(vm, "pgErrMsg",       pg_ns::errMsg_);
    registerFunction(vm, "pgIsOpen",       pg_ns::isOpen_);
    registerFunction(vm, "pgListDbs",      pg_ns::listDbs_);
    registerFunction(vm, "pgListTables",   pg_ns::listTables_);
    registerAlias(vm, "PG连接",             "pgConnect");
    registerAlias(vm, "PG查询",             "pgQuery");
    registerAlias(vm, "PG执行",             "pgExec");
    registerAlias(vm, "PG关闭",             "pgClose");
    registerAlias(vm, "PG错误",             "pgErrMsg");
    registerAlias(vm, "PG是否打开",         "pgIsOpen");
    registerAlias(vm, "PG列出库",           "pgListDbs");
    registerAlias(vm, "PG列出表",           "pgListTables");
}

} // namespace cplang
