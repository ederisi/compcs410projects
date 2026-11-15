#include "processhelpers.h"

#include <csignal>
#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <vector>
#include <csignal>

// Registry mapping service and CLI program names to their process defs
static const std::vector<std::pair<std::string, Process>> PROCESS_REGISTRY = {
    {"dlopen", {"dlopen", true}},
    {"wsserver", {"wsserver", false}},
    {"encryption", {"encryption", true}}
};

// Look up a process definition in the registry by name
const Process* find_process(const std::string& name) {
    for (const auto& entry : PROCESS_REGISTRY) {
        if (entry.first == name)
            return &entry.second;
    }
    return nullptr;
}

// Volatile flag indicating that an exit (SIGINT) has been requested
volatile std::sig_atomic_t exit_requested = 0; 
extern "C" void sigint_handler(int) {
    exit_requested = 1;
}

int main(int argc, char* argv[]) {
    // Install SIGING handler
    struct sigaction action {};
    action.sa_handler = sigint_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0; 
    if (sigaction(SIGINT, &action, nullptr) == -1) {
        std::perror("sigaction");
        return 1;
    }

    // Ignore SIGTTOU to prevent background processes from stopping
    // when writing to terminal
    if (std::signal(SIGTTOU, SIG_IGN) == SIG_ERR) {
        std::perror("signal");
        return 1;
    }

    std::unordered_map<std::string, pid_t> running_services; // Track running services
    pid_t launcher_group = getpgrp();

    // Run one requested program from launcher arguments.
    if (argc > 1) {
        if (std::strcmp(argv[1], "run") != 0 || argc < 3) {
            std::cerr << "Usage: " << argv[0] << " run <program> [arguments...]\n";
            return 1;
        }

        std::string name = argv[2];
        const Process* process = find_process(name);
        if (!process) {
            std::cerr << "Unknown program: " << name << '\n';
            return 1;
        }

        std::vector<std::string> arguments;
        for (int i = 3; i < argc; ++i)
            arguments.emplace_back(argv[i]);

        if (process->cli) {
            if (ProcessHelpers::run_process(name, *process, launcher_group, arguments) < 0) {
                std::cerr << "Failure running CLI: " << name << '\n';
                exit_requested = 1;
            }
        } else {
            ProcessHelpers::reap_services(running_services);
            if (running_services.find(name) != running_services.end()) {
                std::cout << name << " is already running\n";
            } else {
                pid_t pid = ProcessHelpers::run_process(name, *process, launcher_group, arguments);
                if (pid < 0) {
                    std::cerr << "Failed running service: " << name << '\n';
                    exit_requested = 1;
                } else {
                    running_services.emplace(name, pid);
                }
            }
        }
    }

    // Run main launcher cli
    std::string line;
    while (!exit_requested) {
        ProcessHelpers::reap_services(running_services);

        if (exit_requested)
            break;

        std::cout << "launcher> " << std::flush;

        if (!std::getline(std::cin, line))
            break;

        if (exit_requested)
            break;

        std::istringstream input(line);
        std::string command;
        input >> command;

        if (exit_requested)
            break; 

        if (command.empty())
            continue;

        // Handle 'quit' command to exit launcher
        if (command == "quit") {
            break;
        
        // Handle 'status' command to display the 
        // running state of background services
        } else if (command == "status") {
            ProcessHelpers::reap_services(running_services);

            for (const auto& [name, process] : PROCESS_REGISTRY) {
                if (process.cli)
                    continue;

                auto it = running_services.find(name);
                std::cout << name << ": ";
                if (it == running_services.end())
                    std::cout << "stopped\n";
                else
                    std::cout << "running (PID " << it->second << ")\n";
            }
        
        // Handle 'kill' command to terminate a specific running background service
        } else if (command == "kill") {
            std::string name, extra;
            input >> name;

            if (name.empty() || (input >> extra)) {
                std::cerr << "Usage: kill <service>\n";
                continue;
            }

            const auto* process = find_process(name);
            if (!process || process->cli) {
                std::cout << "Unknown service: " << name << '\n';
                continue;
            }

            ProcessHelpers::reap_services(running_services);
            auto it = running_services.find(name);

            if (it == running_services.end()) {
                std::cout << "Process '" << name << "' is not running.\n";
                continue; 
            }

            ProcessHelpers::stop_service(it->second);
            running_services.erase(it);

        // Handle 'run' command to start services/interactive CLIs dynamically
        } else if (command == "run") {
            std::string name;
            if (!(input >> std::quoted(name))) {
                std::cerr << "Usage: run <program> [arguments...]\n";
                continue;
            }

            const Process* process = find_process(name);
            if (!process) {
                std::cout << "Unknown program: " << name << '\n';
                continue;
            }

            std::vector<std::string> arguments;
            std::string argument;
            while (input >> std::quoted(argument))
                arguments.push_back(argument);

            if (process->cli) {
                if (ProcessHelpers::run_process(name, *process, launcher_group, arguments) < 0)
                    std::cerr << "Failure running CLI: " << name << '\n';
            } else {
                ProcessHelpers::reap_services(running_services);
                if (running_services.find(name) != running_services.end()) {
                    std::cout << name << " is already running\n";
                    continue;
                }

                pid_t pid = ProcessHelpers::run_process(name, *process, launcher_group, arguments);
                if (pid < 0) {
                    std::cerr << "Failed running service: " << name << '\n';
                } else {
                    running_services.emplace(name, pid);
                }
            }
        } else {
            std::cout << "Commands: run <program> [arguments...], "
                         "status, kill <service>, quit\n";
        }
    }

    ProcessHelpers::stop_services(running_services);
    std::cout << "Launcher exited\n";

    return 0;
}