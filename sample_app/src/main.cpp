#include <iostream>

int calculate_sum(int a, int b) {
    return a + b;
}

int main() {
    std::cout << "Hello from deployed application on ALT Linux!\n";
    std::cout << "2 + 3 = " << calculate_sum(2, 3) << "\n";
    return 0;
}
