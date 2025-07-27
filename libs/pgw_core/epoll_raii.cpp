#include <sys/epoll.h>

#include "epoll_raii.h"
#include "spdlog/spdlog.h"

epoll_raii::epoll_raii() : fd_(epoll_create1(0)) {}

epoll_raii::~epoll_raii() {
    if (fd_ >= 0) {
        close(fd_);
        spdlog::debug("epoll {} закрыт", fd_);
    }
}

epoll_raii::epoll_raii(epoll_raii &&other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
}

epoll_raii& epoll_raii::operator=(epoll_raii &&other) noexcept {
    if (this != &other) {
        if (fd_ >= 0) {
            close(fd_);
        }

        fd_ = other.fd_;
        other.fd_ = -1;
    }
    spdlog::debug("epoll {} перемещён", fd_);
    return *this;
}

int epoll_raii::get() const {
    return fd_;
}
