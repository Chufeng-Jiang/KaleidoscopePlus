#include <iostream>
extern "C" {
extern double counter, offset, untouched;
double bump(double);
double localcounter(double);
double localvar();
double loopshadow();
double readcounter();
}
int main() {
    if (counter != 10 || offset != -2 || untouched != 0)
        return 1;
    if (bump(5) != 15 || counter != 15)
        return 2;
    if (localcounter(1) != 101 || counter != 15)
        return 3;
    if (localvar() != 98 || counter != 15)
        return 4;
    if (loopshadow() != 0 || counter != 15 || readcounter() != 15)
        return 5;
    std::cout << "counter: " << counter << "\nlocal: " << localvar() << '\n';
}
