#ifndef VIDEO_RECORDER_HPP
#define VIDEO_RECORDER_HPP

#include "string"
#include "vector"
#include "memory"
#include "image.hpp"
#include "uimage.hpp"

#ifndef DISABLE_FFMPEG
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}
#endif

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
    int width = 1920;
    int height = 1080;
    int fps = 30;
    int bitrate = 8000000; // 8 Mbps
    std::string codec = "libx264";
    std::string format = "mp4"; // Default to MP4 container
    std::string preset = "ultrafast";
};

class VideoRecorder
{
private:
    RecordingState state;
    RecordingOptions options;
    std::string error_message;
    
#ifndef DISABLE_FFMPEG
    std::string temp_filename; // Filename in virtual filesystem

    // FFmpeg context - always available
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

    /**
     * Initialize video recording with H.264/MP4 encoding
     * 
     * CRITICAL INITIALIZATION ORDER (DO NOT CHANGE):
     * 1. Create format context and codec context
     * 2. Set all codec parameters (width, height, bitrate, etc.)
     * 3. Set codec-specific options (preset, profile, etc.)
     * 4. Open codec with avcodec_open2() - THIS GENERATES SPS/PPS
     * 5. Copy parameters to stream with avcodec_parameters_from_context()
     * 6. Write format header with avformat_write_header()
     * 
     * WHY THIS ORDER MATTERS:
     * - SPS/PPS parameter sets are only generated when codec is opened
     * - MP4 container needs these in extradata for proper playback
     * - Copying parameters before opening = empty extradata = unplayable video
     * 
     * @return true if initialization successful, false otherwise
     */
    bool initialize_video();
    void cleanup();
    bool encode_frame(const uimage &img);
#else
    // Stub members for when FFmpeg is disabled
    int frame_count;
    std::vector<uint8_t> output_buffer;
#endif

public:
    VideoRecorder();
    ~VideoRecorder();

    bool start_recording(const RecordingOptions &opts);
    bool stop_recording();
    bool add_frame(const uimage &img);
    bool add_frame_rgba(const uint8_t* rgba_data, int width, int height);

#ifndef DISABLE_FFMPEG
    // Utility functions
    bool validate_format_codec();
    bool initialize_from_image(const uimage &img);
    bool start_recording_adaptive(const uimage &first_frame, const RecordingOptions &base_opts);
    bool are_dimensions_valid(int width, int height);
#endif

    // Getters
    RecordingState get_state() const { return state; }
    std::string get_error() const { return error_message; }
    int get_frame_count() const { return frame_count; }
    RecordingOptions get_options() const { return options; }
    const std::vector<uint8_t> &get_output_buffer() const { return output_buffer; }
};

#endif