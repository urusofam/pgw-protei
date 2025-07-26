#pragma once

class socket_raii {
    int fd_;

public:
    explicit socket_raii(int fd);
    ~socket_raii();

    // Запрещаем копирование
    socket_raii(const socket_raii&) = delete;
    socket_raii& operator=(const socket_raii&) = delete;

    // Разрешаем перемещение
    socket_raii(socket_raii&& other) noexcept;
    socket_raii& operator=(socket_raii&& other) noexcept;

    int get() const;
};
