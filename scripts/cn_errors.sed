# Chinese error messages migration
# VM
s/"division by zero"/"除零错误"/g
s/"not callable"/"不可调用：该值不是函数"/g
s/"null function"/"空函数"/g
s/"PC out of bounds"/"程序计数器越界"/g
s/"import: bad module index"/"导入失败：模块索引无效"/g
s/"import: module name must be string"/"导入失败：模块名必须是字符串"/g
s/"import not supported"/"导入功能不支持"/g
# Lexer
s/"bad float: "/"无效浮点数: "/g
s/"bad int: "/"无效整数: "/g
s/"unterminated string"/"字符串未闭合"/g
# Parser
s/"Expected member name after type"/"类型后缺少成员名"/g
s/"Expected member declaration in struct"/"结构体中缺少成员声明"/g
s/"Expected '{' after case"/"case后缺少 {"/g
s/"Expected '{' after default"/"default后缺少 {"/g
s/"Expected variable name or '(' in for-each"/"for-each中缺少变量名或("/g
s/"Expected field name in table literal"/"表字面量中缺少字段名"/g
s/"Expected field name in struct literal"/"结构体字面量中缺少字段名"/g
s/"Unexpected token: "/"未预期的符号: "/g
# Semantic
s/ is already defined in this scope/ 在此作用域中重复定义/g
s/ must be inside a loop/必须在循环内使用/g
s/For loop condition must be boolean/for循环条件必须是布尔值/g
