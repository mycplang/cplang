#include "lexer/lexer.hpp"
#include <iostream>
#include <string>

using namespace cplang;

int main() {
    std::string dang = u8"\xe5\xbd\x93";  // 当
    auto r = KeywordTable::instance().find(dang);
    std::cout << "find dang = " << (r.has_value() ? static_cast<int>(r.value()) : -1) << std::endl;
    
    std::string ruguo = u8"\xe5\xa6\x82\xe6\x9e\x9c";  // 如果
    r = KeywordTable::instance().find(ruguo);
    std::cout << "find ruguo = " << (r.has_value() ? static_cast<int>(r.value()) : -1) << std::endl;
    
    return 0;
}
