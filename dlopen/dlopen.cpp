#include "dlopen.h"
#include <dlfcn.h>
#include <iostream>
#include <sstream>
#include <cmath>
#include <fstream>

// Function pointer signatures for dynamic loading
typedef double (*BinaryMathFunc)(double, double);
typedef double (*UnaryMathFunc)(double);

static constexpr std::string_view BINARY_MATH_LIB = "./libmathlib.so";
static constexpr std::string_view UNARY_MATH_LIB = "./libmathlib2.so";

// Binary operations
double DLOPEN::calculate(double a, double b, Operand operand) {
    // Choose the function (from binary) and resolve library name by operand
    std::string func_name;
    switch (operand) {
    case ADD: 
        func_name = "add";
        break;
    case SUBTRACT: 
        func_name = "subtract";
        break;
    case MULTIPLY: 
        func_name = "multiply";
        break;
    case DIVIDE: 
        func_name = "divide";
        break;
    case POWER:
        func_name = "power";
        break; 
    default:
        return NAN;
    }

    // Open linked library from relative path
    void* handle = dlopen(std::string(BINARY_MATH_LIB).c_str(), RTLD_LAZY);
    if (!handle) {
        std::cerr <<"Error running library: " << dlerror() << "\n";
        return NAN; 
    }

    dlerror();
    BinaryMathFunc op_func = reinterpret_cast<BinaryMathFunc>(dlsym(handle, func_name.c_str()));

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

// Unary operations
double DLOPEN::calculate(double a, Operand operand) {
    std::string func_name;
    switch (operand) {
    case SQRT:
        func_name = "sqrt";
        break;
    case SIN:
        func_name = "sin";
        break;
    case COS:
        func_name = "cos";
        break;
    case TAN:
        func_name = "tan";
        break;
    default:
        return NAN;
    }

    void* handle = dlopen(std::string(UNARY_MATH_LIB).c_str(), RTLD_LAZY);
    if (!handle) {
        std::cerr << "Error running library: " << dlerror() << "\n";
        return NAN; 
    }

    dlerror(); 
    UnaryMathFunc op_func = reinterpret_cast<UnaryMathFunc>(dlsym(handle, func_name.c_str()));

    char* error = dlerror();
    if (error != nullptr) {
        std::cerr << "Error when loading symbol '" << func_name << "': " << error << '\n';
        dlclose(handle);
        return NAN;
    }

    double result = op_func(a);
    dlclose(handle);

    return result;
}

bool DLOPEN::execute_script(const std::string& filepath) {
    std::ifstream infile(filepath);
    if (!infile) {
        std::cerr << "Error: File '" << filepath << "' couldn't be opened. \n";
        return false;
    }

    std::string line;
    // Read file line at a time
    while (std::getline(infile, line)) {
        if (line.empty() || line[0] == '#') continue; // skip empty and commented lines

        std::istringstream iss(line);
        std::string lib_name, func_name;
        double arg1;

        // Every instruction needs at least: <library_path> <func_name> <arg1>
        if (!(iss >> lib_name >> func_name >> arg1)) {
            std::cerr << "Erroneous line format: " << line << "\n";
            continue;
        }

        // Identify if this is a unary function call from libmathlib2.so
        bool is_unary = (lib_name.find("libmathlib2.so") != std::string::npos);

        double arg2;
        if (!is_unary) {
            // For binary libraries, reading arg2 is mandatory
            if (!(iss >> arg2)) {
                std::cerr << "Missing second argument for binary function: " << line << "\n";
                continue;
            }
        }
        
        // Load dynamic library
        void* handle = dlopen(lib_name.c_str(), RTLD_LAZY);
        if (!handle) {
            std::cerr << "Library load failed (" << lib_name << "): " << dlerror() << "\n";
            continue;
        }

        // Reset and search for func
        dlerror();
        if (is_unary) {
            UnaryMathFunc func = reinterpret_cast<UnaryMathFunc>(dlsym(handle, func_name.c_str()));
            const char* dlsym_error = dlerror();
            if (dlsym_error) {
                std::cerr << "Function '" << func_name << "' was not found: " << dlsym_error << "\n";
                dlclose(handle);
                continue;
            }

            double result = func(arg1);
            std::cout << "Applying: " << lib_name << " -> " << func_name
                      << "(" << arg1 << ") = " << result << "\n";
        } else {
            BinaryMathFunc func = reinterpret_cast<BinaryMathFunc>(dlsym(handle, func_name.c_str()));
            const char* dlsym_error = dlerror();
            if (dlsym_error) {
                std::cerr << "Function '" << func_name << "' was not found: " << dlsym_error << "\n";
                dlclose(handle);
                continue;
            }

            double result = func(arg1, arg2);
            std::cout << "Applying: " << lib_name << " -> " << func_name
                      << "(" << arg1 << ", " << arg2 << ") = " << result << "\n";
        }

        // Close library
        dlclose(handle);
    }
    return true;
}
