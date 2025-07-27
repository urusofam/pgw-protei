#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

#include "bcd.h"
#include "logger.h"

// Тесты bcd

// Пустой imsi
TEST(bcd_test, empty_imsi) {
    ASSERT_THROW(imsi_to_bcd(""), std::invalid_argument);
}

// Валидная ситуация
TEST(bcd_test, correct_situation) {
    const std::vector<uint8_t> bcd = imsi_to_bcd("346051602396626");
    const std::string imsi = bcd_to_imsi(bcd);
    ASSERT_EQ(imsi, "346051602396626");
}

// Не только числа в imsi
TEST(bcd_test, not_a_number_in_imsi) {
    ASSERT_THROW(imsi_to_bcd("34605160239662x"), std::invalid_argument);
}

// Тесты настройки логгера
class logger_test : public ::testing::Test {
protected:
    std::string log_filename = "test_log.txt";

    void TearDown() override {
        std::filesystem::remove_all("logs");
    }
};

// Валидная ситуация
TEST_F(logger_test, correct_situation) {
    ASSERT_NO_THROW(setup_logger(log_filename, "info"));

    // Проверяем, что логгер был установлен
    auto logger = spdlog::default_logger();
    EXPECT_NE(logger, nullptr);

    // Проверяем уровень логирования
    EXPECT_EQ(spdlog::get_level(), spdlog::level::info);

    // Проверяем, что файл был создан
    EXPECT_TRUE(std::filesystem::exists("logs/" + log_filename));

    // Записываем сообщение в лог
    spdlog::info("Test message");
    spdlog::default_logger()->flush();

    // Проверяем, что в файле есть содержимое
    std::ifstream log_file("logs/" + log_filename);
    std::string content;
    std::getline(log_file, content);
    EXPECT_TRUE(content.contains("Test message"));
}

// Двойной вызов настройки логгера
TEST_F(logger_test, twice_setup_logger_call) {
    ASSERT_NO_THROW(setup_logger(log_filename, "info"));
    ASSERT_NO_THROW(setup_logger(log_filename, "error"));

    // Проверяем, что последний вызов определяет настройки
    EXPECT_EQ(spdlog::get_level(), spdlog::level::err);
}

// Несуществующий уровень логирования
TEST_F(logger_test, invalid_log_level) {
    ASSERT_THROW(setup_logger("test.log", "invalid"), std::invalid_argument);
}


int main() {
    testing::InitGoogleTest();
    spdlog::set_level(spdlog::level::off);
    return RUN_ALL_TESTS();
}