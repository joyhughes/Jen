#ifndef VIDEO_RECORDER_HPP
#define VIDEO_RECORDER_HPP

#include "string"
#include "vector"
#include "memory"
#include "image.hpp"
#include "uimage.hpp"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

enum class RecordingState
{
    IDLE,
    RECORDING,
    ENCODING,
    ERROR
};

struct RecordingOptions
{
    int width = 512;
    int height = 512;
    int fps = 30;
    int bitrate = 2000000; // 2 Mbps
    std::string codec = "libx264";
    std::string format = "webm"; // Default to WebM container
    std::string preset = "medium";
};

class VideoRecorder
{
private:
    RecordingState state;
    RecordingOptions options;
    std::string error_message;
    std::string temp_filename; // Filename in virtual filesystem

    // FFmpeg context
    AVFormatContext *format_context;
    AVCodecContext *codec_context;
    AVStream *video_stream;
    SwsContext *sws_context;

    // FFmpeg frame management
    AVFrame *frame;
    AVFrame *tmp_frame;
    int frame_count;

    // Buffer for final output
    std::vector<uint8_t> output_buffer;

    bool initialize_video();
    void cleanup();
    bool encode_frame(const uimage &img);

public:
    VideoRecorder();
    ~VideoRecorder();

    bool start_recording(const RecordingOptions &opts);
    bool stop_recording();
    bool add_frame(const uimage &img);

    // Utility functions
    bool validate_format_codec();
    bool initialize_from_image(const uimage &img);
    bool start_recording_adaptive(const uimage &first_frame, const RecordingOptions &base_opts);
    bool are_dimensions_valid(int width, int height);

    // Getters
    RecordingState get_state() const { return state; }
    std::string get_error() const { return error_message; }
    int get_frame_count() const { return frame_count; }
    RecordingOptions get_options() const { return options; }
    const std::vector<uint8_t> &get_output_buffer() const { return output_buffer; }
};

#endif