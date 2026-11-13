#include "processhelpers.h"

#include <csignal>
#include <cstring>
#include <iostream>
#include <sstream>
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
    {"wsserver", {"wsserver", false}}
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

    // If arguments were given, run the requested processes first
    if (argc > 1) {
        if (std::strcmp(argv[1], "run") != 0 || argc < 3) {
            std::cerr << "Usage: " << argv[0]
                      << " [run --all | run <name> [name...]]\n";
            return 1;
        }

        std::vector<std::pair<std::string, const Process*>> requested;
        if (argc == 3 && std::strcmp(argv[2], "--all") == 0) {
            // Gather all processes from registry
            for (const auto& [name, def] : PROCESS_REGISTRY) {
                requested.emplace_back(name, &def);
            }
        }
        else {
            // Try getting requested processes from registry by names
            for (int i = 2; i < argc; ++i) {
                std::string name = argv[i];

                const Process* process = find_process(name);
                if (!process) {
                    std::cerr << "Unknown program: " << name << '\n';
                    return 1;
                }

                requested.emplace_back(name, process);
            }
        }

        // Start background services before running any CLI.
        for (const auto& [name, process] : requested) {
            if (process->cli)
                continue; // Skip CLIs in this phase

            ProcessHelpers::reap_services(running_services);

            if (running_services.find(name) != running_services.end()) {
                std::cout << name << " is already running\n";
                continue;
            }

            pid_t pid = ProcessHelpers::run_process(
                name, *process, launcher_group);

            if (pid < 0) {
                std::cerr << "Failed running service: " << name << '\n';
                exit_requested = 1;
                break;
            }

            running_services.emplace(name, pid);
        }

        // Run CLIs sequentially. 
        if (!exit_requested) {
            for (const auto& [name, process] : requested) {
                if (!process->cli)
                    continue;

                if (ProcessHelpers::run_process(name, *process, launcher_group) < 0) {
                    std::cerr << "Failure running CLI: " << name << '\n';
                    exit_requested = 1;
                    break;
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
            std::vector<std::pair<std::string, const Process*>> requested;
            std::string name;
            bool run_failed = false; 

            while (input >> name) {
                if (name == "--all") {
                    if (!requested.empty() || (input >> name)) {
                        std::cout << "Use --all alone\n";
                        requested.clear();
                        break;
                    }

                    for (const auto& [registered_name, definition] : PROCESS_REGISTRY) {
                        requested.emplace_back(registered_name, &definition);
                    }
                    break;
                }

                const auto* process = find_process(name);
                if (!process) {
                    std::cout << "Unknown program: " << name << '\n';
                    requested.clear();
                    break;
                }

                requested.emplace_back(name, process);
            }

            if (!requested.empty()) {
                // Run all services
                for (const auto& [requested_name, process] : requested) {
                    if (process->cli)
                        continue;

                    ProcessHelpers::reap_services(running_services);

                    if (running_services.find(requested_name) != running_services.end()) {
                        std::cout << requested_name << " is already running\n";
                        continue;
                    }

                    pid_t pid = ProcessHelpers::run_process(
                        requested_name, *process, launcher_group);

                    if (pid < 0) {
                        std::cerr << "Failed running service: "
                                << requested_name << '\n';
                        run_failed = true;
                        break;
                    }

                    running_services.emplace(requested_name, pid);
                }

                if (run_failed) continue;

                // Run all cli's (waits on each cli)
                for (const auto& [requested_name, process] : requested) {
                    if (!process->cli)
                        continue;

                    if (ProcessHelpers::run_process(requested_name, *process, launcher_group) < 0) {
                        std::cerr << "Failure running CLI: "
                                << requested_name << '\n';
                        break; 
                    }
                }
            }
        } else {
            std::cout << "Commands: run <names>, run --all, "
                         "status, kill <service>, quit\n";
        }
    }

    ProcessHelpers::stop_services(running_services);
    std::cout << "Launcher exited\n";

    return 0;
}