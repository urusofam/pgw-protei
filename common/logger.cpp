#include "logger.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <iostream>

// Настройка логгера
void setup_logger(const std::string& log_file, const std::string& log_level) {
    try {
        if (log_file.empty()) {
            throw std::invalid_argument("Путь к файлу логов пустой");
        }

        // Консольный и файловый логгер
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        // Сам создаёт папку и файл, если они не существуют, главное чтобы log_file не был пустой
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/" + log_file, true);

        // Объединённый логгер
        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
        auto logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());
        spdlog::set_default_logger(logger);

        // Уровень логгирования
        auto level = spdlog::level::from_str(log_level);
        if (level == spdlog::level::off) {
            throw std::invalid_argument("Несуществующий уровень логирования: " + log_level);
        }
        spdlog::set_level(level);
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
    }
}


