#include <httplib.h>

#include "bcd.h"
#include "logger.h"
#include "session_manager.h"
#include "spdlog/spdlog.h"

std::atomic running_ = false;

// Обработка sigint и sigterm
void signal_handler(int sig) {
    spdlog::warn("Получен сигнал: {}.", sig);
    running_ = false;
}

class pgw_server {
    server_config config_;
    std::shared_ptr<session_manager> session_manager_;
    httplib::Server http_server_;
    std::jthread udp_thread_;
    std::jthread http_thread_;

    // Остановка PGW сервера
    void stop() {
        spdlog::info("PGW сервер выключается...");

        // Останавливаем все потоки
        session_manager_->graceful_shutdown();
        if (udp_thread_.joinable()) {
            udp_thread_.join();
        }
        http_server_.stop();

        spdlog::info("PGW сервер выключился");
    }

    // Запуск UDP сервера
    void run_udp_server() {
        spdlog::info("UDP сервер {}:{} запускается", config_.udp_ip, config_.udp_port);

        // Создаём сокет
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            spdlog::critical("Не удалось создать UDP сокет: {}", strerror(errno));
            return;
        }
        spdlog::debug("Создан сокет");

        // Добавляем таймер
        timeval tv{};
        tv.tv_sec = config_.udp_timer_sec;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        spdlog::debug("Добавлен таймер к сокету");

        // Настриваем IP адрес
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(config_.udp_port);
        if (inet_pton(AF_INET, config_.udp_ip.c_str(), &server_addr.sin_addr) <= 0) {
            spdlog::critical("Неправильный IP адрес {}", config_.udp_ip);
            close(sockfd);
            running_ = false;
            return;
        }
        spdlog::debug("IP адрес настроен");

        // Привязываем сокет к адресу
        if (bind(sockfd, (sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
            spdlog::critical("Не удалось привязать UDP сокет к адресу: {}", strerror(errno));
            close(sockfd);
            running_ = false;
            return;
        }
        spdlog::info("UDP сервер запущен");

        // Создаём буфер
        char buffer[config_.udp_buffer_size];
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);

        // Читаем данные от клиента
        while (running_) {
            ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (sockaddr*) &client_addr, &addr_len);
            if (n == 0) continue;

            if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                spdlog::error("Ошибка recvfrom: {}", strerror(errno));
                running_ = false;
                break;
            }

            if (n == sizeof(buffer)) {
                spdlog::warn("Возможно, запрос был обрезан (получено максимум байт)");
            }

            // Декодируем bcd
            std::vector<uint8_t> bcd(buffer, buffer + n);
            std::string imsi = bcd_to_imsi(bcd);
            spdlog::info("Получен UDP запрос для imsi {}", imsi);

            // Отправляем ответ
            std::string response = session_manager_->process_request(imsi);
            if (sendto(sockfd, response.c_str(), response.length(), 0, (sockaddr*) &client_addr, addr_len) < 0) {
                spdlog::error("Не удалось отправить ответ для imsi {}: {}", imsi, strerror(errno));
            }
        }

        close(sockfd);
        spdlog::info("UDP сервер остановлен");
    }

    // Запуск HTTP сервера
    void run_http_server() {
        spdlog::info("HTTP сервер на порту {} запускается...", config_.http_port);

        // Настройка ручек
        http_server_.Get("/check_subscriber", [this](const httplib::Request& req, httplib::Response& res) {
            // Нет IMSI
            if (!req.has_param("imsi")) {
                spdlog::warn("Получен http запрос без imsi");
                res.set_content("Ошибка: требуется imsi", "text/plain");
                res.status = 400;
                return;
            }

            std::string imsi = req.get_param_value("imsi");
            spdlog::info("Получен http запрос на проверку сессии с imsi {}", imsi);
            if (session_manager_->is_session_active(imsi)) {
                res.set_content("active", "text/plain");
            } else {
                res.set_content("not active", "text/plain");
            }
            res.status = 200;
        });

        http_server_.Get("/stop", [this](const httplib::Request&, httplib::Response& res) {
            spdlog::warn("Получен /stop http запрос.");
            res.set_content("Остановка запущена", "text/plain");
            res.status = 200;
            running_ = false;
        });

        spdlog::info("HTTP сервер на порту {} запустился", config_.http_port);
        http_server_.listen("0.0.0.0", config_.http_port);
        spdlog::info("HTTP сервер остановлен");
    }
public:
    explicit pgw_server(const server_config& config) : config_(config),
    session_manager_(std::make_shared<session_manager>(config)) {}

    // Запуск PGW сервера
    void start() {
        spdlog::info("PGW сервер запускается...");

        // Запускаем потоки для чистки сессий, udp и http
        running_ = true;
        session_manager_->start_cleaning();
        udp_thread_ = std::jthread(&pgw_server::run_udp_server, this);
        http_thread_ = std::jthread(&pgw_server::run_http_server, this);

        spdlog::info("PGW сервер запустился");
        while (running_) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        stop();
    }
};

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try {
        server_config config = load_server_config("configs/server.json");
        setup_logger(config.log_file, config.log_level);
        spdlog::info("Конфиг и логгер загружен");

        pgw_server server(config);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}