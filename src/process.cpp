#include <libsulfur/process.hpp>
#include <libsulfur/error.hpp>
#include <libsulfur/pipe.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdio>

namespace {
    void exit_with_error(const sulfur::pipe& channel, std::string const& prefix) {
        const auto msg = prefix + ": " + std::strerror(errno);
        channel.write(reinterpret_cast<const std::byte*>(msg.data()), msg.size());
        exit(1);
    }
}

sulfur::stop_reason::stop_reason(const int wait_status) {
    if (WIFEXITED(wait_status)) {
        reason = process_state::exited;
        info = WEXITSTATUS(wait_status);
    }
    else if (WIFSIGNALED(wait_status)) {
        reason = process_state::terminated;
        info = WTERMSIG(wait_status);
    }
    else if (WIFSTOPPED(wait_status)) {
        reason = process_state::stopped;
        info = WSTOPSIG(wait_status);
    }
}

std::unique_ptr<sulfur::process> sulfur::process::launch(const std::filesystem::path& path, const bool debug) {
    // create a pipe for error reporting from child to parent
    pipe channel(true);

    pid_t pid{};

    if ((pid = fork()) < 0) {
        error::send_errno("fork failed");
    }

    // child process
    if (pid == 0) {
        channel.close_read();

        if (debug and ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) < 0) {
            exit_with_error(channel, "ptrace traceme failed");
        }

        if (execlp(path.c_str(), path.c_str(), nullptr) < 0) {
            exit_with_error(channel, "execlp failed");
        }
    }

    // parent process
    channel.close_write();
    auto error = channel.read();
    channel.close_read();

    if (!error.empty()) {
        // wait for child to terminate in order to avoid a zombie process
        waitpid(pid, nullptr, 0);
        const auto msg = reinterpret_cast<char*>(error.data());
        error::send(std::string(msg, msg + error.size()));
    }

    std::unique_ptr<process> proc(new process(pid, true, debug));

    if (debug) {
        proc->wait_on_signal();
    }

    return proc;
}

std::unique_ptr<sulfur::process> sulfur::process::attach(const pid_t pid) {
    if (pid <= 0) {
        error::send("invalid pid");
    }

    if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) < 0) {
        error::send_errno("ptrace attach failed");
    }

    std::unique_ptr<process> proc(new process(pid, false, true));
    proc->wait_on_signal();

    return proc;
}

sulfur::process::~process() {
    if (pid_ != 0) {
        int status;

        if (is_attached_) {
            // for PTRACE_DETACH to work, the process must be stopped
            if (state_ == process_state::running) {
                kill(pid_, SIGSTOP);
                waitpid(pid_, &status, 0);
            }

            ptrace(PTRACE_DETACH, pid_, nullptr, nullptr);
            kill(pid_, SIGCONT);
        }

        if (terminate_on_end_) {
            kill(pid_, SIGKILL);
            waitpid(pid_, &status, 0);
        }
    }
}

void sulfur::process::resume() {
    if (ptrace(PTRACE_CONT, pid_, nullptr, nullptr) < 0) {
        error::send_errno("ptrace continue failed");
    }

    state_ = process_state::running;
}

sulfur::stop_reason sulfur::process::wait_on_signal() {
    int status{0};

    if (constexpr int options{0}; waitpid(pid_, &status, options) < 0) {
        error::send_errno("waitpid failed");
    }

    const stop_reason reason(status);
    state_ = reason.reason;
    return reason;
}
