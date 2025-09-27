#pragma once

#include <stdexcept>
#include <cstring>

namespace sulfur {
    class error final : public std::runtime_error {
    public:
        [[noreturn]] static void send(const std::string& what) { throw error(what); }

        [[noreturn]] static void send_errno(const std::string& prefix) {
            throw error(prefix + ": " + std::strerror(errno));
        }

    private:
        explicit error(const std::string& what) : std::runtime_error(what) {}
    };
}
