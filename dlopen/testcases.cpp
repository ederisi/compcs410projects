#include <cmath>

// extern C blocks name problems, in order for dlsym to find functions
extern "C" {
    
    double power(double base, double exp) {
        return std::pow(base, exp);
    }

    double hypotenuse(double a, double b) {
        return std::sqrt((a * a) + (b * b));
    }
}