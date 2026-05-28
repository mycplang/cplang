#pragma once
#include "common/types.hpp"
#include "vm/vm.hpp"
#include <vector>
#include <functional>
#include <exception>

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  异常类型
// ═══════════════════════════════════════════════════════════════════

enum class ExceptionType {
    RuntimeError,
    TypeError,
    IndexError,
    KeyError,
    ValueError,
    ZeroDivisionError,
    OverflowError,
    MemoryError,
    IOError,
    ImportError,
    NameError,
    AttributeError,
    NotImplementedError,
    Custom
};

// ═══════════════════════════════════════════════════════════════════
//  异常对象
// ═══════════════════════════════════════════════════════════════════

struct Exception {
    ExceptionType type;
    String message;
    String stackTrace;
    Value value;
    String file;
    int line = 0;
    int column = 0;

    Exception(ExceptionType t, const String& msg)
        : type(t), message(msg) {}
    Exception(const String& msg)
        : type(ExceptionType::RuntimeError), message(msg) {}

    String toString() const;
};

// ═══════════════════════════════════════════════════════════════════
//  异常栈帧（用于 try-catch-finally）
// ═══════════════════════════════════════════════════════════════════

struct CatchHandler {
    ExceptionType type = ExceptionType::RuntimeError;
    String typeName;        // 自定义类型名
    String varName;         // catch(e) 中的变量名
    int handlerPC = -1;     // handler 代码起始 PC
    int endPC = -1;         // try 块结束 PC
};

struct ExceptionFrame {
    std::vector<CatchHandler> catchers;
    int finallyPC = -1;     // finally 块 PC，-1 表示无 finally
    int finallyEndPC = -1;
    size_t stackBase;
    bool isActive = false;
};

// ═══════════════════════════════════════════════════════════════════
//  异常处理系统
// ═══════════════════════════════════════════════════════════════════

class ExceptionHandler {
public:
    explicit ExceptionHandler(VM* vm);

    // 抛出异常
    void throwException(const Exception& ex);
    void throwException(ExceptionType type, const String& message);

    // 查询
    bool hasException() const { return currentException_.has_value(); }
    const Exception& getException() const { return *currentException_; }
    void clearException() { currentException_.reset(); }

    // 栈帧管理
    void pushFrame(const ExceptionFrame& frame);
    void popFrame();

    // 查找匹配的 catch
    const CatchHandler* findHandler(ExceptionType type) const;
    const CatchHandler* findHandler(const String& typeName) const;

    // 堆栈跟踪
    String generateStackTrace() const;
    void addStackFrame(const String& function, const String& file, int line);
    void popStackFrame();

    // 异常匹配
    bool matches(const CatchHandler& handler, const Exception& ex) const;

private:
    [[maybe_unused]] VM* vm_;
    std::optional<Exception> currentException_;
    std::vector<ExceptionFrame> frameStack_;
    std::vector<String> callStack_;
};

// ═══════════════════════════════════════════════════════════════════
//  C++ 异常（用于编译器内部抛出）
// ═══════════════════════════════════════════════════════════════════

class CPLangException : public std::exception {
public:
    CPLangException(ExceptionType type, const String& msg)
        : type_(type), message_(msg) {}

    const char* what() const noexcept override { return message_.c_str(); }
    ExceptionType getType() const { return type_; }
    const String& getMessage() const { return message_; }

private:
    ExceptionType type_;
    String message_;
};

// ═══════════════════════════════════════════════════════════════════
//  异常辅助宏
// ═══════════════════════════════════════════════════════════════════

#define THROW_TYPE_ERROR(msg) \
    throw CPLangException(ExceptionType::TypeError, msg)

#define THROW_INDEX_ERROR(msg) \
    throw CPLangException(ExceptionType::IndexError, msg)

#define THROW_KEY_ERROR(msg) \
    throw CPLangException(ExceptionType::KeyError, msg)

#define THROW_VALUE_ERROR(msg) \
    throw CPLangException(ExceptionType::ValueError, msg)

#define THROW_ZERO_DIVISION() \
    throw CPLangException(ExceptionType::ZeroDivisionError, "Division by zero")

#define THROW_NAME_ERROR(name) \
    throw CPLangException(ExceptionType::NameError, "Name '" + String(name) + "' is not defined")

} // namespace cplang
