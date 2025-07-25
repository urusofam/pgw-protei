#pragma once

#include <fstream>

#include "spdlog/spdlog.h"

class cdr_writer {
    std::ofstream file_;
    std::mutex mutex_;

public:
    explicit cdr_writer(const std::string& filename);
    void write(const std::string& imsi, const std::string& action);
};