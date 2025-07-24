#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>
#include "cdr_writer.h"
#include "config.h"
#include "session_manager.h"
#include "spdlog/spdlog.h"

// Тесты для cdr_writer
class cdr_writer_test : public ::testing::Test {
protected:
    std::string test_filename = "test_cdr.csv";

    void TearDown() override {
        if (std::filesystem::exists(test_filename)) {
            std::filesystem::remove(test_filename);
        }
    }

    static std::string read_file(const std::string& filename) {
        std::ifstream file(filename);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
};

// Создание файла
TEST_F(cdr_writer_test, constructor_create_file) {
    cdr_writer writer(test_filename);
    ASSERT_TRUE(std::filesystem::exists(test_filename));
}

// Неправильный путь
TEST_F(cdr_writer_test, invalid_path) {
    ASSERT_THROW(cdr_writer("/invalid/path/to/cdr.csv"), std::runtime_error);
}

// Базовая запись
TEST_F(cdr_writer_test, write_create_correct_record) {
    std::string imsi = "123456789012345";
    std::string action = "Сессия создана";

    // Пишем imsi и action
    cdr_writer writer(test_filename);
    writer.write(imsi, action);

    // Читаем файл
    std::string content = read_file(test_filename);

    // Проверяем наличие IMSI и action в файле
    EXPECT_TRUE(content.contains(imsi));
    EXPECT_TRUE(content.contains(action));

    // Проверяем формат записи (дата,imsi,action)
    EXPECT_TRUE(content.contains("," + imsi + "," + action));
}

// Несколько записей
TEST_F(cdr_writer_test, write_multiple) {
    cdr_writer writer(test_filename);
    writer.write("111111111111111", "Сессия создана");
    writer.write("222222222222222", "Сессия закрыта по времени");
    writer.write("333333333333333", "Сессия создана по выключению");

    std::string content = read_file(test_filename);

    EXPECT_TRUE(content.contains("111111111111111"));
    EXPECT_TRUE(content.contains("222222222222222"));
    EXPECT_TRUE(content.contains("333333333333333"));
    EXPECT_TRUE(content.contains("Сессия создана"));
    EXPECT_TRUE(content.contains("Сессия закрыта по времен"));
    EXPECT_TRUE(content.contains("Сессия создана по выключению"));

    // Проверяем количество строк
    size_t line_count = std::ranges::count(content, '\n');
    EXPECT_EQ(line_count, 3);
}

// Конкурентная запись
TEST_F(cdr_writer_test, write_concurrent) {
    constexpr int threads_count = 10;
    constexpr int writes_per_thread = 100;

    cdr_writer writer(test_filename);
    std::vector<std::thread> threads;

    // Запускаем несколько потоков, которые пишут одновременно
    for (int i = 0; i < threads_count; ++i) {
        threads.emplace_back([&writer, i, writes_per_thread]() {
            for (int j = 0; j < writes_per_thread; ++j) {
                writer.write(
                "imsi_" + std::to_string(i) + "_" + std::to_string(j),
                "сессия_" + std::to_string(i) + "_" + std::to_string(j)
                );
            }
        });
    }

    // Ждем завершения всех потоков
    for (auto& t : threads) {
        t.join();
    }

    size_t line_count = std::ranges::count(read_file(test_filename), '\n');
    ASSERT_EQ(line_count, threads_count * writes_per_thread);
}

// Запись из двух разных писателей в один файл
TEST_F(cdr_writer_test, different_writers) {
    std::string first_imsi = "111111111111111";
    std::string second_imsi = "222222222222222";

    // Первая запись в первом писателе
    cdr_writer writer(test_filename);
    writer.write(first_imsi, "Сессия создана");

    // Вторая запись во втором писателе
    cdr_writer writer2(test_filename);
    writer2.write(second_imsi, "Сессия создана");

    std::string content = read_file(test_filename);

    // Проверяем, что обе записи присутствуют
    EXPECT_TRUE(content.contains(first_imsi));
    EXPECT_TRUE(content.contains(second_imsi));
    EXPECT_TRUE(content.contains("Сессия создана"));

    size_t line_count = std::ranges::count(content, '\n');
    EXPECT_EQ(line_count, 2);
}

// Пустые строки
TEST_F(cdr_writer_test, empty_strings) {
    cdr_writer writer(test_filename);
    writer.write("", "");
    writer.write("imsi", "");
    writer.write("", "action");

    std::string content = read_file(test_filename);

    EXPECT_TRUE(content.contains("imsi"));
    EXPECT_TRUE(content.contains("action"));

    size_t line_count = std::ranges::count(content, '\n');
    ASSERT_EQ(line_count, 3);
}

// Тесты session_managerа
class session_manager_test : public ::testing::Test {
protected:
    server_config config;
    std::unique_ptr<session_manager> manager;

    void SetUp() override {
        config.cdr_file = "test_cdr.csv";
        config.session_timeout_sec = 1;
        config.graceful_shutdown_rate = 100;
        config.blacklist = {"999999999999999"};

        manager = std::make_unique<session_manager>(config);
    }

    void TearDown() override {
        if (std::filesystem::exists(config.cdr_file)) {
            std::filesystem::remove(config.cdr_file);
        }
    }
};

// Создание новой сессии
TEST_F(session_manager_test, create_new_session) {
    std::string imsi = "123456789012345";

    // Обработка запроса и получение ответа
    std::string result = manager->process_request(imsi);

    // Проверка ответа и создания сессии
    EXPECT_EQ(result, "created");
    EXPECT_TRUE(manager->is_session_active(imsi));
}

// Отклонение запроса уже созданной сессии
TEST_F(session_manager_test, reject_duplicate_session) {
    std::string imsi = "123456789012345";

    // Создаем первую сессию
    std::string result1 = manager->process_request(imsi);
    EXPECT_EQ(result1, "created");

    // Пытаемся создать дубликат
    std::string result2 = manager->process_request(imsi);
    EXPECT_EQ(result2, "rejected");
}

// Отклонение запроса от imsi в блэклисте
TEST_F(session_manager_test, reject_blacklist_imsi) {
    std::string blacklisted_imsi = "999999999999999";

    std::string result = manager->process_request(blacklisted_imsi);

    EXPECT_EQ(result, "rejected");
    EXPECT_FALSE(manager->is_session_active(blacklisted_imsi));
}

// Проверка активности сессии
TEST_F(session_manager_test, session_not_active) {
    std::string imsi = "111111111111111";

    ASSERT_FALSE(manager->is_session_active(imsi));
}

// Удаление сессии по таймеру
TEST_F(session_manager_test, session_deleted_by_time) {
    std::string imsi = "123456789012345";

    // Создаем сессию
    manager->process_request(imsi);
    EXPECT_TRUE(manager->is_session_active(imsi));

    // Запускаем очистку
    manager->start_cleaning();

    // Ждем больше времени таймаута
    std::this_thread::sleep_for(std::chrono::milliseconds(2100));
    // Проверяем, что сессия удалена
    EXPECT_FALSE(manager->is_session_active(imsi));

    // Останавливаем очистку
    manager->stop_cleaning();
}

// Удаление нескольких сессий по таймеру
TEST_F(session_manager_test, multiple_session_deleted_by_time) {
    std::vector<std::string> imsis = {
        "111111111111111",
        "222222222222222",
        "333333333333333"
    };

    // Создаем несколько сессий
    for (const std::string& imsi : imsis) {
        manager->process_request(imsi);
        EXPECT_TRUE(manager->is_session_active(imsi));
    }

    manager->start_cleaning();

    // Ждем истечения таймаута
    std::this_thread::sleep_for(std::chrono::milliseconds(2100));

    // Проверяем, что все сессии удалены
    for (const std::string& imsi : imsis) {
        EXPECT_FALSE(manager->is_session_active(imsi));
    }

    manager->stop_cleaning();
}

// Выключение
TEST_F(session_manager_test, graceful_shutdown) {
    std::vector<std::string> imsis = {
        "111111111111111",
        "222222222222222",
        "333333333333333",
        "444444444444444",
        "555555555555555"
    };

    // Создаем сессии
    for (const std::string& imsi : imsis) {
        manager->process_request(imsi);
    }

    // Выполняем graceful shutdown
    auto start = std::chrono::steady_clock::now();
    manager->graceful_shutdown();
    auto end = std::chrono::steady_clock::now();

    // Проверяем, что все сессии закрыты
    for (const std::string& imsi : imsis) {
        EXPECT_FALSE(manager->is_session_active(imsi));
    }

    // Проверяем, что shutdown занял примерно правильное время
    // (5 сессий при rate=100/сек должны закрыться примерно за 50мс)
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_GE(duration.count(), 40); // Не менее 40мс
    EXPECT_LE(duration.count(), 60); // Не более 60мс
}

// Конкурентное создание сессий
TEST_F(session_manager_test, concurrent_session_creation) {
    constexpr int threads_count = 10;
    constexpr int sessions_per_thread = 100;
    std::atomic created_count{0};

    // Запускаем потоки для создания сессий
    std::vector<std::thread> threads;
    for (int i = 0; i < threads_count; ++i) {
        threads.emplace_back([this, i, sessions_per_thread, &created_count]() {
            for (int j = 0; j < sessions_per_thread; ++j) {
                std::string imsi = "IMSI_" + std::to_string(i) + "_" + std::to_string(j);
                std::string result = manager->process_request(imsi);

                if (result == "created") {
                    ++created_count;
                }
            }
        });
    }

    // Ждем завершения всех потоков
    for (auto& t : threads) {
        t.join();
    }

    // Все уникальные IMSI должны быть созданы
    EXPECT_EQ(created_count, threads_count * sessions_per_thread);
}

// Начало чистки дважды
TEST_F(session_manager_test, start_cleaning_twice) {
    manager->start_cleaning();

    // Второй запуск не должен ломать программу
    manager->start_cleaning();

    manager->stop_cleaning();
}

// Остановка очистки без начала
TEST_F(session_manager_test, stop_cleaning_without_start) {
    // Должно работать без ошибок, даже если очистка не была запущена
    manager->stop_cleaning();
}


int main() {
    testing::InitGoogleTest();
    spdlog::set_level(spdlog::level::off);
    return RUN_ALL_TESTS();
}
