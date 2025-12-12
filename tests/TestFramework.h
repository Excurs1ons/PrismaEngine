#pragma once

#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <chrono>
#include <iomanip>

namespace TestFramework {

// 测试结果
struct TestResult {
    std::string testName;
    bool passed;
    std::string failureMessage;
    double executionTime;
};

// 测试用例基类
class TestCase {
public:
    TestCase(const std::string& name) : m_name(name) {}
    virtual ~TestCase() = default;

    // 执行测试
    TestResult Run() {
        TestResult result;
        result.testName = m_name;

        auto start = std::chrono::high_resolution_clock::now();

        try {
            // 设置
            SetUp();

            // 运行测试
            RunTest();

            result.passed = true;
        }
        catch (const std::exception& e) {
            result.passed = false;
            result.failureMessage = e.what();
        }
        catch (...) {
            result.passed = false;
            result.failureMessage = "Unknown exception";
        }

        auto end = std::chrono::high_resolution_clock::now();
        result.executionTime = std::chrono::duration<double, std::milli>(end - start).count();

        try {
            // 清理
            TearDown();
        }
        catch (...) {
            // 忽略清理错误
        }

        return result;
    }

protected:
    // 子类重写
    virtual void SetUp() {}
    virtual void TearDown() {}
    virtual void RunTest() = 0;

    // 断言宏
    void Assert(bool condition, const std::string& message = "") {
        if (!condition) {
            throw std::runtime_error(message.empty() ? "Assertion failed" : message);
        }
    }

    void AssertEquals(const std::string& expected, const std::string& actual, const std::string& message = "") {
        if (expected != actual) {
            throw std::runtime_error(message + " - Expected: " + expected + ", Actual: " + actual);
        }
    }

    template<typename T>
    void AssertEquals(T expected, T actual, const std::string& message = "") {
        if (expected != actual) {
            throw std::runtime_error(message + " - Expected: " + std::to_string(expected) + ", Actual: " + std::to_string(actual));
        }
    }

    void AssertNotNull(const void* ptr, const std::string& message = "") {
        if (ptr == nullptr) {
            throw std::runtime_error(message.empty() ? "Pointer is null" : message);
        }
    }

    void AssertNull(const void* ptr, const std::string& message = "") {
        if (ptr != nullptr) {
            throw std::runtime_error(message.empty() ? "Pointer is not null" : message);
        }
    }

    void AssertTrue(bool condition, const std::string& message = "") {
        if (!condition) {
            throw std::runtime_error(message.empty() ? "Condition is false" : message);
        }
    }

    void AssertFalse(bool condition, const std::string& message = "") {
        if (condition) {
            throw std::runtime_error(message.empty() ? "Condition is true" : message);
        }
    }

    template<typename T>
    void AssertNear(T expected, T actual, T tolerance, const std::string& message = "") {
        if (std::abs(expected - actual) > tolerance) {
            throw std::runtime_error(message + " - Expected: " + std::to_string(expected) +
                                    ", Actual: " + std::to_string(actual) +
                                    ", Tolerance: " + std::to_string(tolerance));
        }
    }

private:
    std::string m_name;
};

// 测试套件
class TestSuite {
public:
    TestSuite(const std::string& name) : m_name(name) {}

    ~TestSuite() {
        for (auto test : m_tests) {
            delete test;
        }
    }

    void AddTest(TestCase* test) {
        m_tests.push_back(test);
    }

    std::vector<TestResult> RunAllTests() {
        std::vector<TestResult> results;

        std::cout << "\nRunning test suite: " << m_name << "\n";
        std::cout << std::string(m_name.length() + 19, '=') << "\n";

        for (auto test : m_tests) {
            TestResult result = test->Run();
            results.push_back(result);

            // 输出结果
            std::cout << "[";
            if (result.passed) {
                std::cout << "PASS";
            } else {
                std::cout << "FAIL";
            }
            std::cout << "] " << result.testName;

            // 显示执行时间
            if (result.executionTime > 0) {
                std::cout << " (" << std::fixed << std::setprecision(2) << result.executionTime << "ms)";
            }

            std::cout << "\n";

            // 如果失败，显示错误信息
            if (!result.passed) {
                std::cout << "  Error: " << result.failureMessage << "\n";
            }
        }

        return results;
    }

private:
    std::string m_name;
    std::vector<TestCase*> m_tests;
};

// 便利宏定义测试用例
#define TEST_CASE(className, testName) \
    class className##_##testName : public TestFramework::TestCase { \
    public: \
        className##_##testName() : TestFramework::TestCase(#className "::" #testName) {} \
    protected: \
        void RunTest() override

#define END_TEST_CASE };

// 测试运行器
class TestRunner {
public:
    static void AddSuite(TestSuite* suite) {
        m_suites.push_back(suite);
    }

    static int RunAllSuites() {
        int totalPassed = 0;
        int totalFailed = 0;
        double totalTime = 0;

        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "PrismaEngine Unit Test Runner\n";
        std::cout << std::string(60, '=') << "\n";

        for (auto suite : m_suites) {
            auto results = suite->RunAllTests();

            for (const auto& result : results) {
                if (result.passed) {
                    totalPassed++;
                } else {
                    totalFailed++;
                }
                totalTime += result.executionTime;
            }
        }

        // 输出总结
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "Test Results Summary:\n";
        std::cout << "  Total: " << (totalPassed + totalFailed) << "\n";
        std::cout << "  Passed: " << totalPassed << "\n";
        std::cout << "  Failed: " << totalFailed << "\n";
        std::cout << "  Time: " << std::fixed << std::setprecision(2) << totalTime << "ms\n";
        std::cout << std::string(60, '=') << "\n";

        return totalFailed > 0 ? 1 : 0;
    }

private:
    static std::vector<TestSuite*> m_suites;
};

// 静态成员定义
std::vector<TestSuite*> TestRunner::m_suites;

// 便利宏注册测试套件
#define REGISTER_TEST_SUITE(suiteName) \
    static struct suiteName##_Registrar { \
        suiteName##_Registrar() { \
            TestRunner::AddSuite(new suiteName()); \
        } \
    } suiteName##_Registrar_instance;

} // namespace TestFramework