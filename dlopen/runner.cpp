#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <dlfcn.h>

// Define function-pointer type, which corresponds to the libraries' functions
typedef double (*MathFunc)(double, double);

int main(int argc, char* argv[]) {

    // Ensure that a text file is given as the argument
    if (argc < 2) {
        std::cerr << "Use: ./dynamic_runner <instruction.txt>\n";
        return 1;
    }

    std::ifstream infile(argv[1]);
    if (!infile) {
        std::cerr << "Error: File '" << argv[1] << "' couldn't be opened. \n";
        return 1;
    }

    std::string line;
    // Read file line at a time
    while (std::getline(infile, line)) {
        if (line.empty() || line[0] == '#') continue; // skip empty and commented lines

        std::istringstream iss(line);
        std::string lib_name, func_name;
        double arg1, arg2;

        // Presumed state: ./library.so function name val1 val2
        if (!(iss >> lib_name >> func_name >> arg1 >> arg2)) {
            std::cerr << "Erroneous line format: " << line << "\n";
            continue;
        }
        
        // Load dynamic library
        void* handle = dlopen(lib_name.c_str(), RTLD_LAZY);
        if (!handle) {
            std::cerr << "Library load failed (" << lib_name << "): " << dlerror() << "\n";
            continue;
        }

        // Reset and search for func
        dlerror();
        MathFunc func = (MathFunc)dlsym(handle, func_name.c_str());
        const char* dlsym_error = dlerror();

        if (dlsym_error) {
            std::cerr << "Function '" << func_name << "' was not found: "<< dlsym_error << "\n";
            dlclose(handle);
            continue;
        }
        
        // Apply function
        double result = func(arg1, arg2);
        std::cout << "Applying: "<< lib_name << " -> " << func_name <<
         "(" << arg1 << ", " << arg2 << ") = " << result << "\n";

        // Close library
        dlclose(handle);
    }
    return 0;
}