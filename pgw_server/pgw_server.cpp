#include <httplib.h>
#include <sys/epoll.h>

#include "bcd.h"
#include "epoll_raii.h"
#include "logger.h"
#include "session_manager.h"
#include "socket_raii.h"
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
        if (http_thread_.joinable()) {
            http_thread_.join();
        }

        spdlog::info("PGW сервер выключился");
    }

    // Запуск UDP сервера
    void run_udp_server() {
        spdlog::info("UDP сервер {}:{} запускается", config_.udp_ip, config_.udp_port);

        // Создаём сокет с RAII
        socket_raii sockfd(socket(AF_INET, SOCK_DGRAM, 0));
        if (sockfd.get() < 0) {
            spdlog::critical("Не удалось создать UDP сокет: {}", strerror(errno));
            running_ = false;
            return;
        }
        spdlog::debug("Создан сокет");

        // Делаем сокет неблокирующим
        int flags = fcntl(sockfd.get(), F_GETFL, 0);
        if (flags == -1) {
            spdlog::critical("Не удалось получить флаги сокета: {}", strerror(errno));
            running_ = false;
            return;
        }
        if (fcntl(sockfd.get(), F_SETFL, flags | O_NONBLOCK) == -1) {
            spdlog::critical("Не удалось установить неблокирующий режим для сокета: {}", strerror(errno));
            running_ = false;
            return;
        }
        spdlog::debug("Сокет переведен в неблокирующий режим");

        // Создаем epoll
        epoll_raii epollfd;
        spdlog::debug("Epoll создан0");

        // Регистрируем сокет в epoll
        epoll_event event{};
        event.events = EPOLLIN;
        event.data.fd = sockfd.get();
        if (epoll_ctl(epollfd.get(), EPOLL_CTL_ADD, sockfd.get(), &event) < 0) {
            spdlog::critical("Не удалось добавить сокет в epoll: {}", strerror(errno));
            running_ = false;
            return;
        }
        spdlog::debug("Сокет добавлен в epoll для отслеживания");

        // Настриваем IP адрес
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(config_.udp_port);
        if (inet_pton(AF_INET, config_.udp_ip.c_str(), &server_addr.sin_addr) <= 0) {
            spdlog::critical("Неправильный IP адрес {}", config_.udp_ip);
            running_ = false;
            return;
        }
        spdlog::debug("IP адрес настроен");

        // Привязываем сокет к адресу
        if (bind(sockfd.get(), reinterpret_cast<sockaddr*> (&server_addr), sizeof(server_addr)) < 0) {
            spdlog::critical("Не удалось привязать UDP сокет к адресу: {}", strerror(errno));
            running_ = false;
            return;
        }
        spdlog::info("UDP сервер запущен");

        // Создаём буфер для событий epoll
        epoll_event events[config_.epoll_max_events];

        // Создаём буфер для данных
        char buffer[config_.udp_buffer_size];
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);

        // Читаем данные от клиентов
        while (running_) {
            int n_events = epoll_wait(epollfd.get(), events, config_.epoll_max_events,
                1000 * config_.epoll_timeout_sec);

            if (n_events < 0) {
                if (errno == EINTR) {
                    continue;
                }
                spdlog::critical("Ошибка epoll_wait: {}", strerror(errno));
                running_ = false;
                break;
            }

            for (int i = 0; i < n_events; i++) {
                if (events[i].data.fd == sockfd.get()) {
                    while (true) {
                        ssize_t n = recvfrom(sockfd.get(), buffer, sizeof(buffer), 0,
                            reinterpret_cast<sockaddr*> (&client_addr), &addr_len);

                        if (n < 0) {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                break;
                            }
                            spdlog::error("Ошибка recvfrom: {}", strerror(errno));
                            break;
                        }

                        if (n == sizeof(buffer)) {
                            spdlog::warn("Возможно, запрос был обрезан (получено максимум байт)");
                        }

                        // Декодируем bcd
                        std::vector<uint8_t> bcd(buffer, buffer + n);
                        std::string imsi = bcd_to_imsi(bcd);
                        spdlog::debug("Получен UDP запрос для imsi {}", imsi);

                        // Отправляем ответ
                        std::string response = session_manager_->process_request(imsi);
                        if (sendto(sockfd.get(), response.c_str(), response.length(), 0,
                            reinterpret_cast<sockaddr*> (&client_addr), addr_len) < 0) {
                            spdlog::error("Не удалось отправить ответ для imsi {}: {}", imsi, strerror(errno));
                            }
                    }
                }
            }
        }

        spdlog::info("UDP сервер остановлен");
    }

    // Запуск HTTP сервера
    void run_http_server() {
        spdlog::info("HTTP сервер {}:{} запускается...", config_.http_ip, config_.http_port);

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

        spdlog::info("HTTP сервер {}:{} запустился", config_.http_ip, config_.http_port);
        http_server_.listen(config_.http_ip, config_.http_port);
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