// CP语言 Token 定义
#pragma once

#include "common/types.hpp"
#include <unordered_map>
#include <unordered_set>

namespace cplang {

// Token 类型
enum class TokenType {
    // 特殊 Token
    END_OF_FILE,
    IDENTIFIER,
    
    // 字面量
    INTEGER,
    FLOAT,
    STRING,
    BOOLEAN,
    CHAR,
    
    // 关键字 (中文)
    K_PACKAGE,    // 包名
    K_IMPORT,     // 导入
    K_CLASS,      // 类
    K_INTERFACE,  // 接口
    K_ENUM,       // 枚举
    K_STRUCT,     // 结构体
    K_FUNC,       // 函数
    K_RETURN,     // 返回
    K_IF,         // 如果
    K_ELSE,       // 否则
    K_SWITCH,     // 选择
    K_MATCH,      // 匹配
    K_CASE,       // 情况
    K_DEFAULT,    // 其他
    K_FOR,        // 循环
    K_FOREACH,    // 遍历
    K_WHILE,      // 当
    K_DO,         // 为
    K_BREAK,      // 跳出
    K_CONTINUE,   // 继续
    K_PUBLIC,     // 公有
    K_PRIVATE,    // 私有
    K_PROTECTED,  // 保护
    K_TRUE,       // 真
    K_FALSE,      // 假
    K_NULL,       // 空
    K_NEW,        // 新建
    K_DELETE,     // 删除
    K_THIS,       // 这个
    K_SUPER,      // 继承
    K_EXTENDS,    // 扩展/继承（类继承类，接口继承接口）
    K_IMPLEMENTS, // 实现（类实现接口）
    K_TRY,        // 尝试
    K_CATCH,      // 捕获
    K_THROW,      // 抛出
    K_FINALLY,    // 最终
    K_WITH,       // 使用 / with
    K_ASYNC,      // 异步
    K_AWAIT,      // 等待
    K_GO,         // 协程 / go（启动并发）
    K_CONST,      // 常量
    K_VAR,        // 变量
    K_STATIC,     // 静态
    K_EXPORT,     // 导出
    K_ABSTRACT,   // 抽象
    K_VIRTUAL,    // 虚拟
    K_OVERRIDE,   // 重写
    K_MUTABLE,    // 可变

    // === 双语运算符关键词 ===
    // 控制流连接词
    K_THEN,        // 则（if条件后）
    K_DO_KEYWORD,  // 做（while/for/do body）
    K_IN,          // 在（foreach）
    K_FROM,        // 从（for-range start）
    K_TO,          // 到（for-range end）
    K_WHILE_MARK,  // 时（while循环条件后）
    // 逻辑运算符
    K_AND,         // 且
    K_OR,          // 或
    K_NOT,         // 非
    // 比较运算符
    K_EQ,          // 是 / 等于
    K_NE,          // 不等于
    K_GT,          // 大于
    K_LT,          // 小于
    K_GE,          // 大于等于
    K_LE,          // 小于等于
    // 赋值/定义
    K_LET,         // 设（设 x 为 10）
    K_DEFER,       // 推迟 (defer)

    // 运算符
    OP_PLUS,      // +
    OP_MINUS,     // -
    OP_MUL,       // *
    OP_DIV,       // /
    OP_MOD,       // %
    OP_ASSIGN,    // =
    OP_EQ,        // ==
    OP_NE,        // !=
    OP_GT,        // >
    OP_LT,        // <
    OP_GE,        // >=
    OP_LE,        // <=
    OP_AND,       // &&
    OP_OR,        // ||
    OP_NOT,       // !
    OP_BIT_AND,   // &
    OP_BIT_OR,    // |
    OP_BIT_XOR,   // ^
    OP_LSHIFT,    // <<
    OP_RSHIFT,    // >>
    OP_PLUS_ASSIGN,   // +=
    OP_MINUS_ASSIGN,  // -=
    OP_MUL_ASSIGN,    // *=
    OP_DIV_ASSIGN,    // /=
    OP_MOD_ASSIGN,    // %=
    OP_INC,       // ++
    OP_DEC,       // --
    OP_QUESTION,  // ?
    OP_COLON,     // :
    OP_DBL_COLON, // ::
    OP_DOT,       // .
    OP_ARROW,     // ->
    OP_PIPE,      // |>
    OP_FAT_ARROW, // =>
    OP_SCOPE,     // ::

    OP_OPTIONAL_CHAIN, // ?.
    OP_NULL_COALESCE,  // ??
    OP_SPREAD,         // ...
    
    // 分隔符
    LPAREN,       // (
    RPAREN,       // )
    LBRACKET,     // [
    RBRACKET,     // ]
    LBRACE,       // {
    RBRACE,       // }
    COMMA,        // ,
    SEMICOLON,    // ;
    
    // 注释
    COMMENT_SINGLE,  // 单行注释
    COMMENT_MULTI,   // 多行注释
    
    // 预处理器
    HASH,        // #
    
    // 装饰器
    OP_AT,       // @
    
    // 生成器
    K_YIELD,     // 产出 / yield
    
    // 宏
    K_MACRO,     // 宏 / macro
    
    // 条件编译
    K_COND_IF,     // 编译如果 / #if
    K_COND_ELSE,   // 编译否则 / #else
    K_COND_ENDIF,  // 编译结束如果 / #endif
    K_COND_DEFINED, // 编译定义 / defined
    
    // where 子句
    K_WHERE,     // 满足 / where
    
    // 类型别名
    K_TYPE,      // 类型 / type
    
    // 类型守卫
    K_IS,        // 是 / is
    
    // 错误
    INVALID
};

// Token 结构
struct Token {
    TokenType type;
    String text;
    String raw;
    Int32 line;
    Int32 column;
    Variant<Int64, Float64, String, bool> value;
    
    Token() : type(TokenType::END_OF_FILE), line(0), column(0) {}
    Token(TokenType t, const String& txt, Int32 l, Int32 c) 
        : type(t), text(txt), raw(txt), line(l), column(c) {}
    Token(TokenType t, const String& txt, Int32 l, Int32 c, Int64 v) 
        : type(t), text(txt), raw(txt), line(l), column(c), value(v) {}
    Token(TokenType t, const String& txt, Int32 l, Int32 c, Float64 v) 
        : type(t), text(txt), raw(txt), line(l), column(c), value(v) {}
    Token(TokenType t, const String& txt, Int32 l, Int32 c, const String& v) 
        : type(t), text(txt), raw(txt), line(l), column(c), value(v) {}
    Token(TokenType t, const String& txt, Int32 l, Int32 c, bool v) 
        : type(t), text(txt), raw(txt), line(l), column(c), value(v) {}
};

// 关键字映射表
class KeywordTable {
public:
    static const KeywordTable& instance() {
        static KeywordTable inst;
        return inst;
    }
    
    Optional<TokenType> find(const String& kw) const {
        auto it = keywords_.find(kw);
        if (it != keywords_.end()) {
            return it->second;
        }
        return std::nullopt;
    }
    
    bool isKeyword(const String& txt) const {
        return keywords_.count(txt) > 0;
    }

private:
    KeywordTable() {
        // 中文关键字
        keywords_ = {
            {"包名", TokenType::K_PACKAGE},
            {"导入", TokenType::K_IMPORT},
            {"类", TokenType::K_CLASS},
            {"接口", TokenType::K_INTERFACE},
            {"枚举", TokenType::K_ENUM},
            {"结构体", TokenType::K_STRUCT},
            {"函数", TokenType::K_FUNC},
            {"返回", TokenType::K_RETURN},
            {"如果", TokenType::K_IF},
            {"否则", TokenType::K_ELSE},
            {"选择", TokenType::K_SWITCH},
            {"匹配", TokenType::K_MATCH},
            {"情况", TokenType::K_CASE},
            {"其他", TokenType::K_DEFAULT},
            {"循环", TokenType::K_FOR},
            {"对于", TokenType::K_FOR},
            // 遍历/forEach 从关键字表移除，在parser中通过标识符文本判断
            // (避免与 stdlib forEach 函数名冲突)
            {"当", TokenType::K_WHILE},
            {"为", TokenType::K_DO},
            {"跳出", TokenType::K_BREAK},
            {"继续", TokenType::K_CONTINUE},
            {"公有", TokenType::K_PUBLIC},
            {"私有", TokenType::K_PRIVATE},
            {"保护", TokenType::K_PROTECTED},
            {"真", TokenType::K_TRUE},
            {"假", TokenType::K_FALSE},
            {"空", TokenType::K_NULL},
            {"新建", TokenType::K_NEW},
            {"删除", TokenType::K_DELETE},
            {"这个", TokenType::K_THIS},
            {"继承", TokenType::K_SUPER},
            {"实现", TokenType::K_IMPLEMENTS},
            {"尝试", TokenType::K_TRY},
            {"捕获", TokenType::K_CATCH},
            {"抛出", TokenType::K_THROW},
            {"最终", TokenType::K_FINALLY},
            {"使用", TokenType::K_WITH},
            {"异步", TokenType::K_ASYNC},
            {"等待", TokenType::K_AWAIT},
            {"协程", TokenType::K_GO},
            {"常量", TokenType::K_CONST},
            {"变量", TokenType::K_VAR},
            {"静态", TokenType::K_STATIC},
            {"导出", TokenType::K_EXPORT},
            {"抽象", TokenType::K_ABSTRACT},
            {"虚拟", TokenType::K_VIRTUAL},
            {"重写", TokenType::K_OVERRIDE},
            {"可变", TokenType::K_MUTABLE},

            // === 双语运算符关键词 ===
            {"则", TokenType::K_THEN},
            {"做", TokenType::K_DO_KEYWORD},
            {"在", TokenType::K_IN},
            {"从", TokenType::K_FROM},
            {"到", TokenType::K_TO},
            {"时", TokenType::K_WHILE_MARK},
            {"且", TokenType::K_AND},
            {"或", TokenType::K_OR},
            {"非", TokenType::K_NOT},
            {"是", TokenType::K_EQ},
            {"等于", TokenType::K_EQ},
            {"不等于", TokenType::K_NE},
            {"大于", TokenType::K_GT},
            {"小于", TokenType::K_LT},
            {"大于等于", TokenType::K_GE},
            {"小于等于", TokenType::K_LE},
            {"不低于", TokenType::K_GE},
            {"不超过", TokenType::K_LE},
            {"设", TokenType::K_LET},
            {"推迟", TokenType::K_DEFER},
            {"产出", TokenType::K_YIELD},
            {"宏", TokenType::K_MACRO},
            {"编译如果", TokenType::K_COND_IF},
            {"编译否则", TokenType::K_COND_ELSE},
            {"编译结束如果", TokenType::K_COND_ENDIF},
            {"编译定义", TokenType::K_COND_DEFINED},
            {"满足", TokenType::K_WHERE},
            {"类型", TokenType::K_TYPE},
            {"属于", TokenType::K_IS},

            // 英文备选关键字
            {"package", TokenType::K_PACKAGE},
            {"import", TokenType::K_IMPORT},
            {"class", TokenType::K_CLASS},
            {"interface", TokenType::K_INTERFACE},
            {"enum", TokenType::K_ENUM},
            {"struct", TokenType::K_STRUCT},
            {"func", TokenType::K_FUNC},
            {"function", TokenType::K_FUNC},
            {"return", TokenType::K_RETURN},
            {"if", TokenType::K_IF},
            {"else", TokenType::K_ELSE},
            {"switch", TokenType::K_SWITCH},
            {"match", TokenType::K_MATCH},
            {"case", TokenType::K_CASE},
            {"default", TokenType::K_DEFAULT},
            {"for", TokenType::K_FOR},
            // "foreach" 已从关键字表移除，在parser中通过标识符文本判断
            {"while", TokenType::K_WHILE},
            {"do", TokenType::K_DO},
            {"break", TokenType::K_BREAK},
            {"continue", TokenType::K_CONTINUE},
            {"public", TokenType::K_PUBLIC},
            {"private", TokenType::K_PRIVATE},
            {"protected", TokenType::K_PROTECTED},
            {"true", TokenType::K_TRUE},
            {"false", TokenType::K_FALSE},
            {"null", TokenType::K_NULL},
            {"new", TokenType::K_NEW},
            {"delete", TokenType::K_DELETE},
            {"this", TokenType::K_THIS},
            {"super", TokenType::K_SUPER},
            {"extends", TokenType::K_EXTENDS},
            {"implements", TokenType::K_IMPLEMENTS},
            {"try", TokenType::K_TRY},
            {"catch", TokenType::K_CATCH},
            {"throw", TokenType::K_THROW},
            {"finally", TokenType::K_FINALLY},
            {"with", TokenType::K_WITH},
            {"async", TokenType::K_ASYNC},
            {"await", TokenType::K_AWAIT},
            {"go", TokenType::K_GO},
            {"const", TokenType::K_CONST},
            {"var", TokenType::K_VAR},
            {"mutable", TokenType::K_MUTABLE},
            {"static", TokenType::K_STATIC},
            {"export", TokenType::K_EXPORT},
            {"abstract", TokenType::K_ABSTRACT},
            {"virtual", TokenType::K_VIRTUAL},
            {"override", TokenType::K_OVERRIDE},
            {"in", TokenType::K_IN},
            {"from", TokenType::K_FROM},
            {"then", TokenType::K_THEN},
            {"and", TokenType::K_AND},
            {"or", TokenType::K_OR},
            {"not", TokenType::K_NOT},
            {"let", TokenType::K_LET},
            {"defer", TokenType::K_DEFER},
            {"yield", TokenType::K_YIELD},
            {"macro", TokenType::K_MACRO},
            {"cond_if", TokenType::K_COND_IF},
            {"cond_else", TokenType::K_COND_ELSE},
            {"cond_endif", TokenType::K_COND_ENDIF},
            {"defined", TokenType::K_COND_DEFINED},
            {"where", TokenType::K_WHERE},
            {"type", TokenType::K_TYPE},
            {"is", TokenType::K_IS},
        };
    }
    
    std::unordered_map<String, TokenType> keywords_;
};

} // namespace cplang