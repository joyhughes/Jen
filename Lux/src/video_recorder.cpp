#include "video_recorder.hpp"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#define DEBUG(msg) { std::string debug_msg = msg; std::cout << debug_msg << std::endl; }
#define ERROR(msg) { error_message = msg; state = RecordingState::ERROR; return false; }

VideoRecorder::VideoRecorder() : state(RecordingState::IDLE),
                                format_context(nullptr),
                                codec_context(nullptr),
                                video_stream(nullptr),
                                sws_context(nullptr),
                                frame(nullptr),
                                tmp_frame(nullptr),
                                frame_count(0),
                                skip_counter(0) {
    // Generate unique temp filename
    temp_filename = "/tmp/recording_" + std::to_string(reinterpret_cast<uintptr_t>(this)) + ".data";
}

VideoRecorder::~VideoRecorder() {
    if (state == RecordingState::RECORDING) {
        stop_recording();
    }
    cleanup();

    // Clean up temporary file if it exists
    if (std::remove(temp_filename.c_str()) == 0) {
        DEBUG("Temporary file removed: " + temp_filename);
    }
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
bool VideoRecorder::initialize_video() {
    DEBUG("Initializing video with VP8/WebM for optimal browser performance");
    DEBUG("Using temp file: " + temp_filename);

    validate_format_codec();

    // Create output format context
    avformat_alloc_output_context2(&format_context, nullptr, options.format.c_str(), temp_filename.c_str());
    if (!format_context) {
        ERROR("Failed to create output format context");
    }

    DEBUG("Using format: " + options.format + " with codec: " + options.codec);

    // More thorough codec search with additional debug info
    const char* codec_names[] = {"libvpx_vp8", "libvpx-vp8", "libvpx", "vp8"};
    const AVCodec *codec = nullptr;

    // Try each codec name with detailed debugging
    for (const char* name : codec_names) {
        DEBUG("Trying to find encoder: " + std::string(name));
        codec = avcodec_find_encoder_by_name(name);
        if (codec) {
            DEBUG("Successfully found encoder: " + std::string(name));
            options.codec = name;
            break;
        }
    }

    // Last resort - try by codec ID
    if (!codec) {
        DEBUG("Trying to find VP8 encoder by codec ID");
        codec = avcodec_find_encoder(AV_CODEC_ID_VP8);
        if (codec) {
            DEBUG("Found encoder by codec ID: " + std::string(codec->name));
            options.codec = codec->name;
        } else {
            ERROR("Failed to find any VP8 encoder");
        }
    }

    // Validate codec before proceeding
    if (!codec) {
        ERROR("No codec available for VP8 encoding");
    }

    // Create stream with NULL instead of codec for FFmpeg 7.x compatibility
    video_stream = avformat_new_stream(format_context, NULL);
    if (!video_stream) {
        ERROR("Failed to create video stream");
    }

    // Initialize codec context
    codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) {
        ERROR("Failed to allocate codec context");
    }

    // Configure codec parameters
    codec_context->codec_id = codec->id;
    codec_context->codec_type = AVMEDIA_TYPE_VIDEO;
    codec_context->width = options.width;
    codec_context->height = options.height;
    codec_context->time_base = (AVRational) {1, options.fps};
    codec_context->framerate = (AVRational) {options.fps, 1};
    codec_context->gop_size = options.fps;
    codec_context->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_context->bit_rate = options.bitrate;
    codec_context->thread_count = options.threads;

    // Configure VP8-specific options
    if (codec_context->codec_id == AV_CODEC_ID_VP8) {
        set_vp8_options();
    }

    // Copy parameters to stream
    int ret = avcodec_parameters_from_context(video_stream->codecpar, codec_context);
    if (ret < 0) {
        ERROR("Failed to copy codec parameters to stream");
    }

    // Ensure stream timebase matches codec context
    video_stream->time_base = codec_context->time_base;

    // Explicitly check codec before opening
    if (!codec) {
        ERROR("Codec became NULL before opening");
    }

    // Open codec with detailed error reporting
    ret = avcodec_open2(codec_context, codec, nullptr);
    if (ret < 0) {
        char error_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, error_buf, AV_ERROR_MAX_STRING_SIZE);
        ERROR(std::string("Failed to open codec: ") + error_buf);
    }

    // Set up frames
    if (!setup_frames_and_conversion()) {
        return false;
    }

    // Open output file
    if (avio_open(&format_context->pb, temp_filename.c_str(), AVIO_FLAG_WRITE) < 0) {
        ERROR("Failed to open output file");
    }

    // Write header
    if (avformat_write_header(format_context, nullptr) < 0) {
        ERROR("Failed to write header");
    }

    DEBUG("Video initialization complete with dimensions: " +
        std::to_string(options.width) + "x" + std::to_string(options.height));

    return true;
}


void VideoRecorder::set_vp8_options() {
    // VP8-specific options for maximum speed
    av_opt_set(codec_context->priv_data, "quality", "realtime", 0);
    av_opt_set(codec_context->priv_data, "speed", "8", 0);  // Maximum speed (0-8)
    av_opt_set_int(codec_context->priv_data, "cpu-used", options.cpu_used, 0);

    // Disable features that slow down encoding
    av_opt_set_int(codec_context->priv_data, "lag-in-frames", 0, 0);  // No frame lag
    av_opt_set_int(codec_context->priv_data, "arnr-maxframes", 0, 0); // Disable temporal filtering
    av_opt_set_int(codec_context->priv_data, "arnr-strength", 0, 0);  // Disable filter strength
    av_opt_set_int(codec_context->priv_data, "auto-alt-ref", 0, 0);   // Disable alt refs

    // Set quality parameter (CRF)
    av_opt_set_int(codec_context->priv_data, "crf", options.quality, 0);

    DEBUG("VP8 encoder configured with speed=" + std::to_string(options.cpu_used) +
          ", quality=" + std::to_string(options.quality));
}


bool VideoRecorder::setup_frames_and_conversion() {
    // Allocate and initialize frames
    frame = av_frame_alloc();
    if (!frame) {
        ERROR("Failed to allocate frame");
    }

    frame->format = codec_context->pix_fmt;
    frame->width = codec_context->width;
    frame->height = codec_context->height;

    if (av_frame_get_buffer(frame, 32) < 0) {  // 32-byte alignment for better performance
        ERROR("Failed to allocate frame buffer");
    }

    tmp_frame = av_frame_alloc();
    if (!tmp_frame) {
        ERROR("Failed to allocate temporary frame");
    }

    tmp_frame->format = AV_PIX_FMT_RGBA;
    tmp_frame->width = codec_context->width;
    tmp_frame->height = codec_context->height;

    if (av_frame_get_buffer(tmp_frame, 32) < 0) {
        ERROR("Failed to allocate memory for temp frame buffer");
    }

    // Create SWS context for pixel format conversion (RGBA -> YUV420P)
    // Using SWS_FAST_BILINEAR for better performance
    sws_context = sws_getContext(
        options.width, options.height, AV_PIX_FMT_RGBA,
        options.width, options.height, codec_context->pix_fmt,
        SWS_FAST_BILINEAR, nullptr, nullptr, nullptr  // Fast bilinear for better performance
    );

    if (!sws_context) {
        ERROR("Failed to create SWS context");
    }

    return true;

}


#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
bool VideoRecorder::add_frame(const uimage &img) {
    if (state != RecordingState::RECORDING) {
        return false;
    }

    // Performance optimization: Skip frames if needed
    static int frame_counter = 0;
    if (frame_count > 10) {  // After initial frames
        if (++frame_counter % 2 != 0) {  // Skip every other frame
            return true;  // Skip this frame but report success
        }
    }

    // Direct memory access for faster processing
    const uint8_t *src_data[4] = {
        (const uint8_t*)img.get_base_ptr(), nullptr, nullptr, nullptr
    };
    int src_linesize[4] = {img.get_dim().x * 4, 0, 0, 0}; // RGBA = 4 bytes

    // Convert directly to the encoder frame (skipping the pixel copying loop)
    sws_scale(sws_context, src_data, src_linesize, 0,
              img.get_dim().y, frame->data, frame->linesize);

    // Set frame timestamp
    frame->pts = frame_count;

    // Encode with minimal buffering
    int ret = avcodec_send_frame(codec_context, frame);
    if (ret < 0) {
        ERROR("Error sending frame to encoder");
    }

    // Process packets but don't wait for all of them
    AVPacket pkt = {0};
    ret = avcodec_receive_packet(codec_context, &pkt);

    if (ret >= 0) {
        // We have a packet, write it
        av_packet_rescale_ts(&pkt, codec_context->time_base, video_stream->time_base);
        pkt.stream_index = video_stream->index;

        ret = av_interleaved_write_frame(format_context, &pkt);
        if (ret < 0) {
            ERROR("Error writing packet to file");
        }

        av_packet_unref(&pkt);
    } else if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
        ERROR("Error receiving packet from encoder");
    }

    frame_count++;
    return true;
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
bool VideoRecorder::start_recording(const RecordingOptions &opts) {
    if (state == RecordingState::RECORDING) {
        stop_recording();
    }

    options = opts;
    frame_count = 0;

    // Clear previous output buffer
    output_buffer.clear();

    if (!initialize_video()) {
        cleanup();
        return false;
    }

    state = RecordingState::RECORDING;
    DEBUG("Recording started");
    return true;
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
bool VideoRecorder::stop_recording() {
    if (state != RecordingState::RECORDING) {
        DEBUG("Cannot stop recording - not in recording state");
        return false;
    }

    state = RecordingState::ENCODING;
    DEBUG("Finalizing video file...");

    // Flush the encoder
    int ret = avcodec_send_frame(codec_context, nullptr);
    if (ret < 0) {
        ERROR("Error sending flush frame to encoder");
    }

    // Process remaining packets
    while (true) {
        AVPacket pkt = {0};
        ret = avcodec_receive_packet(codec_context, &pkt);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            ERROR("Error receiving packet from encoder during flush");
        }

        // Rescale packet timestamps
        av_packet_rescale_ts(&pkt, codec_context->time_base, video_stream->time_base);
        pkt.stream_index = video_stream->index;

        // Write packet to file
        ret = av_interleaved_write_frame(format_context, &pkt);
        if (ret < 0) {
            ERROR("Error writing packet to file during flush");
        }

        av_packet_unref(&pkt);
    }

    // Write trailer
    ret = av_write_trailer(format_context);
    if (ret < 0) {
        ERROR("Error writing file trailer");
    }

    // Close the output file
    if (format_context && format_context->pb) {
        avio_closep(&format_context->pb);
    }

    // Load the encoded file from the virtual filesystem into the output buffer
    FILE* file = fopen(temp_filename.c_str(), "rb");
    if (!file) {
        ERROR("Failed to open temporary file for reading");
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    DEBUG("Reading " + std::to_string(file_size) + " bytes from temporary file");

    // Resize buffer and read file contents
    output_buffer.resize(file_size);
    if (fread(output_buffer.data(), 1, file_size, file) != (size_t)file_size) {
        fclose(file);
        ERROR("Failed to read temporary file");
    }

    fclose(file);

    // Clean up resources
    cleanup();

    DEBUG("Recording finished with " + std::to_string(frame_count) +
          " frames, output size: " + std::to_string(output_buffer.size()) + " bytes");

    state = RecordingState::IDLE;
    return true;
}

void VideoRecorder::cleanup() {
    if (frame) {
        av_frame_free(&frame);
        frame = nullptr;
    }

    if (tmp_frame) {
        av_frame_free(&tmp_frame);
        tmp_frame = nullptr;
    }

    if (codec_context) {
        avcodec_free_context(&codec_context);
        codec_context = nullptr;
    }

    if (format_context) {
        avformat_free_context(format_context);
        format_context = nullptr;
    }

    if (sws_context) {
        sws_freeContext(sws_context);
        sws_context = nullptr;
    }
}

bool VideoRecorder::validate_format_codec() {
    // WebM works with VP8/VP9
    if (options.format == "webm") {
        if (options.codec != "libvpx" && options.codec != "libvpx-vp9") {
            options.codec = "libvpx-vp8"; // Default to VP8 for WebM
            DEBUG("WebM format selected, switching codec to VP8");
        }
        return true;
    }

    // MP4 works with H.264/H.265
    if (options.format == "mp4") {
        if (options.codec != "libx264" && options.codec != "libx265") {
            options.codec = "libx264"; // Default to H.264 for MP4
            DEBUG("MP4 format selected, switching codec to H.264");
        }
        return true;
    }

    // Default to WebM/VP8 if unsupported combination
    if (options.format != "webm" && options.format != "mp4") {
        options.format = "webm";
        options.codec = "libvpx-vp8";
        DEBUG("Unsupported format specified, defaulting to WebM/VP8");
    }
    return true;
}

bool VideoRecorder::are_dimensions_valid(int width, int height) {
    // Most codecs require dimensions to be even
    if (width % 2 != 0 || height % 2 != 0) {
        return false;
    }

    // Some codecs have minimum dimensions requirements
    if (width < 16 || height < 16) {
        return false;
    }

    // Most browsers have maximum resolution limitations
    if (width > 4096 || height > 4096) {
        return false;
    }

    return true;
}

bool VideoRecorder::initialize_from_image(const uimage &img) {
    options.width = img.get_dim().x;
    options.height = img.get_dim().y;

    // Ensure dimensions are even
    options.width = options.width % 2 == 0 ? options.width : options.width - 1;
    options.height = options.height % 2 == 0 ? options.height : options.height - 1;

    DEBUG("Recording dimensions set to match input image: " +
        std::to_string(options.width) + "x" + std::to_string(options.height));

    return initialize_video();
}

bool VideoRecorder::start_recording_adaptive(const uimage &first_frame, const RecordingOptions &base_opts) {
    if (state == RecordingState::RECORDING) {
        stop_recording();
    }

    options = base_opts;
    DEBUG("Start recording with codec: " + options.codec);

    // Get dimensions from the first frame
    int orig_width = first_frame.get_dim().x;
    int orig_height = first_frame.get_dim().y;

    // Maintain aspect ratio while potentially reducing dimensions for performance
    double aspect_ratio = (double)orig_width / orig_height;

    // If dimensions are large, scale down for better performance
    const int MAX_DIMENSION = 720;  // Maximum dimension for good performance

    if (orig_width > MAX_DIMENSION || orig_height > MAX_DIMENSION) {
        if (orig_width >= orig_height) {
            // Landscape orientation
            options.width = MAX_DIMENSION;
            options.height = (int)(MAX_DIMENSION / aspect_ratio);
        } else {
            // Portrait orientation
            options.height = MAX_DIMENSION;
            options.width = (int)(MAX_DIMENSION * aspect_ratio);
        }
    } else {
        // Use original dimensions if already small enough
        options.width = orig_width;
        options.height = orig_height;
    }

    // Ensure dimensions are even (required by most codecs)
    options.width = options.width % 2 == 0 ? options.width : options.width - 1;
    options.height = options.height % 2 == 0 ? options.height : options.height - 1;

    frame_count = 0;
    skip_counter = 0;

    DEBUG("Adaptive recording starting with dimensions: " +
        std::to_string(options.width) + "x" + std::to_string(options.height) +
        " (source: " + std::to_string(orig_width) + "x" + std::to_string(orig_height) + ")");

    // Clear previous output buffer
    output_buffer.clear();

    if (!initialize_video()) {
        cleanup();
        return false;
    }

    state = RecordingState::RECORDING;
    return true;
}