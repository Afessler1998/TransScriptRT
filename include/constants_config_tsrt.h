#ifndef constants_config_tsrt_h
#define constants_config_tsrt_h

#include <cstddef>

// General constants
#define THREAD_SLEEP_MS 5

// Audio constants
constexpr int SAMPLE_RATE = 16000;
constexpr int SEGMENT_DURATION = 50; // milliseconds
constexpr int MS_PER_SEC = 1000;
constexpr float MS_PER_SEC_F = 1000.0f;
constexpr int SAMPLES_PER_SEGMENT = SAMPLE_RATE / MS_PER_SEC * SEGMENT_DURATION; // maintain this ordering of operations to avoid truncation
constexpr int SAMPLES_PER_HALF_SEGMENT = SAMPLES_PER_SEGMENT / 2;

// Ring buffer constants
constexpr size_t AUDIO_BUFFER_SIZE = 16; // half segments of audio, use power of 2 for faster wrap around case

// AVLib filter graph constants
constexpr const char* SRC_SAMPLE_FMT = "flt";
constexpr const char* SRC_CHANNEL_LAYOUT = "mono";
constexpr int BANDPASS_F = 1700;
constexpr int BANDPASS_W = 3100;
constexpr float AFFTDN_NR = 0.3f;
constexpr int AFFTDN_NF = -50;

// Speaker ID constants
constexpr int VOCAL_EMBEDDINGS_SIZE = 512;

#endif