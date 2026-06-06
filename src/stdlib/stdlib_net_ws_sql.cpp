#include "stdlib/stdlib.hpp"

namespace cplang {

// Network, WebSocket, Sqlite functions
// #include'd from stdlib.cpp, already inside namespace cplang

namespace net {

static bool g_wsaInit = false;
static std::unordered_map<int, SOCKET> g_sockets;
static int g_nextId = 1;

static bool initWinsock() {
    if (g_wsaInit) return true;
    WSADATA wsaData;
    g_wsaInit = (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);
    return g_wsaInit;
}

static int storeSocket(SOCKET s) {
    int id = g_nextId++;
    g_sockets[id] = s;
    return id;
}

static SOCKET getSocket(int id) {
    auto it = g_sockets.find(id);
    if (it != g_sockets.end()) return it->second;
    return INVALID_SOCKET;
}

static void removeSocket(int id) {
    g_sockets.erase(id);
}

static std::string getStr(const Value& v) {
    if (!v.isString()) return "";
    return std::string(v.asString()->data, v.asString()->length);
}

Value tcpConnect(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isString() || !args[1].isNumber()) return Value::Float(-1);
    if (!initWinsock()) return Value::Float(-1);
    std::string host = getStr(args[0]);
    int port = static_cast<int>(args[1].asFloat());
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return Value::Float(-1);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(s);
        return Value::Float(-1);
    }
    return Value::Float(static_cast<double>(storeSocket(s)));
}

Value tcpSend(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isNumber() || !args[1].isString()) return Value::Bool(false);
    SOCKET s = getSocket(static_cast<int>(args[0].asFloat()));
    if (s == INVALID_SOCKET) return Value::Bool(false);
    std::string data = getStr(args[1]);
    int sent = send(s, data.c_str(), static_cast<int>(data.size()), 0);
    return Value::Bool(sent != SOCKET_ERROR);
}

Value tcpReceive(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isNumber() || !args[1].isNumber()) return Value::nil();
    SOCKET s = getSocket(static_cast<int>(args[0].asFloat()));
    if (s == INVALID_SOCKET) return Value::nil();
    int maxBytes = static_cast<int>(args[1].asFloat());
    std::vector<char> buf(maxBytes + 1, '\0');
    int received = recv(s, buf.data(), maxBytes, 0);
    if (received <= 0) return Value::nil();
    return Value::String(VMString::create(std::string(buf.data(), received)));
}

Value tcpClose(std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::Bool(false);
    int id = static_cast<int>(args[0].asFloat());
    SOCKET s = getSocket(id);
    if (s != INVALID_SOCKET) closesocket(s);
    removeSocket(id);
    return Value::Bool(true);
}

Value tcpListen(std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::Float(-1);
    if (!initWinsock()) return Value::Float(-1);
    int port = static_cast<int>(args[0].asFloat());
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return Value::Float(-1);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(s);
        return Value::Float(-1);
    }
    if (listen(s, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(s);
        return Value::Float(-1);
    }
    return Value::Float(static_cast<double>(storeSocket(s)));
}

Value tcpAccept(std::vector<Value>& args) {
    if (args.empty() || !args[0].isNumber()) return Value::Float(-1);
    SOCKET s = getSocket(static_cast<int>(args[0].asFloat()));
    if (s == INVALID_SOCKET) return Value::Float(-1);
    struct sockaddr_in addr;
    int len = sizeof(addr);
    SOCKET client = accept(s, (struct sockaddr*)&addr, &len);
    if (client == INVALID_SOCKET) return Value::Float(-1);
    return Value::Float(static_cast<double>(storeSocket(client)));
}

Value udpSocket(std::vector<Value>& args) {
    if (!initWinsock()) return Value::Float(-1);
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) return Value::Float(-1);
    return Value::Float(static_cast<double>(storeSocket(s)));
}

Value udpSend(std::vector<Value>& args) {
    if (args.size() < 4 || !args[0].isNumber() || !args[1].isString() || !args[2].isNumber() || !args[3].isString()) return Value::Bool(false);
    SOCKET s = getSocket(static_cast<int>(args[0].asFloat()));
    if (s == INVALID_SOCKET) return Value::Bool(false);
    std::string host = getStr(args[1]);
    int port = static_cast<int>(args[2].asFloat());
    std::string data = getStr(args[3]);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
    int sent = sendto(s, data.c_str(), static_cast<int>(data.size()), 0, (struct sockaddr*)&addr, sizeof(addr));
    return Value::Bool(sent != SOCKET_ERROR);
}

Value udpReceive(std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isNumber() || !args[1].isNumber()) return Value::nil();
    SOCKET s = getSocket(static_cast<int>(args[0].asFloat()));
    if (s == INVALID_SOCKET) return Value::nil();
    int maxBytes = static_cast<int>(args[1].asFloat());
    std::vector<char> buf(maxBytes + 1, '\0');
    struct sockaddr_in addr;
    int len = sizeof(addr);
    int received = recvfrom(s, buf.data(), maxBytes, 0, (struct sockaddr*)&addr, &len);
    if (received <= 0) return Value::nil();
    return Value::String(VMString::create(std::string(buf.data(), received)));
}
} // namespace net

// ==================== WebSocket (WinHTTP) ====================
namespace ws_ns {

static std::string genSecKey() {
    std::random_device rd; unsigned char k[16];
    for(int i=0;i<16;i++) k[i]=(unsigned char)(rd()&0xFF);
    static const char b64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string r;
    for(int i=0;i<16;i+=3){unsigned long n=((unsigned long)k[i]<<16)|((unsigned long)k[i+1]<<8)|k[i+2];r+=b64[(n>>18)&0x3F];r+=b64[(n>>12)&0x3F];r+=b64[(n>>6)&0x3F];r+=b64[n&0x3F];}
    return r+"==";
}

static std::string genSecAccept(const std::string& key) {
    std::string combined=key+"258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string hex=crypto::sha1Hash(combined); unsigned char raw[20];
    const char* hx="0123456789abcdef";
    for(int i=0;i<20;i++) raw[i]=(unsigned char)(((strchr(hx,hex[i*2])-hx)<<4)|(strchr(hx,hex[i*2+1])-hx));
    static const char b64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string r;
    for(int i=0;i<20;i+=3){unsigned long n=((unsigned long)raw[i]<<16)|((i+1<20?(unsigned long)raw[i+1]:0)<<8)|(i+2<20?raw[i+2]:0);r+=b64[(n>>18)&0x3F];r+=b64[(n>>12)&0x3F];r+=b64[(n>>6)&0x3F];r+=b64[n&0x3F];}
    r.resize(r.size()-4); unsigned long m=((unsigned long)raw[18]<<16)|((unsigned long)raw[19]<<8);
    r+=b64[(m>>18)&0x3F];r+=b64[(m>>12)&0x3F];r+=b64[(m>>6)&0x3F];r+='='; return r;
}

Value connect_(std::vector<Value>& args) {
    if(args.empty()||!args[0].isString()) return Value::nil();
    std::string url(args[0].asString()->data,args[0].asString()->length);
    bool secure=(url.find("wss://")==0); size_t pe=secure?6:(url.find("ws://")==0?5:0);
    if(pe==0) return Value::nil();
    size_t ps=url.find('/',pe); std::string hp=(ps!=std::string::npos)?url.substr(pe,ps-pe):url.substr(pe); std::string pt=(ps!=std::string::npos)?url.substr(ps):"/";
    int port=secure?443:80; size_t cp=hp.find(':'); if(cp!=std::string::npos){port=std::stoi(hp.substr(cp+1));hp=hp.substr(0,cp);}
    std::wstring wh(hp.begin(),hp.end()), wp(pt.begin(),pt.end());
    HINTERNET hS=WinHttpOpen(L"CP/1.0",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);if(!hS)return Value::nil();
    HINTERNET hC=WinHttpConnect(hS,wh.c_str(),(INTERNET_PORT)port,0);if(!hC){WinHttpCloseHandle(hS);return Value::nil();}
    DWORD fl=secure?WINHTTP_FLAG_SECURE:0;
    HINTERNET hR=WinHttpOpenRequest(hC,L"GET",wp.c_str(),NULL,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,fl);if(!hR){WinHttpCloseHandle(hC);WinHttpCloseHandle(hS);return Value::nil();}
    std::string sk=genSecKey();std::wstring wk(sk.begin(),sk.end());
    WinHttpAddRequestHeaders(hR,(L"Sec-WebSocket-Key: "+wk+L"\r\n").c_str(),(DWORD)-1,WINHTTP_ADDREQ_FLAG_ADD);
    WinHttpAddRequestHeaders(hR,L"Upgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Version: 13\r\n",(DWORD)-1,WINHTTP_ADDREQ_FLAG_ADD);
    if(!WinHttpSendRequest(hR,WINHTTP_NO_ADDITIONAL_HEADERS,0,WINHTTP_NO_REQUEST_DATA,0,0,0)){WinHttpCloseHandle(hR);WinHttpCloseHandle(hC);WinHttpCloseHandle(hS);return Value::nil();}
    if(!WinHttpReceiveResponse(hR,NULL)){WinHttpCloseHandle(hR);WinHttpCloseHandle(hC);WinHttpCloseHandle(hS);return Value::nil();}
    DWORD sc=0,ss=sizeof(sc); WinHttpQueryHeaders(hR,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&sc,&ss,WINHTTP_NO_HEADER_INDEX);
    if(sc!=101){WinHttpCloseHandle(hR);WinHttpCloseHandle(hC);WinHttpCloseHandle(hS);return Value::nil();}
    HINTERNET hWS=WinHttpWebSocketCompleteUpgrade(hR,0);if(!hWS){WinHttpCloseHandle(hR);WinHttpCloseHandle(hC);WinHttpCloseHandle(hS);return Value::nil();}
    VMWebSocket* ws=VMWebSocket::create();ws->hWebSocket=hWS;ws->hSession=hS;ws->hConnect=hC;ws->closed=false;
    VMTable* tbl=VMTable::create(); tbl->set(Value::String(VMString::create("_ws")),Value::Int(reinterpret_cast<Int64>(ws))); tbl->set(Value::String(VMString::create("_url")),args[0]);
    return Value::Table(tbl);
}

Value send_(std::vector<Value>& args) {
    if(args.size()<2)return Value::nil(); Value& w=args[0];
    if(!w.isTable())return Value::Bool(false); VMTable*t=w.asTable();Value k=Value::String(VMString::create("_ws"));
    if(!t->has(k))return Value::Bool(false); VMWebSocket*ws=reinterpret_cast<VMWebSocket*>(t->get(k).asInt());
    if(!ws||ws->closed)return Value::Bool(false);
    VMString* s=args[1].asString();
    return Value::Bool(WinHttpWebSocketSend((HINTERNET)ws->hWebSocket,WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,(PVOID)s->data,(DWORD)s->length)==ERROR_SUCCESS);
}

Value recv_(std::vector<Value>& args) {
    if(args.empty())return Value::nil(); Value& w=args[0];
    if(!w.isTable())return Value::nil(); VMTable*t=w.asTable();Value k=Value::String(VMString::create("_ws"));
    if(!t->has(k))return Value::nil(); VMWebSocket*ws=reinterpret_cast<VMWebSocket*>(t->get(k).asInt());
    if(!ws||ws->closed)return Value::nil();
    DWORD br=0;WINHTTP_WEB_SOCKET_BUFFER_TYPE bt;char b[65536];
    if(WinHttpWebSocketReceive((HINTERNET)ws->hWebSocket,b,sizeof(b)-1,&br,&bt)!=ERROR_SUCCESS){ws->closed=true;return Value::nil();}
    b[br]=0;VMTable*rt=VMTable::create();
    rt->set(Value::String(VMString::create("data")),Value::String(VMString::create(std::string(b,br))));
    rt->set(Value::String(VMString::create("type")),Value::String(VMString::create(bt==WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE?"binary":"text")));
    rt->set(Value::String(VMString::create("isClose")),Value::Bool(bt==WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE));
    return Value::Table(rt);
}

Value close_(std::vector<Value>& args) {
    if(args.empty())return Value::Bool(false); Value& w=args[0];
    if(!w.isTable())return Value::Bool(false); VMTable*t=w.asTable();Value k=Value::String(VMString::create("_ws"));
    if(!t->has(k))return Value::Bool(false); VMWebSocket*ws=reinterpret_cast<VMWebSocket*>(t->get(k).asInt());
    if(!ws||ws->closed)return Value::Bool(false); ws->closed=true;
    USHORT cc=1000;if(args.size()>=2&&args[1].isInt())cc=(USHORT)args[1].asInt();
    std::string re=args.size()>=3?std::string(args[2].asString()->data,args[2].asString()->length):"";
    WinHttpWebSocketClose((HINTERNET)ws->hWebSocket,cc,(PVOID)re.data(),(DWORD)re.size());
    WinHttpCloseHandle((HINTERNET)ws->hWebSocket);WinHttpCloseHandle((HINTERNET)ws->hConnect);WinHttpCloseHandle((HINTERNET)ws->hSession);
    ws->hWebSocket=nullptr;ws->hSession=nullptr;ws->hConnect=nullptr;return Value::Bool(true);
}

Value isOpen_(std::vector<Value>& args) {
    if(args.empty())return Value::Bool(false);Value&w=args[0];
    if(!w.isTable())return Value::Bool(false);VMTable*t=w.asTable();Value k=Value::String(VMString::create("_ws"));
    if(!t->has(k))return Value::Bool(false);VMWebSocket*ws=reinterpret_cast<VMWebSocket*>(t->get(k).asInt());
    return Value::Bool(ws&&!ws->closed);
}
} // namespace ws_ns

// ==================== SQLite3 ====================
namespace sql_ns {
static sqlite3* getDb(Value& v){if(!v.isTable())return nullptr;VMTable*t=v.asTable();Value k=Value::String(VMString::create("_db"));if(!t->has(k))return nullptr;return reinterpret_cast<sqlite3*>(t->get(k).asInt());}

Value open_(std::vector<Value>& args) {
    if(args.empty()||!args[0].isString())return Value::nil();
    std::string p(args[0].asString()->data,args[0].asString()->length);sqlite3*db=nullptr;
    if(sqlite3_open(p.c_str(),&db)!=SQLITE_OK){if(db)sqlite3_close(db);return Value::nil();}
    VMTable*tbl=VMTable::create();tbl->set(Value::String(VMString::create("_db")),Value::Int(reinterpret_cast<Int64>(db)));tbl->set(Value::String(VMString::create("_path")),args[0]);return Value::Table(tbl);
}

Value exec_(std::vector<Value>& args) {
    if(args.size()<2)return Value::Bool(false);sqlite3*db=getDb(args[0]);if(!db)return Value::Bool(false);
    std::string s(args[1].asString()->data,args[1].asString()->length);char*em=nullptr;
    int rc=sqlite3_exec(db,s.c_str(),nullptr,nullptr,&em);if(rc!=SQLITE_OK){if(em)sqlite3_free(em);return Value::Bool(false);}return Value::Bool(true);
}

static int qcb(void*d,int argc,char**argv,char**cols){VMArray*rows=reinterpret_cast<VMArray*>(d);VMTable*row=VMTable::create();for(int i=0;i<argc;i++){row->set(Value::String(VMString::create(cols[i]?cols[i]:"")),argv[i]?Value::String(VMString::create(argv[i])):Value::nil());}rows->data.push_back(Value::Table(row));return 0;}

Value query_(std::vector<Value>& args) {
    if(args.size()<2)return Value::nil();sqlite3*db=getDb(args[0]);if(!db)return Value::nil();
    std::string s(args[1].asString()->data,args[1].asString()->length);char*em=nullptr;VMArray*rows=VMArray::create();
    int rc=sqlite3_exec(db,s.c_str(),qcb,rows,&em);if(rc!=SQLITE_OK){if(em)sqlite3_free(em);return Value::nil();}return Value::Array(rows);
}

Value close_(std::vector<Value>& args) {if(args.empty())return Value::Bool(false);sqlite3*db=getDb(args[0]);if(!db)return Value::Bool(false);sqlite3_close(db);if(args[0].isTable())args[0].asTable()->remove(Value::String(VMString::create("_db")));return Value::Bool(true);}
Value errMsg_(std::vector<Value>& args) {if(args.empty())return Value::String(VMString::create("no db"));sqlite3*db=getDb(args[0]);if(!db)return Value::String(VMString::create("invalid"));return Value::String(VMString::create(sqlite3_errmsg(db)));}
Value lastInsertRowId_(std::vector<Value>& args) {if(args.empty())return Value::Int(0);sqlite3*db=getDb(args[0]);if(!db)return Value::Int(0);return Value::Int(static_cast<Int64>(sqlite3_last_insert_rowid(db)));}
Value changes_(std::vector<Value>& args) {if(args.empty())return Value::Int(0);sqlite3*db=getDb(args[0]);if(!db)return Value::Int(0);return Value::Int(static_cast<Int64>(sqlite3_changes(db)));}
Value isOpen_(std::vector<Value>& args) {if(args.empty())return Value::Bool(false);return Value::Bool(getDb(args[0])!=nullptr);}

// ═══════════════════════════════════════════════════════════════════
//  事务支持
// ═══════════════════════════════════════════════════════════════════
Value begin_(std::vector<Value>& args) {
    if(args.empty())return Value::Bool(false);sqlite3*db=getDb(args[0]);if(!db)return Value::Bool(false);
    char*em=nullptr;int rc=sqlite3_exec(db,"BEGIN",nullptr,nullptr,&em);
    if(rc!=SQLITE_OK){if(em)sqlite3_free(em);return Value::Bool(false);}return Value::Bool(true);
}
Value commit_(std::vector<Value>& args) {
    if(args.empty())return Value::Bool(false);sqlite3*db=getDb(args[0]);if(!db)return Value::Bool(false);
    char*em=nullptr;int rc=sqlite3_exec(db,"COMMIT",nullptr,nullptr,&em);
    if(rc!=SQLITE_OK){if(em)sqlite3_free(em);return Value::Bool(false);}return Value::Bool(true);
}
Value rollback_(std::vector<Value>& args) {
    if(args.empty())return Value::Bool(false);sqlite3*db=getDb(args[0]);if(!db)return Value::Bool(false);
    char*em=nullptr;int rc=sqlite3_exec(db,"ROLLBACK",nullptr,nullptr,&em);
    if(rc!=SQLITE_OK){if(em)sqlite3_free(em);return Value::Bool(false);}return Value::Bool(true);
}

// ═══════════════════════════════════════════════════════════════════
//  预编译语句（Prepared Statement）
//  用法:
//    stmt = 数据库准备(db, "SELECT * FROM t WHERE id=? AND name=?")
//    数据库绑定整数(stmt, 1, 42)
//    数据库绑定文本(stmt, 2, "张三")
//    while (数据库步进(stmt)) { row = 数据库读取行(stmt); ... }
//    数据库结束(stmt)
// ═══════════════════════════════════════════════════════════════════
static sqlite3_stmt* getStmt(Value& v) {
    if(!v.isTable())return nullptr;
    VMTable*t=v.asTable();Value k=Value::String(VMString::create("_stmt"));
    if(!t->has(k))return nullptr;
    return reinterpret_cast<sqlite3_stmt*>(t->get(k).asInt());
}

Value prepare_(std::vector<Value>& args) {
    if(args.size()<2||!args[1].isString())return Value::nil();
    sqlite3*db=getDb(args[0]);if(!db)return Value::nil();
    std::string sql(args[1].asString()->data,args[1].asString()->length);
    sqlite3_stmt*stmt=nullptr;
    if(sqlite3_prepare_v2(db,sql.c_str(),-1,&stmt,nullptr)!=SQLITE_OK)
        return Value::nil();
    VMTable*tbl=VMTable::create();
    tbl->set(Value::String(VMString::create("_stmt")),Value::Int(reinterpret_cast<Int64>(stmt)));
    return Value::Table(tbl);
}

Value bindInt_(std::vector<Value>& args) {
    if(args.size()<2)return Value::Bool(false);
    sqlite3_stmt*stmt=getStmt(args[0]);if(!stmt)return Value::Bool(false);
    int idx=args[1].isInt()?static_cast<int>(args[1].asInt()):1;
    Int64 val=args.size()>=3?(args[2].isInt()?args[2].asInt():0):0;
    return Value::Bool(sqlite3_bind_int64(stmt,idx,val)==SQLITE_OK);
}

Value bindText_(std::vector<Value>& args) {
    if(args.size()<2)return Value::Bool(false);
    sqlite3_stmt*stmt=getStmt(args[0]);if(!stmt)return Value::Bool(false);
    int idx=args[1].isInt()?static_cast<int>(args[1].asInt()):1;
    const char*val=args.size()>=3&&args[2].isString()?args[2].asString()->data:"";
    return Value::Bool(sqlite3_bind_text(stmt,idx,val,-1,SQLITE_TRANSIENT)==SQLITE_OK);
}

Value bindNull_(std::vector<Value>& args) {
    if(args.size()<2)return Value::Bool(false);
    sqlite3_stmt*stmt=getStmt(args[0]);if(!stmt)return Value::Bool(false);
    int idx=args[1].isInt()?static_cast<int>(args[1].asInt()):1;
    return Value::Bool(sqlite3_bind_null(stmt,idx)==SQLITE_OK);
}

Value step_(std::vector<Value>& args) {
    if(args.empty())return Value::Bool(false);
    sqlite3_stmt*stmt=getStmt(args[0]);if(!stmt)return Value::Bool(false);
    int rc=sqlite3_step(stmt);
    if(rc==SQLITE_ROW)return Value::Bool(true);
    return Value::Bool(false);
}

Value readRow_(std::vector<Value>& args) {
    if(args.empty())return Value::nil();
    sqlite3_stmt*stmt=getStmt(args[0]);if(!stmt)return Value::nil();
    int nCols=sqlite3_column_count(stmt);
    VMTable*row=VMTable::create();
    for(int i=0;i<nCols;i++){
        const char*colName=sqlite3_column_name(stmt,i);
        Value key=Value::String(VMString::create(colName?colName:""));
        int colType=sqlite3_column_type(stmt,i);
        Value val;
        switch(colType){
            case SQLITE_INTEGER: val=Value::Int(sqlite3_column_int64(stmt,i));break;
            case SQLITE_FLOAT:   val=Value::Float(sqlite3_column_double(stmt,i));break;
            case SQLITE_TEXT:{
                const char*txt=(const char*)sqlite3_column_text(stmt,i);
                val=txt?Value::String(VMString::create(txt)):Value::nil();
                break;
            }
            case SQLITE_BLOB:    val=Value::String(VMString::create("(blob)"));break;
            default:             val=Value::nil();break;
        }
        row->set(key,val);
    }
    return Value::Table(row);
}

Value column_(std::vector<Value>& args) {
    if(args.size()<2)return Value::nil();
    sqlite3_stmt*stmt=getStmt(args[0]);if(!stmt)return Value::nil();
    int idx=static_cast<int>(args[1].asInt());
    int nCols=sqlite3_column_count(stmt);
    if(idx<0||idx>=nCols)return Value::nil();
    int colType=sqlite3_column_type(stmt,idx);
    switch(colType){
        case SQLITE_INTEGER: return Value::Int(sqlite3_column_int64(stmt,idx));
        case SQLITE_FLOAT:   return Value::Float(sqlite3_column_double(stmt,idx));
        case SQLITE_TEXT:{
            const char*txt=(const char*)sqlite3_column_text(stmt,idx);
            return txt?Value::String(VMString::create(txt)):Value::nil();
        }
        default: return Value::nil();
    }
}

Value finalize_(std::vector<Value>& args) {
    if(args.empty())return Value::Bool(false);
    sqlite3_stmt*stmt=getStmt(args[0]);if(!stmt)return Value::Bool(false);
    int rc=sqlite3_finalize(stmt);
    if(args[0].isTable())args[0].asTable()->remove(Value::String(VMString::create("_stmt")));
    return Value::Bool(rc==SQLITE_OK);
}

// ═══════════════════════════════════════════════════════════════════
//  表结构 + SQLite 版本
// ═══════════════════════════════════════════════════════════════════
Value tableInfo_(std::vector<Value>& args) {
    if(args.size()<2||!args[1].isString())return Value::nil();
    sqlite3*db=getDb(args[0]);if(!db)return Value::nil();
    std::string sql=std::string("PRAGMA table_info(")+args[1].asString()->data+")";
    char*em=nullptr;VMArray*cols=VMArray::create();
    int rc=sqlite3_exec(db,sql.c_str(),qcb,cols,&em);
    if(rc!=SQLITE_OK){if(em)sqlite3_free(em);return Value::nil();}
    return Value::Array(cols);
}

Value version_(std::vector<Value>& args) {
    (void)args;
    return Value::String(VMString::create(sqlite3_libversion()));
}

} // namespace sql_ns

void StdLib::registerWebSocket(VM* vm) {
    registerFunction(vm, "wsConnect",        ws_ns::connect_);
    registerFunction(vm, "wsSend",           ws_ns::send_);
    registerFunction(vm, "wsRecv",           ws_ns::recv_);
    registerFunction(vm, "wsClose",          ws_ns::close_);
    registerFunction(vm, "wsIsOpen",         ws_ns::isOpen_);
    registerAlias(vm, "WS连接",              "wsConnect");
    registerAlias(vm, "WS发送",              "wsSend");
    registerAlias(vm, "WS接收",              "wsRecv");
    registerAlias(vm, "WS关闭",              "wsClose");
}

void StdLib::registerSqlite(VM* vm) {
    registerFunction(vm, "sqliteOpen",        sql_ns::open_);
    registerFunction(vm, "sqliteExec",        sql_ns::exec_);
    registerFunction(vm, "sqliteQuery",       sql_ns::query_);
    registerFunction(vm, "sqliteClose",       sql_ns::close_);
    registerFunction(vm, "sqliteErrMsg",      sql_ns::errMsg_);
    registerFunction(vm, "sqliteLastInsertId", sql_ns::lastInsertRowId_);
    registerFunction(vm, "sqliteChanges",     sql_ns::changes_);
    registerFunction(vm, "sqliteIsOpen",      sql_ns::isOpen_);
    registerFunction(vm, "sqliteVersion",     sql_ns::version_);
    registerFunction(vm, "sqliteBegin",       sql_ns::begin_);
    registerFunction(vm, "sqliteCommit",      sql_ns::commit_);
    registerFunction(vm, "sqliteRollback",    sql_ns::rollback_);
    registerFunction(vm, "sqlitePrepare",     sql_ns::prepare_);
    registerFunction(vm, "sqliteBindInt",     sql_ns::bindInt_);
    registerFunction(vm, "sqliteBindText",    sql_ns::bindText_);
    registerFunction(vm, "sqliteBindNull",    sql_ns::bindNull_);
    registerFunction(vm, "sqliteStep",        sql_ns::step_);
    registerFunction(vm, "sqliteReadRow",     sql_ns::readRow_);
    registerFunction(vm, "sqliteColumn",      sql_ns::column_);
    registerFunction(vm, "sqliteFinalize",    sql_ns::finalize_);
    registerFunction(vm, "sqliteTableInfo",   sql_ns::tableInfo_);
    registerAlias(vm, "数据库打开",           "sqliteOpen");
    registerAlias(vm, "数据库执行",           "sqliteExec");
    registerAlias(vm, "数据库查询",           "sqliteQuery");
    registerAlias(vm, "数据库关闭",           "sqliteClose");
    registerAlias(vm, "数据库错误",           "sqliteErrMsg");
    registerAlias(vm, "数据库最后插入ID",     "sqliteLastInsertId");
    registerAlias(vm, "数据库开始事务",         "sqliteBegin");
    registerAlias(vm, "数据库提交",             "sqliteCommit");
    registerAlias(vm, "数据库回滚",             "sqliteRollback");
    registerAlias(vm, "数据库准备",             "sqlitePrepare");
    registerAlias(vm, "数据库绑定整数",         "sqliteBindInt");
    registerAlias(vm, "数据库绑定文本",         "sqliteBindText");
    registerAlias(vm, "数据库绑定空",           "sqliteBindNull");
    registerAlias(vm, "数据库步进",             "sqliteStep");
    registerAlias(vm, "数据库读取行",           "sqliteReadRow");
    registerAlias(vm, "数据库取列",             "sqliteColumn");
    registerAlias(vm, "数据库结束",             "sqliteFinalize");
    registerAlias(vm, "数据库表信息",           "sqliteTableInfo");
    registerAlias(vm, "数据库版本",             "sqliteVersion");
    registerAlias(vm, "数据库变更数",           "sqliteChanges");
    registerAlias(vm, "数据库已打开",           "sqliteIsOpen");
}

} // namespace cplang
