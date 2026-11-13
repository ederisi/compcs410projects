#include <cmath>

// Use extern "C" to enforce C linkage, prevents C++ name mangling.
// This ensures function names remain unaltered in compiled
// shared object, allowing successfull location and load at runtime via dlsym()
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