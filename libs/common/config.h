#pragma once

#include "nlohmann/json.hpp"

// Структура конфига для сервера
struct server_config {
    std::string udp_ip;
    int udp_port{};
    int udp_buffer_size{};
    int session_timeout_sec{};
    std::string cdr_file;
    int http_port{};
    int graceful_shutdown_rate{};
    std::string log_file;
    std::string log_level;
    std::vector<std::string> blacklist;

    server_config() = default;
};

// Структура конфига для клиента
struct client_config {
    std::string server_ip;
    int server_port;
    int udp_buffer_size;
    std::string log_file;
    std::string log_level;
};

server_config load_server_config(const std::string& path);
client_config load_client_config(const std::string& path);