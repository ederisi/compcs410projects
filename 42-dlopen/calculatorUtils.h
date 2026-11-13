#pragma once

#include "dlopen.h"

#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <stack>
#include <string>

namespace CalculatorUtils
{
    // Define operator precedence (bigger value is calculated first)
    static int precedence(char op) {
        if (op == '+' || op == '-') return 1;
        if (op == '*' || op == '/') return 2;
        if (op == '^') return 3;
        return 0;
    }

    // Convert operator character to dlopen.h Operand enum
    // true if character is a supported operator
    static bool getOperandEnum(char op, Operand& out_op) {
        switch (op) {
            case '+': out_op = ADD; return true;
            case '-': out_op = SUBTRACT; return true;
            case '*': out_op = MULTIPLY; return true;
            case '/': out_op = DIVIDE; return true;
            case '^': out_op = POWER; return true;
            default: return false;
        }
    }

    // Check whether given character is a digit
    static bool isDigit(char c) {
        return std::isdigit(static_cast<unsigned char>(c)) != 0;
    }

    // Check whether given character is a supported operator
    static bool isOperator(char c) {
        return c == '+' || c == '-' || c == '*' || c == '/' || c == '^';
    }

    // Read a decimal number at input[pos]
    // (Supports optional signs (e.g. 1e-3, -2.5E+4)
    static bool parseNumber(const std::string& input, std::size_t& pos,
                    double& value, bool allow_sign) {
        const std::size_t start = pos;
        
        // Handle optional leading
        if (allow_sign && pos < input.size() &&
            (input[pos] == '+' || input[pos] == '-')) {
            ++pos;
        }

        bool has_digit = false;

        // Parse integer part of number
        while (pos < input.size() && isDigit(input[pos])) {
            has_digit = true;
            ++pos;
        }

        // Parse decimal
        if (pos < input.size() && input[pos] == '.') {
            ++pos;
            while (pos < input.size() && isDigit(input[pos])) {
                has_digit = true;
                ++pos;
            }
        }
        // Fails if no digits present
        if (!has_digit) {
            pos = start;
            return false;
        }

        // Optional scientific notation: 1e-3, -2.5E+4
        if (pos < input.size() && (input[pos] == 'e' || input[pos] == 'E')) {
            std::size_t exponent_pos = pos + 1;

            if (exponent_pos < input.size() &&
                (input[exponent_pos] == '+' || input[exponent_pos] == '-')) {
                ++exponent_pos;
            }

            const std::size_t exponent_digits = exponent_pos;
            while (exponent_pos < input.size() && isDigit(input[exponent_pos])) {
                ++exponent_pos;
            }
            
            // Fail if 'e' or 'E' is not followed by exponent digits
            if (exponent_pos == exponent_digits) {
                pos = start;
                return false;
            }

            pos = exponent_pos;
        }

        // Convert token substring to a double
        const std::string token = input.substr(start, pos - start);
        char* end = nullptr;
        errno = 0;
        value = std::strtod(token.c_str(), &end);

        //Check for conversion errors/range overflows
        if (end != token.c_str() + token.size() || errno == ERANGE) {
            pos = start;
            return false;
        }

        return true;
    }

    // Apply top operator from operator stack to two operands
    // Pushes computed result back to the top of the stack
    static bool applyTopOp(std::stack<double>& num_stack, std::stack<char>& op_stack) {
        if (op_stack.empty() || num_stack.size() < 2) {
            return false;
        }

        Operand op_enum;
        const char op_char = op_stack.top();

        if (!getOperandEnum(op_char, op_enum)) {
            return false;
        }

        op_stack.pop();

        // Pop right-hand operand first
        const double right = num_stack.top();
        num_stack.pop();

        // Pop left-hand operand second
        const double left = num_stack.top();
        num_stack.pop();

        // Calculate using dynamically loaded library (DLOPEN)
        num_stack.push(DLOPEN::calculate(left, right, op_enum));
        return true;
    }

    // Only the existing unary-function parentheses are supported
    static bool parseUnaryFunction(const std::string& input, std::size_t& pos,
                            double& result) {
        Operand operand;
        std::size_t name_length;
        
        // Match supported unary function names
        if (input.compare(pos, 4, "sqrt") == 0) {
            operand = SQRT;
            name_length = 4;
        } else if (input.compare(pos, 3, "sin") == 0) {
            operand = SIN;
            name_length = 3;
        } else if (input.compare(pos, 3, "cos") == 0) {
            operand = COS;
            name_length = 3;
        } else if (input.compare(pos, 3, "tan") == 0) {
            operand = TAN;
            name_length = 3;
        } else {
            return false;
        }

        pos += name_length;

        // Expect an opening in parenthesis '(' following func name
        if (pos >= input.size() || input[pos] != '(') {
            return false;
        }
        ++pos;

        // Parse inner argument of func
        double argument;
        if (!parseNumber(input, pos, argument, true)) {
            return false;
        }

        // Expect a closing parenthesis ')'
        if (pos >= input.size() || input[pos] != ')') {
            return false;
        }
        ++pos;

        // Perform single-operand calculation(via DLOPEN)
        result = DLOPEN::calculate(argument, operand);
        return true;
    }

    // Main evaluation function to parse and compute mathematical expression
    inline bool evaluate(const std::string& input, double& result) {
        std::stack<double> num_stack;
        std::stack<char> op_stack;

        std::size_t pos = 0;
        bool expect_operand = true; // Tracks if the next token should be op or dig

        while (pos < input.size()) {
            const char c = input[pos];

            // Operand up next
            if (expect_operand) {
                double value;
                bool negative = false; 

                // Consume negative sign if present
                if (c == '-') {
                    negative = true; 
                    ++pos;

                    if (pos >= input.size()) {
                        return false;
                    }
                }

                const char next = input[pos];

                // Parse either a numeric literal or unary func call
                if (isDigit(next) || next == '.') {
                    if (!parseNumber(input, pos, value, false)) {
                        return false;
                    }
                } else if (std::isalpha(static_cast<unsigned char>(next))) {
                    if (!parseUnaryFunction(input, pos, value)) {
                        return false;
                    }
                } else {
                    return false;
                }
                // Push parsed number onto the stack
                num_stack.push(negative ? -value : value);
                expect_operand = false;
                continue;
            }

            // operator is expected
            if (!isOperator(c)) {
                return false;
            }

            // Keep the original left-associative handling of equal precedence.
            while (!op_stack.empty() &&
                precedence(op_stack.top()) >= precedence(c)) {
                if (!applyTopOp(num_stack, op_stack)) {
                    return false;
                }
            }

            op_stack.push(c);
            ++pos;
            expect_operand = true;
        }

        if (expect_operand) {
            return false; // Empty input or trailing operator.
        }

        // Empty the remaining operators from stack
        while (!op_stack.empty()) {
            if (!applyTopOp(num_stack, op_stack)) {
                return false;
            }
        }

        // Exactly one final result from stack
        if (num_stack.size() != 1) {
            return false;
        }

        result = num_stack.top();
        return true;
    }
}