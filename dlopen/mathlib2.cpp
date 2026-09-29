#include <cmath>

extern "C" {
    
    double mathlib_sqrt(double a) {
        return std::sqrt(a);
    }

    double mathlib_sin(double a) {
        return std::sin(a);
    }

    double mathlib_cos(double a) {
        return std::cos(a);
    }

    double mathlib_tan(double a) {
        return std::tan(a);
    }
}