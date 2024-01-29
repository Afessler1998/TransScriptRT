#ifndef audio_tsrt_h
#define audio_tsrt_h

#include "constants_config_tsrt.h"
#include "exceptions_tsrt.h"
#include "status_codes_tsrt.h"

#include <chrono>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <portaudio.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
// explicit IID declaration to get around linker error
extern const IID IID_ICodecAPI;
}

/**
 * @brief A deleter for PortAudio
 * 
 * @param stream PortAudio stream to be cleaned up
*/
void stream_deleter(PaStream* stream);

/**
 * @brief A deleter for AVFrame
 * 
 * @param avframe AVFrame to be cleaned up
*/
void avframe_deleter(AVFrame* avframe);

/**
 * @brief A deleter for AVFilterGraph
 * 
 * @param avfilter_graph AVFilterGraph to be cleaned up
*/
void avfilter_graph_deleter(AVFilterGraph* avfilter_graph);

class Audio_tsrt {

private:

    std::unique_ptr<PaStream, decltype(&stream_deleter)> stream;
    std::unique_ptr<AVFrame, decltype(&avframe_deleter)> avframe_filter_in;
    std::unique_ptr<AVFrame, decltype(&avframe_deleter)> avframe_filter_out;
    std::unique_ptr<AVFilterGraph, decltype(&avfilter_graph_deleter)> avfilter_graph;
    // ctx's outside of init_avfilter_graph() because preprocess_audio() reads them
    // the other filters contexts are not needed outside of init_avfilter_graph()
    AVFilterContext* src_ctx;
    AVFilterContext* sink_ctx;

    /**
     * @brief Construct a new Audio_tsrt object
     * 
     * Initializes PortAudio, finds an input device, opens a stream, and initializes FFmpeg.
     * 
     * @throw tsrt_exception
    */
    Audio_tsrt();

    /**
     * @brief Initialize the AVFrames for input and ouput to the AVFilterGraph
    */
    void init_avframes();

    /**
     * @brief Initialize the AVFilterGraph
    */
    void init_avfilter_graph();
    
    /**
     * @brief Initialize PortAudio
     * 
     * @throw tsrt_exception
    */
    void handle_ffmpeg_errors(std::function<int()> bound_func, const std::string& error_context, std::string file, int line);

    /**
     * @brief Initialize PortAudio
     * 
     * @throw tsrt_exception
    */
    static void ffmpeg_log_callback(void* ptr, int level, const char* fmt, va_list vargs);

public:

    /**
     * @brief Start the audio stream
     * 
     * @return tsrt_status_code 
    */
    tsrt_status_code start_stream();
    
    /**
     * @brief Stop the audio stream
     * 
     * @return tsrt_status_code 
    */
    tsrt_status_code stop_stream();

    /**
     * @brief Read in the next audio segment
     * 
     * @param segment A pointer to a buffer to store the audio segment
     * @return tsrt_status_code 
    */
    tsrt_status_code read_audio_segment(float* segment, int segment_size);

    /**
     * @brief Preprocess the audio segment with FFmpeg
     * 
     * @param segment A pointer to a buffer containing the audio segment
     * @return tsrt_status_code
    */
    tsrt_status_code preprocess_audio_segment(float* segment);

    /**
     * @brief Check if the audio stream is running
     * 
     * @return bool
    */
    bool is_streaming() const noexcept;

    /**
     * @brief Get the instance of Audio_tsrt
     * 
     * Exists to enforce singleton pattern.
     * 
     * @return Audio_tsrt& 
    */
    static Audio_tsrt& get_instance();

    // delete copy and move constructors and assign operators to enforce singleton pattern
    Audio_tsrt(Audio_tsrt const&) = delete;        // copy constructor
    Audio_tsrt(Audio_tsrt&&) = delete;             // move constructor
    void operator=(Audio_tsrt const&) = delete;    // copy assignment operator
    void operator=(Audio_tsrt&&) = delete;         // move assignment operator
};

#endif