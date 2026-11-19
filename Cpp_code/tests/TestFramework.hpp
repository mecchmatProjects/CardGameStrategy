#pragma once

#include <cmath>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace mini_test {

struct TestCase {
    std::string name;
    std::function<void()> body;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar {
    Registrar(const char* name, std::function<void()> body) {
        registry().push_back({name, std::move(body)});
    }
};

struct Failure : std::runtime_error {
    using std::runtime_error::runtime_error;
};

inline std::string where(const char* file, int line) {
    return std::string(file) + ":" + std::to_string(line) + ": ";
}

}

#define TEST_CASE(name)                                                   \
    static void name();                                                   \
    static const ::mini_test::Registrar name##_registrar(#name, &name);   \
    static void name()

#define CHECK(cond)                                                                        \
    do {                                                                                   \
        if (!(cond)) {                                                                     \
            throw ::mini_test::Failure(::mini_test::where(__FILE__, __LINE__) +            \
                                       "CHECK(" #cond ")");                                \
        }                                                                                  \
    } while (0)

#define CHECK_EQ(actual, expected)                                                         \
    do {                                                                                   \
        const auto check_a_ = (actual);                                                    \
        const auto check_e_ = (expected);                                                  \
        if (!(check_a_ == check_e_)) {                                                     \
            std::ostringstream check_os_;                                                  \
            check_os_ << ::mini_test::where(__FILE__, __LINE__) << "CHECK_EQ(" #actual     \
                      << ", " #expected "): " << check_a_ << " != " << check_e_;           \
            throw ::mini_test::Failure(check_os_.str());                                   \
        }                                                                                  \
    } while (0)

#define CHECK_NEAR(actual, expected, tolerance)                                            \
    do {                                                                                   \
        const double check_a_ = (actual);                                                  \
        const double check_e_ = (expected);                                                \
        if (std::fabs(check_a_ - check_e_) > (tolerance)) {                                \
            std::ostringstream check_os_;                                                  \
            check_os_ << ::mini_test::where(__FILE__, __LINE__) << "CHECK_NEAR(" #actual   \
                      << "): " << check_a_ << " vs " << check_e_ << " ± " << (tolerance);  \
            throw ::mini_test::Failure(check_os_.str());                                   \
        }                                                                                  \
    } while (0)

#define CHECK_THROWS(expr)                                                                 \
    do {                                                                                   \
        bool check_thrown_ = false;                                                        \
        try {                                                                              \
            (void)(expr);                                                                  \
        } catch (...) {                                                                    \
            check_thrown_ = true;                                                          \
        }                                                                                  \
        if (!check_thrown_) {                                                              \
            throw ::mini_test::Failure(::mini_test::where(__FILE__, __LINE__) +            \
                                       "очікувався виняток: " #expr);                      \
        }                                                                                  \
    } while (0)
