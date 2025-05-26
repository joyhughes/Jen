#include "video_recorder.hpp"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

// Add FFmpeg headers
extern "C"
{
#include <libavutil/dict.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#define DEBUG(msg)                           \
    {                                        \
        std::string debug_msg = msg;         \
        std::cout << debug_msg << std::endl; \
    }
#define ERROR(msg)                     \
    {                                  \
        error_message = msg;           \
        state = RecordingState::ERROR; \
        return false;                  \
    }

VideoRecorder::VideoRecorder() : state(RecordingState::IDLE),
                                 format_context(nullptr),
                                 codec_context(nullptr),
                                 video_stream(nullptr),
                                 sws_context(nullptr),
                                 frame(nullptr),
                                 tmp_frame(nullptr),
                                 frame_count(0)
{
    // Generate unique temp filename
    temp_filename = "/tmp/recording_" + std::to_string(reinterpret_cast<uintptr_t>(this)) + ".data";
}

VideoRecorder::~VideoRecorder()
{
    if (state == RecordingState::RECORDING)
    {
        stop_recording();
    }
    cleanup();

    // Clean up temporary file if it exists
    if (std::remove(temp_filename.c_str()) == 0)
    {
        DEBUG("Temporary file removed: " + temp_filename);
    }
}

bool VideoRecorder::initialize_video()
{
    DEBUG("Initializing H.264/MP4 video with virtual filesystem approach");
    DEBUG("Using temp file: " + temp_filename);

    // Validate codec/format combination
    validate_format_codec();

    // Create output format context
    avformat_alloc_output_context2(&format_context, nullptr, "mp4", temp_filename.c_str());
    if (!format_context)
    {
        ERROR("Failed to create MP4 output format context");
    }

    DEBUG("Using format: " + options.format + " with codec: " + options.codec);

    // Find encoder
    const AVCodec *codec = avcodec_find_encoder_by_name(options.codec.c_str());
    if (!codec)
    {
        ERROR("H.264 codec not found: " + options.codec);
    }

    // Create a new video stream
    video_stream = avformat_new_stream(format_context, codec);
    if (!video_stream)
    {
        ERROR("Failed to create video stream");
    }

    // Allocate codec context
    codec_context = avcodec_alloc_context3(codec);
    if (!codec_context)
    {
        ERROR("Failed to allocate codec context");
    }

    // Set codec parameters
    codec_context->codec_id = codec->id;
    codec_context->codec_type = AVMEDIA_TYPE_VIDEO;
    codec_context->width = options.width;
    codec_context->height = options.height;
    codec_context->time_base = AVRational{1, options.fps};
    codec_context->framerate = AVRational{options.fps, 1};
    
    // MOBILE COMPATIBILITY: Set adaptive GOP size based on resolution
    // This will be used for keyframe forcing and must match the keyint setting
    bool isHighRes = (options.width >= 1280 || options.height >= 720);
    bool isLowRes = (options.width <= 640 && options.height <= 480);
    codec_context->gop_size = isHighRes ? 60 : isLowRes ? 15 : 30;
    
    codec_context->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_context->bit_rate = options.bitrate;

    // CRITICAL: Set proper color space and range for RGBA->YUV420P conversion
    codec_context->color_range = AVCOL_RANGE_JPEG;  // Full range (0-255)
    codec_context->color_primaries = AVCOL_PRI_BT709;
    codec_context->color_trc = AVCOL_TRC_BT709;
    codec_context->colorspace = AVCOL_SPC_BT709;

    if (codec_context->codec_id == AV_CODEC_ID_H264)
    {
        std::string preset = options.preset.empty() || options.preset == "realtime" ? "ultrafast" : options.preset;
        av_opt_set(codec_context->priv_data, "preset", preset.c_str(), 0);
        av_opt_set(codec_context->priv_data, "tune", "zerolatency", 0);
        av_opt_set(codec_context->priv_data, "profile", "baseline", 0);
        av_opt_set_int(codec_context->priv_data, "crf", 23, 0);

        // CRITICAL: Configure for MP4 container - use AVCC format, not Annex B
        codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        
        // Force AVCC format (length-prefixed NAL units) for MP4 compatibility
        av_opt_set_int(codec_context->priv_data, "annexb", 0, 0);
        
        // MOBILE COMPATIBILITY: Adaptive settings based on resolution
        DEBUG("Configuring H.264 for " + std::to_string(options.width) + "x" + std::to_string(options.height) + 
              (isHighRes ? " (high resolution)" : isLowRes ? " (low resolution)" : " (standard resolution)"));
        
        // CRITICAL: Force keyframe generation and SPS/PPS inclusion
        av_opt_set_int(codec_context->priv_data, "force-cfr", 1, 0);
        
        // MOBILE COMPATIBILITY: Adaptive keyframe interval based on resolution
        int keyframe_interval = isHighRes ? 60 : isLowRes ? 15 : 30;
        av_opt_set_int(codec_context->priv_data, "keyint", keyframe_interval, 0);
        av_opt_set_int(codec_context->priv_data, "keyint_min", 1, 0);
        av_opt_set_int(codec_context->priv_data, "scenecut", 0, 0);
        av_opt_set_int(codec_context->priv_data, "intra-refresh", 0, 0);
        
        DEBUG("Using keyframe interval: " + std::to_string(keyframe_interval) + " frames for mobile compatibility");
        
        // CRITICAL: Force full range encoding to match input
        av_opt_set(codec_context->priv_data, "colorprim", "bt709", 0);
        av_opt_set(codec_context->priv_data, "transfer", "bt709", 0);
        av_opt_set(codec_context->priv_data, "colormatrix", "bt709", 0);
        av_opt_set(codec_context->priv_data, "range", "pc", 0);  // Full range
        
        // CRITICAL: Ensure SPS/PPS are written
        av_opt_set_int(codec_context->priv_data, "repeat-headers", 1, 0);  // Repeat SPS/PPS
        av_opt_set_int(codec_context->priv_data, "aud", 1, 0);  // Add access unit delimiters
        
        // MOBILE COMPATIBILITY: Optimize encoding settings for different resolutions
        if (isLowRes) {
            // Low resolution: prioritize speed and compatibility
            av_opt_set(codec_context->priv_data, "level", "3.0", 0);  // H.264 Level 3.0 for mobile
            av_opt_set_int(codec_context->priv_data, "refs", 1, 0);  // Single reference frame
            av_opt_set_int(codec_context->priv_data, "b-frames", 0, 0);  // No B-frames for simplicity
        } else if (isHighRes) {
            // High resolution: balance quality and compatibility
            av_opt_set(codec_context->priv_data, "level", "4.0", 0);  // H.264 Level 4.0 for HD
            av_opt_set_int(codec_context->priv_data, "refs", 2, 0);  // Two reference frames
            av_opt_set_int(codec_context->priv_data, "b-frames", 1, 0);  // Minimal B-frames
        } else {
            // Standard resolution: balanced settings
            av_opt_set(codec_context->priv_data, "level", "3.1", 0);  // H.264 Level 3.1
            av_opt_set_int(codec_context->priv_data, "refs", 1, 0);  // Single reference frame
            av_opt_set_int(codec_context->priv_data, "b-frames", 0, 0);  // No B-frames
        }
        
        // Use standard x264 options for MP4 compatibility
        av_opt_set(codec_context->priv_data, "x264opts", "force-cfr=1:no-scenecut=1", 0);

        codec_context->thread_count = 1;
        codec_context->thread_type = FF_THREAD_FRAME;

        codec_context->rc_buffer_size = options.bitrate;
        codec_context->rc_max_rate = static_cast<int>(options.bitrate * 1.2);
        codec_context->rc_min_rate = static_cast<int>(options.bitrate * 0.5);

        av_opt_set_int(codec_context->priv_data, "fast-pskip", 1, 0);
        av_opt_set_int(codec_context->priv_data, "no-dct-decimate", 1, 0);

        DEBUG("Using H.264 preset: " + preset + " with AVCC format, adaptive keyframes, and mobile-optimized settings for MP4 container");
    }

    video_stream->time_base = codec_context->time_base;

    // Open the codec
    if (avcodec_open2(codec_context, codec, nullptr) < 0)
    {
        ERROR("Failed to open H.264 codec");
    }

    // CRITICAL: Copy codec context params to stream AFTER opening codec
    // This ensures SPS/PPS extradata is available for MP4 container
    if (avcodec_parameters_from_context(video_stream->codecpar, codec_context) < 0)
    {
        ERROR("Failed to copy codec parameters to stream");
    }
    
    // Verify extradata was copied for H.264
    if (codec_context->codec_id == AV_CODEC_ID_H264) {
        if (video_stream->codecpar->extradata_size > 0) {
            DEBUG("H.264 extradata (SPS/PPS) copied successfully: " + std::to_string(video_stream->codecpar->extradata_size) + " bytes");
            
            // VALIDATION: Verify SPS/PPS structure
            uint8_t* extradata = video_stream->codecpar->extradata;
            if (extradata[0] == 0x01) {  // AVCC format marker
                DEBUG("✓ AVCC format detected in extradata");
            } else {
                DEBUG("⚠️  WARNING: Unexpected extradata format - may cause playback issues");
            }
        } else {
            ERROR("CRITICAL: No H.264 extradata found - SPS/PPS missing! Video will be unplayable.");
        }
        
        // VALIDATION: Verify codec is actually opened
        if (!avcodec_is_open(codec_context)) {
            ERROR("CRITICAL: Codec context not properly opened despite success return");
        }
    }

    // Allocate YUV420P frame (for encoder)
    frame = av_frame_alloc();
    if (!frame)
    {
        ERROR("Failed to allocate YUV420P frame");
    }

    frame->format = codec_context->pix_fmt;
    frame->width = codec_context->width;
    frame->height = codec_context->height;

    if (av_frame_get_buffer(frame, 0) < 0)
    {
        ERROR("Failed to allocate buffer for YUV420P frame");
    }

    // Allocate RGBA temporary frame (input from app)
    tmp_frame = av_frame_alloc();
    if (!tmp_frame)
    {
        ERROR("Failed to allocate temporary RGBA frame");
    }

    tmp_frame->format = AV_PIX_FMT_RGBA;
    tmp_frame->width = codec_context->width;
    tmp_frame->height = codec_context->height;

    if (av_frame_get_buffer(tmp_frame, 0) < 0)
    {
        ERROR("Failed to allocate buffer for temporary RGBA frame");
    }

    // Create sws context to convert RGBA -> YUV420P
    sws_context = sws_getContext(
        options.width, options.height, AV_PIX_FMT_RGBA,
        options.width, options.height, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!sws_context)
    {
        ERROR("Failed to create SWS context for RGBA to YUV420P conversion");
    }

    // CRITICAL: Set color space conversion parameters for full range
    int *inv_table, *table;
    int srcRange, dstRange, brightness, contrast, saturation;
    
    // Get current conversion parameters
    sws_getColorspaceDetails(sws_context, &inv_table, &srcRange, &table, &dstRange, 
                            &brightness, &contrast, &saturation);
    
    // Set full range for both input (RGBA) and output (YUV420P)
    srcRange = 1; // Full range input (0-255)
    dstRange = 1; // Full range output (0-255)
    
    // Apply the color space settings
    sws_setColorspaceDetails(sws_context, inv_table, srcRange, table, dstRange,
                            brightness, contrast, saturation);

    // Open the virtual output file
    if (avio_open(&format_context->pb, temp_filename.c_str(), AVIO_FLAG_WRITE) < 0)
    {
        ERROR("Failed to open output file in virtual filesystem");
    }

    // Set MP4 specific options for proper MOOV atom handling
    AVDictionary *format_opts = nullptr;
    
    // CRITICAL: Configure MP4 muxer for proper H.264 stream handling
    av_dict_set(&format_opts, "movflags", "faststart+frag_keyframe+empty_moov+default_base_moof", 0);
    
    // Force proper H.264 stream format in MP4 container
    av_dict_set(&format_opts, "video_track_timescale", "30000", 0); // Match framerate
    
    // Set proper MP4 brand and compatibility
    av_dict_set(&format_opts, "brand", "mp42", 0);
    av_dict_set(&format_opts, "compatible_brands", "mp42,mp41,isom", 0);
    
    // Enable metadata writing
    av_dict_set(&format_opts, "use_metadata_tags", "1", 0);
    
    // Set proper fragment duration for better seeking
    av_dict_set(&format_opts, "frag_duration", "1000000", 0); // 1 second fragments
    
    // Ensure proper MOOV atom structure
    av_dict_set(&format_opts, "write_tmcd", "0", 0); // Disable timecode track
    
    // CRITICAL: Force proper H.264 parameter set handling
    av_dict_set(&format_opts, "avoid_negative_ts", "make_zero", 0);
    av_dict_set(&format_opts, "fflags", "+genpts", 0);
    
    DEBUG("Writing MP4 header with H.264 stream configuration...");
    if (avformat_write_header(format_context, &format_opts) < 0)
    {
        ERROR("Failed to write MP4 header with H.264 configuration");
    }

    av_dict_free(&format_opts);
    DEBUG("MP4 header with H.264 stream configuration written successfully");
    
    // VALIDATION: Verify MP4 container state after header write
    if (format_context->nb_streams != 1) {
        DEBUG("⚠️  WARNING: Expected 1 stream, got " + std::to_string(format_context->nb_streams));
    }
    
    if (video_stream->codecpar->codec_id != AV_CODEC_ID_H264) {
        ERROR("CRITICAL: Stream codec ID mismatch - expected H.264");
    }
    
    if (video_stream->codecpar->extradata_size == 0) {
        ERROR("CRITICAL: Stream extradata empty after header write - MP4 will be invalid");
    }
    
    DEBUG("✓ MP4 container validation passed - ready for frame encoding");

    return true;
}

bool VideoRecorder::add_frame(const uimage &img)
{
    if (state != RecordingState::RECORDING)
        return false;

    const int src_width = img.get_dim().x;
    const int src_height = img.get_dim().y;
    const ucolor *ucolor_data = img.get_base_ptr();

    // Allocate temporary RGBA buffer
    std::vector<uint8_t> rgba_buffer(src_width * src_height * 4);
    uint8_t *rgba_data = rgba_buffer.data();

    // Convert ucolor (ARGB) to RGBA
    for (int i = 0; i < src_width * src_height; ++i)
    {
        rgba_data[i * 4 + 0] = rc(ucolor_data[i]); // R
        rgba_data[i * 4 + 1] = gc(ucolor_data[i]); // G
        rgba_data[i * 4 + 2] = bc(ucolor_data[i]); // B
        rgba_data[i * 4 + 3] = ac(ucolor_data[i]); // A
    }

    // Use the efficient RGBA method
    return add_frame_rgba(rgba_data, src_width, src_height);
}

bool VideoRecorder::add_frame_rgba(const uint8_t* rgba_data, int width, int height)
{
    std::cout << "[VideoRecorder] === ADD_FRAME_RGBA START ===" << std::endl;
    std::cout << "[VideoRecorder] Input parameters:" << std::endl;
    std::cout << "[VideoRecorder] - Width: " << width << std::endl;
    std::cout << "[VideoRecorder] - Height: " << height << std::endl;
    std::cout << "[VideoRecorder] - RGBA data pointer: " << (rgba_data ? "valid" : "null") << std::endl;
    
    if (state != RecordingState::RECORDING)
    {
        std::cerr << "[VideoRecorder] ERROR: Not in recording state" << std::endl;
        std::cerr << "[VideoRecorder] - Current state: " << static_cast<int>(state) << std::endl;
        return false;
    }

    if (!rgba_data || width <= 0 || height <= 0)
    {
        std::cerr << "[VideoRecorder] ERROR: Invalid input parameters" << std::endl;
        std::cerr << "[VideoRecorder] - rgba_data: " << (rgba_data ? "valid" : "null") << std::endl;
        std::cerr << "[VideoRecorder] - width: " << width << std::endl;
        std::cerr << "[VideoRecorder] - height: " << height << std::endl;
        return false;
    }

    std::cout << "[VideoRecorder] Codec context validation:" << std::endl;
    std::cout << "[VideoRecorder] - codec_context: " << (codec_context ? "valid" : "null") << std::endl;
    if (codec_context) {
        std::cout << "[VideoRecorder] - codec_context->width: " << codec_context->width << std::endl;
        std::cout << "[VideoRecorder] - codec_context->height: " << codec_context->height << std::endl;
        std::cout << "[VideoRecorder] - codec_context->pix_fmt: " << codec_context->pix_fmt << std::endl;
    }

    // ADAPTIVE DIMENSIONS: Handle different input sizes with automatic scaling
    std::cout << "[VideoRecorder] Adaptive dimension handling:" << std::endl;
    std::cout << "[VideoRecorder] - Input frame: " << width << "x" << height << std::endl;
    std::cout << "[VideoRecorder] - Recording target: " << codec_context->width << "x" << codec_context->height << std::endl;
    
    bool needsScaling = (width != codec_context->width || height != codec_context->height);
    if (needsScaling) {
        std::cout << "[VideoRecorder] ✓ Automatic scaling enabled for adaptive dimensions" << std::endl;
    } else {
        std::cout << "[VideoRecorder] ✓ Direct conversion - dimensions match" << std::endl;
    }

    // Convert RGBA to YUV420P with scaling if needed
    std::cout << "[VideoRecorder] Setting up conversion parameters..." << std::endl;
    const uint8_t *src_slices[1] = { rgba_data };
    int src_stride[1] = { width * 4 };
    std::cout << "[VideoRecorder] - Source stride: " << src_stride[0] << std::endl;

    std::cout << "[VideoRecorder] Creating SWS context for adaptive scaling..." << std::endl;
    std::cout << "[VideoRecorder] - Input: " << width << "x" << height << " RGBA" << std::endl;
    std::cout << "[VideoRecorder] - Output: " << codec_context->width << "x" << codec_context->height << " YUV420P" << std::endl;
    
    // MOBILE COMPATIBILITY: Use high-quality scaling for better mobile playback
    int scalingAlgorithm = needsScaling ? SWS_LANCZOS : SWS_BILINEAR;
    std::cout << "[VideoRecorder] - Scaling algorithm: " << (needsScaling ? "SWS_LANCZOS (high quality)" : "SWS_BILINEAR (fast)") << std::endl;
    
    SwsContext *conversion_context = sws_getContext(
        width, height, AV_PIX_FMT_RGBA,
        codec_context->width, codec_context->height, codec_context->pix_fmt,
        scalingAlgorithm, nullptr, nullptr, nullptr);

    if (!conversion_context)
    {
        std::cerr << "[VideoRecorder] ERROR: Failed to create conversion context" << std::endl;
        ERROR("Failed to create conversion context");
    }
    std::cout << "[VideoRecorder] SWS context created successfully" << std::endl;

    // CRITICAL: Set full range color space conversion
    int *inv_table, *table;
    int srcRange, dstRange, brightness, contrast, saturation;
    
    // Get current conversion parameters
    sws_getColorspaceDetails(conversion_context, &inv_table, &srcRange, &table, &dstRange, 
                            &brightness, &contrast, &saturation);
    
    // Set full range for both input (RGBA) and output (YUV420P)
    srcRange = 1; // Full range input (0-255)
    dstRange = 1; // Full range output (0-255)
    
    // Apply the color space settings
    sws_setColorspaceDetails(conversion_context, inv_table, srcRange, table, dstRange,
                            brightness, contrast, saturation);
    std::cout << "[VideoRecorder] Full range color space conversion configured" << std::endl;

    std::cout << "[VideoRecorder] Frame buffer validation:" << std::endl;
    std::cout << "[VideoRecorder] - frame: " << (frame ? "valid" : "null") << std::endl;
    if (frame) {
        std::cout << "[VideoRecorder] - frame->data[0]: " << (frame->data[0] ? "valid" : "null") << std::endl;
        std::cout << "[VideoRecorder] - frame->linesize[0]: " << frame->linesize[0] << std::endl;
        std::cout << "[VideoRecorder] - frame->width: " << frame->width << std::endl;
        std::cout << "[VideoRecorder] - frame->height: " << frame->height << std::endl;
        std::cout << "[VideoRecorder] - frame->format: " << frame->format << std::endl;
    }

    std::cout << "[VideoRecorder] Performing color space conversion..." << std::endl;
    int conversion_result = sws_scale(
        conversion_context,
        src_slices, src_stride,
        0, height,
        frame->data, frame->linesize);
    
    std::cout << "[VideoRecorder] sws_scale returned: " << conversion_result << std::endl;
    if (conversion_result <= 0) {
        std::cerr << "[VideoRecorder] ERROR: Color space conversion failed" << std::endl;
        sws_freeContext(conversion_context);
        return false;
    }

    sws_freeContext(conversion_context);
    std::cout << "[VideoRecorder] Color space conversion completed successfully" << std::endl;

    frame->pts = frame_count;
    std::cout << "[VideoRecorder] Frame PTS set to: " << frame->pts << std::endl;
    std::cout << "[VideoRecorder] Current frame count: " << frame_count << std::endl;

    // CRITICAL: Force keyframes at adaptive intervals for H.264 mobile compatibility
    int keyframe_interval = codec_context->gop_size; // Use the GOP size set during initialization
    if (frame_count % keyframe_interval == 0) {
        frame->pict_type = AV_PICTURE_TYPE_I;  // Force I-frame (keyframe)
        std::cout << "[VideoRecorder] Forcing keyframe at frame " << frame_count << " (interval: " << keyframe_interval << ")" << std::endl;
    } else {
        frame->pict_type = AV_PICTURE_TYPE_NONE;  // Let encoder decide
    }

    std::cout << "[VideoRecorder] Sending frame to encoder..." << std::endl;
    int ret = avcodec_send_frame(codec_context, frame);
    std::cout << "[VideoRecorder] avcodec_send_frame returned: " << ret << std::endl;
    
    if (ret < 0)
    {
        char error_buf[256];
        av_strerror(ret, error_buf, sizeof(error_buf));
        std::cerr << "[VideoRecorder] ERROR: Error sending frame to encoder: " << error_buf << std::endl;
        ERROR("Error sending frame");
    }

    std::cout << "[VideoRecorder] Processing encoder output packets..." << std::endl;
    int packet_count = 0;
    while (ret >= 0)
    {
        AVPacket pkt = {};
        ret = avcodec_receive_packet(codec_context, &pkt);
        
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            std::cout << "[VideoRecorder] No more packets available (EAGAIN/EOF)" << std::endl;
            break;
        } else if (ret < 0) {
            char error_buf[256];
            av_strerror(ret, error_buf, sizeof(error_buf));
            std::cerr << "[VideoRecorder] ERROR: Error receiving packet: " << error_buf << std::endl;
            ERROR("Error receiving packet");
        }

        packet_count++;
        std::cout << "[VideoRecorder] Received packet #" << packet_count << std::endl;
        std::cout << "[VideoRecorder] - Packet size: " << pkt.size << std::endl;
        std::cout << "[VideoRecorder] - Packet pts: " << pkt.pts << std::endl;
        std::cout << "[VideoRecorder] - Packet dts: " << pkt.dts << std::endl;

        av_packet_rescale_ts(&pkt, codec_context->time_base, video_stream->time_base);
        pkt.stream_index = video_stream->index;

        std::cout << "[VideoRecorder] Writing packet to container..." << std::endl;
        int write_result = av_interleaved_write_frame(format_context, &pkt);
        if (write_result < 0) {
            char error_buf[256];
            av_strerror(write_result, error_buf, sizeof(error_buf));
            std::cerr << "[VideoRecorder] ERROR: Error writing packet: " << error_buf << std::endl;
            av_packet_unref(&pkt);
            ERROR("Error writing packet");
        }
        std::cout << "[VideoRecorder] Packet written successfully" << std::endl;

        av_packet_unref(&pkt);
    }

    frame_count++;
    std::cout << "[VideoRecorder] Frame processing completed successfully" << std::endl;
    std::cout << "[VideoRecorder] - Total packets generated: " << packet_count << std::endl;
    std::cout << "[VideoRecorder] - New frame count: " << frame_count << std::endl;
    std::cout << "[VideoRecorder] === ADD_FRAME_RGBA END ===" << std::endl;

    return true;
}

bool VideoRecorder::start_recording(const RecordingOptions &opts)
{
    if (state == RecordingState::RECORDING)
    {
        stop_recording();
    }

    options = opts;
    frame_count = 0;

    // Clear previous output buffer
    output_buffer.clear();

    if (!initialize_video())
    {
        cleanup();
        return false;
    }

    state = RecordingState::RECORDING;
    DEBUG("Recording started");
    return true;
}

bool VideoRecorder::stop_recording()
{
    if (state != RecordingState::RECORDING)
    {
        DEBUG("Cannot stop recording - not in recording state");
        return false;
    }

    if (frame_count == 0)
    {
        DEBUG("No frames recorded, cannot create video");
        state = RecordingState::IDLE;
        return false;
    }

    state = RecordingState::ENCODING;
    DEBUG("Finalizing video with " + std::to_string(frame_count) + " frames");

    // CRITICAL: Flush encoder properly to ensure all frames are written
    DEBUG("Flushing H.264 encoder...");
    int ret = avcodec_send_frame(codec_context, nullptr);
    if (ret < 0)
    {
        char error_buf[256];
        av_strerror(ret, error_buf, sizeof(error_buf));
        DEBUG("Warning: Error sending flush frame to encoder: " + std::string(error_buf));
        // Continue anyway - some packets might still be available
    }

    // Process ALL remaining packets with detailed logging
    bool encoding_finished = false;
    int flush_packet_count = 0;
    while (!encoding_finished)
    {
        AVPacket pkt = {0};
        ret = avcodec_receive_packet(codec_context, &pkt);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            encoding_finished = true;
            DEBUG("Encoder flush completed - no more packets available");
        }
        else if (ret < 0)
        {
            char error_buf[256];
            av_strerror(ret, error_buf, sizeof(error_buf));
            DEBUG("Warning: Error receiving packet from encoder during flush: " + std::string(error_buf));
            encoding_finished = true;  // Stop trying
        }
        else
        {
            // Got a packet, process it
            flush_packet_count++;
            DEBUG("Processing flush packet #" + std::to_string(flush_packet_count) + " (size: " + std::to_string(pkt.size) + ")");
            
            av_packet_rescale_ts(&pkt, codec_context->time_base, video_stream->time_base);
            pkt.stream_index = video_stream->index;

            ret = av_interleaved_write_frame(format_context, &pkt);
            if (ret < 0)
            {
                char error_buf[256];
                av_strerror(ret, error_buf, sizeof(error_buf));
                DEBUG("Warning: Error writing flush packet: " + std::string(error_buf));
            }
            av_packet_unref(&pkt);
        }
    }
    
    DEBUG("Encoder flush completed with " + std::to_string(flush_packet_count) + " additional packets");

    // Calculate accurate duration and set metadata BEFORE writing trailer
    double duration_seconds = static_cast<double>(frame_count) / options.fps;

    // Set stream duration in stream timebase
    video_stream->duration = av_rescale_q(
        frame_count,
        AVRational{1, options.fps},
        video_stream->time_base);

    // Set container duration in AV_TIME_BASE units
    format_context->duration = static_cast<int64_t>(duration_seconds * AV_TIME_BASE);

    // Write essential metadata BEFORE trailer
    AVDictionary *metadata = NULL;
    char duration_str[32];
    snprintf(duration_str, sizeof(duration_str), "%.3f", duration_seconds);
    av_dict_set(&metadata, "DURATION", duration_str, 0);
    av_dict_set(&metadata, "ENCODER", "Lux Video Recorder", 0);
    av_dict_set(&metadata, "ENCODER_OPTIONS", "h264_baseline", 0);

    // Set stream metadata
    av_dict_set(&video_stream->metadata, "language", "und", 0);

    // CRITICAL: Write trailer with proper error handling for MOOV atom
    DEBUG("Writing MP4 trailer with MOOV atom...");
    ret = av_write_trailer(format_context);
    if (ret < 0)
    {
        // Don't fail completely - try to recover
        char error_buf[256];
        av_strerror(ret, error_buf, sizeof(error_buf));
        DEBUG("Warning: Error writing trailer (MOOV atom): " + std::string(error_buf));
        DEBUG("Attempting MOOV atom recovery...");
        
        // Force another flush and try again
        if (format_context->pb)
        {
            avio_flush(format_context->pb);
            // Try writing a minimal trailer
            ret = av_write_trailer(format_context);
            if (ret < 0)
            {
                DEBUG("MOOV atom recovery failed, but continuing with file output");
            }
            else
            {
                DEBUG("MOOV atom recovery successful");
            }
        }
    }
    else
    {
        DEBUG("Trailer with MOOV atom written successfully");
    }

    // CRITICAL: Force final flush and sync
    if (format_context && format_context->pb)
    {
        avio_flush(format_context->pb);
        DEBUG("Final buffer flush completed");
        
        int close_ret = avio_closep(&format_context->pb);
        if (close_ret < 0)
        {
            char error_buf[256];
            av_strerror(close_ret, error_buf, sizeof(error_buf));
            DEBUG("Warning: Error closing AVIO: " + std::string(error_buf));
        }
        else
        {
            DEBUG("AVIO closed successfully");
        }
    }

// Sync the virtual FS to ensure file is available
#ifdef __EMSCRIPTEN__
    EM_ASM(
        FS.syncfs(false, function(){
            console.log('Virtual filesystem synced for MOOV atom');
        });
    );
#endif

    // Read temp file directly into memory
    DEBUG("Attempting to open temp file for reading: " + temp_filename);
    FILE *src_file = fopen(temp_filename.c_str(), "rb");
    if (!src_file)
    {
        ERROR("Failed to open temp file for reading: " + temp_filename + " : " + std::string(strerror(errno)));
    }
    fseek(src_file, 0, SEEK_END);
    long file_size = ftell(src_file);
    fseek(src_file, 0, SEEK_SET);
    if (file_size < 32)
    {
        fclose(src_file);
        ERROR("Temp file too small, likely missing MOOV atom: " + std::to_string(file_size) + " bytes");
    }
    output_buffer.resize(file_size);
    size_t bytes_read = fread(output_buffer.data(), 1, file_size, src_file);
    fclose(src_file);
    if (bytes_read != file_size)
    {
        ERROR("Failed to read all bytes from temp file: got " + std::to_string(bytes_read) + " expected " + std::to_string(file_size));
    }
    
    // Verify MOOV atom presence
    bool moov_found = false;
    for (size_t i = 0; i < output_buffer.size() - 4; ++i)
    {
        if (output_buffer[i] == 'm' && output_buffer[i+1] == 'o' && 
            output_buffer[i+2] == 'o' && output_buffer[i+3] == 'v')
        {
            moov_found = true;
            DEBUG("MOOV atom found at position " + std::to_string(i));
            break;
        }
    }
    
    if (!moov_found)
    {
        DEBUG("WARNING: MOOV atom not found in output file - video may not play correctly");
    }
    
    // Debug: print first 16 bytes
    DEBUG("First 16 bytes of temp file:");
    for (int i = 0; i < 16 && i < output_buffer.size(); ++i)
    {
        printf("%02x ", output_buffer[i]);
    }
    printf("\n");

    // Clean up resources
    cleanup();

    state = RecordingState::IDLE;
    return true;
}

void VideoRecorder::cleanup()
{
    if (frame)
    {
        av_frame_free(&frame);
        frame = nullptr;
    }

    if (tmp_frame)
    {
        av_frame_free(&tmp_frame);
        tmp_frame = nullptr;
    }

    if (codec_context)
    {
        avcodec_free_context(&codec_context);
        codec_context = nullptr;
    }

    if (format_context)
    {
        avformat_free_context(format_context);
        format_context = nullptr;
    }

    if (sws_context)
    {
        sws_freeContext(sws_context);
        sws_context = nullptr;
    }
}

bool VideoRecorder::validate_format_codec()
{
    // Always use MP4/H.264 for best mobile and desktop compatibility
    options.format = "mp4";
    options.codec = "libx264";
    if (options.preset.empty())
    {
        options.preset = "ultrafast"; // Default to ultrafast preset for real-time encoding
    }
    DEBUG("Using MP4/H.264 for best mobile and desktop compatibility");
    return true;
}

bool VideoRecorder::are_dimensions_valid(int width, int height)
{
    // Most codecs require dimensions to be even
    if (width % 2 != 0 || height % 2 != 0)
    {
        return false;
    }

    // Some codecs have minimum dimensions requirements
    if (width < 16 || height < 16)
    {
        return false;
    }

    // Most browsers have maximum resolution limitations
    if (width > 4096 || height > 4096)
    {
        return false;
    }

    return true;
}

bool VideoRecorder::initialize_from_image(const uimage &img)
{
    options.width = img.get_dim().x;
    options.height = img.get_dim().y;

    // Ensure dimensions are even
    options.width = options.width % 2 == 0 ? options.width : options.width - 1;
    options.height = options.height % 2 == 0 ? options.height : options.height - 1;

    DEBUG("Recording dimensions set to match input image: " +
          std::to_string(options.width) + "x" + std::to_string(options.height));

    return initialize_video();
}

bool VideoRecorder::start_recording_adaptive(const uimage &first_frame, const RecordingOptions &base_opts)
{
    if (state == RecordingState::RECORDING)
    {
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

    if (!initialize_video())
    {
        cleanup();
        return false;
    }

    state = RecordingState::RECORDING;
    return true;
}