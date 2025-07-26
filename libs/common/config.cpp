#include <fstream>

#include "config.h"

// Парсинг json
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

    // Загрузка и валидация UDP
    config.udp_ip = get_required_field<std::string>(data, "udp_ip");
    config.udp_port = get_required_field<int>(data, "udp_port");
    if (config.udp_port < 1 || config.udp_port > 65535) {
        throw std::runtime_error("Неверный UDP порт: " + std::to_string(config.udp_port) +
            ". Порт должен быть от 1 до 65535");
    }

    config.udp_buffer_size = get_optional_field<int>(data, "udp_buffer_size", 1024);
    if (config.udp_buffer_size < 512 || config.udp_buffer_size > 65536) {
        throw std::runtime_error("Размер UDP буфера должен быть от 512 до 65536 байт");
    }

    config.udp_timer_sec = get_optional_field<int>(data, "udp_timer_sec", 1);
    if (config.udp_timer_sec <= 0) {
        throw std::runtime_error("Время таймера должно быть положительным числом");
    }

    // Загрузка и валидация таймаута сессии
    config.session_timeout_sec = get_optional_field<int>(data, "session_timeout_sec", 5);
    if (config.session_timeout_sec <= 0) {
        throw std::runtime_error("Таймаут сессии должен быть положительным числом");
    }

    // Загрузка и валидация cdr файла
    config.cdr_file = get_optional_field<std::string>(data, "cdr_file", "cdr.csv");
    if (config.cdr_file.empty()) {
        throw std::runtime_error("Путь к CDR файлу не может быть пустым");
    }

    // Загрузка и валидация HTTP порта
    config.http_port = get_required_field<int>(data, "http_port");
    if (config.http_port < 1 || config.http_port > 65535) {
        throw std::runtime_error("Неверный HTTP порт: " + std::to_string(config.http_port)
            +". Порт должен быть от 1 до 65535");
    }

    // Проверка, что UDP и HTTP порты разные
    if (config.udp_port == config.http_port) {
        throw std::runtime_error("UDP и HTTP порты должны быть разными");
    }

    // Загрузка и валидация graceful shutdown rate
    config.graceful_shutdown_rate = get_optional_field<int>(data, "graceful_shutdown_rate", 10);
    if (config.graceful_shutdown_rate <= 0) {
        throw std::runtime_error("Graceful shutdown rate должен быть положительным числом");
    }

    // Загрузка и валидация логгера
    config.log_file = get_optional_field<std::string>(data, "log_file", "server.log");
    if (config.log_file.empty()) {
        throw std::runtime_error("Путь к файлу логов не может быть пустым");
    }
    config.log_level = get_optional_field<std::string>(data, "log_level", "info");

    // Загрузка блэклиста
    if (data.contains("blacklist")) {
        if (!data["blacklist"].is_array()) {
            throw std::runtime_error("Blacklist должен быть массивом");
        }
        config.blacklist = data["blacklist"];
    } else {
        config.blacklist = std::vector<std::string>{};
    }

    return config;
}

// Функция загрузки конфига для клиента
client_config load_client_config(const std::string& path) {
    json data = load_json_from_file(path);

    client_config config;

    // Загрузка и валидация сервера
    config.server_ip = get_required_field<std::string>(data, "server_ip");
    config.server_port = get_required_field<int>(data, "server_port");
    if (config.server_port < 1 || config.server_port > 65535) {
        throw std::runtime_error("Неверный порт сервера: " + std::to_string(config.server_port)
            + ". Порт должен быть от 1 до 65535");
    }

    // Загрузка и валидация UDP
    config.udp_buffer_size = get_optional_field<int>(data, "udp_buffer_size", 1024);
    if (config.udp_buffer_size < 512 || config.udp_buffer_size > 65536) {
        throw std::runtime_error("Размер UDP буфера должен быть от 512 до 65536 байт");
    }

    config.udp_timer_sec = get_optional_field<int>(data, "udp_timer_sec", 1);
    if (config.udp_timer_sec <= 0) {
        throw std::runtime_error("Время таймера должно быть положительным числом");
    }

    // Загрузка и валидация логгера
    config.log_file = get_optional_field<std::string>(data, "log_file", "client.log");
    if (config.log_file.empty()) {
        throw std::runtime_error("Путь к лог файлу не может быть пустым");
    }
    config.log_level = get_optional_field<std::string>(data, "log_level", "info");

    return config;
}