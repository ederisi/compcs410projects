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

struct Process {
    std::string executable;
    bool cli;
};

namespace ProcessHelpers 
{
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

                // TODO: Redirect stdout/stderr to logs if they disrupt the CLI.
            }

            execl(process.executable.c_str(),
                process.executable.c_str(),
                static_cast<char*>(nullptr));

            std::perror("execl");
            _exit(127);
        }

        // Do this in the parent too, before handing the terminal to a CLI.
        if (setpgid(pid, pid) < 0 && errno != EACCES && errno != ESRCH)
            std::perror("setpgid(parent)");

        std::cout << "Started " << name << " (PID " << pid << ")\n";
        return pid;
    }

    pid_t run_process(const std::string& name, const Process& process, pid_t launcher_group) {
        pid_t pid = spawn(name, process);
        if (pid < 0 || !process.cli) // On failure or spawned background service do not waitpid
            return pid;

        if (tcsetpgrp(STDIN_FILENO, pid) < 0) {
            std::perror("tcsetpgrp(cli)");
            kill(-pid, SIGTERM);
            while (waitpid(pid, nullptr, 0) < 0 && errno == EINTR) {
            }
            return -1;
        }

        int status = 0;
        pid_t result;

        do {
            result = waitpid(pid, &status, 0);
        } while (result < 0 && errno == EINTR);

        if (tcsetpgrp(STDIN_FILENO, launcher_group) < 0) {
            std::perror("tcsetpgrp(launcher)");
            return -1;
        }

        if (result < 0) {
            std::perror("waitpid(cli)");
            return -1;
        }

        if (WIFSIGNALED(status)) {
            std::cout << name << " ended on signal "
                    << WTERMSIG(status) << '\n';
        }

        return 0;
    }

    void reap_services(std::unordered_map<std::string, pid_t>& running_services) {
        for (auto it = running_services.begin();
            it != running_services.end();) {
            int status = 0;
            pid_t result = waitpid(it->second, &status, WNOHANG);

            if (result == it->second ||
                (result < 0 && errno == ECHILD)) {
                std::cout << it->first << " exited\n";
                it = running_services.erase(it);
            } else {
                ++it;
            }
        }
    }

    void stop_service(pid_t pid) {
        // Signal the child's process group, not the launcher.
        if (kill(-pid, SIGTERM) < 0 && errno != ESRCH)
            std::perror("kill");

        while (waitpid(pid, nullptr, 0) < 0 && errno == EINTR) {
        }
    }
}

#endif