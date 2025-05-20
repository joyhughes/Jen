#ifndef VIDEO_RECORDER_HPP
#define VIDEO_RECORDER_HPP

#include "string"
#include "vector"
#include "memory"
#include "functional"
#include "image.hpp"
#include "uimage.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}

enum class RecordingState {
    IDLE,
    RECORDING,
    ENCODING,
    ERROR
};

struct RecordingOptions {
    int width = 512;
    int height = 512;
    int fps = 30;
    int bitrate = 2000000; // 2 Mbps
    std::string codec = "libx264";
    std::string format = "mp4";
    std::string preset = "medium";
};

class VideoRecorder {
private:
    RecordingState state;
    RecordingOptions options;
    std::string error_message;

    // ffmpeg context
    AVFormatContext* format_context;
    AVCodecContext* codec_context;
    AVStream* video_stream;
    SwsContext* sws_context;

    // ffmpeg frame management
    std::vector<uint8_t> frame_buffer;
    AVFrame* frame;
    AVFrame* tmp_frame;
    int frame_count;

    bool initialize_video();
    void cleanup();

public:
    VideoRecorder();
    ~VideoRecorder();

    bool start_recording(const RecordingOptions& opts);
    bool stop_recording();
    bool is_recording() const { return state == RecordingState::RECORDING; }
    RecordingState get_state() const { return state; }
    std::string get_error() const { return error_message; }

    // frame capture
    bool add_frame(const uimage& img);

    // utils
    int get_frame_count() const { return frame_count; }
    RecordingOptions get_options() const { return options; }
};


#endif
