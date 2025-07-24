#include "bcd.h"

#include <spdlog/spdlog.h>

// Перевод из imsi в bcd
std::vector<uint8_t> imsi_to_bcd(const std::string& imsi) {
    spdlog::debug("imsi_to_bcd, imsi: {}. Начало функции", imsi);
    if (imsi.empty()) {
        spdlog::critical("imsi_to_bcd. imsi пустой");
        throw std::invalid_argument("imsi пустой");
    }

    std::vector<uint8_t> bcd;
    std::string updated_imsi = imsi;

    // Дополняем до чётности
    if (imsi.length() % 2 != 0) {
        updated_imsi += 'F';
        spdlog::debug("imsi_to_bcd, imsi: {}. Дополнили imsi до чётности", imsi);
    }

    // Кодируем цифры попарно
    spdlog::debug("imsi_to_bcd, imsi: {}. Начало кодировки", imsi);
    for (size_t i = 0; i < updated_imsi.length(); i += 2) {
        uint8_t byte = 0;


        if ((!isdigit(updated_imsi[i]) && updated_imsi[i] != 'F')
            || (!isdigit(updated_imsi[i + 1]) && updated_imsi[i + 1] != 'F')) {
            spdlog::critical("imsi_to_bcd, imsi: {}. В imsi должны быть только цифры");
            throw std::invalid_argument("В imsi должны быть только цифры");
        }

        // Первая цифра
        byte |= updated_imsi[i] - '0';

        // Вторая цифра
        byte |= updated_imsi[i + 1] >= 'A' ? 0xF0 : (updated_imsi[i + 1] - '0') << 4;

        bcd.push_back(byte);
    }

    spdlog::debug("imsi_to_bcd, imsi: {}. Конец кодировки и функции", imsi);
    return bcd;
}

// Перевод из bcd в imsi
std::string bcd_to_imsi(const std::vector<uint8_t>& bcd) {
    spdlog::debug("bcd_to_imsi. Начало функции");
    std::string imsi;

    // Декодируем байты
    spdlog::debug("bcd_to_imsi. Начало декодировки");
    for (const unsigned char& byte : bcd) {
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

    spdlog::debug("bcd_to_imsi. Конец декодировки и функции, получившийся imsi: {}", imsi);
    return imsi;
}
