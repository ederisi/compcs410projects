#include <cmath>

// extern C blocks name problems, in order for dlsym to find functions
extern "C" {
    
    double sqrt(double a) {
        return std::sqrt(a);
    }

    double sin(double a) {
        return std::sin(a);
    }

    double cos(double a) {
        return std::cos(a);
    }

    double tan(double a) {
        return std::tan(a);
    }
}