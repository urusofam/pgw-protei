#include <iostream>
#include <arpa/inet.h>

#include "bcd.h"
#include "config.h"
#include "logger.h"
#include "socket.h"

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Использование: pgw_client <IMSI>" << '\n';
            return 1;
        }

        client_config config = load_client_config("configs/client.json");
        setup_logger(config.log_file, config.log_level);
        spdlog::info("Конфиг и логгер загружен");

        std::string imsi = argv[1];
        std::vector<uint8_t> bcd = imsi_to_bcd(imsi);
        spdlog::info("IMSI получен и успешно перекодирован в BCD");

        spdlog::info("Подготовка к отправке UDP пакета IMSI {} на сервер {}:{}", imsi,
            config.server_ip, config.server_port);

        // Создание сокета с RAII
        socket_raii sockfd(socket(AF_INET, SOCK_DGRAM, 0));
        if (sockfd.get() < 0) {
            spdlog::critical("Не удалось создать UDP сокет: {}", strerror(errno));
            return 1;
        }
        spdlog::debug("Сокет создан");

        // Создание таймера
        timeval tv{};
        tv.tv_sec = config.udp_timer_sec;
        if (setsockopt(sockfd.get(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            spdlog::error("Не удалось установить таймаут: {}", strerror(errno));
        }
        spdlog::debug("Таймер к сокету создан");

        // Настройка IP адреса
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(config.server_port);
        if (inet_pton(AF_INET, config.server_ip.c_str(), &server_addr.sin_addr) <= 0) {
            spdlog::critical("Неправильный IP адрес {}", config.server_ip);
            return 1;
        }
        spdlog::debug("IP адрес настроен");

        // Отправка UDP пакета
        if (sendto(sockfd.get(), bcd.data(), bcd.size(), 0, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
            spdlog::critical("Не удалось отправить UDP пакет: {}", strerror(errno));
            return 1;
        }
        spdlog::info("UDP пакет отправлен");

        // Чтение ответа
        char buffer[config.udp_buffer_size];
        ssize_t n = recvfrom(sockfd.get(), buffer, sizeof(buffer) - 1, 0, nullptr, nullptr);

        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                spdlog::critical("Таймаут при получении ответа от сервера");
            } else {
                spdlog::critical("Не удалось получить ответ от сервера: {}", strerror(errno));
            }
            return 1;
        }

        if (n == sizeof(buffer) - 1) {
            spdlog::warn("Возможно, ответ был обрезан (получено максимум байт)");
        }

        buffer[n] = '\0';
        std::string response(buffer);
        spdlog::info("Получен ответ от сервера: {}", response);
        std::cout << response << '\n';
    } catch (std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}