#pragma once

#include "nlohmann/json.hpp"

using json = nlohmann::json;

template<typename T>
T get_required_field(const json& data, const std::string& field_name) {
    if (!data.contains(field_name)) {
        throw std::runtime_error("Обязательное поле отсутствует: " + field_name);
    }

    try {
        return data[field_name].get<T>();
    } catch (const json::type_error& e) {
        throw std::runtime_error("Неверный тип для поля " + field_name + ": " + e.what());
    }
}

template<typename T>
T get_optional_field(const json& data, const std::string& field_name, const T& default_value) {
    if (!data.contains(field_name)) {
        return default_value;
    }

    try {
        return data[field_name].get<T>();
    } catch (const json::type_error& e) {
        throw std::runtime_error("Неверный тип для поля " + field_name + ": " + e.what());
    }
}

// Структура конфига для сервера
struct server_config {
    std::string udp_ip;
    int udp_port{};
    int udp_buffer_size{};
    int epoll_max_events{};
    int epoll_timeout_sec{};
    int session_timeout_sec{};
    std::string cdr_file;
    std::string http_ip;
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
    int server_port{};
    int udp_buffer_size{};
    int udp_timer_sec{};
    std::string log_file;
    std::string log_level;
};

server_config load_server_config(const std::string& path);
client_config load_client_config(const std::string& path);