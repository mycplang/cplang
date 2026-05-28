#pragma once
#include "vm/value.hpp"
#include <vector>

namespace cplang {

namespace regex_ {
    Value match(std::vector<Value>& args);
    Value search(std::vector<Value>& args);
    Value replace(std::vector<Value>& args);
    Value split(std::vector<Value>& args);
    Value findAll(std::vector<Value>& args);
}

namespace encoding {
    Value urlEncode(std::vector<Value>& args);
    Value urlDecode(std::vector<Value>& args);
    Value base64Encode(std::vector<Value>& args);
    Value base64Decode(std::vector<Value>& args);
    Value hexEncode(std::vector<Value>& args);
    Value hexDecode(std::vector<Value>& args);
}

namespace crypto {
    Value md5(std::vector<Value>& args);
    Value sha1(std::vector<Value>& args);
    Value sha256(std::vector<Value>& args);
    Value base64Encode(std::vector<Value>& args);
    Value base64Decode(std::vector<Value>& args);
}

} // namespace cplang
