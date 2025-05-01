#include "video_recorder.hpp"
#include "iostream"
#include "stdexcept"
#include "chrono"

#define DEBUG( msg ) { std::string debug_msg = msg; std::cout << debug_msg << std::endl; }
#define ERROR( msg ) { error_message = msg; state = RecordingState::ERROR; return false; }

VideoRecorder::VideoRecorder() : state(RecordingState::IDLE),
                                 format_context(nullptr),
                                 codec_context(nullptr),
                                 video_stream(nullptr),
                                 sws_context(nullptr),
                                 frame(nullptr),
                                 tmp_frame(nullptr),
                                 frame_count(0) {
}

VideoRecorder::~VideoRecorder() {
    if (state == RecordingState::RECORDING) {
        stop_recording();
    }
    cleanup();
}

bool VideoRecorder::initialize_video() {
    avformat_alloc_output_context2(&format_context, nullptr, options.format.c_str(), nullptr);
    if (!format_context) {
        ERROR("Fail to create output format context");
    }

    // find the codec
    const AVCodec *codec = avcodec_find_encoder_by_name(options.codec.c_str());
    if (!codec) {
        ERROR("Codec not found: " + options.codec);
    }

    // create a new video stream
    video_stream = avformat_new_stream(format_context, codec);
    if (!video_stream) {
        ERROR("Failed to create video stream");
    }

    // initialize codec context
    codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) {
        ERROR("Failed to allocate codec context");
    }

    codec_context->codec_id = codec->id;
    codec_context->codec_type = AVMEDIA_TYPE_VIDEO;
    codec_context->width = options.width;
    codec_context->height = options.height;
    codec_context->time_base = (AVRational){1, options.fps};
    codec_context->framerate = (AVRational){options.fps, 1};
    codec_context->gop_size = 12; // keyframe interval
    codec_context->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_context->bit_rate = options.bitrate;

    // for MP4, H264

    if (codec_context->codec_id == AV_CODEC_ID_H264) {
        av_opt_set(codec_context->priv_data, "preset", options.preset.c_str(), 0);
    }

    // copy codec parameters to the stream
    avcodec_parameters_from_context(video_stream->codecpar, codec_context);

    // open the codec
    if (avcodec_open2(codec_context, codec, nullptr) < 0) {
        ERROR("Fail to open the codec")
    }

    frame = av_frame_alloc();
    if (!frame) {
        ERROR("Failed to allocate frame");
    }

    frame->format = codec_context->pix_fmt;
    frame->width = codec_context->width;
    frame->height = codec_context->height;

    if (av_frame_get_buffer(frame, 0) < 0) {
        ERROR("Failed to allocate frame buffer")
    }

    tmp_frame = av_frame_alloc();
    if (!tmp_frame) {
        ERROR("Failed to allocate temporary frame");
    }

    tmp_frame->format = AV_PIX_FMT_RGBA;
    tmp_frame->width = codec_context->width;
    tmp_frame->height = codec_context->height;

    if (av_frame_get_buffer(tmp_frame, 0) < 0) {
        ERROR("Fail to allocate memory for temp frame buffer")
    }

    sws_context = sws_getContext(
        options.width, options.height, AV_PIX_FMT_RGBA,
        options.width, options.height, codec_context->pix_fmt,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    if (!sws_context) {
        ERROR("Fail to create SWS context");
    }

    // opening the output file
    std::string filename = "recording." + options.format;
    if (avio_open(&format_context->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0) {
        ERROR("Fail to open output file");
    }

    if (avformat_write_header(format_context, nullptr) < 0) {
        ERROR("Fail to write file header");
    }

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
        if (format_context->pb) {
            avio_closep(&format_context->pb);
        }
        avformat_free_context(format_context);
        format_context = nullptr;
    }

    if (sws_context) {
        sws_freeContext(sws_context);
        sws_context = nullptr;
    }
}

bool VideoRecorder::start_recording(const RecordingOptions &opts) {
    if (state == RecordingState::RECORDING) {
        stop_recording();
    }

    options = opts;
    frame_count = 0;

    if (!initialize_video()) {
        cleanup();
        return false;
    }

    state = RecordingState::RECORDING;
    DEBUG("Recording started");
    return true;
}

bool VideoRecorder::add_frame(const uimage &img) {
    if (state != RecordingState::RECORDING) {
        return false;
    }

    if (img.get_dim().x != options.width || img.get_dim().y != options.height) {
        ERROR("Image dimensions don't match the recording options");
    }


    const unsigned char *img_data = (const unsigned char *) img.get_base_ptr();
    for (int y = 0; y < options.height; y++) {
        for (int x = 0; x < options.width; x++) {
            int img_idx = (y * options.width + x) * 4; // RGBA;
            int frame_idx = y * tmp_frame->linesize[0] + x * 4; // RGBA;

            tmp_frame->data[0][frame_idx] = img_data[img_idx]; // R
            tmp_frame->data[0][frame_idx + 1] = img_data[img_idx + 1]; // G
            tmp_frame->data[0][frame_idx + 2] = img_data[img_idx + 2]; // B
            tmp_frame->data[0][frame_idx + 3] = img_data[img_idx + 3]; // A
        }
    }

    sws_scale(sws_context, tmp_frame->data, tmp_frame->linesize, 0, options.height, frame->data, frame->linesize);

    // set frame timestamps
    frame->pts = frame_count;


    // encode frame
    int ret = avcodec_send_frame(codec_context, frame);
    if (ret < 0) {
        ERROR("Error sending frame to encoder");
    }
    while (ret >= 0) {
        AVPacket pkt = { 0 };
        ret = avcodec_receive_packet(codec_context, &pkt);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            ERROR("Error receiving packet from encoder");
        }

        // rescale packet timestamps
        av_packet_rescale_ts(&pkt, codec_context->time_base, video_stream->time_base);
        pkt.stream_index = video_stream->index;

        // write packet to file
        ret = av_interleaved_write_frame(format_context, &pkt);
        if (ret < 0) {
            ERROR("Error while writing packet to the file")
        }

        av_packet_unref(&pkt);
    }

    frame_count++;
    return true;
}


bool VideoRecorder::stop_recording() {
    if (state != RecordingState::ENCODING) {
        return false;
    }

    state = RecordingState::ENCODING;
    DEBUG("Finalizing video file...");

    // flush encoder
    avcodec_send_frame(codec_context, nullptr);

    while (true) {
        AVPacket pkt = { 0 };
        int ret = avcodec_receive_packet(codec_context, &pkt);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            ERROR("Error receiving packet from encoder");
        }

        av_packet_rescale_ts(&pkt, codec_context->time_base, video_stream->time_base);
        pkt.stream_index = video_stream->index;

        ret = av_interleaved_write_frame(format_context, &pkt);
        if (ret < 0) {
            ERROR("Error while writing packet to the file");
        }

        av_packet_unref(&pkt);
    }

    // write file trailer
    if (av_write_trailer(format_context) < 0) {
        ERROR("Error while writing file trailer")
    }

    cleanup();
    state = RecordingState::IDLE;
    DEBUG("Recording finished with " + std::to_string(frame_count) + " frames");
    return true;
}

