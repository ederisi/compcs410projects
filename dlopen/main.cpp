#include "dlopen.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <stack>
#include <cctype>
#include <algorithm>

// Operator precedence
int precedence(char op) {
    if (op == '+' || op == '-') return 1;
    if (op == '*' || op == '/') return 2;
    if (op == '^' || op == 'r') return 3;
    return 0;
}

// Convert operator character to dlopen.h Operand enum
bool getOperandEnum(char op, Operand& out_op) {
    switch (op) {
        case '+': out_op = ADD; return true;
        case '-': out_op = SUBTRACT; return true;
        case '*': out_op = MULTIPLY; return true;
        case '/': out_op = DIVIDE; return true;
        case '^': out_op = POWER; return true;
        default: return false;
    }
}

void applyTopOp(std::stack<double>& num_stack, std::stack<char>& op_stack) {
    if (op_stack.empty() || num_stack.empty()) return;

    char op_char = op_stack.top(); op_stack.pop();
    
    Operand op_enum;
    if (!getOperandEnum(op_char, op_enum)) {
        std::cerr << "Unknown operator: " << op_char << '\n';
        num_stack.push(0);
        return; 
    } 

    if (num_stack.size() < 2) return;
    
    double right = num_stack.top(); num_stack.pop();
    double left = num_stack.top(); num_stack.pop();
    
    double result = DLOPEN::calculate(left, right, op_enum);
    num_stack.push(result);
}

int main() {
    // define inputs
    std::string input;

    while (true) {  
        std::cout << "Enter your input: ";
        if (!std::getline(std::cin, input)) {
            break;
        }
        
        // Check if the input contains '.txt' -> run dlrunner
        if (input.find(".txt") != std::string::npos) {
            std::istringstream iss(input);
            std::string token;
            std::string filepath = "";

            // Supports entering both "instructions.txt" and "run instructions.txt"
            while (iss >> token) {
                if (token.find(".txt") != std::string::npos) {
                    filepath = token;
                    break;
                }
            }

            if (!filepath.empty()) {
                DLOPEN::execute_script(filepath);
            }
            continue; 
        }

        // erase spaces
        input.erase(std::remove(input.begin(), input.end(), ' '), input.end());

        std::stack<double> num_stack;
        std::stack<char> op_stack;

        size_t i = 0;
        while (i < input.size()) {
            char c = input[i];

            // Check if the current character is an operand
            if ( c == '+' || c == '-' || c == '*' || c == '/' || c == '^') {
                while (!op_stack.empty() && precedence(op_stack.top()) >= precedence(c)) {
                    applyTopOp(num_stack, op_stack);
                }
                op_stack.push(c);
                ++i;
            }
            // Unary functions special case (sqrt, sin, cos, tan)
            else if (input.compare(i, 4, "sqrt") == 0 || 
                     input.compare(i, 3, "sin") == 0 || 
                     input.compare(i, 3, "cos") == 0 || 
                     input.compare(i, 3, "tan") == 0) {
                
                std::string func_name;
                if (input.compare(i, 4, "sqrt") == 0) {
                    func_name = "sqrt";
                    i += 4;
                } else {
                    func_name = input.substr(i, 3); // "sin", "cos", or "tan"
                    i += 3;
                }

                bool has_paren = (i < input.size() && input[i] == '(');
                if (has_paren) ++i;

                std::string num_str = "";
                while (i < input.size() && (std::isdigit(input[i]) || input[i] == '.')) {
                    num_str += input[i];
                    ++i;
                }

                if (has_paren && i < input.size() && input[i] == ')') {
                    ++i;
                }

                if (!num_str.empty()) {
                    double val = std::stod(num_str);
                    if (func_name == "sqrt") num_stack.push(DLOPEN::calculate(val, SQRT));
                    else if (func_name == "sin") num_stack.push(DLOPEN::calculate(val, SIN));
                    else if (func_name == "cos") num_stack.push(DLOPEN::calculate(val, COS));
                    else if (func_name == "tan") num_stack.push(DLOPEN::calculate(val, TAN));
                }
            }

            // Check if the current character starts a number (digital or decimal) and parse
            else if (std::isdigit(c) || (c == '.' && i+1 < input.size() && std::isdigit(input[i+1]))) {
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