#ifndef status_codes_tsrt_h
#define status_codes_tsrt_h

#include <string>

/**
 * @brief Status codes for the tsrt library
 * 
 * The status codes are used to indicate the result of a function call.
*/
enum tsrt_status_code {
    SUCCESS,
    INSUFFICIENT_MEMORY,
    IO_ERROR,
    INVALID_ARGUMENT,
    CONFIGURATION_ERROR,
    RUNTIME_ERROR,
    OUT_OF_RANGE_ERROR,
    TRY_AGAIN,
    INVALID_OPERATION,
    UNKNOWN_ERROR,
    STATUS_CODE_COUNT,
};

/**
 * @brief Converts a tsrt_status_code to a string
 * 
 * @param status_code The tsrt_status_code to convert
 * @return std::string The string representation of the tsrt_status_code
*/
inline std::string status_code_to_string(tsrt_status_code status_code) {   
    switch (status_code) {
        case SUCCESS:
            return "SUCCESS";
        case INSUFFICIENT_MEMORY:
            return "INSUFFICIENT_MEMORY";
        case IO_ERROR:
            return "IO_ERROR";
        case INVALID_ARGUMENT:
            return "INVALID_ARGUMENT";
        case CONFIGURATION_ERROR:
            return "CONFIGURATION_ERROR";
        case RUNTIME_ERROR:
            return "RUNTIME_ERROR";
        case OUT_OF_RANGE_ERROR:
            return "OUT_OF_RANGE_ERROR";
        case TRY_AGAIN:
            return "TRY_AGAIN";
        case INVALID_OPERATION:
            return "INVALID_OPERATION";
        default:
            return "UNKNOWN_ERROR";
    }
}

#endif