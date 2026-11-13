#include "calculatorUtils.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

int main() {
    std::string input;
    // Configure output stream to print
    std::cout << std::setprecision(std::numeric_limits<double>::max_digits10);

    // Main interactive calc loop running until EOF(CTRL+D)
    while (true) {
        std::cout << "Enter your input: ";
        if (!std::getline(std::cin, input)) break;

        // Check if input contains '.txt' presume script was requested
        if (input.find(".txt") != std::string::npos) {
            std::istringstream iss(input);
            std::string token;
            std::string filepath;

            // Extract the token containing ".txt" as the target file path
            while (iss >> token) {
                if (token.find(".txt") != std::string::npos) {
                    filepath = token;
                    break;
                }
            }

            // Execute the script (if valid filepath)
            if (!filepath.empty()) DLOPEN::execute_script(filepath);
            continue;
        }

        // Remove whitespaces and try to run the given math expression
        input.erase(
            std::remove_if(input.begin(), input.end(),
                           [](unsigned char c) { return std::isspace(c) != 0; }),
            input.end()
        );
        // Skip empty lines/inputs consisting of whitespaces
        if (input.empty()) continue;

        double result;
        // Evaluate the cleaned mathematical expression
        if (CalculatorUtils::evaluate(input, result)) {
            std::cout << "Result: " << result << "\n\n";
        } else {
            std::cerr << "Invalid expression: " << input << "\n\n";
        }
    }

    return 0;
}