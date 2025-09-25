#include <iostream>
#include <unistd.h>
#include <string_view>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <editline/readline.h>
#include <string>

namespace {
    /**
      * Attaches to a process or forks a new process for tracing.
      *
      * @param argc The number of command-line arguments.
      * @param argv The array of command-line arguments.
      *             - If `-p <pid>` is provided, attaches to the process with the given PID.
      *             - Otherwise, forks a new process and executes the specified program.
      *
      * @return The PID of the attached or forked process, or -1 on failure.
    */
    pid_t attach(const int argc, const char** argv) {
        pid_t pid{0};

        if (argc == 3 && argv[1] == std::string_view{"-p"}) {
            pid = std::stoi(argv[2]);

            if (pid <= 0) {
                std::cerr << "Invalid pid" << std::endl;
                return -1;
            }

            if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) < 0) {
                std::perror("Attach failed");
                return -1;
            }
        }
        else {
            if ((pid = fork() < 0)) {
                std::perror("Fork failed");
                return -1;
            }

            // if we are the child process
            if (pid == 0) {
                // enable process tracing
                if (ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) < 0) {
                    std::perror("Trace me failed");
                    return -1;
                }

                if (const char* path = argv[1]; execlp(path, path, nullptr) < 0) {
                    std::perror("Exec failed");
                    return -1;
                }
            }
        }

        return pid;
    }

    void run_command(pid_t pid, std::string_view line) {
        std::cerr << line << std::endl;
    }
}

int main(const int argc, const char** argv) {
    if (argc == 1) {
        std::cerr << "Arguments missing" << std::endl;
        return -1;
    }

    const pid_t pid = attach(argc, argv);

    // wait for the inferior to stop after attach
    int status{0};

    if (constexpr int options{0}; waitpid(pid, &status, options) < 0) {
        std::perror("Waitpid failed");
        return -1;
    }

    char* line{nullptr};

    while ((line = readline("sulfur> ")) != nullptr) {
        std::string line_str;

        // an empty line means to repeat the last command
        if (line == std::string_view("")) {
            free(line);
            if (history_length > 0) {
                line_str = history_list()[history_length - 1]->line;
            }
        }
        else {
            line_str = line;
            add_history(line);
            free(line);
        }

        if (!line_str.empty()) {
            run_command(pid, line_str);
        }
    }
}
