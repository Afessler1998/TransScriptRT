#ifndef audio_segment_tsrt_h
#define audio_segment_tsrt_h

#include "constants_config_tsrt.h"
#include "exceptions_tsrt.h"
#include "status_codes_tsrt.h"

#include <chrono>
#include <memory>

/**
 * @brief Represents an audio segment.
 * 
 * Audio_Segment is a wrapper for a float array of audio samples. It also contains a timestamp.
 * 
 * @param audio A float array of audio samples.
 * @param midpoint A pointer to the midpoint of the audio array.
 * @param timestamp The timestamp of the segment.
 * @param size The size of the audio array.
*/
class Audio_Segment {

private:
    std::unique_ptr<float[]> audio;
    float* midpoint;
    std::chrono::time_point<std::chrono::system_clock> timestamp;
    size_t size;

    // Private swap method
    void swap(Audio_Segment& other) noexcept {
        std::swap(audio, other.audio);
        std::swap(timestamp, other.timestamp);
        std::swap(size, other.size);
    }

public:

    Audio_Segment() : audio(nullptr), midpoint(nullptr), timestamp(std::chrono::system_clock::now()), size(0) {}

    Audio_Segment(size_t size) : audio(std::make_unique<float[]>(size)), midpoint(audio.get() + size / 2), timestamp(std::chrono::system_clock::now()), size(size) {}

    // Copy constructor
    Audio_Segment(const Audio_Segment& other) : 
        audio(std::make_unique<float[]>(other.size)),
        midpoint(audio.get() + other.size / 2),
        timestamp(other.timestamp),
        size(other.size) {
        std::copy(other.audio.get(), other.audio.get() + other.size, audio.get());
    }

    // Move constructor
    Audio_Segment(Audio_Segment&& other) noexcept : 
        audio(std::move(other.audio)),
        midpoint(other.midpoint),
        timestamp(other.timestamp),
        size(other.size) {
    }

    // Copy assignment operator
    Audio_Segment& operator=(const Audio_Segment& other) {
        if (this != &other) {
            Audio_Segment temp(other); // Use copy constructor
            swap(temp);
        }
        return *this;
    }

    // Move assignment operator
    Audio_Segment& operator=(Audio_Segment&& other) noexcept {
        swap(other);
        return *this;
    }

    // Equality comparison operator
    bool operator==(const Audio_Segment& other) const noexcept {
        return this->timestamp == other.timestamp;
    }

    void reset_audio() noexcept {
        audio = std::make_unique<float[]>(size);
        midpoint = audio.get() + size / 2;
    }

    float* get_audio() const noexcept {
        return audio.get();
    }

    float* get_midpoint() const noexcept {
        return midpoint;
    }

    void set_timestamp(std::chrono::time_point<std::chrono::system_clock> timestamp) noexcept {
        this->timestamp = timestamp;
    }

    std::chrono::time_point<std::chrono::system_clock> get_timestamp() const noexcept {
        return timestamp;
    }

    size_t get_size() const noexcept {
        return size;
    }

    void lazy_initialize(size_t size) {
        if (size == 0)
            throw Tsrt_Exception(INVALID_ARGUMENT, "Audio segment size must be greater than 0.", std::chrono::system_clock::now(), __FILE__, __LINE__);
        audio = std::make_unique<float[]>(size);
        midpoint = audio.get() + size / 2;
        timestamp = std::chrono::system_clock::now();
        this->size = size;
    }
};

#endif