#include <iostream>
#include <unistd.h>
#include <string_view>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <editline/readline.h>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <ranges>

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

    /**
     * Splits a command into tokens based on a specified delimiter.
     *
     * @param cmd The input command to be split.
     * @param delim The character used as the delimiter to split the string.
     * @return A `std::vector<std::string>` containing the tokens extracted from the input string.
     *
     * Split the input string `cmd` into tokens using the specified `delim` character. Tokens are stored in a vector and returned.
     */
    std::vector<std::string> split(const std::string_view cmd, char delim) {
        std::vector<std::string> tokens{};
        std::stringstream ss{std::string{cmd}};
        std::string item;

        while (std::getline(ss, item, delim)) {
            tokens.push_back(item);
        }

        return tokens;
    }

    bool is_prefix(const std::string_view str, const std::string_view of) {
        return str.size() <= of.size() &&
            std::ranges::equal(str, of | std::views::take(str.size()));
    }

    void resume(const pid_t pid) {
        if (ptrace(PTRACE_CONT, pid, nullptr, nullptr) < 0) {
            std::cerr << "Failed to resume process" << std::endl;
            std::exit(-1);
        }
    }

    void wait_on_signal(const pid_t pid) {
        int status{0};

        if (constexpr int options{0}; waitpid(pid, &status, options) < 0) {
            std::perror("waitpid failed");
            std::exit(-1);
        }
    }

    void run_command(const pid_t pid, const std::string_view line) {
        const auto args = split(line, ' ');
        const auto& command = args[0];

        if (is_prefix(command, "continue")) {
            resume(pid);
            wait_on_signal(pid);
        }
        else {
            std::cerr << "Unknown command: " << command << std::endl;
        }
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
