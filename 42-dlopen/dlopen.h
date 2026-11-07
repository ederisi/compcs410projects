#ifndef DLOPEN_H
#define DLOPEN_H

#include <string>

enum Operand { ADD, SUBTRACT, MULTIPLY, DIVIDE, POWER, SQRT, SIN, COS, TAN}; 

namespace DLOPEN {
    // Main calculation call
    double calculate(double a, double b, Operand operand); 
    double calculate(double a, Operand operand); 

    // Executes calculations from script in txt-file
    bool execute_script(const std::string& filepath); 
}


#endif