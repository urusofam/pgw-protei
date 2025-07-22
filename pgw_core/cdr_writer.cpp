#include "cdr_writer.h"
#include "spdlog/spdlog.h"

// Констркутор писателя cdr
cdr_writer::cdr_writer(const std::string &filename) {
    spdlog::debug("cdr_writer конструктор: Начало функции");

    file_.open(filename, std::ios::app);
    if (!file_.is_open()) {
        spdlog::critical("Не удалось открыть cdr файл: {}", filename);
        throw std::runtime_error("Не удалось открыть cdr файл:" + filename);
    }

    spdlog::debug("cdr_writer конструктор: Конец функции");
}

// Запись в cdr
void cdr_writer::write(const std::string &imsi, const std::string &action) {
    spdlog::debug("Запись cdr_writer, imsi: {}, action: {}: Начало функции", imsi, action);

    std::lock_guard lock(mutex_);
    file_ << std::format("{:%Y-%m-%d %H:%M:%S}", std::chrono::system_clock::now()) << ','
    << imsi << ',' << action << '\n';

    spdlog::debug("Запись cdr_writer, imsi: {}, action: {}: Конец функции", imsi, action);
}

