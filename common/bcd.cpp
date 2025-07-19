#include "bcd.h"

#include <stdexcept>
#include <spdlog/spdlog.h>

// Перевод из imsi в bcd
std::vector<uint8_t> imsi_to_bcd(const std::string& imsi) {
    if (imsi.empty()) {
        spdlog::error("imsi_to_bcd: Imsi пустой");
        throw std::invalid_argument("Imsi пустой");
    }

    std::vector<uint8_t> bcd;
    std::string temp_imsi = imsi;

    // Дополняем до чётности
    if (imsi.length() % 2 != 0) {
        temp_imsi += 'F';
    }

    // Кодируем цифры попарно
    for (size_t i = 0; i < temp_imsi.length(); i += 2) {
        uint8_t byte = 0;

        if (!isxdigit(temp_imsi[i]) || !isxdigit(temp_imsi[i + 1])) {
            spdlog::error("imsi_to_bcd: В imsi должны быть только цифры");
            throw std::invalid_argument("В imsi должны быть только цифры");
        }

        // Первая цифра
        byte |= temp_imsi[i] - '0';

        // Вторая цифра
        byte |= temp_imsi[i + 1] >= 'A' ? 0xF0 : (temp_imsi[i + 1] - '0') << 4;

        bcd.push_back(byte);
    }

    return bcd;
}

// Перевод из bcd в imsi
std::string bcd_to_imsi(const std::vector<uint8_t>& bcd) {
    std::string imsi;

    // Декодируем байты
    for (size_t i = 0; i < bcd.size(); i++) {
        uint8_t byte = bcd[i];

        // Первая цифра
        imsi += '0' + (byte & 0xF);

        // Вторая цифра
        uint8_t value = byte >> 4 & 0xF;
        // F означает конец (для нечетной длины)
        if (value == 0xF) {
            break;
        }
        imsi += '0' + value;
    }

    return imsi;
}
