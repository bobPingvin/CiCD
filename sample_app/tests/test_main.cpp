#include "test_framework.hpp"
#include <thread>
#include <chrono>

int calculate_sum(int a, int b) {
    return a + b;
}

TEST(MathSuite, TestAddition) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    ASSERT_EQ(calculate_sum(2, 2), 4);
    ASSERT_EQ(calculate_sum(-1, 1), 0);
}

TEST(MathSuite, TestNegative) {
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    ASSERT_EQ(calculate_sum(-10, -5), -15);
}

TEST(LogicSuite, TestBooleans) {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    ASSERT_TRUE(5 > 2);
    ASSERT_TRUE(!false);
}

int main() {
    return UnitTestRegistry::instance().run_all();
}
