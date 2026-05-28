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
    registerAlias(vm, "数据库打开",           "sqliteOpen");
    registerAlias(vm, "数据库执行",           "sqliteExec");
    registerAlias(vm, "数据库查询",           "sqliteQuery");
    registerAlias(vm, "数据库关闭",           "sqliteClose");
    registerAlias(vm, "数据库错误",           "sqliteErrMsg");
    registerAlias(vm, "数据库最后插入ID",     "sqliteLastInsertId");
}
