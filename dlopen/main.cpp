#include "dlopen.h"
#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <cctype>
#include <algorithm>

// Operator precedence
int precedence(char op) {
    if (op == '+' || op == '-') return 1;
    if (op == '*' || op == '/') return 2;
    return 0;
}

// Convert operator character to dlopen.h Operand enum
bool getOperandEnum(char op, Operand& out_op) {
    switch (op) {
        case '+': out_op = ADD; return true;
        case '-': out_op = SUBTRACT; return true;
        case '*': out_op = MULTIPLY; return true;
        case '/': out_op = DIVIDE; return true;
        default: return false;
    }
}

void applyTopOp(std::stack<double>& num_stack, std::stack<char>& op_stack) {
    // get right and left side numbers and operand
    double right = num_stack.top(); num_stack.pop();
    double left = num_stack.top(); num_stack.pop();
    char op_char = op_stack.top(); op_stack.pop();
    
    Operand op_enum;
    if (getOperandEnum(op_char, op_enum)) {
        double result = DLOPEN::calculate(left, right, op_enum);
        num_stack.push(result);    
    } else {
        std::cerr << "Unknown operator: " << op_char << '\n';
        num_stack.push(0);
    }
}

int main() {
    // define inputs
    std::string input;

    while (true) {  
        std::cout << "Enter your input: ";
        if (!std::getline(std::cin, input)) {
            break;
        }
        
        // erase spaces
        input.erase(' ');
        

        std::stack<double> num_stack;
        std::stack<char> op_stack;

        size_t i = 0;
        while (i < input.size()) {
            char c = input[i];


            // Check if the current character is an operand
            if ( c == '+' || c == '-' || c == '*' || c == '/') {
                while (!op_stack.empty() && precedence(op_stack.top() >= precedence(c))) {
                    applyTopOp(num_stack, op_stack);
                }
                op_stack.push(c);
                ++i;
            }

            // Check if the current character starts a number (digital or decimal) and parse
            else if (std::isdigit(c) || (c == '.' & i+1 < input.size() && std::isdigit(input[i+1]))) {
                std::string num_str = "";

                // Combine all consecutive digits and decimal points
                while ( i < input.size() && (std::isdigit(input[i]) || input[i] == '.')) {
                    num_str += input[i];
                    ++i;
                }
                num_stack.push(std::stod(num_str));
                
            }
            else {
                ++i;
            }

        }
        while (!op_stack.empty()) {
            applyTopOp(num_stack, op_stack);
        }

        if (!num_stack.empty()) {
            std::cout << "Result: " << num_stack.top() << "\n\n";
        }
    }
    return 0;
}