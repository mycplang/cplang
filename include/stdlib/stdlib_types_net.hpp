#pragma once
#include "vm/value.hpp"
#include <vector>

namespace cplang {

namespace types {
    Value isNil(std::vector<Value>& args);
    Value isBool(std::vector<Value>& args);
    Value isInt(std::vector<Value>& args);
    Value isFloat(std::vector<Value>& args);
    Value isNumber(std::vector<Value>& args);
    Value isString(std::vector<Value>& args);
    Value isArray(std::vector<Value>& args);
    Value isTable(std::vector<Value>& args);
    Value isFunction(std::vector<Value>& args);
    Value isObject(std::vector<Value>& args);
    Value typeOf(std::vector<Value>& args);
    Value toString(std::vector<Value>& args);
    Value toInt(std::vector<Value>& args);
    Value toFloat(std::vector<Value>& args);
    Value toBool(std::vector<Value>& args);
}

namespace network {
    Value httpGet(std::vector<Value>& args);
    Value httpPost(std::vector<Value>& args);
    Value tcpConnect(std::vector<Value>& args);
    Value udpSocket(std::vector<Value>& args);
    Value resolve(std::vector<Value>& args);
    Value download(std::vector<Value>& args);
    Value upload(std::vector<Value>& args);
}

namespace net {
    Value tcpConnect(std::vector<Value>& args);
    Value tcpSend(std::vector<Value>& args);
    Value tcpReceive(std::vector<Value>& args);
    Value tcpClose(std::vector<Value>& args);
    Value tcpListen(std::vector<Value>& args);
    Value tcpAccept(std::vector<Value>& args);
    Value udpSocket(std::vector<Value>& args);
    Value udpSend(std::vector<Value>& args);
    Value udpReceive(std::vector<Value>& args);
}

namespace http {
    Value httpGet(std::vector<Value>& args);
    Value httpPost(std::vector<Value>& args);
    Value httpDownload(std::vector<Value>& args);
}

namespace json {
    Value parse(std::vector<Value>& args);
    Value stringify(std::vector<Value>& args);
    Value validate(std::vector<Value>& args);
    Value path(std::vector<Value>& args);
    Value jsonParse(std::vector<Value>& args);
    Value jsonStringify(std::vector<Value>& args);
    Value jsonPretty(std::vector<Value>& args);
    Value jsonValidate(std::vector<Value>& args);
}

namespace color {
    Value hexToRgb(std::vector<Value>& args);
    Value rgbToHex(std::vector<Value>& args);
    Value rgbToHsl(std::vector<Value>& args);
    Value hslToRgb(std::vector<Value>& args);
}

namespace matrix {
    Value matrixAdd(std::vector<Value>& args);
    Value matrixMul(std::vector<Value>& args);
    Value matrixTranspose(std::vector<Value>& args);
    Value matrixIdentity(std::vector<Value>& args);
    Value matrixDet(std::vector<Value>& args);
    Value vectorDot(std::vector<Value>& args);
    Value vectorCross(std::vector<Value>& args);
    Value vectorNorm(std::vector<Value>& args);
    Value vectorNormalize(std::vector<Value>& args);
}

namespace reflect {
    Value keys(std::vector<Value>& args);
    Value values(std::vector<Value>& args);
    Value hasKey(std::vector<Value>& args);
    Value getField(std::vector<Value>& args);
    Value setField(std::vector<Value>& args);
}

} // namespace cplang
