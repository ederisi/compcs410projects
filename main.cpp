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

static const std::vector<std::pair<std::string, Process>> PROCESS_REGISTRY = {
    {"dlopen", {"./bin/dlopen_runner", true}}
};

std::unordered_map<std::string, pid_t> running_services; 

const Process* find_process(const std::string& name) {
    for (const auto& entry : PROCESS_REGISTRY) {
        if (entry.first == name)
            return &entry.second;
    }
    return nullptr;
}

int main(int argc, char* argv[]) {
    // Setup launcher process group
    if (std::signal(SIGTTOU, SIG_IGN) == SIG_ERR) {
        std::perror("signal");
        return 1;
    }
    pid_t launcher_group = getpgrp();

    // If commands are given, try launching the given processes
    
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

                if (const auto* process = find_process(name)) {
                    requested.emplace_back(name, process);
                }
                else {
                    std::cerr << "Unknown program: " << name << '\n';
                    return 1; 
                }
            }
        }

        // Run all services
        auto it = requested.begin();
        while (it != requested.end()) {
            if (!it->second->cli) {
                auto name = it->first;
                pid_t pid = ProcessHelpers::run_process(name, *it->second, launcher_group);
                if (pid < 0) {
                    std::cerr << "Failed running service: " << name << '\n';
                    return 1; 
                }
                running_services.emplace(name, pid);
                requested.erase(it);
            }
            ++it;
        }

        // Run all cli's (waits on each cli)
        it = requested.begin();
        while (it != requested.end()) {
            if (ProcessHelpers::run_process(it->first, *it->second, launcher_group) < 0) {
                std::cerr << "Failure running CLI: " << it->first << '\n';
                return 1;
            }
        }
    }

    // Run launcher cli
    std::string line;
    while (true) {
        ProcessHelpers::reap_services(running_services);
        std::cout << "launcher> " << std::flush;

        if (!std::getline(std::cin, line))
            break;

        std::istringstream input(line);
        std::string command;
        input >> command;

        if (command.empty())
            continue;

        if (command == "quit") {
            break;
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
        } else if (command == "kill") {
            std::string name;
            input >> name;

            const auto* process = find_process(name);
            if (!process || process->cli)
                std::cout << "Unknown service: " << name << '\n';
            else {
                auto it = running_services.find(name);
                if (it == running_services.end()) {
                    std::cout << "Process '" << name << "' is not running." << '\n';
                    break; 
                }
                ProcessHelpers::stop_service(it->second);
                running_services.erase(it);
            }
        } else if (command == "run") {
            std::vector<std::pair<std::string, const Process*>> requested;
            std::string name;

            while (input >> name) {
                if (name == "--all") {
                    if (!requested.empty() || (input >> name)) {
                        std::cout << "Use --all alone\n";
                        requested.clear();
                        break;
                    }

                    for (const auto& [registered_name, definition]
                         : PROCESS_REGISTRY) {
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
                auto it = requested.begin();
                while (it != requested.end()) {
                    if (!it->second->cli) {
                        auto name = it->first;
                        pid_t pid = ProcessHelpers::run_process(name, *it->second, launcher_group);
                        if (pid < 0) {
                            std::cerr << "Failed running service: " << name << '\n';
                            return 1; 
                        }
                        running_services.emplace(name, pid);
                        requested.erase(it);
                    }
                    ++it;
                }

                // Run all cli's (waits on each cli)
                it = requested.begin();
                while (it != requested.end()) {
                    if (ProcessHelpers::run_process(it->first, *it->second, launcher_group) < 0) {
                        std::cerr << "Failure running CLI: " << it->first << '\n';
                        return 1;
                    }
                }
            }
        } else {
            std::cout << "Commands: run <names>, run --all, "
                         "status, kill <service>, quit\n";
        }
    }

    // Request shutdown of every service before waiting for any one.
    for (const auto& [name, pid] : running_services) {
        if (kill(-pid, SIGTERM) < 0 && errno != ESRCH)
            std::perror("kill");
    }

    for (const auto& [name, pid] : running_services) {
        while (waitpid(pid, nullptr, 0) < 0 && errno == EINTR) {
        }
        std::cout << "Stopped " << name << '\n';
    }

    return 0;
}