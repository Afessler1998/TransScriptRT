#include "audio_tsrt.h"
#include "audio_segment_tsrt.h"
#include "constants_config_tsrt.h"
#include "exceptions_tsrt.h"
#include "logger_tsrt.h"
#include "ring_buffer_tsrt.h"
#include "script_engine_tsrt.h"
#include "status_codes_tsrt.h"

#include <chrono>
#include <exception>
#include <iostream>
#include <tbb/tbb.h>
#include <tbb/parallel_invoke.h>

/**
 * @brief Manages the recording of audio data into segments.
 *
 * This thread is responsible for capturing audio data and encapsulating it into Audio_Segment objects
 * with precise timestamps. The audio data is read into a local Audio_Segment, timestamped, and then 
 * moved into a shared, thread-safe Ring_Buffer for subsequent processing. The Audio_Segment is then
 * reset with a new buffer, preparing it for the next round of audio capture. This cycle continues until
 * the engine signals to stop recording.
 *
 * @param shared_audio_ring_buffer A thread-safe shared ring buffer for storing audio segments.
 */
void audio_recording_thread(Ring_Buffer<Audio_Segment, true, AUDIO_BUFFER_SIZE>& shared_audio_ring_buffer) {
    try {
    Script_Engine& engine = Script_Engine::get_instance();
    Audio_tsrt& audio_tsrt = Audio_tsrt::get_instance();
    tsrt_status_code status;
    bool err_on_last_iteration = false;

    Audio_Segment audio_segment;
    audio_segment.lazy_initialize(SAMPLES_PER_HALF_SEGMENT);

    while (engine.is_running()) {
        while (!engine.is_recording()) {
            if (audio_tsrt.is_streaming()) {
                status = audio_tsrt.stop_stream();
                if (status != SUCCESS) {
                    if (err_on_last_iteration)
                        throw Tsrt_Exception(IO_ERROR, "Consecutive errors stopping stream", std::chrono::system_clock::now(), __FILE__, __LINE__);
                    log_error(IO_ERROR, "Error stopping stream", std::chrono::system_clock::now(), __FILE__, __LINE__);
                    err_on_last_iteration = true;
                    continue;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_MS));
        }

        if (!audio_tsrt.is_streaming()) {
            status = audio_tsrt.start_stream();
            if (status != SUCCESS) {
                if (err_on_last_iteration)
                    throw Tsrt_Exception(IO_ERROR, "Consecutive errors starting stream", std::chrono::system_clock::now(), __FILE__, __LINE__);
                log_error(IO_ERROR, "Error starting stream", std::chrono::system_clock::now(), __FILE__, __LINE__);
                err_on_last_iteration = true;
                continue;
            }
        }

        status = audio_tsrt.read_audio_segment(audio_segment.get_audio(), SAMPLES_PER_HALF_SEGMENT);
        if (status != SUCCESS) {
            if (err_on_last_iteration)
                throw Tsrt_Exception(IO_ERROR, "Consecutive errors reading audio segment", std::chrono::system_clock::now(), __FILE__, __LINE__);
            log_error(IO_ERROR, "Error reading audio segment", std::chrono::system_clock::now(), __FILE__, __LINE__);
            err_on_last_iteration = true;
            continue;
        }

        audio_segment.get_timestamp() = std::chrono::system_clock::now();

        shared_audio_ring_buffer.push(std::move(audio_segment));

        audio_segment.reset_audio();

        if (err_on_last_iteration)
            err_on_last_iteration = false;
    }
    } catch(const Tsrt_Exception) {
        throw;
    } catch(const std::exception) {
        throw;
    } catch(...) {
        throw;
    }
}

/**
 * @brief Handles the asynchronous audio processing pipeline.
 *
 * The audio preprocessing thread operates asynchronously with respect to the audio capture thread.
 * It retrieves audio segments from a shared Ring_Buffer, which have been timestamped by the recording thread.
 * The audio data is then fed into an FFmpeg filter graph for processing. Due to the nature of FFmpeg,
 * the output from the processed audio might not be immediately available. Therefore, the thread concurrently
 * feeds new data into the filter graph and awaits the processed output. This thread ensures synchronization 
 * of the audio data with its respective timestamps by using an additional Ring_Buffer. When the FFmpeg filter 
 * graph releases a processed audio segment, the thread aligns it with the corresponding timestamp and then
 * makes it available to the Script_Engine for further analysis. This strategy helps maintain the integrity
 * of audio data synchronization throughout the processing pipeline and accommodates any latency introduced
 * by batched processing in FFmpeg.
 *
 * @param shared_audio_ring_buffer A thread-safe shared ring buffer from which raw audio segments are retrieved.
 */
void audio_preprocessing_thread(Ring_Buffer<Audio_Segment, true, AUDIO_BUFFER_SIZE>& shared_audio_ring_buffer) {
    Script_Engine& engine = Script_Engine::get_instance();
    Audio_tsrt& audio_tsrt = Audio_tsrt::get_instance();
    tsrt_status_code status;
    bool first_segment = true;

    // audio_segment for passing onto the next stage of the pipeline
    Audio_Segment full_audio_segment;
    full_audio_segment.lazy_initialize(SAMPLES_PER_SEGMENT);

    // keep track of previous half segments timestamp, its the timestamp for the current full segment
    std::chrono::system_clock::time_point current_timestamp = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point last_timestamp = std::chrono::system_clock::now();

    while (engine.is_running()) {
        while (!engine.is_recording()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_MS));
        }

        // get next segment if it exists, otherwise wait
        std::optional<Audio_Segment> latest_half_segment_opt = shared_audio_ring_buffer.pop();
        if (!latest_half_segment_opt.has_value()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_MS));
            continue;
        }

        Audio_Segment latest_half_segment = std::move(latest_half_segment_opt.value());

        // assign new timestamp and push to ring buffer for alignment after processing
        current_timestamp = latest_half_segment.get_timestamp();

        status = audio_tsrt.preprocess_audio_segment(latest_half_segment.get_audio());
        if (status != SUCCESS) 
            throw Tsrt_Exception(UNKNOWN_ERROR, "Error preprocessing audio segment", std::chrono::system_clock::now(), __FILE__, __LINE__);

        // skip the rest on the first segment since there is no previous segment to combine with
        if (first_segment) {
            memcpy(full_audio_segment.get_audio(), latest_half_segment.get_audio(), SAMPLES_PER_HALF_SEGMENT * sizeof(float));
            first_segment = false;
            continue;
        }

        // finish the audio_segments buffer by combining the previous segment with the current one
        memcpy(full_audio_segment.get_midpoint(), latest_half_segment.get_audio(), SAMPLES_PER_HALF_SEGMENT * sizeof(float));
        
        full_audio_segment.set_timestamp(last_timestamp);
        last_timestamp = current_timestamp;
        
        engine.push_to_audio_buffer(std::move(full_audio_segment));

        // copy half segment to beginning of full segment for next iteration
        full_audio_segment.reset_audio();
        memcpy(full_audio_segment.get_audio(), latest_half_segment.get_audio(), SAMPLES_PER_HALF_SEGMENT * sizeof(float));
    }
}

void speaker_diarization_thread() {
/*
    speaker diarization loops until the running flag is set.
    It waits for the recording flag to be set, then reads
    audio from the ring buffer and performs speaker diarization
    on it. Segments of speech will be divided into buckets
    of speakers.

    If speaker identification and or emotional recognition
    are enabled, their behavior must be modified to handle
    the case where there are multiple speakers in a segment.
    They will instead be passed the audio from buckets of
    speakers instead of the segments of audio.
*/

    Script_Engine& engine = Script_Engine::get_instance();

    while (engine.is_running()) {
        while (!engine.is_recording()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_MS));
        }

        //std::cout << "Speaker diarization..." << std::endl;
    }
}

void speech_recognition_thread() {
/*
    speech recognition loops until the running flag is set.
    It waits for the recording flag to be set, then reads
    audio from the ring buffer and performs speech recognition
    on it. Segments of speech will be passed to the script
    writing thread to for deduplication of overlapping 
    segments before being concatenated and aligned in the script.
*/

    Script_Engine& engine = Script_Engine::get_instance();

    while ((engine).is_running()) {
        while (!engine.is_recording()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_MS));
        }

        if (engine.speaker_diarization_enabled()) {
            //continue;
        }

        //std::cout << "Speech recognition..." << std::endl;
    }
}


void speaker_identification_thread() {
/*
    speaker identification loops until the running flag is set.
    It waits for the recording flag to be set. If speaker diarization
    is enabled, it will wait for the speaker diarization thread
    to finish processing the audio and pass it the audio from
    buckets of speakers. Otherwise it will read audio from the
    ring buffer and perform speaker identification on it. 
    A struct with a timestamp and the speaker's name will be
    passed to the script writing thread.
*/  

    Script_Engine& engine = Script_Engine::get_instance();
    
    while (engine.is_running()) {
        while (!engine.is_recording()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_MS));
        }

        if (engine.speaker_diarization_enabled()) {
            //continue;
        }

        //std::cout << "Speaker identification..." << std::endl;
    }
}

void emotion_recognition_thread() {
/*
    emotion recognition loops until the running flag is set.
    It waits for the recording flag to be set. If speaker diarization
    is enabled, it will wait for the speaker diarization thread
    to finish processing the audio and pass it the audio from
    buckets of speakers. Otherwise it will read audio from the
    ring buffer and perform emotion recognition on it.
    A struct with a timestamp and the speaker's emotion will be
    passed to the script writing thread.
*/
    Script_Engine& engine = Script_Engine::get_instance();

    while (engine.is_running()) {
        while(!engine.is_recording()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_MS));
        }

        if (engine.speaker_diarization_enabled()) {
            //continue;
        }

        //std::cout << "Emotion recognition..." << std::endl;
    }
}

void script_writing_thread() {
/*
    script writing loops until the running flag is set.
    It waits for the recording flag to be set. It's precise
    behavior depends on the enabled analyses. If all analyses
    are enabled, it will wait for all results for a given
    timestamp to be available before writing to the script.
    Any analysis that is disabled will simply be left out of
    the script data structure.
*/
    Script_Engine& engine = Script_Engine::get_instance();

    while (engine.is_running()) {
        while (!engine.is_recording()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_MS));
        }

        //std::cout << "Script writing..." << std::endl;
    }
}

int main() {
    try {
        init_logging();
        Script_Engine& engine = Script_Engine::get_instance();
        engine.enable_speaker_diarization();
        engine.enable_speech_recognition();
        engine.enable_speaker_identification();
        engine.enable_emotion_recognition();
        engine.start_engine();
        engine.start_recording();

        auto shared_audio_ring_buffer = std::make_shared<Ring_Buffer<Audio_Segment, true, AUDIO_BUFFER_SIZE>>();

        std::vector<std::function<void()>> tasks;
        tasks.push_back([shared_audio_ring_buffer]() { audio_recording_thread(*shared_audio_ring_buffer); });
        tasks.push_back([shared_audio_ring_buffer]() { audio_preprocessing_thread(*shared_audio_ring_buffer); });
        if (engine.speech_recognition_enabled())
            tasks.push_back([&]() { speech_recognition_thread(); });
        if (engine.speaker_diarization_enabled())
            tasks.push_back([&]() { speaker_diarization_thread(); });
        if (engine.speaker_identification_enabled())
            tasks.push_back([&]() { speaker_identification_thread(); });
        if (engine.emotion_recognition_enabled())
            tasks.push_back([&]() { emotion_recognition_thread(); });
        tasks.push_back([&] { script_writing_thread(); });
        tbb::parallel_for(size_t(0), tasks.size(), [&](size_t i) {
            tasks[i]();
        });

    } catch (const std::bad_alloc &e) {
        return handle_exception(e);
    } catch (const std::ios_base::failure &e) {
        return handle_exception(e);
    } catch (const std::runtime_error &e) {
        return handle_exception(e);
    } catch (const std::out_of_range &e) {
        return handle_exception(e);
    } catch (const std::invalid_argument &e) {
        return handle_exception(e);
    } catch (const std::logic_error &e) {
        return handle_exception(e);
    } catch (const Tsrt_Exception &e) {
        return handle_exception(e);
    } catch (const std::exception &e) {
        return handle_exception(e);
    } catch (...) {
        log_error(UNKNOWN_ERROR,
                "Unknown error occurred during engine execution",
                std::chrono::system_clock::now(),
                __FILE__,
                __LINE__);
        return UNKNOWN_ERROR;
    }

    return SUCCESS;
}