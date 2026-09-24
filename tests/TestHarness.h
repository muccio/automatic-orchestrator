#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <cmath>
#include <sstream>

namespace OrchestratorTest {

struct TestCase {
    std::string suite;
    std::string name;
    std::function<void()> fn;
};

class TestRegistry {
public:
    static TestRegistry& instance() {
        static TestRegistry reg;
        return reg;
    }

    void addTest(const std::string& suite, const std::string& name, std::function<void()> fn) {
        tests.push_back({suite, name, fn});
    }

    int run(const std::string& filter = "") {
        int passed = 0;
        int failed = 0;
        std::cout << "\n========================================\n";
        std::cout << " RUNNING ORCHESTRATOR TEST SUITE\n";
        if (!filter.empty()) {
            std::cout << " Filter: " << filter << "\n";
        }
        std::cout << "========================================\n\n";

        for (const auto& t : tests) {
            if (!filter.empty() && t.suite.find(filter) == std::string::npos && t.name.find(filter) == std::string::npos) {
                continue;
            }

            std::cout << "[RUN] " << t.suite << " :: " << t.name << " ... ";
            try {
                t.fn();
                std::cout << "\033[32mPASSED\033[0m\n";
                passed++;
            } catch (const std::exception& e) {
                std::cout << "\033[31mFAILED\033[0m: " << e.what() << "\n";
                failed++;
            } catch (...) {
                std::cout << "\033[31mFAILED (Unknown exception)\033[0m\n";
                failed++;
            }
        }

        std::cout << "\n----------------------------------------\n";
        std::cout << " SUMMARY: " << passed << " passed, " << failed << " failed, "
                  << (passed + failed) << " total executed.\n";
        std::cout << "----------------------------------------\n";

        return (failed == 0) ? 0 : 1;
    }

private:
    std::vector<TestCase> tests;
};

struct TestRegistrar {
    TestRegistrar(const std::string& suite, const std::string& name, std::function<void()> fn) {
        TestRegistry::instance().addTest(suite, name, fn);
    }
};

#define TEST_CASE(suite, name) \
    static void test_##suite##_##name(); \
    static OrchestratorTest::TestRegistrar reg_##suite##_##name(#suite, #name, test_##suite##_##name); \
    static void test_##suite##_##name()

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            std::ostringstream ss; \
            ss << "Assertion failed: (" #condition ") at " << __FILE__ << ":" << __LINE__; \
            throw std::runtime_error(ss.str()); \
        } \
    } while (0)

#define ASSERT_FALSE(condition) ASSERT_TRUE(!(condition))

#define ASSERT_EQ(actual, expected) \
    do { \
        if (!((actual) == (expected))) { \
            std::ostringstream ss; \
            ss << "Assertion failed: (" #actual " == " #expected ") -> (" \
               << (actual) << " != " << (expected) << ") at " << __FILE__ << ":" << __LINE__; \
            throw std::runtime_error(ss.str()); \
        } \
    } while (0)

#define ASSERT_NEAR(actual, expected, eps) \
    do { \
        if (std::abs((actual) - (expected)) > (eps)) { \
            std::ostringstream ss; \
            ss << "Assertion failed: |" #actual " - " #expected "| <= " #eps " -> (" \
               << (actual) << " vs " << (expected) << ") at " << __FILE__ << ":" << __LINE__; \
            throw std::runtime_error(ss.str()); \
        } \
    } while (0)

} // namespace OrchestratorTest
