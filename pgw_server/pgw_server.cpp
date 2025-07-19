#include <iostream>

#include "config.h"
#include "logger.h"
#include "spdlog/spdlog.h"

int main() {
    try {
        std::cout << "Current path: " << std::filesystem::current_path() << std::endl;
        server_config config = load_server_config("configs/server.json");
        setup_logger(config.log_file, config.log_level);
        spdlog::info("Конфиг и логгер загружен");
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
    }
}
