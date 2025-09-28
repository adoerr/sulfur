#pragma once

#include <filesystem>
#include <memory>
#include <sys/types.h>

namespace sulfur {
    enum class process_state {
        running,
        stopped,
        exited,
        terminated
    };

    struct stop_reason {
        explicit stop_reason(int wait_status);

        process_state reason;
        std::uint8_t info;
    };

    class process {
    public:
        ~process();

        static std::unique_ptr<process> launch(const std::filesystem::path& path);
        static std::unique_ptr<process> attach(pid_t pid);

        void resume();
        stop_reason wait_on_signal();
        [[nodiscard]] pid_t pid() const { return pid_; }
        [[nodiscard]] process_state state() const { return state_; }

        // `process` is neither copyable nor movable
        process() = delete;
        process(const process&) = delete;
        process& operator=(const process&) = delete;

    private:
        process(const pid_t pid, const bool terminate_on_end) : pid_(pid), terminate_on_end_(terminate_on_end) {
        };

        pid_t pid_{0};
        bool terminate_on_end_{true};
        process_state state_{process_state::stopped};
    };
} // namespace sulfur
