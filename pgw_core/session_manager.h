#pragma once

#include <string>
#include <thread>
#include <unordered_set>

#include "cdr_writer.h"
#include "config.h"

class session_manager {
    std::unordered_set<std::string> blacklist_;
    server_config config_;
    std::mutex mutex_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> sessions_;
    std::unique_ptr<cdr_writer> cdr_writer_;
    std::jthread cleaning_thread_;

    void clean_expired_sessions(const std::stop_token &stop_token);
public:
    explicit session_manager(const server_config& config);
    ~session_manager();

    std::string process_request(const std::string& imsi);
    bool is_session_active(const std::string& imsi);

    void start_cleaning();
    void stop_cleaning();

    void graceful_shutdown();
};
