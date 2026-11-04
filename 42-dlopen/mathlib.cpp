#include <cmath>

extern "C" {

    double mathlib_add(double a, double b) {
        return a+ b;
    }

    double mathlib_subtract(double a, double b) {
        return a - b;
    }

    double mathlib_multiply(double a, double b) {
        return a * b;
    }

    double mathlib_divide(double a, double b) {
        if (b == 0) return NAN;
        return a / b;
    }

    double mathlib_power(double a, double b) {
        return std::pow(a, b); 
    }
}