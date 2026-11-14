#include <iostream>
extern "C" double debugdemo(double);
int main() {
    const auto result = debugdemo(3);
    std::cout << "debugdemo(3): " << result << '\n';
    return result == 11 ? 0 : 1;
}
