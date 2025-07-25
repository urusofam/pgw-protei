#include <iostream>
#include <arpa/inet.h>

#include "bcd.h"
#include "config.h"
#include "logger.h"

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Использование: pgw_client <IMSI>" << '\n';
            return 1;
        }

        client_config config = load_client_config("configs/client.json");
        setup_logger(config.log_file, config.log_level);
        spdlog::info("Конфиг и логгер загружен");

        std::string IMSI = argv[1];
        std::vector<uint8_t> BCD = imsi_to_bcd(IMSI);
        spdlog::info("IMSI получен и успешно перекодирован в BCD");

        spdlog::info("Подготовка к отправке UDP пакета IMSI {} на сервер {}:{}", IMSI,
            config.server_ip, config.server_port);

        // Создание сокета
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            spdlog::critical("Не удалось создать UDP сокет: {}", strerror(errno));
            return 1;
        }
        spdlog::debug("Сокет создан");

        // Создание таймера
        timeval tv{};
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        spdlog::debug("Таймер к сокету создан");

        // Настройка IP адреса
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(config.server_port);
        if (inet_pton(AF_INET, config.server_ip.c_str(), &server_addr.sin_addr) <= 0) {
            spdlog::critical("Неправильный IP адрес {}", config.server_ip);
            close(sockfd);
            return 1;
        }
        spdlog::debug("IP адрес настроен");

        // Отправка UDP пакета
        if (sendto(sockfd, BCD.data(), BCD.size(), 0, (sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
            spdlog::critical("Не удалось отправить UDP пакет: {}", strerror(errno));
            close(sockfd);
            return 1;
        }
        spdlog::info("UDP пакет отправлен");

        // Чтение ответа
        char buffer[config.udp_buffer_size];
        ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, nullptr, nullptr);

        if (n < 0) {
            spdlog::critical("Не удалось получить ответ от сервера: {}", strerror(errno));
            close(sockfd);
            return 1;
        }

        if (n == sizeof(buffer) - 1) {
            spdlog::warn("Возможно, ответ был обрезан (получено максимум байт)");
        }

        buffer[n] = '\0';
        std::string response(buffer);
        spdlog::info("Получен ответ от сервера: {}", response);
        std::cout << response << '\n';

        close(sockfd);
    } catch (std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}