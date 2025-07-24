#include "config.h"
#include <fstream>

using json = nlohmann::json;

// TODO: Добавить валидацию конфига

json load_json_from_file(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Не получается открыть конфиг файл: " + path);
    }

    json data;
    try {
        data = json::parse(f);
    } catch (const json::parse_error& e) {
        throw std::runtime_error("Ошибка парсинга json: " + std::string(e.what()));
    }
    return data;
}

// Функция загрузки конфига для сервера
server_config load_server_config(const std::string& path) {
    json data = load_json_from_file(path);
    
    server_config config;
    config.udp_ip = data["udp_ip"];
    config.udp_port = data["udp_port"];
    config.udp_buffer_size = data["udp_buffer_size"];
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
    json data = load_json_from_file(path);

    client_config config;
    config.server_ip = data["server_ip"];
    config.server_port = data["server_port"];
    config.udp_buffer_size = data["udp_buffer_size"];
    config.log_file = data["log_file"];
    config.log_level = data["log_level"];

    return config;
}