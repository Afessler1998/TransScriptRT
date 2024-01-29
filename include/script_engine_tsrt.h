#ifndef script_engine_tsrt_h
#define script_engine_tsrt_h

#include "audio_tsrt.h"
#include "audio_segment_tsrt.h"
#include "constants_config_tsrt.h"
#include "exceptions_tsrt.h"
#include "ring_buffer_tsrt.h"
#include "speaker_id_tsrt.h"
#include "status_codes_tsrt.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <tbb/tbb.h>
#include <tbb/scalable_allocator.h>
#include <vector>

/**
 * @brief Represents the main engine of the application.
 *
 * Script_Engine manages the state of the engine, including audio ring buffer
 * and speakers vector. The audio ring buffer stores audio segments for analysis,
 * and the speakers vector stores speakers for identification.
 *
 * @param speech_recognition Flag indicating whether speech recognition is enabled.
 * @param speaker_diarization Flag indicating whether speaker diarization is enabled.
 * @param speaker_identification Flag indicating whether speaker identification is enabled.
 * @param emotion_recognition Flag indicating whether emotion recognition is enabled.
 */
class Script_Engine {

private:
    bool speaker_diarization;
    bool speech_recognition;
    bool speaker_identification;
    bool emotion_recognition;
    bool running;
    bool recording;
    std::vector<Speaker_ID, tbb::scalable_allocator<Speaker_ID>> speakers;
    Ring_Buffer<Audio_Segment, true, AUDIO_BUFFER_SIZE> audio_buffer;

    /**
     * @brief Default constructor.
     * 
    */
    Script_Engine();
    
public:
    /**
     * @brief Starts the engine.
     */
    void start_engine() noexcept;

    /**
     * @brief Stops the engine.
     * 
     * Stopping the engine terminates all running threads.
     */
    void stop_engine() noexcept;

    /**
     * @brief Starts recording.
     */
    void start_recording() noexcept;

    /**
     * @brief Stops recording.
     */
    void stop_recording() noexcept;

    /**
     * @brief Pushes an audio segment to the audio ring buffer.
     * 
     * @param segment The audio segment to push to the audio ring buffer.
     */
    void push_to_audio_buffer(Audio_Segment&& segment) noexcept;
    
    /**
     * @brief Adds a speaker to the speakers vector.
     * 
     * @param name The name of the speaker to add.
     * @param vector The speaker embedding vector of the speaker to add.
     * @return tsrt_status_code The status code of the operation.
     */
    tsrt_status_code add_speaker(std::string name, float* embedding);

    /**
     * @brief Removes a speaker from the speakers vector.
     * 
     * @param name The name of the speaker to remove.
     */
    void remove_speaker(std::string name) noexcept;

    /**
     * @brief Enables speaker diarization.
     * 
     * One time operation. Once enabled, speaker diarization cannot be disabled.
     * Recalling this method returns a tsrt_status_code of INVALID_OPERATION.
     * Engine must be running to enable speaker diarization.
     * Calling while the engine is running will also return a tsrt_status_code of INVALID_OPERATION.
     * 
     * @return tsrt_status_code The status code of the operation.
    */
    tsrt_status_code enable_speaker_diarization() noexcept;

    /**
     * @brief Returns whether speaker diarization is enabled.
     * 
     * @return bool Whether speaker diarization is enabled.
     */
    bool speaker_diarization_enabled() const noexcept;

    /**
     * @brief Enables speech recognition.
     *
     * One time operation. Once enabled, speech recognition cannot be disabled.
     * Recalling this method returns a tsrt_status_code of INVALID_OPERATION.
     * Engine must be running to enable speech recognition.
     * Calling while the engine is running will also return a tsrt_status_code of INVALID_OPERATION.
     *
     * @return tsrt_status_code The status code of the operation.
    */
    tsrt_status_code enable_speech_recognition() noexcept;

    /**
     * @brief Returns whether speech recognition is enabled.
     * 
     * @return bool Whether speech recognition is enabled.
     */
    bool speech_recognition_enabled() const noexcept;

    /**
     * @brief Enables speaker identification.
     * 
     * One time operation. Once enabled, speaker identification cannot be disabled.
     * Recalling this method returns a tsrt_status_code of INVALID_OPERATION.
     * Engine must be running to enable speaker identification.
     * Calling while the engine is running will also return a tsrt_status_code of INVALID_OPERATION.
     * 
     * @return tsrt_status_code The status code of the operation.
    */
    tsrt_status_code enable_speaker_identification() noexcept;

    /**
     * @brief Returns whether speaker identification is enabled.
     * 
     * @return bool Whether speaker identification is enabled.
     */
    bool speaker_identification_enabled() const noexcept;

    /**
     * @brief Enables emotion recognition.
     * 
     * One time operation. Once enabled, emotion recognition cannot be disabled.
     * Recalling this method returns a tsrt_status_code of INVALID_OPERATION.
     * Engine must be running to enable emotion recognition.
     * Calling while the engine is running will also return a tsrt_status_code of INVALID_OPERATION.
     * 
     * @return tsrt_status_code The status code of the operation.
    */
    tsrt_status_code enable_emotion_recognition() noexcept;

    /**
     * @brief Returns whether emotion recognition is enabled.
     * 
     * @return bool Whether emotion recognition is enabled.
     */
    bool emotion_recognition_enabled() const noexcept;

    /**
     * @brief Returns whether the engine is running.
     * 
     * @return bool Whether the engine is running.
     */
    bool is_running() const noexcept;

    /**
     * @brief Returns whether the engine is recording.
     * 
     * @return bool Whether the engine is recording.
     */
    bool is_recording() const noexcept;

    /**
     * @brief Returns a const reference to the speakers vector.
     * 
     * @return std::vector<Speaker_ID, tbb::scalable_allocator<Speaker_ID>> The speakers vector.
     */
    const std::vector<Speaker_ID, tbb::scalable_allocator<Speaker_ID>>& get_speakers() const noexcept;

    /**
     * @brief Returns a reference to the script_engine.
     * 
     * @return Script_Engine& The script_engine singleton instance.
     */
    static Script_Engine& get_instance() noexcept;

    // Delete copy and move constructors and assign operators to enforce singleton pattern
    Script_Engine(Script_Engine const&) = delete;             // Copy construct
    Script_Engine(Script_Engine&&) = delete;                  // Move construct
    Script_Engine& operator=(Script_Engine const&) = delete;  // Copy assign
    Script_Engine& operator=(Script_Engine &&) = delete;      // Move assign
};

#endif