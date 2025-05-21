#include "video_recorder.hpp"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

// Add FFmpeg headers
extern "C" {
#include <libavutil/dict.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

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

bool VideoRecorder::initialize_video() {
    DEBUG("Initializing video with virtual filesystem approach");
    DEBUG("Using temp file: " + temp_filename);

    // Validate codec/format combination
    validate_format_codec();

    // Create output format context with the temporary file
    avformat_alloc_output_context2(&format_context, nullptr, "webm", temp_filename.c_str());
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
    codec_context->gop_size = 30;  // Keyframe every second at 30fps
    codec_context->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_context->bit_rate = options.bitrate;

    // Mobile-optimized encoding settings
    if (codec_context->codec_id == AV_CODEC_ID_VP8) {
        // Optimize for mobile performance
        av_opt_set(codec_context->priv_data, "quality", "realtime", 0);
        av_opt_set(codec_context->priv_data, "speed", "8", 0);  // Fastest encoding
        av_opt_set_int(codec_context->priv_data, "crf", 30, 0); // Balanced quality
        av_opt_set_int(codec_context->priv_data, "deadline", 1, 0); // Realtime encoding
        av_opt_set_int(codec_context->priv_data, "cpu-used", 16, 0); // Maximum speed
        
        // Optimize buffer sizes
        codec_context->thread_count = 1;  // Use 1 thread for encoding
        codec_context->thread_type = FF_THREAD_FRAME;
        
        // Reduce memory usage
        codec_context->rc_buffer_size = options.bitrate / 2;  // Smaller buffer
        codec_context->rc_max_rate = options.bitrate;  // Cap bitrate
        codec_context->rc_min_rate = options.bitrate / 2;  // Minimum bitrate
    }

    // Copy codec parameters to the stream
    avcodec_parameters_from_context(video_stream->codecpar, codec_context);

    // Set stream time base
    video_stream->time_base = codec_context->time_base;

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

    // Set WebM specific options
    AVDictionary* options = nullptr;
    av_dict_set(&options, "movflags", "faststart", 0);
    av_dict_set(&options, "brand", "webm", 0);
    av_dict_set(&options, "use_metadata_tags", "1", 0);

    // Write the header
    if (avformat_write_header(format_context, &options) < 0) {
        ERROR("Failed to write header");
    }

    // Free options dictionary
    av_dict_free(&options);

    return true;
}

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
        // Direct memory copy for same dimensions
        const int total_bytes = options.width * options.height * 4;
        memcpy(tmp_frame->data[0], img_data, total_bytes);
    }

    // Convert from RGBA to the codec's pixel format (usually YUV420P)
    sws_scale(sws_context, tmp_frame->data, tmp_frame->linesize, 0,
              options.height, frame->data, frame->linesize);

    // Set frame timestamp in codec timebase
    frame->pts = frame_count;
    frame->duration = 1; // Each frame lasts 1 timebase unit

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

        // Rescale packet timestamps to stream timebase
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

bool VideoRecorder::stop_recording() {
    if (state != RecordingState::RECORDING) {
        DEBUG("Cannot stop recording - not in recording state");
        return false;
    }
    
    if (frame_count == 0) {
        DEBUG("No frames recorded, cannot create video");
        state = RecordingState::IDLE;
        return false;
    }

    state = RecordingState::ENCODING;
    DEBUG("Finalizing video with " + std::to_string(frame_count) + " frames");

    // First send NULL frame to signal end of stream
    int ret = avcodec_send_frame(codec_context, nullptr);
    if (ret < 0) {
        ERROR("Error sending flush frame to encoder: " + std::to_string(ret));
    }

    // Process ALL remaining packets
    bool encoding_finished = false;
    while (!encoding_finished) {
        AVPacket pkt = {0};
        ret = avcodec_receive_packet(codec_context, &pkt);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            encoding_finished = true;
        } else if (ret < 0) {
            ERROR("Error receiving packet from encoder during flush: " + std::to_string(ret));
        } else {
            // Got a packet, process it
            av_packet_rescale_ts(&pkt, codec_context->time_base, video_stream->time_base);
            pkt.stream_index = video_stream->index;

            ret = av_interleaved_write_frame(format_context, &pkt);
            if (ret < 0) {
                ERROR("Error writing packet to file during flush: " + std::to_string(ret));
            }
            av_packet_unref(&pkt);
        }
    }

    // Calculate accurate duration
    double duration_seconds = static_cast<double>(frame_count) / options.fps;
    
    // Set stream duration in stream timebase
    video_stream->duration = av_rescale_q(
        frame_count, 
        AVRational{1, options.fps}, 
        video_stream->time_base
    );
    
    // Set container duration in AV_TIME_BASE units
    format_context->duration = static_cast<int64_t>(duration_seconds * AV_TIME_BASE);
    
    // Write WebM metadata
    AVDictionary* metadata = NULL;
    char duration_str[32];
    snprintf(duration_str, sizeof(duration_str), "%.3f", duration_seconds);
    av_dict_set(&metadata, "DURATION", duration_str, 0);
    
    // Add WebM specific metadata
    av_dict_set(&metadata, "ENCODER", "Lux Video Recorder", 0);
    av_dict_set(&metadata, "ENCODER_OPTIONS", "realtime", 0);
    
    // Set stream metadata
    av_dict_set(&video_stream->metadata, "language", "und", 0);
    
    
    // Write trailer including metadata
    ret = av_write_trailer(format_context);
    if (ret < 0) {
        ERROR("Error writing file trailer: " + std::to_string(ret));
    } else {
        DEBUG("Trailer written successfully");
    }

    // Close the output file
    if (format_context && format_context->pb) {
        int close_ret = avio_closep(&format_context->pb);
        if (close_ret < 0) {
            ERROR("Error closing AVIO: " + std::to_string(close_ret));
        } else {
            DEBUG("AVIO closed successfully");
        }
    }

    // Sync the virtual FS to ensure file is available
    #ifdef __EMSCRIPTEN__
    EM_ASM(
        FS.syncfs(false, function() {
            // FS synced
        });
    );
    #endif

    // Read temp file directly into memory
    DEBUG("Attempting to open temp file for reading: " + temp_filename);
    FILE* src_file = fopen(temp_filename.c_str(), "rb");
    if (!src_file) {
        ERROR("Failed to open temp file for reading: " + temp_filename + " : " + std::string(strerror(errno)));
    }
    fseek(src_file, 0, SEEK_END);
    long file_size = ftell(src_file);
    fseek(src_file, 0, SEEK_SET);
    if (file_size < 32) {
        fclose(src_file);
        ERROR("Temp file too small, likely not a valid WebM: " + std::to_string(file_size) + " bytes");
    }
    output_buffer.resize(file_size);
    size_t bytes_read = fread(output_buffer.data(), 1, file_size, src_file);
    fclose(src_file);
    if (bytes_read != file_size) {
        ERROR("Failed to read all bytes from temp file: got " + std::to_string(bytes_read) + " expected " + std::to_string(file_size));
    }
    // Debug: print first 16 bytes
    DEBUG("First 16 bytes of temp file:");
    for (int i = 0; i < 16 && i < output_buffer.size(); ++i) {
        printf("%02x ", output_buffer[i]);
    }
    printf("\n");

    // Clean up resources
    cleanup();

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
    // Always use WebM/VP8 for best mobile compatibility
    options.format = "webm";
    options.codec = "libvpx";
    DEBUG("Using WebM/VP8 for best mobile compatibility");
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