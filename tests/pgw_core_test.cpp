#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>
#include "cdr_writer.h"
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


int main() {
    testing::InitGoogleTest();
    spdlog::set_level(spdlog::level::off);
    return RUN_ALL_TESTS();
}
