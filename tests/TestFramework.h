#pragma once

// Minimal test framework: TEST_CASE registers a function, CHECK records a
// failure without stopping the test, and test_main.cpp runs everything.

#include <exception>
#include <iostream>
#include <vector>

namespace testing {

struct TestCase {
    const char* name;
    void (*function)();
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Counters {
    int checks = 0;
    int failures = 0;
};

inline Counters& counters() {
    static Counters c;
    return c;
}

struct Registrar {
    Registrar(const char* name, void (*function)()) { registry().push_back({name, function}); }
};

}  // namespace testing

#define TEST_CASE(name)                                                   \
    static void name();                                                   \
    static const testing::Registrar name##_registrar(#name, name);        \
    static void name()

#define CHECK(condition)                                                              \
    do {                                                                              \
        ++testing::counters().checks;                                                 \
        if (!(condition)) {                                                           \
            ++testing::counters().failures;                                           \
            std::cerr << "  " << __FILE__ << ":" << __LINE__ << ": CHECK(" #condition \
                      << ") failed\n";                                                \
        }                                                                             \
    } while (false)

#define CHECK_THROWS(expression)                                                           \
    do {                                                                                   \
        ++testing::counters().checks;                                                      \
        bool threw_ = false;                                                               \
        try {                                                                              \
            (void)(expression);                                                            \
        } catch (const std::exception&) {                                                  \
            threw_ = true;                                                                 \
        }                                                                                  \
        if (!threw_) {                                                                     \
            ++testing::counters().failures;                                                \
            std::cerr << "  " << __FILE__ << ":" << __LINE__ << ": CHECK_THROWS(" #expression \
                      << ") did not throw\n";                                              \
        }                                                                                  \
    } while (false)
