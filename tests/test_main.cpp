#include "TestFramework.h"

int main() {
    int failedTests = 0;
    for (const auto& test : testing::registry()) {
        int failuresBefore = testing::counters().failures;
        try {
            test.function();
        } catch (const std::exception& e) {
            // Count the escaped exception as a failed check so the totals stay consistent.
            ++testing::counters().checks;
            ++testing::counters().failures;
            std::cerr << "  unexpected exception: " << e.what() << '\n';
        }
        bool passed = testing::counters().failures == failuresBefore;
        if (!passed) {
            ++failedTests;
        }
        std::cout << (passed ? "[PASS] " : "[FAIL] ") << test.name << '\n';
    }

    const auto& c = testing::counters();
    std::cout << '\n'
              << testing::registry().size() - failedTests << "/" << testing::registry().size()
              << " tests passed, " << c.checks - c.failures << "/" << c.checks
              << " checks passed\n";
    return failedTests == 0 ? 0 : 1;
}
