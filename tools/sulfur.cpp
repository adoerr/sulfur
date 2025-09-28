#include <iostream>
#include <string_view>
#include <sys/wait.h>
#include <editline/readline.h>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <ranges>
#include <string.h>
#include <libsulfur/process.hpp>
#include <libsulfur/error.hpp>

namespace {
    /**
     * Attaches to an existing process or launches a new one based on command-line arguments.
     *
     * @param argc The number of command-line arguments.
     * @param argv The array of command-line argument strings.
     * @return A `std::unique_ptr<sulfur::process>` representing the attached or launched process.
     *
     * If the arguments indicate attaching to an existing process (using `-p <pid>`), it attaches to that process.
     * Otherwise, it launches a new process using the provided program path.
     */
    std::unique_ptr<sulfur::process> attach(const int argc, const char** argv) {
        // attach to an existing process using PID
        if (argc == 3 && argv[1] == std::string_view{"-p"}) {
            pid_t pid{0};
            pid = std::stoi(argv[2]);
            return sulfur::process::attach(pid);
        }

        // launch a new process using program path
        const char* path = argv[1];
        return sulfur::process::launch(path);
    }

    /**
     * Splits a command string into individual tokens based on spaces.
     *
     * @param cmd The command string to split.
     * @return A vector of tokens extracted from the command string.
     */
    std::vector<std::string> split(const std::string_view cmd) {
        std::vector<std::string> tokens{};
        std::stringstream ss{std::string{cmd}};
        std::string item;

        while (std::getline(ss, item, ' ')) {
            tokens.push_back(item);
        }

        return tokens;
    }

    bool is_prefix(const std::string_view str, const std::string_view of) {
        return str.size() <= of.size() &&
            std::ranges::equal(str, of | std::views::take(str.size()));
    }

    void print_stop_reason(const sulfur::process& process, const sulfur::stop_reason& reason) {
        std::cout << "Process " << process.pid() << " ";

        switch (reason.reason) {
        case sulfur::process_state::exited:
            std::cout << "exited with code " << static_cast<int>(reason.info);
            break;
        case sulfur::process_state::stopped:
            std::cout << "stopped with signal " << sigabbrev_np(reason.info);
            break;
        case sulfur::process_state::terminated:
            std::cout << "terminated with signal " << sigabbrev_np(reason.info);
            break;
        default:
            std::cout << "in unknown state";
            break;
        }

        std::cout << std::endl;
    }

    void run_command(const std::unique_ptr<sulfur::process>& process, const std::string_view line) {
        const auto args = split(line);
        const auto command = args[0];

        if (is_prefix(command, "continue")) {
            process->resume();
            const auto reason = process->wait_on_signal();
            print_stop_reason(*process, reason);
        }
        else {
            std::cerr << "Unknown command: " << command << std::endl;
        }
    }

    void command_loop(const std::unique_ptr<sulfur::process>& process) {
        char* line = nullptr;
        while ((line = readline("sulfur> ")) != nullptr) {
            std::string line_str;

            if (line == std::string_view("")) {
                free(line);
                if (history_length > 0) {
                    line_str = history_get(history_length - 1)->line;
                }
            }
            else {
                line_str = line;
                add_history(line);
                free(line);
            }

            if (!line_str.empty()) {
                try {
                    run_command(process, line_str);
                }
                catch (const sulfur::error& e) {
                    std::cerr << e.what() << std::endl;
                }
            }
        }
    }
}

int main(const int argc, const char** argv) {
    if (argc == 1) {
        std::cerr << "Arguments missing" << std::endl;
        return -1;
    }

    try {
        const auto process = attach(argc, argv);
        command_loop(process);
    }
    catch (const sulfur::error& e) {
        std::cerr << e.what() << std::endl;
    }
}
