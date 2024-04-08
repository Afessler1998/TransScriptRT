#include "logger_tsrt.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>
#include <vector>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <windows.h>
#elif __unix__
#include <unistd.h>
#include <limits.h>
#endif


/**
 * @brief Returns the path to the directory containing the executable.
 * 
 * @return std::string
*/
static std::string get_executable_path() {
    char buffer[PATH_MAX];
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    DWORD copied = 0;
    do {
        buffer[copied] = '\0';
        copied = GetModuleFileNameA(NULL, reinterpret_cast<LPSTR>(buffer), MAX_PATH);
        if (copied == 0) {
            throw std::runtime_error("GetModuleFileName failed with error " + std::to_string(GetLastError()));
        }
    } while (copied >= MAX_PATH);
#elif __unix__
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len == -1) {
        throw std::runtime_error("Unable to read symlink /proc/self/exe");
    }
    buffer[len] = '\0';
#endif
    return std::filesystem::path(buffer).parent_path().string();
}


/**
 * @brief Returns the path to the log file.
 * 
 * @return std::string
*/
static std::string get_log_file_path() {
    std::string exe_dir = get_executable_path();
    std::filesystem::path log_path = std::filesystem::path(exe_dir) / "../../logs/log.txt";
    log_path = log_path.lexically_normal();
    return log_path.string();
}

tsrt_status_code init_logging() {
    try {
        auto log_file_path = get_log_file_path();
        auto logger = spdlog::basic_logger_mt("basic_logger", log_file_path);
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
        logger->set_level(spdlog::level::trace);
        spdlog::flush_every(std::chrono::seconds(1));
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
        return CONFIGURATION_ERROR;
    }
    return SUCCESS;
}

void log_error(tsrt_status_code status_code,
               const std::string& message,
               std::chrono::system_clock::time_point timestamp,
               const std::string& filename,
               int line_number) {

    if (status_code < 0 || status_code >= STATUS_CODE_COUNT ||
        message.empty() || filename.empty() || line_number < 0) {
        std::cerr << "Invalid argument(s) passed to log_error." << std::endl;
        return;
    }

    std::string status_string = status_code_to_string(status_code);

    std::time_t time = std::chrono::system_clock::to_time_t(timestamp);
    struct tm timeinfo;
    // if windows, use localtime_s, else use localtime_r
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    localtime_s(&timeinfo, &time);
#elif __unix__
    localtime_r(&time, &timeinfo);
#endif
    std::stringstream ss;
    ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
    std::string formatted_time = ss.str();

    static std::shared_ptr<spdlog::logger> logger = spdlog::get("basic_logger");
    if (logger == nullptr) {
        std::cerr << "Error " << status_string << ": " << message << ", occurred at "
        << formatted_time << " in " << filename << ":" << line_number << std::endl;
        return;
    }

    logger->error("Error {}: {}, occurred at {} in {}:{}",
                  status_string, message, formatted_time, filename, line_number);
}

void log_info(const std::string& message,
              std::chrono::system_clock::time_point timestamp,
              const std::string& filename,
              int line_number) {

    if (message.empty() || filename.empty() || line_number < 0) {
        std::cerr << "Invalid argument(s) passed to log_info." << std::endl;
        return;
    }

    std::time_t time = std::chrono::system_clock::to_time_t(timestamp);
    struct tm timeinfo;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    localtime_s(&timeinfo, &time);
#elif __unix__
    localtime_r(&time, &timeinfo);
#endif
    std::stringstream ss;
    ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
    std::string formatted_time = ss.str();

    static std::shared_ptr<spdlog::logger> logger = spdlog::get("basic_logger");
    if (logger == nullptr) {
        std::cout << "Info: " << message << ", logged at "
        << formatted_time << " in " << filename << ":" << line_number << std::endl;
        return;
    }

    logger->info("Info: {}, logged at {} in {}:{}",
                 message, formatted_time, filename, line_number);
}