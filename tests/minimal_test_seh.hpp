// CP语言 最小化测试框架（不依赖 GTest）
// 提供 GTest 兼容的 TEST/EXPECT/ASSERT 宏，支持 << 链式输出
#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <functional>
#include <cmath>
#include <sstream>
#include <cstdlib>

// Forward declarations
struct TestRegistry;

// ---- Test failure reporting function (defined after TestRegistry) ----
void reportTestFailure(const char* expr, int line, const std::string& msg);

// ---- Helper class for EXPECT_* << chaining ----
class TestAssertHelper {
public:
    TestAssertHelper(bool condition, const char* expr, int line)
        : ok_(condition), expr_(expr), line_(line) {}
    
    ~TestAssertHelper() {
        if (!ok_ && !reported_) {
            reported_ = true;
            std::string msg = msg_.str();
            reportTestFailure(expr_, line_, msg);
        }
    }
    
    template<typename T>
    TestAssertHelper& operator<<(const T& val) {
        if (!ok_ && !reported_) { 
            msg_ << val; 
        }
        return *this;
    }

    operator bool() const { return ok_; }

private:
    bool ok_;
    const char* expr_;
    int line_;
    bool reported_ = false;
    std::ostringstream msg_;
};

// ---- 断言宏 ----
#define EXPECT_TRUE(cond) \
    ::TestAssertHelper((cond), "EXPECT_TRUE(" #cond ")", __LINE__)

#define EXPECT_FALSE(cond) \
    ::TestAssertHelper(!(cond), "EXPECT_FALSE(" #cond ")", __LINE__)

#define EXPECT_EQ(a, b) \
    ::TestAssertHelper(((a) == (b)), "EXPECT_EQ(" #a ", " #b ")", __LINE__)

#define EXPECT_NE(a, b) \
    ::TestAssertHelper(((a) != (b)), "EXPECT_NE(" #a ", " #b ")", __LINE__)

#define EXPECT_GE(a, b) \
    ::TestAssertHelper(((a) >= (b)), "EXPECT_GE(" #a ", " #b ")", __LINE__)

#define EXPECT_LE(a, b) \
    ::TestAssertHelper(((a) <= (b)), "EXPECT_LE(" #a ", " #b ")", __LINE__)

#define EXPECT_GT(a, b) \
    ::TestAssertHelper(((a) > (b)), "EXPECT_GT(" #a ", " #b ")", __LINE__)

#define EXPECT_LT(a, b) \
    ::TestAssertHelper(((a) < (b)), "EXPECT_LT(" #a ", " #b ")", __LINE__)

#define EXPECT_STREQ(a, b) \
    ::TestAssertHelper(((a) == (b)), "EXPECT_STREQ(" #a ", " #b ")", __LINE__)

#define EXPECT_STRNE(a, b) \
    ::TestAssertHelper(((a) != (b)), "EXPECT_STRNE(" #a ", " #b ")", __LINE__)

#define EXPECT_FLOAT_EQ(a, b) \
    ::TestAssertHelper((std::abs((a) - (b)) <= 0.0001f), "EXPECT_FLOAT_EQ(" #a ", " #b ")", __LINE__)

#define ASSERT_NE(a, b) EXPECT_NE(a, b)
#define ASSERT_EQ(a, b) EXPECT_EQ(a, b)
#define ASSERT_TRUE(cond) EXPECT_TRUE(cond)
#define ASSERT_FALSE(cond) EXPECT_FALSE(cond)

#define SUCCEED() do {} while(0)

// ---- 测试注册 ----
#define TEST(suite, name) \
    static void _test_##suite##_##name(); \
    namespace { \
        struct _Reg_##suite##_##name { \
            _Reg_##suite##_##name() { \
                getTestRegistry().registerTest(#suite, #name, _test_##suite##_##name); \
            } \
        } _reg_##suite##_##name; \
    } \
    static void _test_##suite##_##name()

struct TestCase {
    std::string suite;
    std::string name;
    std::function<void()> fn;
};

struct TestRegistry {
    std::vector<TestCase> tests;
    int failures = 0;
    int total = 0;

    void registerTest(const std::string& suite, const std::string& name, std::function<void()> fn) {
        tests.push_back({suite, name, fn});
    }
};

// Global registry access
inline TestRegistry& getTestRegistry() {
    static TestRegistry reg;
    return reg;
}

// Report test failure (called by TestAssertHelper destructor)
inline void reportTestFailure(const char* expr, int line, const std::string& msg) {
    std::cerr << "  FAIL at line " << line << ": " << expr;
    if (!msg.empty()) {
        std::cerr << " (" << msg << ")";
    }
    std::cerr << std::endl;
    getTestRegistry().failures++;
}

inline int runAllTests() {
    TestRegistry& reg = getTestRegistry();
    std::cout << "\n========== CP Language Test Suite ==========\n";
    std::string currentSuite;
    int suiteFailures = 0;
    int suiteTotal = 0;

    for (auto& t : reg.tests) {
        if (t.suite != currentSuite) {
            if (!currentSuite.empty()) {
                std::cout << "  Suite complete: " << suiteTotal - suiteFailures << "/" << suiteTotal << " passed\n";
            }
            currentSuite = t.suite;
            suiteFailures = 0;
            suiteTotal = 0;
            std::cout << "\n[" << t.suite << "]\n";
        }

        reg.failures = 0;
        reg.total++;
        suiteTotal++;
        std::cout << "  " << t.name << "... ";
        try {
            t.fn();
        } catch (const std::exception& e) {
            std::cerr << "  EXCEPTION: " << e.what() << std::endl;
            reg.failures++;
        } catch (...) {
            std::cerr << "  UNKNOWN EXCEPTION" << std::endl;
            reg.failures++;
        }

        if (reg.failures > 0) {
            std::cout << "FAILED\n";
            suiteFailures += reg.failures;
        } else {
            std::cout << "OK\n";
        }
    }

    if (!currentSuite.empty()) {
        std::cout << "  Suite complete: " << suiteTotal - suiteFailures << "/" << suiteTotal << " passed\n";
    }

    std::cout << "\n==========================================\n";
    std::cout << "Total: " << reg.total - suiteFailures << "/" << reg.total << " passed";
    if (suiteFailures > 0) {
        std::cout << ", " << suiteFailures << " FAILED";
    }
    std::cout << "\n";
    return suiteFailures > 0 ? 1 : 0;
}

#define TEST_MAIN() \
    int main() { \
        __try { \
            return runAllTests(); \
        } __except(1) { \
            std::cerr << "SEH EXCEPTION in test execution!" << std::endl; \
            return 1; \
        } \
    } \
    int main() { \
        return runAllTests(); \
    }
