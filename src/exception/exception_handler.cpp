// 异常处理实现

#include "exception/exception_handler.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  Exception 实现
// ═══════════════════════════════════════════════════════════════════

String Exception::toString() const {
    std::stringstream ss;

    const char* typeName = "运行时错误";
    switch (type) {
        case ExceptionType::TypeError:          typeName = "类型错误"; break;
        case ExceptionType::IndexError:         typeName = "索引错误"; break;
        case ExceptionType::KeyError:           typeName = "键错误"; break;
        case ExceptionType::ValueError:         typeName = "值错误"; break;
        case ExceptionType::ZeroDivisionError:  typeName = "除零错误"; break;
        case ExceptionType::OverflowError:      typeName = "溢出错误"; break;
        case ExceptionType::MemoryError:        typeName = "内存错误"; break;
        case ExceptionType::IOError:            typeName = "输入输出错误"; break;
        case ExceptionType::ImportError:        typeName = "导入错误"; break;
        case ExceptionType::NameError:          typeName = "名称错误"; break;
        case ExceptionType::AttributeError:     typeName = "属性错误"; break;
        case ExceptionType::NotImplementedError: typeName = "未实现错误"; break;
        case ExceptionType::Custom:             typeName = "自定义异常"; break;
        default:                                typeName = "运行时错误"; break;
    }

    ss << typeName << ": " << message;

    if (!file.empty()) {
        ss << " (" << file << ":" << line;
        if (column > 0) ss << ":" << column;
        ss << ")";
    }

    if (!stackTrace.empty()) {
        ss << "\n\n堆栈跟踪:\n" << stackTrace;
    }

    return ss.str();
}

// ═══════════════════════════════════════════════════════════════════
//  ExceptionHandler 实现
// ═══════════════════════════════════════════════════════════════════

ExceptionHandler::ExceptionHandler(VM* vm) : vm_(vm) {}

void ExceptionHandler::throwException(const Exception& ex) {
    currentException_ = ex;
    if (!callStack_.empty()) {
        currentException_->stackTrace = generateStackTrace();
    }

    // 查找匹配的处理器
    for (auto it = frameStack_.rbegin(); it != frameStack_.rend(); ++it) {
        if (findHandler(ex.type)) return;
    }

    // 未捕获异常
    std::cerr << "未捕获异常:\n" << ex.toString() << std::endl;
}

void ExceptionHandler::throwException(ExceptionType type, const String& message) {
    throwException(Exception(type, message));
}

void ExceptionHandler::pushFrame(const ExceptionFrame& frame) {
    frameStack_.push_back(frame);
}

void ExceptionHandler::popFrame() {
    if (!frameStack_.empty()) {
        frameStack_.pop_back();
    }
}

const CatchHandler* ExceptionHandler::findHandler(ExceptionType type) const {
    for (const auto& frame : frameStack_) {
        for (const auto& handler : frame.catchers) {
            if (matches(handler, Exception(type, ""))) {
                return &handler;
            }
        }
    }
    return nullptr;
}

const CatchHandler* ExceptionHandler::findHandler(const String& typeName) const {
    for (const auto& frame : frameStack_) {
        for (const auto& handler : frame.catchers) {
            if (handler.typeName == typeName) {
                return &handler;
            }
        }
    }
    return nullptr;
}

String ExceptionHandler::generateStackTrace() const {
    std::stringstream ss;
    int frameNum = 0;
    for (auto it = callStack_.rbegin(); it != callStack_.rend(); ++it) {
        ss << "  位于 " << *it;
        if (frameNum < static_cast<int>(callStack_.size()) - 1) ss << "\n";
        frameNum++;
    }
    return ss.str();
}

void ExceptionHandler::addStackFrame(const String& function, const String& file, int line) {
    std::stringstream ss;
    ss << function;
    if (!file.empty()) {
        ss << " (" << file;
        if (line > 0) ss << ":" << line;
        ss << ")";
    }
    callStack_.push_back(ss.str());
}

void ExceptionHandler::popStackFrame() {
    if (!callStack_.empty()) callStack_.pop_back();
}

bool ExceptionHandler::matches(const CatchHandler& handler, const Exception& ex) const {
    if (handler.typeName.empty() && handler.type == ExceptionType::RuntimeError) return true;
    if (handler.type == ex.type) return true;
    if (!handler.typeName.empty() && handler.typeName == "Exception") return true;
    return false;
}

} // namespace cplang
