#include "audio_tsrt.h"
#include "constants_config_tsrt.h"
#include "exceptions_tsrt.h"
#include "status_codes_tsrt.h"

#include <functional>
#include <memory>
#include <sstream>
#include <string>
extern "C" {
#include <libavutil/log.h>
#include <libavfilter/avfilter.h>
#include <libavutil/frame.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
// explicit IID definition to get around linker error
const IID IID_ICodecAPI = 
{ 0x901db4c7, 0x31ce, 0x41a2, { 0x85, 0xdc, 0x8f, 0xa0, 0xbf, 0x41, 0xb8, 0xda } };
}

void avframe_deleter(AVFrame* avframe) {
    if (avframe != nullptr)
        av_frame_free(&avframe);
}

void Audio_tsrt::init_avframe() {
    avframe_filter.reset(av_frame_alloc());
    if (!avframe_filter) {
        throw Tsrt_Exception(INSUFFICIENT_MEMORY, "Error allocating memory for avframe_filter", std::chrono::system_clock::now(), __FILE__, __LINE__);
    }

    // avframe.channels and avframe.channel_layout are apparently deprecated
    // but it doesn't work with the what the documentation says to use instead
    avframe_filter->channels = 1;
    avframe_filter->channel_layout = AV_CH_LAYOUT_MONO;
    avframe_filter->format = AV_SAMPLE_FMT_FLT;
    avframe_filter->sample_rate = SAMPLE_RATE;
    avframe_filter->nb_samples = SAMPLES_PER_HALF_SEGMENT;
}

void Audio_tsrt::handle_ffmpeg_errors(std::function<int()> bound_func, const std::string& error_context, std::string file, int line) {
    int ret = bound_func();
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
        throw Tsrt_Exception(RUNTIME_ERROR, error_context + ": " + err_buf, std::chrono::system_clock::now(), file, line);
    }
}

void Audio_tsrt::ffmpeg_log_callback(void* ptr, int level, const char* fmt, va_list vargs) {
    static char message[8192]; // if this is ever used by multiple threads, this will need to be made thread-safe. As of now it's fine.
    vsnprintf(message, sizeof(message), fmt, vargs);
    if (level > AV_LOG_WARNING)
        log_error(RUNTIME_ERROR, std::string("ffmpeg: ") + message, std::chrono::system_clock::now(), __FILE__, __LINE__);
}

void avfilter_graph_deleter(AVFilterGraph* avfilter_graph) {
    if (avfilter_graph != nullptr)
        avfilter_graph_free(&avfilter_graph);
}

void Audio_tsrt::init_avfilter_graph() {
    avfilter_graph.reset(avfilter_graph_alloc());
    if (avfilter_graph == nullptr) {
        throw Tsrt_Exception(INSUFFICIENT_MEMORY, "Error allocating memory for avfilter_graph", std::chrono::system_clock::now(), __FILE__, __LINE__);
    }

    AVFilterContext *bandpass_ctx = nullptr, *afftdn_ctx = nullptr;

    const AVFilter *src = avfilter_get_by_name("abuffer");
    std::ostringstream src_args;
    src_args << "sample_rate=" << SAMPLE_RATE << ":sample_fmt=" << SRC_SAMPLE_FMT << ":channel_layout=" << SRC_CHANNEL_LAYOUT;
    handle_ffmpeg_errors([&]() -> int { return avfilter_graph_create_filter(&src_ctx, src, "src", src_args.str().c_str(), nullptr, avfilter_graph.get()); }, "Error creating source filter", __FILE__, __LINE__);

    const AVFilter *bandpass = avfilter_get_by_name("bandpass");
    std::ostringstream bandpass_args;
    bandpass_args << "f=" << BANDPASS_F << ":w=" << BANDPASS_W;
    handle_ffmpeg_errors([&]() -> int { return avfilter_graph_create_filter(&bandpass_ctx, bandpass, "bandpass", bandpass_args.str().c_str(), nullptr, avfilter_graph.get()); }, "Error creating bandpass filter", __FILE__, __LINE__);

    const AVFilter *afftdn = avfilter_get_by_name("afftdn");
    std::ostringstream afftdn_args;
    afftdn_args << "nr=" << AFFTDN_NR << ":nf=" << AFFTDN_NF;
    handle_ffmpeg_errors([&]() -> int { return avfilter_graph_create_filter(&afftdn_ctx, afftdn, "afftdn", afftdn_args.str().c_str(), nullptr, avfilter_graph.get()); }, "Error creating afftdn filter", __FILE__, __LINE__);

    const AVFilter *sink = avfilter_get_by_name("abuffersink");
    handle_ffmpeg_errors([&]() -> int { return avfilter_graph_create_filter(&sink_ctx, sink, "sink", nullptr, nullptr, avfilter_graph.get()); }, "Error creating sink filter", __FILE__, __LINE__);

    handle_ffmpeg_errors([&]() -> int { return avfilter_link(src_ctx, 0, bandpass_ctx, 0); }, "Error linking filters", __FILE__, __LINE__);
    handle_ffmpeg_errors([&]() -> int { return avfilter_link(bandpass_ctx, 0, afftdn_ctx, 0); }, "Error linking filters", __FILE__, __LINE__);
    handle_ffmpeg_errors([&]() -> int { return avfilter_link(afftdn_ctx, 0, sink_ctx, 0); }, "Error linking filters", __FILE__, __LINE__);
    
    avfilter_graph_config(avfilter_graph.get(), nullptr);
}

void stream_deleter(PaStream* stream) {
    PaError paStatus;
    if (stream != nullptr) {
        paStatus = Pa_StopStream(stream);
        if (paStatus != paNoError)
            log_error(IO_ERROR, Pa_GetErrorText(paStatus), std::chrono::system_clock::now(), __FILE__, __LINE__);
        paStatus = Pa_CloseStream(stream);
        if (paStatus != paNoError)
            log_error(IO_ERROR, Pa_GetErrorText(paStatus), std::chrono::system_clock::now(), __FILE__, __LINE__);
    }
    paStatus = Pa_Terminate();
    if (paStatus != paNoError)
        log_error(IO_ERROR, Pa_GetErrorText(paStatus), std::chrono::system_clock::now(), __FILE__, __LINE__);
};

Audio_tsrt::Audio_tsrt() :
    stream{nullptr, stream_deleter},
    avframe_filter{nullptr, avframe_deleter},
    avfilter_graph{nullptr, avfilter_graph_deleter},
    src_ctx{nullptr},
    sink_ctx{nullptr} {

    PaError paStatus;
    paStatus = Pa_Initialize();
    if (paStatus != paNoError)
        throw Tsrt_Exception(RUNTIME_ERROR, Pa_GetErrorText(paStatus), std::chrono::system_clock::now(), __FILE__, __LINE__);
    
    PaStreamParameters input_parameters;
    input_parameters.device = Pa_GetDefaultInputDevice();
    if (input_parameters.device == paNoDevice)
        throw Tsrt_Exception(IO_ERROR, "Error: No default input device", std::chrono::system_clock::now(), __FILE__, __LINE__);
    const PaDeviceInfo* input_devicefo = Pa_GetDeviceInfo(input_parameters.device);

    if (input_devicefo == nullptr)
        throw Tsrt_Exception(IO_ERROR, "Error: No input device found", std::chrono::system_clock::now(), __FILE__, __LINE__);
    input_parameters.channelCount = 1;
    input_parameters.sampleFormat = paFloat32;
    input_parameters.suggestedLatency = input_devicefo->defaultLowInputLatency;
    input_parameters.hostApiSpecificStreamInfo = nullptr;

    PaStream* raw_stream;
    paStatus = Pa_OpenStream(&raw_stream, &input_parameters, nullptr, SAMPLE_RATE, SAMPLES_PER_HALF_SEGMENT, paNoFlag, nullptr, nullptr);
    if (paStatus != paNoError)
        throw Tsrt_Exception(IO_ERROR, Pa_GetErrorText(paStatus), std::chrono::system_clock::now(), __FILE__, __LINE__);
    stream.reset(raw_stream);

    init_avfilter_graph();
    init_avframe();
    av_log_set_callback(ffmpeg_log_callback);
}

tsrt_status_code Audio_tsrt::start_stream() {
    PaError paStatus;
    paStatus = Pa_StartStream(stream.get());
    if (paStatus != paNoError)
        throw Tsrt_Exception(RUNTIME_ERROR, Pa_GetErrorText(paStatus), std::chrono::system_clock::now(), __FILE__, __LINE__);
    return SUCCESS;
}

tsrt_status_code Audio_tsrt::stop_stream() {
    PaError paStatus;
    paStatus = Pa_StopStream(stream.get());
    if (paStatus != paNoError)
        throw Tsrt_Exception(RUNTIME_ERROR, Pa_GetErrorText(paStatus), std::chrono::system_clock::now(), __FILE__, __LINE__);
    return SUCCESS;
}

tsrt_status_code Audio_tsrt::read_audio_segment(float* segment, int segment_size) {
    PaError paStatus;
    paStatus = Pa_ReadStream(stream.get(), segment, segment_size);
    if (paStatus != paNoError)
        throw Tsrt_Exception(IO_ERROR, Pa_GetErrorText(paStatus), std::chrono::system_clock::now(), __FILE__, __LINE__);
    return SUCCESS;
}

tsrt_status_code Audio_tsrt::preprocess_audio_segment(float* segment) {
    int ret_code;

    avframe_filter->data[0] = reinterpret_cast<uint8_t*>(segment);
    handle_ffmpeg_errors([&]() -> int { return av_buffersrc_add_frame_flags(src_ctx, avframe_filter.get(), AV_BUFFERSRC_FLAG_PUSH); }, "Error adding frame to filter", __FILE__, __LINE__);
    /*
    The current filters being used will always return a frame, so this is not necessary.
    However, if the filters are changed, this will need to be revised.
    */

    return SUCCESS;
}

bool Audio_tsrt::is_streaming() const noexcept {
    return Pa_IsStreamActive(stream.get());
}

Audio_tsrt& Audio_tsrt::get_instance() {
    static Audio_tsrt instance;
    return instance;
}