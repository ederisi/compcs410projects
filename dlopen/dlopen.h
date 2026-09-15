#ifndef DLOPEN_H
#define DLOPEN_H

enum Operand { ADD, SUBTRACT, MULTIPLY, DIVIDE }; 

namespace DLOPEN {

    // Main calculation call
    double calculate(double a, double b, Operand operand); 

}


#endif