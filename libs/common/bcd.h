#pragma once

#include "spdlog/spdlog.h"

std::vector<uint8_t> imsi_to_bcd(const std::string& imsi);
std::string bcd_to_imsi(const std::vector<uint8_t>& bcd);