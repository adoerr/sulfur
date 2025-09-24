#include <iostream>
#include <unistd.h>
#include <string_view>
#include <sys/ptrace.h>

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
    pid_t attach(int argc, const char** argv) {
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
            if (pid = fork() < 0) {
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
}

int main(int argc, const char** argv) {
    if (argc == 1) {
        std::cerr << "Arguments missing" << std::endl;
        return -1;
    }

    pid_t pid = attach(argc, argv);
}
