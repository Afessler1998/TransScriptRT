#ifndef ring_buffer_tsrt_h
#define ring_buffer_tsrt_h

#include "audio_segment_tsrt.h"
#include "constants_config_tsrt.h"

#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <type_traits>

/**
 * @brief Checks if the given size is a power of two.
 * 
 * @details This function is used for compile time checking if the buffer size is a power of two for the bitwise & wrap around optimization.
 * 
 * @param Size The size to check.
 * @return True if the size is a power of two, false otherwise.
*/
constexpr bool is_power_of_two(size_t Size) noexcept {
    return Size != 0 && (Size & (Size - 1)) == 0;
}

/**
 * @brief A ring buffer that can be used to store audio segments.
 * 
 * @details This class is a ring buffer that can be used to store audio segments. It can be used in a thread safe manner or not.
 * @details If the buffer size is a power of two, it can benefit the bitwise and wrap around optimization.
 * 
 * @tparam T The type of the elements stored in the buffer.
 * @tparam ThreadSafe Whether or not the buffer should be thread safe.
 * @tparam Size The size of the buffer. Must be greater than 0. Used to compile time check for size being power of 2.
 */
template <typename T, bool ThreadSafe = false, size_t Size = 1>
class Ring_Buffer {
private:
    std::unique_ptr<T[]> buffer;
    size_t head;
    size_t tail;
    std::conditional_t<ThreadSafe, std::mutex, std::nullptr_t> mutex;

    static_assert(Size > 0, "Ring buffer size must be greater than 0");

    /**
     * @brief Wraps the given index around the buffer.
     * 
     * @details If the buffer size is a power of two, it uses the bitwise and optimization.
     * 
     * @param index The index to wrap around the buffer.
     * @return The wrapped index.
    */
    size_t wrap_index(const size_t index) const {
        if constexpr (is_power_of_two(Size))
            return index & (Size - 1);
        else
            return index % Size;
    }

public:
    Ring_Buffer() noexcept : head(0), tail(0) {
        buffer = std::make_unique<T[]>(Size);
    }

    // Move constructor, do not move the mutex
    Ring_Buffer(Ring_Buffer&& other) noexcept 
    : buffer(std::move(other.buffer)), head(other.head), tail(other.tail) {}

    // Move assignment, do not move the mutex
    Ring_Buffer& operator=(Ring_Buffer&& other) noexcept {
        if (this != &other) {
            buffer = std::move(other.buffer);
            head = other.head;
            tail = other.tail;
        }
        return *this;
    }

    // Copy constructor and assignment operator are deleted because the mutex cannot be copied
    Ring_Buffer(const Ring_Buffer&) = delete;
    Ring_Buffer& operator=(const Ring_Buffer&) = delete;

    /**
     * @brief Pushes the given value to the ring buffer.
     * 
     * This function checks if the data type is trivially copyable. If it is, it uses memcpy to copy the value to the buffer.
     * Otherwise, it uses the move constructor to move the value to the buffer.
     * 
     * @param value The value to push to the ring buffer.
    */
    void push(T value) noexcept {
        if constexpr (ThreadSafe)
            std::lock_guard<std::mutex> lock(mutex);

        // Lazy initialize the audio segment because size is not known at compile time
        // and pushing calls the default constructor, so no way to pass the size directly
        if constexpr (std::is_same<T, Audio_Segment>::value)
            buffer[head].lazy_initialize(value.get_size());

        if constexpr (std::is_trivially_copyable<T>::value)
            std::memcpy(&buffer[head], &value, sizeof(T));
        else
            buffer[head] = std::move(value);

        head = wrap_index(head + 1);
        if (head == tail)
            tail = wrap_index(tail + 1);
    }

    /**
     * @brief Pops the first value from the ring buffer.
     * 
     * @return The first value from the ring buffer.
    */
    std::optional<T> pop() noexcept {
        if constexpr (ThreadSafe)
            std::lock_guard<std::mutex> lock(mutex);

        if (head == tail)
            return std::nullopt;

        T value = std::move(buffer[tail]);
        tail = wrap_index(tail + 1);
        return value;
    }

    /**
     * @brief Gets the size of the ring buffer.
     * 
     * @return The size of the ring buffer.
    */
    size_t get_size() const noexcept {
        return Size;
    }

    /**
     * @brief Resets the head and tail ptrs to 0.
     * 
     * @details This function resets the head and tail ptrs to 0. It does not clear the data in the buffer.
     *
     * @note This function is not thread safe.
    */
    void clear() noexcept {
        head = 0;
        tail = 0;
    }
};

template class Ring_Buffer<std::chrono::system_clock::time_point, false, AUDIO_BUFFER_SIZE>;
template class Ring_Buffer<Audio_Segment, true, AUDIO_BUFFER_SIZE>;

#endif // ring_buffer_tsrt_h
