#include "config.h"
#include "logger.h"
#include "spdlog/spdlog.h"
#include <exception>
#include <iostream>

int main() {
    try {
        client_config config = load_client_config("configs/client.json");
        setup_logger(config.log_file, config.log_level);
        spdlog::info("Конфиг и логгер загружен");
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
