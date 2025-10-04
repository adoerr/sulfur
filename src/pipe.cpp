#include <algorithm>
#include <unistd.h>
#include <fcntl.h>
#include <utility>
#include <libsulfur/pipe.h>
#include <libsulfur/error.hpp>

sulfur::pipe::pipe(const bool close_on_exec) {
    if (pipe2(fds_, close_on_exec ? O_CLOEXEC : 0) < 0) {
        error::send_errno("create pipe failed");
    }
}

sulfur::pipe::~pipe() {
    close_read();
    close_write();
}

int sulfur::pipe::release_read() {
    return std::exchange(fds_[read_fd], -1);
}

int sulfur::pipe::release_write() {
    return std::exchange(fds_[write_fd], -1);
}

void sulfur::pipe::close_read() {
    if (fds_[read_fd] != -1) {
        close(fds_[read_fd]);
        fds_[read_fd] = -1;
    }
}

void sulfur::pipe::close_write() {
    if (fds_[write_fd] != -1) {
        close(fds_[write_fd]);
        fds_[write_fd] = -1;
    }
}

std::vector<std::byte> sulfur::pipe::read() const {
    char buf[1014];
    size_t chars_read{0};

    if ((chars_read = ::read(fds_[read_fd], buf, sizeof(buf))) < 0) {
        error::send_errno("pipe read failed");
    }

    const auto bytes = reinterpret_cast<const std::byte*>(buf);
    return {bytes, bytes + chars_read};
}

void sulfur::pipe::write(const std::byte* from, const std::size_t bytes) const {
    if (::write(fds_[write_fd], from, bytes) < 0) {
        error::send_errno("pipe write failed");
    }
}
