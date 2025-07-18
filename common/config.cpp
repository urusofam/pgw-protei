#include "config.h"
#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

// Функция загрузки конфига для сервера
server_config load_server_config(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Не получается открыть конфиг файл: " + path);
    }
    json data = json::parse(f);
    
    server_config config;
    config.udp_ip = data["udp_ip"];
    config.udp_port = data["udp_port"];
    config.session_timeout_sec = data["session_timeout_sec"];
    config.cdr_file = data["cdr_file"];
    config.http_port = data["http_port"];
    config.graceful_shutdown_rate = data["graceful_shutdown_rate"];
    config.log_file = data["log_file"];
    config.log_level = data["log_level"];
    config.blacklist = data["blacklist"];
    
    return config;
}

// Функция загрузки конфига для клиента
client_config load_client_config(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Не получается открыть конфиг файл: " + path);
    }
    json data = json::parse(f);

    client_config config;
    config.server_ip = data["server_ip"];
    config.server_port = data["server_port"];
    config.log_file = data["log_file"];
    config.log_level = data["log_level"];

    return config;
}