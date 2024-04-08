#ifndef exceptions_tsrt_h
#define exceptions_tsrt_h

#include "logger_tsrt.h"
#include "status_codes_tsrt.h"

#include <chrono>
#include <exception>
#include <iostream>
#include <map>
#include <typeindex>

/**
 * @brief Represents an exception thrown by the application.
 * 
 * tsrt_exception is the base class for all exceptions thrown by the application.
*/
class Tsrt_Exception : public std::exception {
private:
    tsrt_status_code status_code;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    std::string filename;
    int line_number;
    
public:
    /**
     * @brief Default constructor.
     * 
     * @param status_code The status status_code of the exception.
     * @param message The message of the exception.
     * @param timestamp The timestamp of the exception.
     * @param filename The filename of the exception.
     * @param line_number The line number of the exception.
    */
    Tsrt_Exception(tsrt_status_code status_code,
                const std::string& message,
                const std::chrono::system_clock::time_point& timestamp,
                const std::string& filename,
                int line_number) :
                status_code(status_code),
                message(message),
                timestamp(timestamp),
                filename(filename),
                line_number(line_number) {}

    virtual ~Tsrt_Exception() noexcept = default;
    
    /**
     * @brief Returns the message of the exception.
     * 
     * Overrides base class what() method.
     * 
     * @return The message of the exception.
    */
    virtual const char* what() const noexcept {
        return message.c_str();
    }

    /**
     * @brief Returns the status status_code of the exception.
     * 
     * @return The status status_code of the exception.
    */
    tsrt_status_code get_status_code() const noexcept {
        return status_code;
    }

    /**
     * @brief Returns the timestamp of the exception.
     * 
     * @return The timestamp of the exception.
    */
    std::chrono::system_clock::time_point get_timestamp() const noexcept {
        return timestamp;
    }

    /**
     * @brief Returns the filename of the exception.
     * 
     * @return The filename of the exception.
    */
    std::string get_filename() const noexcept {
        return filename;
    }

    /**
     * @brief Returns the line number of the exception.
     * 
     * @return The line number of the exception.
    */
    int get_line_number() const noexcept {
        return line_number;
    }
};

inline std::map<std::type_index, tsrt_status_code> exception_status_code_map = {
    {typeid(std::bad_alloc), INSUFFICIENT_MEMORY},
    {typeid(std::ios_base::failure), IO_ERROR},
    {typeid(std::runtime_error), RUNTIME_ERROR},
    {typeid(std::out_of_range), OUT_OF_RANGE_ERROR},
    {typeid(std::invalid_argument), INVALID_ARGUMENT},
    {typeid(std::logic_error), CONFIGURATION_ERROR}
};

/**
 * @brief Handles an exception.
 * 
 * Returns the status status_code of the exception.
 * 
 * @tparam ExceptionType The type of the exception.
 * @param e The exception to be handled.
 * @return tsrt_status_code The status status_code of the exception.
*/
template <typename ExceptionType>
tsrt_status_code handle_exception(const ExceptionType& e) {
    auto it = exception_status_code_map.find(typeid(e));
    tsrt_status_code status_code = (it != exception_status_code_map.end()) ? it->second : UNKNOWN_ERROR;
    return status_code;
}

#endif