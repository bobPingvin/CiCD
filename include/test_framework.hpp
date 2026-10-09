#pragma once
#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <future>
#include <mutex>
#include <chrono>
#include <sstream>

struct TestFailure {
    std::string suite;
    std::string name;
    std::string message;
    std::string file;
    int line;
};

class TestAssertionException : public std::exception {
public:
    std::string msg;
    std::string file;
    int line;
    TestAssertionException(std::string m, std::string f, int l)
        : msg(std::move(m)), file(std::move(f)), line(l) {}
};

struct TestDefinition {
    std::string suite;
    std::string name;
    std::function<void()> func;
};

class UnitTestRegistry {
public:
    static UnitTestRegistry& instance() {
        static UnitTestRegistry reg;
        return reg;
    }

    void register_test(std::string suite, std::string name, std::function<void()> func) {
        tests.push_back({std::move(suite), std::move(name), std::move(func)});
    }

    int run_all() {
        std::vector<TestFailure> failures;
        std::mutex mtx;
        int passed = 0;
        int failed = 0;

        std::cout << "[TEST RUNNER] Running " << tests.size() << " test(s) in parallel via std::async...\n";
        auto start_time = std::chrono::steady_clock::now();

        std::vector<std::future<void>> futures;
        futures.reserve(tests.size());

        for (const auto& t : tests) {
            futures.push_back(std::async(std::launch::async, [&t, &failures, &mtx, &passed, &failed]() {
                try {
                    t.func();
                    std::lock_guard<std::mutex> lock(mtx);
                    passed++;
                    std::cout << "  [PASS] " << t.suite << "." << t.name << "\n";
                } catch (const TestAssertionException& e) {
                    std::lock_guard<std::mutex> lock(mtx);
                    failed++;
                    failures.push_back({t.suite, t.name, e.msg, e.file, e.line});
                    std::cout << "  [FAIL] " << t.suite << "." << t.name << "\n";
                } catch (const std::exception& e) {
                    std::lock_guard<std::mutex> lock(mtx);
                    failed++;
                    failures.push_back({t.suite, t.name, std::string("Unexpected exception: ") + e.what(), "", 0});
                    std::cout << "  [FAIL] " << t.suite << "." << t.name << "\n";
                }
            }));
        }

        for (auto& f : futures) {
            f.get();
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time).count();

        std::cout << "\n================= SUMMARY =================" << "\n";
        std::cout << "Total: " << tests.size() << ", Passed: " << passed
                  << ", Failed: " << failed << " (" << elapsed << " ms)\n";

        if (!failures.empty()) {
            std::cout << "\nDetailed Failures:\n";
            for (const auto& f : failures) {
                std::cout << " - " << f.suite << "." << f.name;
                if (!f.file.empty()) {
                    std::cout << " (" << f.file << ":" << f.line << ")";
                }
                std::cout << ": " << f.message << "\n";
            }
            std::cout << "===========================================\n";
            return 1;
        }
        std::cout << "===========================================\n";
        return 0;
    }

private:
    std::vector<TestDefinition> tests;
};

#define TEST(suite_name, test_name) \
    void suite_name##_##test_name(); \
    static struct AutoReg_##suite_name##_##test_name { \
        AutoReg_##suite_name##_##test_name() { \
            UnitTestRegistry::instance().register_test(#suite_name, #test_name, suite_name##_##test_name); \
        } \
    } global_reg_##suite_name##_##test_name; \
    void suite_name##_##test_name()

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            throw TestAssertionException("Assertion failed: " #cond, __FILE__, __LINE__); \
        } \
    } while (0)

#define ASSERT_EQ(val1, val2) \
    do { \
        if (!((val1) == (val2))) { \
            std::ostringstream oss; \
            oss << "Expected equality of (" #val1 ") and (" #val2 ") -> [" << (val1) << " != " << (val2) << "]"; \
            throw TestAssertionException(oss.str(), __FILE__, __LINE__); \
        } \
    } while (0)
