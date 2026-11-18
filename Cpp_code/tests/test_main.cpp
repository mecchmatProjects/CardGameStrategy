#include "TestFramework.hpp"

#include <exception>
#include <iostream>

int main() {
    int passed = 0;
    int failed = 0;
    for (const auto& test : mini_test::registry()) {
        try {
            test.body();
            ++passed;
            std::cout << "[  OK  ] " << test.name << '\n';
        } catch (const std::exception& e) {
            ++failed;
            std::cout << "[ FAIL ] " << test.name << "\n         " << e.what() << '\n';
        }
    }
    std::cout << "\nПройдено: " << passed << ", провалено: " << failed << '\n';
    return failed == 0 ? 0 : 1;
}
