#include "pushwork.h"
#include "avpublishtime.h"
#include "easylogging++.h"
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavdevice/avdevice.h>

#ifdef __cplusplus
}
#endif
namespace LQF
{
PushWork::PushWork()
{
}

PushWork::~PushWork()
{
    if(audio_capturer_) {
        delete audio_capturer_;
    }
    if(audio_encoder_) {
        delete audio_encoder_;
    }
    if(video_capturer) {
        delete video_capturer;
    }
    if(aac_buf_) {
        delete [] aac_buf_;
    }
    if(audio_resampler_) {
        delete audio_resampler_;
    }
    if(video_encoder_) {
        delete video_encoder_;
    }
    if(rtmp_pusher) {
        delete rtmp_pusher;
    }
    if(h264_fp_) {
        fclose(h264_fp_);
    }
    if(aac_fp_) {
        fclose(aac_fp_);
    }
    if(pcm_flt_fp_) {
        fclose(pcm_flt_fp_);
    }
    if(pcm_s16le_fp_) {
        fclose(pcm_s16le_fp_);
    }
    if(audio_packet_) {
        av_packet_free(&audio_packet_);
    }
    if(video_packet_) {
        av_packet_free(&video_packet_);
    }
}
/**
 * @brief PushWork::Init
 * @param properties
 * @return
 */
RET_CODE PushWork::Init(const Properties &properties)
{
    // 初始化输入输出设备
    avdevice_register_all();
    audio_device_name_ = properties.GetProperty("audio_device_name", "");
    if(audio_device_name_ == "") {
        LOG(ERROR) << "audio_device_name is null";
        return RET_FAIL;
    }

    // 获取视频设备名称
    video_device_name_ = properties.GetProperty("video_device_name", "");
    if (video_device_name_.empty()) {
        // 如果没有指定设备，尝试使用 OBS 虚拟摄像头
        video_device_name_ = "video=OBS Virtual Camera";
    }

    // 音频test模式
    input_pcm_name_ = properties.GetProperty("input_pcm_name", "input_48k_2ch_s16.pcm");
    // 音频编码参数
    audio_sample_rate_ = properties.GetProperty("audio_sample_rate", 48000);
    audio_bitrate_ = properties.GetProperty("audio_bitrate", 128 * 1024);
    audio_channels_ = properties.GetProperty("audio_channels", 2);
    audio_ch_layout_ = av_get_default_channel_layout(audio_channels_);    // 由audio_channels_决定
    // 桌面录制属性
    desktop_x_ = properties.GetProperty("desktop_x", 0);
    desktop_y_ = properties.GetProperty("desktop_y", 0);
    desktop_width_  = properties.GetProperty("desktop_width", 1920);
    desktop_height_ = properties.GetProperty("desktop_height", 1080);
    desktop_format_ = properties.GetProperty("desktop_pixel_format", AV_PIX_FMT_YUV420P);
    desktop_fps_ = properties.GetProperty("desktop_fps", 25);
    // 视频编码属性
    video_width_  = properties.GetProperty("video_width", desktop_width_);     // 宽
    video_height_ = properties.GetProperty("video_height", desktop_height_);   // 高
    video_fps_ = properties.GetProperty("video_fps", desktop_fps_);             // 帧率
    video_gop_ = properties.GetProperty("video_gop", video_fps_);
    video_bitrate_ = properties.GetProperty("video_bitrate", 1024 * 1024); // 先默认1M fixedme
    video_b_frames_ = properties.GetProperty("video_b_frames", 0);   // b帧数量
    enable_rtmp_ = properties.GetProperty("enable_rtmp", false);
    if(enable_rtmp_) {
        // rtmp推流
        rtmp_url_ = properties.GetProperty("rtmp_url", "");
        if(rtmp_url_ == "") {
            LOG(ERROR) << "rtmp url is null";
            return RET_FAIL;
        }
        // 先启动RTMPPusher
        rtmp_pusher = new RTMPPusher();     // 启动后线程推流的线程就启动了，带了线程
        if(!rtmp_pusher) {
            LOG(ERROR) << "new RTMPPusher() failed";
            return RET_FAIL;
        }
        if(!rtmp_pusher->Connect(rtmp_url_)) {
            LOG(ERROR) << "rtmp_pusher connect() failed";
            return RET_FAIL;
        }
    }
    // 初始化publish time
    AVPublishTime::GetInstance()->Rest();   // 推流打时间戳的问题
    // 设置音频编码器，先音频捕获初始化
    audio_packet_ = av_packet_alloc();
    audio_encoder_ = new AACEncoder();
    if(!audio_encoder_) {
        LOG(ERROR) << "new AACEncoder() failed";
        return RET_FAIL;
    }
    Properties audio_properties;
    audio_properties.SetProperty("buffer_size", 8192);
    audio_properties.SetProperty("sample_rate", audio_sample_rate_);
    audio_properties.SetProperty("channels", audio_channels_);
    audio_properties.SetProperty("bitrate", audio_bitrate_);
    audio_properties.SetProperty("profile", 2);  // AAC LC
    // 创建音频时间基准
    AVRational audio_time_base = {1, audio_sample_rate_};
    if(audio_encoder_->Init(audio_properties, audio_time_base) != RET_OK) {
        LOG(ERROR) << "audio encoder init failed";
        return RET_FAIL;
    }
    aac_buf_ = new uint8_t[AAC_BUF_MAX_LENGTH];
    aac_fp_ = fopen("push_dump.aac", "wb");
    if(!aac_fp_) {
        LOG(ERROR) << "fopen push_dump.aac failed";
        return RET_FAIL;
    }
    // 音频重采样，捕获的PCM数据 s16交错模式, 做编码的时候float棋盘格格式的
    // 1-快速掌握音视频开发基础知识.pdf
    // 视频编码
    video_packet_ = av_packet_alloc();
    video_encoder_ = new H264Encoder();
    Properties vid_codec_properties;
    vid_codec_properties.SetProperty("width", 1920);
    vid_codec_properties.SetProperty("height", 1080);
    vid_codec_properties.SetProperty("fps", video_fps_);
    vid_codec_properties.SetProperty("b_frames", video_b_frames_);
    vid_codec_properties.SetProperty("bitrate", 2000000);  // 降低码率到2Mbps
    vid_codec_properties.SetProperty("gop", video_gop_);
    vid_codec_properties.SetProperty("threads", 1);  // 减少线程数以降低内存使用
    vid_codec_properties.SetProperty("preset", properties.GetProperty("encoder_preset", "ultrafast"));
    vid_codec_properties.SetProperty("tune", properties.GetProperty("encoder_tune", "zerolatency"));
    vid_codec_properties.SetProperty("profile", properties.GetProperty("encoder_profile", "baseline"));
    AVRational video_tb = {1, 1000};  //视频的timebase精度如果不够，会导致相邻两个时间可能相同
    if(video_encoder_->Init(vid_codec_properties, video_tb) != RET_OK) {
        LOG(ERROR) << "H264Encoder Init failed";
        return RET_FAIL;
    }
    h264_fp_ = fopen("push_dump.h264", "wb");
    if(!h264_fp_) {
        LOG(ERROR) << "fopen push_dump.h264 failed";
        return RET_FAIL;
    }
    enable_record_ = properties.GetProperty("enable_record", false);
    if(enable_record_) {
        record_file_name_  = properties.GetProperty("record_file_name", "");
        if(record_file_name_ == "") {
            LOG(ERROR) << "record_file_name_ is null";
            return RET_FAIL;
        }
        recorder_ = new Recorder();
        Properties  record_properties;
        record_properties.SetProperty("url", record_file_name_);
        if(recorder_->Init(record_properties) != RET_OK) {
            LOG(ERROR) << "Recorder Init failed";
            return RET_FAIL;
        }
        if(recorder_->ConfigAudioStream((AVCodecContext *)audio_encoder_->GetCodecContext()) != RET_OK) {
            LOG(ERROR) << "Recorder ConfigAudioStream failed";
            return RET_FAIL;
        }
        if(recorder_->ConfigVideoStream((AVCodecContext *)video_encoder_->GetCodecContext()) != RET_OK) {
            LOG(ERROR) << "Recorder ConfigVideoStream failed";
            return RET_FAIL;
        }
        if(recorder_->WriteHeader() != RET_OK) {
            LOG(ERROR) << "Recorder WriteHeader failed";
            return RET_FAIL;
        }
    }
    if(enable_rtmp_) {
        // RTMP -> FLV的格式去发送， metadata
        FLVMetadataMsg *metadata = new FLVMetadataMsg();
        // 设置视频相关
        metadata->has_video = true;
        metadata->width = video_encoder_->get_width();
        metadata->height = video_encoder_->get_height();
        metadata->framerate = video_encoder_->get_framerate();
        metadata->videodatarate = video_encoder_->get_bit_rate();
        // 设置音频相关
        metadata->has_audio = true;
        metadata->channles = audio_encoder_->get_channels();
        metadata->audiosamplerate = audio_encoder_->get_sample_rate();
        metadata->audiosamplesize = 16;
        metadata->audiodatarate = 64;
        metadata->pts = 0;
        rtmp_pusher->Post(RTMP_BODY_METADATA, metadata, false);
    }
    // 设置音频pts的间隔
    double audio_frame_duration = 1000.0 / audio_encoder_->get_sample_rate() * audio_encoder_->get_frame_size();
    LOG(INFO) << "audio_frame_duration: " << audio_frame_duration;
    AVPublishTime::GetInstance()->set_audio_frame_duration(audio_frame_duration);
    AVPublishTime::GetInstance()->set_audio_pts_strategy(AVPublishTime::PTS_RECTIFY);//帧间隔矫正
    // 设置音频捕获
    audio_capturer_ = new AudioCapturer();
    Properties  aud_cap_properties;
    aud_cap_properties.SetProperty("audio_enc_sample_rate", audio_encoder_->get_sample_rate());
    aud_cap_properties.SetProperty("audio_enc_sample_fmt", audio_encoder_->get_sample_fmt());
    aud_cap_properties.SetProperty("audio_enc_channel_layout", audio_encoder_->get_channel_layout());
    aud_cap_properties.SetProperty("audio_enc_frame_size", audio_encoder_->get_frame_size());
    aud_cap_properties.SetProperty("audio_device_name", audio_device_name_);
    aud_cap_properties.SetProperty("audio_ns", properties.GetProperty("audio_ns", -1));     //已经实现了
    aud_cap_properties.SetProperty("audio_agc", properties.GetProperty("audio_agc", 0));    //还没有实现增强
    if(audio_capturer_->Init(aud_cap_properties) != RET_OK) {
        LOG(ERROR) << "AudioCapturer Init failed";
        return RET_FAIL;
    }
    audio_capturer_->AddCallback(std::bind(&PushWork::AudioFrameCallback, this,
                                           std::placeholders::_1));
    if(audio_capturer_->Start() != RET_OK) {
        LOG(ERROR) << "AudioCapturer Start failed";
        return RET_FAIL;
    }
    // 设置视频pts的间隔
    double video_frame_duration = 1000.0 / video_encoder_->get_framerate();
    LOG(INFO) << "video_frame_duration: " << video_frame_duration;
    //    AVPublishTime::GetInstance()->set_video_frame_duration(video_frame_duration);
    AVPublishTime::GetInstance()->set_video_pts_strategy(AVPublishTime::PTS_REAL_TIME);//实时时钟
    video_capturer = new VideoCapturer();
    Properties vid_cap_properties;
    vid_cap_properties.SetProperty("video_test", 1);
    vid_cap_properties.SetProperty("video_device_name", video_device_name_);
    vid_cap_properties.SetProperty("video_enc_width", vid_codec_properties.GetProperty("width", 1920));
    vid_cap_properties.SetProperty("video_enc_height", vid_codec_properties.GetProperty("height", 1080));
    vid_cap_properties.SetProperty("video_enc_pix_fmt", video_encoder_->get_pix_fmt());
    vid_cap_properties.SetProperty("fps", (int)video_encoder_->get_framerate());
    vid_cap_properties.SetProperty("framerate", video_fps_);
    vid_cap_properties.SetProperty("is_streaming", enable_rtmp_);  // 传递是否为推流的信息
    if(video_capturer->Init(vid_cap_properties) != RET_OK) {
        LOG(ERROR) << "VideoCapturer Init failed";
        return RET_FAIL;
    }
    video_capturer->AddCallback(std::bind(&PushWork::VideoFrameCallback, this,
                                          std::placeholders::_1));
    if(video_capturer->Start() != RET_OK) {
        LOG(ERROR) << "VideoCapturer Start failed";
        return RET_FAIL;
    }

    // 检查分辨率和内存
    int64_t requiredMemory = video_width_ * video_height_ * 3 / 2;  // YUV420格式
    if (requiredMemory > 8 * 1024 * 1024) {  // 如果需要超过8MB
        // 检查可用内存
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        
        if (memInfo.ullAvailPhys < requiredMemory * 3) {  // 如果可用物理内存不足
            LOG(WARNING) << "Available memory might not be sufficient for current resolution";
            // 可以在这里自动降低分辨率或返回错误
        }
    }

    return RET_OK;
}

void PushWork::DeInit()
{
    LOG(INFO) << "free audio_capturer_";
    if(audio_capturer_) {
        delete audio_capturer_;
        audio_capturer_ = NULL;
    }
    LOG(INFO) << "free audio_encoder_";
    if(audio_encoder_) {
        delete audio_encoder_;
        audio_encoder_ = NULL;
    }
    LOG(INFO) << "free audio_resampler_";
    if(audio_resampler_) {
        delete audio_resampler_;
        audio_resampler_ = NULL;
    }
    LOG(INFO) << "free video_capturer";
    if(video_capturer) {
        delete video_capturer;
        video_capturer = NULL;
    }
    LOG(INFO) << "free video_encoder_";
    if(video_encoder_) {
        delete video_encoder_;
        video_encoder_ = NULL;
    }
    LOG(INFO) << "free recorder_";
    if(recorder_) {
        recorder_->WriteTrailer();
        delete recorder_;
        recorder_ = NULL;
    }
    LOG(INFO) << "free rtmp_pusher";
    if(rtmp_pusher) {
        delete rtmp_pusher;
        rtmp_pusher = NULL;
    }
    LOG(INFO) << "fclose h264_fp_";
    if(h264_fp_) {
        fclose(h264_fp_);
        h264_fp_ = NULL;
    }
    LOG(INFO) << "fclose aac_fp_";
    if(aac_fp_) {
        fclose(aac_fp_);
        aac_fp_ = NULL;
    }
    LOG(INFO) << "fclose pcm_flt_fp_";
    if(pcm_flt_fp_) {
        fclose(pcm_flt_fp_);
        pcm_flt_fp_ = NULL;
    }
    LOG(INFO) << "fclose pcm_s16le_fp_";
    if(pcm_s16le_fp_) {
        fclose(pcm_s16le_fp_);
        pcm_s16le_fp_ = NULL;
    }
}


void PushWork::AudioFrameCallback(AVFrame *frame)
{
    //    return;
    if(need_send_audio_spec_config && enable_rtmp_) { // rtmp推流直播的时候使用
        need_send_audio_spec_config = false;
        AudioSpecMsg *aud_spc_msg = new AudioSpecMsg(audio_encoder_->get_profile(),
                audio_encoder_->get_channels(),
                audio_encoder_->get_sample_rate());
        aud_spc_msg->pts_ = 0;
        rtmp_pusher->Post(RTMP_BODY_AUD_SPEC, aud_spc_msg);
    }
    if(!pcm_flt_fp_) {
        pcm_flt_fp_ = fopen("push_dump_flt.pcm", "wb");
    }
    if(pcm_flt_fp_) {
        // ffplay -ar 48000 -channels 1 -f f32le  -i push_dump_flt.pcm
        fwrite(frame->data[0], 1,
               frame->linesize[0], pcm_flt_fp_);
        //        fwrite(frame->data[1], 1,
        //                frame->linesize[1], pcm_flt_fp_);
        fflush(pcm_flt_fp_);
    }
    // 转成音频编码器需要的timebase
    frame->pts = av_rescale_q(frame->pts, capture_time_base_, audio_encoder_->get_time_base());
    RET_CODE ret = audio_encoder_->Input(frame);
    if(ret == RET_FAIL) {
        // 打印错误信息
        LOG(ERROR) << "audio_encoder_->Input failed";
    }
    do {
        ret = audio_encoder_->Output(audio_packet_);
        if(ret == RET_OK) {
            if(enable_rtmp_) {
                pushRtmpAudio(audio_packet_);
            }
            if(enable_record_) {
                pushRecordAudio(audio_packet_);
            }
            av_packet_unref(audio_packet_);
            continue;
        } else {
            break;
        }
    } while(ret == RET_OK);
}

void PushWork::VideoFrameCallback(AVFrame *frame)
{
    //    return;
    if(need_send_video_config && enable_rtmp_) { // rtmp推流直播的时候使用
        need_send_video_config = false;
        VideoSequenceHeaderMsg * vid_config_msg = new VideoSequenceHeaderMsg(
            video_encoder_->get_sps_data(),
            video_encoder_->get_sps_size(),
            video_encoder_->get_pps_data(),
            video_encoder_->get_pps_size()
        );
        vid_config_msg->nWidth = video_width_;
        vid_config_msg->nHeight = video_height_;
        vid_config_msg->nFrameRate = video_fps_;
        vid_config_msg->nVideoDataRate = video_bitrate_;
        vid_config_msg->pts_ = 0;
        rtmp_pusher->Post(RTMP_BODY_VID_CONFIG, vid_config_msg);
        if(h264_fp_) {
            char start_code[4] = {0, 0, 0, 1};
            fwrite(start_code, 1, 4, h264_fp_);
            fwrite(video_encoder_->get_sps_data(),
                   video_encoder_->get_sps_size(), 1, h264_fp_);
            fwrite(start_code, 1, 4, h264_fp_);
            fwrite( video_encoder_->get_pps_data(),
                    video_encoder_->get_pps_size(), 1, h264_fp_);
        }
    }
    if(video_preview_callback_) {
        Frame temp_frame;
        memset(&temp_frame, 0, sizeof(Frame));
        temp_frame.format = frame->format;
        temp_frame.width = frame->width;
        temp_frame.height = frame->height;
        temp_frame.frame = frame;
        video_preview_callback_(&temp_frame);        // 这里之所以封装Frame结构体是为了和播放器的显示控件一致，方便后续扩展
    }
    // 转成音频编码器需要的timebase
    frame->pts = av_rescale_q(frame->pts, capture_time_base_, video_encoder_->get_time_base());
    RET_CODE ret = video_encoder_->Input(frame);
    //    LOG(INFO) <<"video input %d", ret);
    if(ret == RET_FAIL) {
        // 打印错误信息
        LOG(ERROR) << "audio_encoder_->Input failed";
    }
    do {
        ret = video_encoder_->Output(video_packet_);
        //        LOG(INFO) <<"video out %d", ret);
        if(ret == RET_OK) {
            if(enable_rtmp_) {
                pushRtmpVideo(video_packet_);
            }
            if(enable_record_) {
                pushRecordVideo(video_packet_);
            }
            av_packet_unref(video_packet_);
            continue;
        } else {
            break;
        }
    } while(ret == RET_OK);
}

void PushWork::AddVidePreviewCallback(std::function<int (const Frame *)> callback)
{
    video_preview_callback_ = callback;
}

void PushWork::SetPause(bool pause)
{
    if(pause_ == pause) {
        return;     // 操作不起作用
    }
    pause_ = pause;
    AVPublishTime::GetInstance()->set_pause(pause_);
}

bool PushWork::GetPause()
{
    return pause_;
}

int64_t PushWork::GetTime()
{
    return AVPublishTime::GetInstance()->get_time();
}

RET_CODE PushWork::pushRtmpAudio(AVPacket *packet)
{
    int aac_size = packet->size;
    if(aac_fp_) {
        uint8_t adts_header[7];
        audio_encoder_->GetAdtsHeader(adts_header, aac_size);
        fwrite(adts_header, 1, 7, aac_fp_);
        fwrite(packet->data, 1, aac_size, aac_fp_);
        fflush(aac_fp_);
    }
    AudioRawMsg *aud_raw_msg = new AudioRawMsg(aac_size + 2);
    aud_raw_msg->pts = av_rescale_q(packet->dts, audio_encoder_->get_time_base(), rtmp_time_base_) ;
    aud_raw_msg->data[0] = 0xaf;
    aud_raw_msg->data[1] = 0x01;    // 1 =  raw data数据
    memcpy(&aud_raw_msg->data[2], packet->data, aac_size);
    rtmp_pusher->Post(RTMP_BODY_AUD_RAW, aud_raw_msg);
    return RET_OK;
}

RET_CODE PushWork::pushRtmpVideo(AVPacket *packet)
{
    // 获取到编码数据
    NaluStruct *nalu = new NaluStruct(packet->data + 4, packet->size - 4);
    nalu->type = packet->data[0 + 4] & 0x1f;
    nalu->pts =   av_rescale_q(packet->dts, video_encoder_->get_time_base(), rtmp_time_base_);
    rtmp_pusher->Post(RTMP_BODY_VID_RAW, nalu);
    char start_code[4] = {0, 0, 0, 1};
    fwrite(start_code, 1, 4, h264_fp_);
    fwrite(packet->data,
           packet->size, 1, h264_fp_);
    fflush(h264_fp_);
    return RET_OK;
}

RET_CODE PushWork::pushRecordAudio(AVPacket *packet)
{
    recorder_->WritePacket(packet, E_AUDIO_TYPE);
    return RET_OK;
}

RET_CODE PushWork::pushRecordVideo(AVPacket *packet)
{
    recorder_->WritePacket(packet, E_VIDEO_TYPE);
    return RET_OK;
}
}

