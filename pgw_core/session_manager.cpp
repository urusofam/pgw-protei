#include "session_manager.h"

#include "spdlog/spdlog.h"

// Конструктор для session_manager
session_manager::session_manager(const server_config &config) {
    spdlog::debug("session_manager конструктор. Начало функции");

    config_ = config;
    cdr_writer_ = std::make_unique<cdr_writer>(config.cdr_file);
    blacklist_ = {config_.blacklist.begin(), config_.blacklist.end()};

    spdlog::info("session_manager проинициализирован, в блэклисте {} абонентов", blacklist_.size());
    spdlog::debug("session manager конструктор. Конец функции");
}

// Деструктор для session_manager
session_manager::~session_manager() {
    spdlog::debug("session_manager деструктор. Начало функции");

    // jthread автоматически запросит остановку через stop_token
    // и дождется завершения потока при уничтожении

    spdlog::debug("session_manager деструктор. Конец функции");
}

// Запуск потока, который будет завершать сессии по таймеру
void session_manager::start_cleaning() {
    spdlog::debug("start_cleaning. Начало функции");

    if (cleaning_thread_.joinable()) {
        spdlog::warn("start_cleaning. Повторное начало чистки сессий");
        spdlog::debug("start_cleaning. Конец функции");
        return;
    }

    cleaning_thread_ = std::jthread(&session_manager::clean_expired_sessions, this);

    spdlog::info("Очистка сессий в потоке началась");
    spdlog::debug("start_cleaning. Конец функции");
}

// Остановка потока для чистки
void session_manager::stop_cleaning() {
    spdlog::debug("stop_cleaning. Начало функции");

    if (cleaning_thread_.joinable()) {
        cleaning_thread_.request_stop(); // Запрашиваем остановку
        cleaning_thread_.join();         // Ждем завершения
    } else {
        spdlog::warn("Остановка неработающего потока чистки");
        spdlog::debug("stop_cleaning. Конец функции");
        return;
    }

    spdlog::info("Очистка сессий в потоке завершилась");
    spdlog::debug("stop_cleaning. Конец функции");
}

// Обработка запроса на создание сессии
std::string session_manager::process_request(const std::string &imsi) {
    spdlog::info("Получен запрос на создание сессии от imsi {}", imsi);

    // Если imsi в блэклисте
    if (blacklist_.contains(imsi)) {
        spdlog::info("imsi {} в блэклисте, запрос отклонён", imsi);
        return "rejected";
    }

    std::lock_guard lock(mutex_);
    // Если уже создана сессия
    if (sessions_.contains(imsi)) {
        spdlog::info("Сессия с imsi {} уже существует", imsi);
        return "rejected";
    }

    // Новая сессия
    sessions_[imsi] = std::chrono::steady_clock::now();
    spdlog::info("Новая сессия с imsi {} создана", imsi);
    cdr_writer_->write(imsi, "Сессия создана");
    return "created";
}

// Проверка на существование сессии
bool session_manager::is_session_active(const std::string &imsi) {
    spdlog::info("Пришёл запрос на проверку существовании сессии с imsi {}", imsi);

    std::lock_guard lock(mutex_);
    if (sessions_.contains(imsi)) {
        spdlog::info("Сессия с imsi {} существует", imsi);
        return true;
    }

    spdlog::info("Сессия с imsi {} не существует", imsi);
    return false;

}

// Очистка устаревших сессий
void session_manager::clean_expired_sessions(const std::stop_token &stop_token) {
    spdlog::debug("clean_expired_sessions. Начало функции");

    while (!stop_token.stop_requested()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Проверяем остановку чистки после засыпания потока
        if (stop_token.stop_requested()) {
            break;
        }

        // Ищем устаревшие сессии
        {
            std::lock_guard lock(mutex_);
            auto now = std::chrono::steady_clock::now();

            for (auto it = sessions_.begin(); it != sessions_.end();) {
                auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - it->second);
                if (duration.count() > config_.session_timeout_sec) {
                    spdlog::info("Сессия с imsi {} устарела и была удалена", it->first);
                    cdr_writer_->write(it->first, "Сессия закрыта по времени");
                    it = sessions_.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    spdlog::debug("clean_expired_sessions. Конец функции");
}

// Остановка session_manager
void session_manager::graceful_shutdown() {
    spdlog::info("Начинаем остановку session_manager...");
    stop_cleaning();

    // Получаем список сессий для cdr и очищаем sessions_
    std::vector<std::string> sessions_to_close;
    {
        std::lock_guard lock(mutex_);
        if (sessions_.empty()) {
            spdlog::info("Нет активных сессий для закрытия");
            return;
        }

        for (const auto &imsi: sessions_ | std::views::keys) {
            sessions_to_close.push_back(imsi);
        }
        sessions_.clear();
    }

    spdlog::info("Закрываем {} активных сессий со скоростью {} сессий в секунду...",
                sessions_to_close.size(), config_.graceful_shutdown_rate);
    const auto delay = std::chrono::milliseconds(1000 / config_.graceful_shutdown_rate);

    // Записываем CDR
    for (const auto& imsi : sessions_to_close) {
        spdlog::info("Сессия с imsi {} закрыта", imsi);
        cdr_writer_->write(imsi, "Сессия закрыта по выключению");
        std::this_thread::sleep_for(delay);
    }

    spdlog::info("session_manager остановлен");
}




