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
    std::cout << std::setprecision(std::numeric_limits<double>::max_digits10);

    while (true) {
        std::cout << "Enter your input: ";
        if (!std::getline(std::cin, input)) break;

        // If input contains '.txt' presume script was requested
        if (input.find(".txt") != std::string::npos) {
            std::istringstream iss(input);
            std::string token;
            std::string filepath;

            while (iss >> token) {
                if (token.find(".txt") != std::string::npos) {
                    filepath = token;
                    break;
                }
            }

            if (!filepath.empty()) DLOPEN::execute_script(filepath);
            continue;
        }

        // Remove whitespaces and try to run the given math expression
        input.erase(
            std::remove_if(input.begin(), input.end(),
                           [](unsigned char c) { return std::isspace(c) != 0; }),
            input.end()
        );
        if (input.empty()) continue;

        double result;
        if (CalculatorUtils::evaluate(input, result)) {
            std::cout << "Result: " << result << "\n\n";
        } else {
            std::cerr << "Invalid expression: " << input << "\n\n";
        }
    }

    return 0;
}