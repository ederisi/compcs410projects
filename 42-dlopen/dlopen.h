#ifndef DLOPEN_H
#define DLOPEN_H

#include <string>

// Enum represents all supported mathematical operations
// Binary operations (require two numbers a and b)
// Unary operations (require single number a)
enum Operand { ADD, SUBTRACT, MULTIPLY, DIVIDE, POWER, //Binary
    SQRT, SIN, COS, TAN}; // Unary

// Namespace housing functions responsible for dynamic library loading
// and runtime exectuion of math calculations
namespace DLOPEN {
    // Main calculation call
    double calculate(double a, double b, Operand operand); 
    double calculate(double a, Operand operand); 

    // Executes calculations from script in txt-file
    bool execute_script(const std::string& filepath); 
}


#endif