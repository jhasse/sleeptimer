#include "timer.h"
#include <iostream>

int main() {
    int failures = 0;
    const auto check = [&](bool result, const char* description) {
        if (!result) {
            std::cerr << description << '\n';
            ++failures;
        }
    };
    check(timer::duration(1, 0, 0) == 3600, "one hour");
    check(timer::duration(0, 0, 90) == 90, "seconds carry into minutes");
    check(timer::duration(1, -1, 0) == 3540, "minutes borrow from hours");
    check(timer::duration(0, 1, -1) == 59, "seconds borrow from minutes");
    check(timer::duration(0, 0, -1) == 0, "lower bound");
    check(timer::duration(99, 60, 0) == timer::maxSeconds, "upper bound");
    check(timer::secondsLeft(5000, 4000) == 1, "one second remaining");
    check(timer::secondsLeft(5000, 4999) == 1, "round up before deadline");
    check(timer::secondsLeft(5000, 5000) == 0, "expire at deadline");
    check(timer::secondsLeft(5000, 9000) == 0, "delayed tick expires");
    check(timer::secondsLeft(10000000000ULL, 9999990000ULL) == 10, "64-bit tick count");
    check(timer::progress(30, 60) == 5000, "half remaining");
    check(timer::progress(0, 0) == 0, "zero duration progress");
    check(timer::progress(61, 60) == 10000, "bounded progress");
    return failures == 0 ? 0 : 1;
}
