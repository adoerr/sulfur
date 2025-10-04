#pragma once

#include <vector>

namespace sulfur {
    class pipe {
    public:
        explicit pipe(bool close_on_exec = true);
        ~pipe();

        [[nodiscard]] int get_read() const { return fds_[read_fd]; }
        [[nodiscard]] int get_write() const { return fds_[write_fd]; }
        int release_read();
        int release_write();
        void close_read();
        void close_write();

        [[nodiscard]] std::vector<std::byte> read() const;
        void write(const std::byte* from, std::size_t bytes) const;

    private:
        static constexpr unsigned read_fd = 0;
        static constexpr unsigned write_fd = 1;

        int fds_[2]{-1, -1};
    };
} // namespace sulfur
