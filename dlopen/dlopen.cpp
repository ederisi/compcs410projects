#include "dlopen.h"
#include <dlfcn.h>
#include <iostream>
#include <cmath>

typedef double (*MathOpFunc)(double, double);

double DLOPEN::calculate(double a, double b, Operand operand) {
    // Open linked library from relative path
    void* handle = dlopen("./libmathlib.so", RTLD_LAZY);
    if (!handle) {
        std::cerr <<"Error running library: " << dlerror() << "\n";
        return NAN; 
    }

    // Choose the function (from binary)
    const char* func_name = nullptr;
    switch (operand) {
        case ADD: func_name = "add"; break;
        case SUBTRACT: func_name = "subtract"; break;
        case MULTIPLY: func_name = "multiply"; break;
        case DIVIDE: func_name = "divide"; break;
        default:
            dlclose(handle);
            return 0.0;
    }

    dlerror();
    MathOpFunc op_func = (MathOpFunc)dlsym(handle, func_name);

    char* error = dlerror();
    if (error != nullptr) {
        std::cerr << "Error when loading symbol '" << func_name << ": " << error << '\n';
        dlclose(handle);
        return NAN;
    }

    // Calculate from binary with given function
    double result = op_func(a, b);

    // close handle
    dlclose(handle);

    return result;
}


