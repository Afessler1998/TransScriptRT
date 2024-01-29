#include "audio_tsrt.h"
#include "constants_config_tsrt.h"
#include "exceptions_tsrt.h"
#include "logger_tsrt.h"
#include "script_engine_tsrt.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <tbb/scalable_allocator.h>

Script_Engine::Script_Engine() :
    speaker_diarization(false),
    speech_recognition(false),
    speaker_identification(false),
    emotion_recognition(false),
    running(false),
    recording(false),
    speakers(std::vector<Speaker_ID, tbb::scalable_allocator<Speaker_ID>>()),
    audio_buffer(Ring_Buffer<Audio_Segment, true, AUDIO_BUFFER_SIZE>()) {}

void Script_Engine::start_engine() noexcept {
    running = true;
}

void Script_Engine::stop_engine() noexcept {
    running = false;
}

void Script_Engine::start_recording() noexcept {
    recording = true;
}

void Script_Engine::stop_recording() noexcept {
    recording = false;
}

void Script_Engine::push_to_audio_buffer(Audio_Segment&& segment) noexcept {
    audio_buffer.push(std::move(segment));
}

tsrt_status_code Script_Engine::add_speaker(std::string name, float* embedding) {
    try {
        speakers.reserve(speakers.size() + 1);
    } catch (std::bad_alloc) {
        log_error(INSUFFICIENT_MEMORY,
                  "Error allocating memory for speakers embedding",
                  std::chrono::system_clock::now(),
                  __FILE__,
                  __LINE__);
        return INSUFFICIENT_MEMORY;
    }

    auto embedding_copy = std::make_unique<float[]>(VOCAL_EMBEDDINGS_SIZE);
    std::copy(embedding, embedding + VOCAL_EMBEDDINGS_SIZE, embedding_copy.get());

    Speaker_ID speaker{name, std::move(embedding_copy), VOCAL_EMBEDDINGS_SIZE};
    speakers.push_back(speaker);
    return SUCCESS;
}

void Script_Engine::remove_speaker(std::string name) noexcept {
    speakers.erase(std::remove(speakers.begin(), speakers.end(), name), speakers.end());
}

tsrt_status_code Script_Engine::enable_speaker_diarization() noexcept {
    if (speaker_diarization || running)
        return INVALID_OPERATION;
    speaker_diarization = true;
    return SUCCESS;
}

bool Script_Engine::speaker_diarization_enabled() const noexcept {
    return speaker_diarization;
}

tsrt_status_code Script_Engine::enable_speech_recognition() noexcept {
    if (speech_recognition || running)
        return INVALID_OPERATION;
    speech_recognition = true;
    return SUCCESS;
}

bool Script_Engine::speech_recognition_enabled() const noexcept {
    return speech_recognition;
}

tsrt_status_code Script_Engine::enable_speaker_identification() noexcept {
    if (speaker_identification || running)
        return INVALID_OPERATION;
    speaker_identification = true;
    return SUCCESS;
}

bool Script_Engine::speaker_identification_enabled() const noexcept {
    return speaker_identification;
}

tsrt_status_code Script_Engine::enable_emotion_recognition() noexcept {
    if (emotion_recognition || running)
        return INVALID_OPERATION;
    emotion_recognition = true;
    return SUCCESS;
}

bool Script_Engine::emotion_recognition_enabled() const noexcept {
    return emotion_recognition;
}

bool Script_Engine::is_running() const noexcept {
    return running;
}

bool Script_Engine::is_recording() const noexcept {
    return recording;
}

const std::vector<Speaker_ID, tbb::scalable_allocator<Speaker_ID>>& Script_Engine::get_speakers() const noexcept {
    return speakers;
}

Script_Engine& Script_Engine::get_instance() noexcept {
    static Script_Engine instance;
    return instance;
}