// CPLSP — 代码补全（基于 AST 扫描 + 标准库）
#include "cplsp.hpp"
#include <set>

namespace cplsp {

// 关键词补全列表
static const std::vector<std::pair<std::string, std::string>> KEYWORDS = {
    {"函数", "定义函数"}, {"function", "定义函数 (英文)"},
    {"变量", "声明变量"}, {"var", "声明变量 (英文)"},
    {"常量", "声明常量"}, {"const", "声明常量 (英文)"},
    {"返回", "返回值"}, {"return", "返回值 (英文)"},
    {"如果", "条件判断"}, {"if", "条件判断 (英文)"},
    {"否则", "否则分支"}, {"else", "否则分支 (英文)"},
    {"当", "while 循环"}, {"while", "while 循环 (英文)"},
    {"循环", "for 循环"}, {"for", "for 循环 (英文)"},
    {"遍历", "遍历容器"}, {"foreach", "遍历容器 (英文)"},
    {"跳出", "跳出循环"}, {"break", "跳出循环 (英文)"},
    {"继续", "继续循环"}, {"continue", "继续循环 (英文)"},
    {"选择", "switch 语句"}, {"switch", "switch 语句 (英文)"},
    {"匹配", "match 表达式"}, {"match", "match 表达式 (英文)"},
    {"尝试", "try-catch 块"}, {"try", "try-catch 块 (英文)"},
    {"抛出", "抛出异常"}, {"throw", "抛出异常 (英文)"},
    {"导入", "导入模块"}, {"import", "导入模块 (英文)"},
    {"类", "定义类"}, {"class", "定义类 (英文)"},
    {"结构体", "定义结构体"}, {"struct", "定义结构体 (英文)"},
    {"枚举", "定义枚举"}, {"enum", "定义枚举 (英文)"},
    {"接口", "定义接口"}, {"interface", "定义接口 (英文)"},
    {"真", "布尔值 true"}, {"true", "布尔值 true (英文)"},
    {"假", "布尔值 false"}, {"false", "布尔值 false (英文)"},
    {"空", "空值 nil"}, {"null", "空值 (英文)"},
};

// 标准库函数补全
static const std::vector<std::pair<std::string, std::string>> BUILTINS = {
    // IO
    {"打印", "输出到控制台"}, {"print", "输出到控制台 (英文)"},
    // 数学
    {"绝对值", "abs(n)"}, {"平方根", "sqrt(n)"}, {"圆周率", "π"},
    // 字符串
    {"长度", "字符串/数组长度"}, {"子串", "substr(s,start,len)"},
    {"连接", "concat(a,b)"}, {"查找", "find(s,pat)"},
    {"替换", "replace(s,from,to)"}, {"分割", "split(s,sep)"},
    {"小写", "lower(s)"}, {"大写", "upper(s)"},
    // 转换
    {"转字符串", "toString(v)"}, {"转整数", "parseInt(s)"},
    // 数组
    {"追加", "push(arr,val)"}, {"弹出", "pop(arr)"},
    {"插入", "insert(arr,idx,val)"}, {"删除", "remove(arr,idx)"},
    {"切片", "slice(arr,start,end)"}, {"排序", "sort(arr)"},
    {"映射", "map(arr,fn)"}, {"过滤", "filter(arr,fn)"},
    {"去重", "unique(arr)"},
    // 字节数组
    {"创建字节数组", "byteArrayCreate(size)"},
    {"字节数组获取", "byteArrayGet(buf,idx)"},
    {"字节数组设置", "byteArraySet(buf,idx,val)"},
    {"字节数组长度", "byteArrayLen(buf)"},
    {"字节数组转字符串", "byteArrayToStr(buf)"},
    {"字节数组转十六进制", "byteArrayToHex(buf)"},
    {"字节数组自十六进制", "byteArrayFromHex(hex)"},
    // 表
    {"表取", "tableGet(t,key)"}, {"表设", "tableSet(t,key,val)"},
    // 时间
    {"时间戳", "now()"},
};

std::vector<CompletionItem> computeCompletion(Document* doc, const LspPosition& pos) {
    std::vector<CompletionItem> result;
    
    // 1. 关键词
    for (auto& kw : KEYWORDS) {
        CompletionItem item;
        item.label = kw.first;
        item.detail = kw.second;
        item.kind = 14;  // Keyword
        result.push_back(item);
    }
    
    // 2. 标准库函数
    for (auto& b : BUILTINS) {
        CompletionItem item;
        item.label = b.first;
        item.detail = b.second;
        item.kind = 3;  // Function
        result.push_back(item);
    }
    
    // 3. AST 提取用户定义的标识符
    if (doc && doc->ast) {
        std::set<std::string> seen;
        for (auto& stmt : doc->ast->statements) {
            if (auto func = std::dynamic_pointer_cast<cplang::FuncDeclStmt>(stmt)) {
                if (seen.insert(func->name).second) {
                    CompletionItem item;
                    item.label = func->name;
                    item.detail = "函数 (文档内定义)";
                    item.kind = 3;
                    result.push_back(item);
                }
            } else if (auto var = std::dynamic_pointer_cast<cplang::VarDeclStmt>(stmt)) {
                if (seen.insert(var->name).second) {
                    CompletionItem item;
                    item.label = var->name;
                    item.detail = "变量 (文档内定义)";
                    item.kind = 6;
                    result.push_back(item);
                }
            }
        }
    }
    
    return result;
}

} // namespace cplsp
