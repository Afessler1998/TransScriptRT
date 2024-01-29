#ifndef logger_tsrt_h
#define logger_tsrt_h

#include <chrono>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <string>

#include "status_codes_tsrt.h"

/**
 * @brief Initializes the logger.
 * 
 * @return tsrt_status_code
 * 
*/
tsrt_status_code init_logging();

/**
 * @brief Logs an error message.
 * 
 * @param status_code The status code of the error.
 * @param message The error message.
 * @param timestamp The time the error occurred.
 * @param filename The name of the file the error occurred in.
 * @param line_number The line number the error occurred on.
*/
void log_error(tsrt_status_code status_code,
               const std::string& message,
               std::chrono::system_clock::time_point timestamp,
               const std::string& filename,
               int line_number);

/**
 * @brief Logs an info message.
 * 
 * @param message The info message.
 * @param timestamp The time the info message was logged.
 * @param filename The name of the file the info message was logged in.
 * @param line_number The line number the info message was logged on.
*/
void log_info(const std::string& message,
              std::chrono::system_clock::time_point timestamp,
              const std::string& filename,
              int line_number);

#endif