#pragma once

class epoll_raii {
    int fd_;

public:
    explicit epoll_raii();
    ~epoll_raii();

    // Запрещаем копирование
    epoll_raii(const epoll_raii&) = delete;
    epoll_raii& operator=(const epoll_raii&) = delete;

    // Разрешаем перемещение
    epoll_raii(epoll_raii&& other) noexcept;
    epoll_raii& operator=(epoll_raii&& other) noexcept;

    int get() const;
};
