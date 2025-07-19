#include "config.h"
#include "logger.h"
#include "spdlog/spdlog.h"
#include <exception>
#include <iostream>

#include "bcd.h"

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Использование: pgw_client <imsi>" << '\n';
            return 1;
        }

        client_config config = load_client_config("configs/client.json");
        setup_logger(config.log_file, config.log_level);
        spdlog::info("Конфиг и логгер загружен");

        std::string IMSI = argv[1];
        std::vector<uint8_t> BCD = imsi_to_bcd(IMSI);
        spdlog::info("Imsi получен и успешно перекодирован в BCD");
    } catch (std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}
