#include "logger.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <iostream>

// Настройка логгера
void setup_logger(const std::string& log_file, const std::string& log_level) {
    try {
        // Консольный и файловый логгер
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file, true);

        // Объединённый логгер
        auto logger = std::make_shared<spdlog::logger>("multi_sink", {console_sink, file_sink});
        spdlog::set_default_logger(logger);

        // Уровень логгирования
        if (log_level == "debug") {
            spdlog::set_level(spdlog::level::debug);
        } else if (log_level == "info") {
            spdlog::set_level(spdlog::level::info);
        } else if (log_level == "warn") {
            spdlog::set_level(spdlog::level::warn);
        } else if (log_level == "error") {
            spdlog::set_level(spdlog::level::err);
        } else if (log_level == "critical") {
            spdlog::set_level(spdlog::level::critical);
        } else {
            spdlog::set_level(spdlog::level::info);
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
    }
}


