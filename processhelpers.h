#ifndef PROCESSHELPERS_H
#define PROCESSHELPERS_H

#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

// Represents process definition containing its executable path
struct Process {
    std::string executable;
    bool cli;
};

// Namespace containing helper utils
namespace ProcessHelpers 
{
    // Spawns a new child process for a given service or a CLI program
    pid_t spawn(const std::string& name, const Process& process) {
        pid_t pid = fork();

        if (pid < 0) {
            std::perror("fork");
            return -1;
        }

        if (pid == 0) {
            // Make this child the leader of its own process group.
            if (setpgid(0, 0) < 0) {
                std::perror("setpgid(child)");
                _exit(127);
            }

            // Launcher ignores SIGTTOU to reclaim the terminal.
            // Its children must not inherit that setting.
            std::signal(SIGTTOU, SIG_DFL);

            if (!process.cli) {
                // Background services must not read CLI/launcher input.
                int fd = open("/dev/null", O_RDONLY);
                if (fd < 0 || dup2(fd, STDIN_FILENO) < 0) {
                    std::perror("service stdin");
                    _exit(127);
                }
                close(fd);

                // Launch background service with -b flag
                execl(process.executable.c_str(),
                      process.executable.c_str(),
                      "-b",
                      static_cast<char*>(nullptr));
            } else {
                // Launch interactive CLI normally
                execl(process.executable.c_str(),
                      process.executable.c_str(),
                      static_cast<char*>(nullptr));
            }

            std::perror("execl");
            _exit(127);
        }

        // Do this in the parent too, before handing the terminal to a CLI.
        // (This avoids race conditions)
        if (setpgid(pid, pid) < 0 && errno != EACCES && errno != ESRCH)
            std::perror("setpgid(parent)");

        std::cout << "Started " << name << " (PID " << pid << ")\n";
        return pid;
    }

    // Run a process, handing over terminal control if an interactive CLI
    pid_t run_process(const std::string& name, const Process& process, pid_t launcher_group) {
        pid_t pid = spawn(name, process);
        if (pid < 0 || !process.cli) // On failure or spawned background service do not waitpid
            return pid;

        // Give terminal control to the interactive CLI
        if (tcsetpgrp(STDIN_FILENO, pid) < 0) {
            std::perror("tcsetpgrp(cli)");
            kill(-pid, SIGTERM);
            while (waitpid(pid, nullptr, 0) < 0 && errno == EINTR) {
            }
            return -1;
        }

        int status = 0;
        pid_t result;

        // Wait for the interactive CLI process finish
        do {
            result = waitpid(pid, &status, 0);
        } while (result < 0 && errno == EINTR);


        // Restore terminal control back to the launcher process group
        if (tcsetpgrp(STDIN_FILENO, launcher_group) < 0) {
            std::perror("tcsetpgrp(launcher)");
            return -1;
        }

        if (result < 0) {
            std::perror("waitpid(cli)");
            return -1;
        }

        if (WIFSIGNALED(status)) {
            std::cout << '\n' << name << " ended on signal "
                    << WTERMSIG(status) << '\n';
        }

        return 0;
    }


    // Checks and reaps any terminated background service non-blockingly
    void reap_services(std::unordered_map<std::string, pid_t>& services) {
        for (auto it = services.begin();
            it != services.end();) {
            int status = 0;
            // Check process status without blocking using WNOHANG
            pid_t result = waitpid(it->second, &status, WNOHANG);

            if (result == it->second ||
                (result < 0 && errno == ECHILD)) {
                std::cout << it->first << " exited\n";
                it = services.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Stops a specific running service by sending SIGTERM to its process group
    void stop_service(pid_t pid) {
        // Signal the child's process group, not the launcher.
        if (kill(-pid, SIGTERM) < 0 && errno != ESRCH)
            std::perror("kill");

        while (waitpid(pid, nullptr, 0) < 0 && errno == EINTR) {
        }
    }

    // Terminates all tracked background services
    void stop_services(std::unordered_map<std::string, pid_t>& services) {
        // Signal every tracked service before waiting for any of them.
        for (const auto& [name, pid] : services) {
            if (kill(-pid, SIGTERM) < 0 && errno != ESRCH)
                std::perror("kill(SIGTERM)");

            // Allow a stopped service to process termination.
            if (kill(-pid, SIGCONT) < 0 && errno != ESRCH)
                std::perror("kill(SIGCONT)");
        }

        // wait for all services to exit cleanly
        for (const auto& [name, pid] : services) {
            pid_t result;
            do {
                result = waitpid(pid, nullptr, 0);
            } while (result < 0 && errno == EINTR);

            if (result < 0 && errno != ECHILD)
                std::perror("waitpid(service)");

            std::cout << "Stopped " << name << '\n';
        }
    }
}

#endif