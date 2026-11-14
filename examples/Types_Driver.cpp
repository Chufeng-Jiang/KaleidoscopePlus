#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
extern "C" {
extern std::int32_t counter;
extern bool enabled;
std::int32_t increment(std::int32_t);
bool less(std::int32_t, std::int32_t);
std::int32_t choose(bool, std::int32_t, std::int32_t);
std::int32_t fibint(std::int32_t);
std::int32_t toint(double);
bool truth(double);
double asdouble(std::int32_t);
bool toggled(bool);
std::int32_t localtyped(std::int32_t, bool);
std::int32_t looptotal(std::int32_t);
}
int main() {
    assert(counter == -2 && enabled);
    assert(increment(5) == 3 && counter == 3);
    assert(less(-1, 2) && !less(2, -1));
    assert(choose(true, -7, 9) == -7 && choose(false, -7, 9) == 9);
    assert(fibint(7) == 13);
    assert(toint(3.9) == 3 && toint(-3.9) == -3);
    assert(toint(std::numeric_limits<double>::infinity()) == INT32_MAX);
    assert(toint(-std::numeric_limits<double>::infinity()) == INT32_MIN);
    assert(toint(std::numeric_limits<double>::quiet_NaN()) == 0);
    assert(!truth(std::numeric_limits<double>::quiet_NaN()));
    assert(!truth(-0.0) && truth(-1.0));
    assert(asdouble(INT32_MIN) == -2147483648.0);
    assert(!toggled(true) && toggled(false));
    assert(localtyped(4, true) == 5 && localtyped(4, false) == 0);
    assert(looptotal(4) == 10);
    enabled = false;
    counter = INT32_MAX;
    assert(increment(1) == INT32_MIN && !enabled);
    std::cout << "Typed scalars: passed\n";
}
