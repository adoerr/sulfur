#include <libsulfur/process.hpp>
#include <libsulfur/error.hpp>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdio>

std::unique_ptr<sulfur::process> sulfur::process::launch(const std::filesystem::path& path) {
    pid_t pid{};

    if ((pid = fork()) < 0) {
        error::send_errno("fork failed");
    }

    if (pid == 0) {
        if (ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) < 0) {
            error::send_errno("ptrace traceme failed");
        }

        if (execlp(path.c_str(), path.c_str(), nullptr) < 0) {
            error::send_errno("execlp failed");
        }
    }

    std::unique_ptr<process> proc(new process(pid, true));
    proc->wait_on_signal();

    return proc;
}

std::unique_ptr<sulfur::process> sulfur::process::attach(const pid_t pid) {
    if (pid <= 0) {
        error::send("invalid pid");
    }

    if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) < 0) {
        error::send_errno("ptrace attach failed");
    }

    std::unique_ptr<process> proc(new process(pid, false));
    proc->wait_on_signal();

    return proc;
}

sulfur::process::~process() {
    if (pid_ != 0) {
        int status;

        // for PTRACE_DETACH to work, the process must be stopped
        if (state_ == process_state::running) {
            kill(pid_, SIGSTOP);
            waitpid(pid_, &status, 0);
        }

        ptrace(PTRACE_DETACH, pid_, nullptr, nullptr);
        kill(pid_, SIGCONT);

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
