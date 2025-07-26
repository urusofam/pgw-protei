#include "socket_raii.h"
#include "spdlog/spdlog.h"

socket_raii::socket_raii(int fd) : fd_(fd) {}

socket_raii::~socket_raii() {
    if (fd_ >= 0) {
        close(fd_);
        spdlog::debug("Сокет {} закрыт", fd_);
    }
}

socket_raii::socket_raii(socket_raii &&other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;  // other больше не владеет сокетом
}

socket_raii& socket_raii::operator=(socket_raii &&other) noexcept {
    if (this != &other) {
        if (fd_ >= 0) {
            close(fd_);
        }

        fd_ = other.fd_;
        other.fd_ = -1;
    }
    spdlog::debug("Сокет {} перемещён", fd_);
    return *this;
}

int socket_raii::get() const {
    return fd_;
}
