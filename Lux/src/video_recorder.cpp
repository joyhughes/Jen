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
                                frame_count(0) {
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
    DEBUG("Initializing video with virtual filesystem approach");
    DEBUG("Using temp file: " + temp_filename);

    // Validate codec/format combination
    validate_format_codec();

    // Create output format context with the temporary file
    avformat_alloc_output_context2(&format_context, nullptr, options.format.c_str(), temp_filename.c_str());
    if (!format_context) {
        ERROR("Failed to create output format context");
    }

    DEBUG("Using format: " + options.format + " with codec: " + options.codec);

    // Find encoder
    const AVCodec* codec = avcodec_find_encoder_by_name(options.codec.c_str());
    if (!codec) {
        ERROR("Codec not found: " + options.codec);
    }

    // Create a new video stream
    video_stream = avformat_new_stream(format_context, codec);
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
    codec_context->time_base = (AVRational){1, options.fps};
    codec_context->framerate = (AVRational){options.fps, 1};
    codec_context->gop_size = 12;  // Keyframe interval
    codec_context->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_context->bit_rate = options.bitrate;

    // Configure codec-specific options
    if (codec_context->codec_id == AV_CODEC_ID_H264) {
        av_opt_set(codec_context->priv_data, "preset", options.preset.c_str(), 0);
        // Better browser compatibility
        av_opt_set(codec_context->priv_data, "profile", "baseline", 0);
        av_opt_set(codec_context->priv_data, "tune", "zerolatency", 0);
    }

    // VP8/VP9 specific options (WebM)
    if (codec_context->codec_id == AV_CODEC_ID_VP8 ||
        codec_context->codec_id == AV_CODEC_ID_VP9) {
        av_opt_set(codec_context->priv_data, "quality", "realtime", 0);
        // Use speed 8 for fastest encoding (0-8, 8 is fastest)
        av_opt_set(codec_context->priv_data, "speed", "8", 0);
        // CRF value (0-63, lower is higher quality, 23-30 is reasonable)
        av_opt_set_int(codec_context->priv_data, "crf", 30, 0);
    }

    // Copy codec parameters to the stream
    avcodec_parameters_from_context(video_stream->codecpar, codec_context);

    // Open the codec
    if (avcodec_open2(codec_context, codec, nullptr) < 0) {
        ERROR("Failed to open codec");
    }

    // Allocate and initialize frames
    frame = av_frame_alloc();
    if (!frame) {
        ERROR("Failed to allocate frame");
    }

    frame->format = codec_context->pix_fmt;
    frame->width = codec_context->width;
    frame->height = codec_context->height;

    if (av_frame_get_buffer(frame, 0) < 0) {
        ERROR("Failed to allocate frame buffer");
    }

    tmp_frame = av_frame_alloc();
    if (!tmp_frame) {
        ERROR("Failed to allocate temporary frame");
    }

    tmp_frame->format = AV_PIX_FMT_RGBA;
    tmp_frame->width = codec_context->width;
    tmp_frame->height = codec_context->height;

    if (av_frame_get_buffer(tmp_frame, 0) < 0) {
        ERROR("Failed to allocate memory for temp frame buffer");
    }

    // Create SWS context for pixel format conversion (RGBA -> YUV420P)
    sws_context = sws_getContext(
        options.width, options.height, AV_PIX_FMT_RGBA,
        options.width, options.height, codec_context->pix_fmt,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    if (!sws_context) {
        ERROR("Failed to create SWS context");
    }

    // Open the output file using the virtual filesystem
    if (avio_open(&format_context->pb, temp_filename.c_str(), AVIO_FLAG_WRITE) < 0) {
        ERROR("Failed to open output file");
    }

    // Write the header
    if (avformat_write_header(format_context, nullptr) < 0) {
        ERROR("Failed to write header");
    }

    DEBUG("Video initialization complete with dimensions: " +
          std::to_string(options.width) + "x" + std::to_string(options.height));

    return true;
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
bool VideoRecorder::add_frame(const uimage &img) {
    if (state != RecordingState::RECORDING) {
        return false;
    }

    bool need_resize = img.get_dim().x != options.width || img.get_dim().y != options.height;
    if (need_resize && frame_count == 0) {
        DEBUG("Input image dimensions (" + std::to_string(img.get_dim().x) + "x" +
            std::to_string(img.get_dim().y) + ") differ from recording dimensions (" +
            std::to_string(options.width) + "x" + std::to_string(options.height) +
            "). Using scaling.");
    }

    const unsigned char *img_data = (const unsigned char *)img.get_base_ptr();
    if (need_resize) {
        // Scale the image to the target dimensions
        SwsContext *resize_context = sws_getContext(
            img.get_dim().x, img.get_dim().y, AV_PIX_FMT_RGBA,
            options.width, options.height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, nullptr, nullptr, nullptr
        );

        if (!resize_context) {
            ERROR("Failed to create resize context");
        }

        // Set up source pointers and stride
        const uint8_t *src_data[4] = {img_data, nullptr, nullptr, nullptr};
        int src_linesize[4] = {img.get_dim().x * 4, 0, 0, 0}; // RGBA = 4 bytes

        // Scale directly to temp frame
        sws_scale(resize_context, src_data, src_linesize, 0, img.get_dim().y,
                  tmp_frame->data, tmp_frame->linesize);

        sws_freeContext(resize_context);
    } else {
        // Copy the image data directly to the temp frame
        for (int y = 0; y < options.height; y++) {
            for (int x = 0; x < options.width; x++) {
                int img_idx = (y * options.width + x) * 4; // RGBA
                int frame_idx = y * tmp_frame->linesize[0] + x * 4; // RGBA

                tmp_frame->data[0][frame_idx]     = img_data[img_idx];     // R
                tmp_frame->data[0][frame_idx + 1] = img_data[img_idx + 1]; // G
                tmp_frame->data[0][frame_idx + 2] = img_data[img_idx + 2]; // B
                tmp_frame->data[0][frame_idx + 3] = img_data[img_idx + 3]; // A
            }
        }
    }

    // Convert from RGBA to the codec's pixel format (usually YUV420P)
    sws_scale(sws_context, tmp_frame->data, tmp_frame->linesize, 0,
              options.height, frame->data, frame->linesize);

    // Set frame timestamp
    frame->pts = frame_count;

    // Encode the frame
    int ret = avcodec_send_frame(codec_context, frame);
    if (ret < 0) {
        ERROR("Error sending frame to encoder");
    }

    while (ret >= 0) {
        AVPacket pkt = {0};
        ret = avcodec_receive_packet(codec_context, &pkt);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            ERROR("Error receiving packet from encoder");
        }

        // Rescale packet timestamps
        av_packet_rescale_ts(&pkt, codec_context->time_base, video_stream->time_base);
        pkt.stream_index = video_stream->index;

        // Write packet to file
        ret = av_interleaved_write_frame(format_context, &pkt);
        if (ret < 0) {
            ERROR("Error writing packet to file");
        }

        av_packet_unref(&pkt);
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
            options.codec = "libvpx"; // Default to VP8 for WebM
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
        options.codec = "libvpx";
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

    // Update dimensions based on first frame
    options.width = first_frame.get_dim().x;
    options.height = first_frame.get_dim().y;

    // Ensure dimensions are even
    options.width = options.width % 2 == 0 ? options.width : options.width - 1;
    options.height = options.height % 2 == 0 ? options.height : options.height - 1;

    frame_count = 0;

    DEBUG("Adaptive recording starting with dimensions: " +
        std::to_string(options.width) + "x" + std::to_string(options.height));

    // Clear previous output buffer
    output_buffer.clear();

    if (!initialize_video()) {
        cleanup();
        return false;
    }

    state = RecordingState::RECORDING;
    return true;
}