#include "audiocapturer.h"
#include "timeutil.h"
#include "avpublishtime.h"
#include <QtGlobal>
#include "easylogging++.h"
namespace LQF
{
//Show Dshow Device 只是为了测试
static void show_dshow_device()
{
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    AVDictionary* options = nullptr;
    av_dict_set(&options, "list_devices", "true", 0);
    AVInputFormat *iformat = (AVInputFormat *)av_find_input_format("dshow");
    LOG(INFO) << "========Device Info=============";
    avformat_open_input(&pFormatCtx, "video=dummy", iformat, &options);
    LOG(INFO) << "================================";
}

//Show Dshow Device Option
static void show_dshow_device_option()
{
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    AVDictionary* options = nullptr;
    av_dict_set(&options, "list_options", "true", 0);
    AVInputFormat *iformat = (AVInputFormat *)av_find_input_format("dshow");
    LOG(INFO) << "========Device Option Info=============";
    avformat_open_input(&pFormatCtx, "video=Integrated Camera", iformat, &options);
    LOG(INFO) << "================================";
}

AudioCapturer::AudioCapturer()
{
    //
}

AudioCapturer::~AudioCapturer()
{
    Stop();
    if(capture2noise_resampler_) {
        audio_resampler_free(capture2noise_resampler_);
        capture2noise_resampler_ = nullptr;
    }
    if(noise2encode_resampler_) {
        audio_resampler_free(noise2encode_resampler_);
        noise2encode_resampler_ = nullptr;
    }
    if(capture_frame_) {
        av_frame_free(&capture_frame_);
    }
    if(noise_frame_) {
        av_frame_free(&noise_frame_);
    }
    if(encode_frame_) {
        av_frame_free(&encode_frame_);
    }
    if(codec_ctx_) {
        avcodec_free_context(&codec_ctx_);
    }
    if(ifmt_ctx_) {
        avformat_close_input(&ifmt_ctx_);
    }
    if(f_agc_before_) {
        fclose(f_agc_before_);
    }
    if(f_agc_after_) {
        fclose(f_agc_after_);
    }
}

RET_CODE AudioCapturer::Init(const Properties &properties)
{
    int ret = 0;
    audio_enc_sample_rate_ = properties.GetProperty("audio_enc_sample_rate", 0);
    audio_enc_sample_fmt_ = properties.GetProperty("audio_enc_sample_fmt", 0);
    audio_enc_channel_layout_ = properties.GetProperty("audio_enc_channel_layout", 0);
    audio_enc_frame_size_ = properties.GetProperty("audio_enc_frame_size", 0);
    device_name_ = properties.GetProperty("audio_device_name", "");
    audio_ns_level_ = (nsLevel) properties.GetProperty("audio_ns", -1);
    audio_agc_level_ = properties.GetProperty("audio_agc", 0);
    if(audio_enc_sample_rate_ == 0
       || audio_enc_sample_fmt_ == 0
       || audio_enc_channel_layout_ == 0
       || audio_enc_frame_size_ == 0) {
        LOG(ERROR) << "AudioCapturer init properties have null parameter, please check it";
        return RET_FAIL;
    }
    if(device_name_.empty()) {
        LOG(ERROR) << "AudioCapturer init failed, device_name is empty";
        return RET_FAIL;
    }
    AVCodec			*pCodec = nullptr;
    ifmt_ctx_ = avformat_alloc_context();
#ifdef Q_OS_WIN
    //Show Dshow Device
    show_dshow_device();   // 这里只是测试
    //Show Device Options
    show_dshow_device_option();// 这里只是测试
    AVInputFormat *ifmt = av_find_input_format("dshow"); //使用dshow
    AVDictionary *param = NULL;
    char str_sample_rate[32] = {0};
    sprintf(str_sample_rate, "%d", capture_sample_rate_);
    av_dict_set(&param, "sample_rate", str_sample_rate, 0);        //设置采样率
    //    set audio device buffer latency size in milliseconds
    av_dict_set(&param, "audio_buffer_size", "10", 0);      // 设置10ms， 每次降噪需要10ms的数据
    //    av_dict_set(&param, "sample_size", "5", 0);
    ret = avformat_open_input(&ifmt_ctx_, device_name_.c_str(), ifmt, &param);
    if(ret != 0) {
        av_strerror(ret, err2str_, sizeof(err2str_));
        LOG(ERROR) << "Couldn't open input audio stream: " <<  device_name_.c_str() << ", err2str:" << err2str_;
        return RET_FAIL;
    }
#endif
#ifdef   Q_OS_LINUX
    AVInputFormat *ifmt = (AVInputFormat *)av_find_input_format("alsa");
    AVDictionary *param = NULL;
    char str_sample_rate[32] = {0};
    sprintf(str_sample_rate, "%d", capture_sample_rate_);
    av_dict_set(&param, "sample_rate", str_sample_rate, 0);        //设置采样率为32k
    //    set audio device buffer latency size in milliseconds
    av_dict_set(&param, "audio_buffer_size", "10", 0);      // 设置10ms， 每次降噪需要10ms的数据
    //    av_dict_set(&param, "sample_size", "5", 0);
    char *deviceName = "hw:1";
    ret = avformat_open_input(&ifmt_ctx_, deviceName, ifmt, &param);
    if(ret != 0) {
        av_strerror(ret, err2str, sizeof(err2str));
        LOG(ERROR) << "Couldn't open input audio stream: " <<  device_name_.c_str() << ", err2str:" << err2str;
        return RET_FAIL;
    }
#endif
    audio_stream_ = -1;
    codec_ctx_   = nullptr;
    for(unsigned int i = 0; i < ifmt_ctx_->nb_streams; i++) {
        if(ifmt_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_ = static_cast<int>(i);
            break;
        }
    }
    if(audio_stream_ == -1) {
        LOG(ERROR) << "Didn't find a audio stream:" << device_name_;
        return RET_FAIL;
    }
    //find the decoder
    codec_ctx_ = avcodec_alloc_context3(nullptr);
    avcodec_parameters_to_context(codec_ctx_, ifmt_ctx_->streams[audio_stream_]->codecpar);
    pCodec = (AVCodec *)avcodec_find_decoder(codec_ctx_->codec_id);
    if(pCodec == nullptr) {
        LOG(ERROR) << "audio Codec not found";
        return RET_FAIL;
    }
    if(avcodec_open2(codec_ctx_, pCodec, nullptr) < 0) {
        LOG(ERROR) << "Could not open audio codec";
        return RET_FAIL;
    }
    // 初始化重采样器
    if(!initResample()) {
        LOG(ERROR) << "initResample failed";
        return RET_FAIL;
    }
    // 初始化帧
    capture_frame_ = av_frame_alloc();
    noise_frame_ = av_frame_alloc();
    noise_frame_->format = noise_sample_fmt_;
    noise_frame_->channel_layout = AV_CH_LAYOUT_STEREO;
    noise_frame_->sample_rate = noise_sample_rate_;
    noise_frame_->nb_samples = noise_frame_size_;  // 32k-320, 16k-160, 8k-80
    ret = av_frame_get_buffer(noise_frame_, 0);
    if (ret < 0) {
        LOG(ERROR) << "Error allocating an audio buffer";
        return RET_FAIL;
    }
    encode_frame_ = av_frame_alloc();
    encode_frame_->format = audio_enc_sample_fmt_;
    encode_frame_->channel_layout = audio_enc_channel_layout_;
    encode_frame_->sample_rate = audio_enc_sample_rate_;
    encode_frame_->nb_samples = audio_enc_frame_size_;  // aac
    ret = av_frame_get_buffer(encode_frame_, 0);
    if (ret < 0) {
        LOG(ERROR) << "Error allocating an audio buffer";
        return RET_FAIL;
    }
    // 降噪相关
    // 左通道
    ns_l_handle_ = WebRtcNs_Create();
    ret = WebRtcNs_Init(ns_l_handle_, noise_sample_rate_);
    if (ret < 0) {
        LOG(ERROR) << "Error WebRtcNs_Init";
        return RET_FAIL;
    }
    if(audio_ns_level_ == kDisable) {
        ret = WebRtcNs_set_policy(ns_l_handle_, kLow);
    } else {
        ret = WebRtcNs_set_policy(ns_l_handle_, audio_ns_level_);
    }
    if (ret < 0) {
        LOG(ERROR) << "Error WebRtcNs_set_policy";
        return RET_FAIL;
    }
    // 右通道
    ns_r_handle_ = WebRtcNs_Create();
    ret = WebRtcNs_Init(ns_r_handle_, noise_sample_rate_);
    if (ret < 0) {
        LOG(ERROR) << "Error WebRtcNs_Init";
        return RET_FAIL;
    }
    if(audio_ns_level_ == kDisable) {
        ret = WebRtcNs_set_policy(ns_r_handle_, kLow);
    } else {
        ret = WebRtcNs_set_policy(ns_r_handle_, audio_ns_level_);
    }
    if (ret < 0) {
        LOG(ERROR) << "Error WebRtcNs_set_policy";
        return RET_FAIL;
    }
    LOG(INFO) << "audio_ns_level_: " <<  audio_ns_level_;
    // 自动增益相关
    gain_control_ = WebRtcAgc_Create();
    if (!gain_control_) {
        LOG(ERROR) << "Error WebRtcAgc_Create";
        return RET_FAIL;
    }
    // 自动增益初始化参数
    WebRtcAgcConfig gain_config;
    int fs = 16000;  // 先用16khz测试
    gain_config.targetLevelDbfs = 3;        // 表示音量均衡结果的目标值，如设置为 1 表示输出音量的目标值为 - 1dB; -3是比较大的声音了。
    gain_config.compressionGaindB = 12;   // 表示音频最大的增益能力，如设置为 12dB，最大可以被提升 12dB；
    gain_config.limiterEnable = kAgcTrue;  // 一般与 targetLevelDbfs 配合使用，compressionGaindB 是调节小音量的增益范围，limiter 则是对超过 targetLevelDbfs 的部分进行限制，避免数据爆音。
    if (WebRtcAgc_Init(gain_control_, 0, 255, kAgcModeAdaptiveDigital, fs) == -1
        || WebRtcAgc_set_config(gain_control_, gain_config) == -1) {
        LOG(ERROR) << "Error WebRtcAgc_Init | WebRtcAgc_set_config";
        return RET_FAIL;
    }
    // https://webrtc.ren/post?id=1428 Webrtc AGC 算法原理介绍（一）
    return RET_OK;
}




void AudioCapturer::Loop()
{
    LOG(INFO) << "into loop";
    AVPacket packet;
    audio_pts_ = 0;
    int64_t out_pts = 0;
    pcm_total_duration_ = 0;
    pcm_start_time_ = TimesUtil::GetTimeMillisecond();
    LOG(INFO) << "into loop while";
    int64_t pts = 0;
    int64_t pre_pts = 0;
    size_t samples = noise_frame_size_;
    int16_t *frameBuffer = (int16_t *) malloc(sizeof(*frameBuffer) * 4 * samples);
    int ret = 0;
    while(true) {
        if(request_exit_) {
            break;
        }
        ret = av_read_frame(ifmt_ctx_, &packet);
        if (ret < 0) {
            av_strerror(ret, err2str_, 1024);
            LOG(WARNING) << "read failed!, ret: " <<  err2str_;
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }
        if(!is_first_frame_) {
            is_first_frame_ = true;
            LOG(INFO) <<  AVPublishTime::GetInstance()->getAInTag() << ":t-" <<
                      AVPublishTime::GetInstance()->getCurrenTime();
        }
        if(packet.stream_index == audio_stream_) {
            if (int ret = avcodec_send_packet(codec_ctx_, &packet) && ret != 0) {
                char buffer[1024] = {0};
                av_strerror(ret, buffer, 1024);
                fprintf(stderr, "input AVPacket to decoder failed! ret = %d %s\n", ret, buffer);
            } else {
                while(1) {
                    int ret = avcodec_receive_frame(codec_ctx_, capture_frame_);
                    if (ret != 0)  {
                        //            char buffer[1024] = {0};
                        //            av_strerror(ret, buffer, 1024);
                        //            fprintf(stderr, "avcodec_receive_frame = %d %s\n", ret, buffer);
                        break;
                    }
                    // 发送数据给capture2noise_resampler_
                    int ret_size = audio_resampler_send_frame2(capture2noise_resampler_, capture_frame_->data, capture_frame_->nb_samples, audio_pts_);
                    audio_pts_ += capture_frame_->nb_samples;
                    do {
                        ret_size = audio_resampler_receive_frame2(capture2noise_resampler_, noise_frame_->data, noise_frame_size_, &out_pts);
                        if(ret_size > 0) {
                            // 对于降噪一般都是要开启的，所以这里如果不开启我们只是不进行降噪，但重采样逻辑还是保留和降噪一致
                            // 降噪处理处理
                            if(audio_ns_level_ != kDisable) {
                                // 左边声道处理
                                int16_t *input = ( int16_t *)noise_frame_->data[0];
                                for (int k = 0; k < samples; k++) {
                                    frameBuffer[k] = input[k];
                                }
                                int16_t *nsIn[1] = {frameBuffer};   //ns input[band][data]
                                int16_t *nsOut[1] = {frameBuffer};  //ns output[band][data]
                                WebRtcNs_Analyze(ns_l_handle_, (const int16_t *)nsIn[0]);
                                WebRtcNs_Process(ns_l_handle_, (const int16_t *const *) nsIn, 1, nsOut);
                                for (int k = 0; k < samples; k++) {
                                    input[k] = frameBuffer[k];
                                }
                                // 右边声道处理
                                input = ( int16_t *)noise_frame_->data[1];
                                for (int k = 0; k < samples; k++) {
                                    frameBuffer[k] = input[k];
                                }
                                int16_t *nsInR[1] = {frameBuffer};   //ns input[band][data]
                                int16_t *nsOutR[1] = {frameBuffer};  //ns output[band][data]
                                WebRtcNs_Analyze(ns_r_handle_, (const int16_t *)nsInR[0]);
                                WebRtcNs_Process(ns_r_handle_, (const int16_t *const *) nsInR, 1, nsOutR);
                                for (int k = 0; k < samples; k++) {
                                    input[k] = frameBuffer[k];
                                }
                            }
                            // 自动增益  先把自动增益关闭了
                            //                            {
                            //                                int16_t *input = ( int16_t *)noise_frame_->data[0];
                            //                                // 将起保存到本地
                            //                                if(!f_agc_before_) {
                            //                                    f_agc_before_ = fopen("f_agc_before_s16_1ch_16k.pcm", "wb");
                            //                                    f_agc_after_  = fopen("f_agc_after_s16_1ch_16k.pcm", "wb");
                            //                                }
                            //                                // s16 2字节
                            //                                fwrite(input, 2, samples, f_agc_before_);
                            //                                fflush(f_agc_before_);
                            //                                uint8_t saturationWarning = 0;
                            //                                int32_t outMicLevel = 0;
                            //                                int16_t *nsInL[1] = {input};   //ns input[band][data]
                            //                                int16_t *nsOutL[1] = {frameBuffer};  //ns output[band][data]
                            //                                WebRtcAgc_Process(gain_control_, nsInL, 1, samples,
                            //                                                  nsOutL, 0, &outMicLevel, 0, &saturationWarning);
                            //                                for (int k = 0; k < samples; k++) {
                            //                                    input[k] = frameBuffer[k];
                            //                                }
                            //                                fwrite(input, 2, samples, f_agc_after_);
                            //                                fflush(f_agc_after_);
                            //                            }
                            // 将数据发送noise->encode的重采样器
                            ret_size = audio_resampler_send_frame2(noise2encode_resampler_, noise_frame_->data, noise_frame_->nb_samples, out_pts);
                        } else {
                            //                        printf("can't get %d samples, ret_size:%d, cur_size:%d\n", audio_enc_frame_size_, ret_size,
                            //                               audio_resampler_get_fifo_size(audio_resampler_));
                            break;
                        }
                    } while (1);
                    do {
                        ret_size = audio_resampler_receive_frame2(noise2encode_resampler_, encode_frame_->data, audio_enc_frame_size_, &out_pts);
                        if(ret_size > 0) {
                            pts = AVPublishTime::GetInstance()->get_audio_pts();
                            //                            printf("in_pts:%ldms  ", TimesUtil::GetTimeMillisecond() - pcm_start_time_ );
                            //                            printf("out_pts:%ldms ",  int64_t(out_pts * av_q2d(tb) *1000));
                            //                            printf("ret_size:%d\n",  ret_size);
                            if(pts >= 0) {
                                encode_frame_->pts = pts;
                                //                                LOG(INFO) << "out_pts:" << encode_frame_->pts <<", delta:" <<  encode_frame_->pts -pre_pts;
                                pre_pts = encode_frame_->pts;
                                callback_get_frame_(encode_frame_);
                            } else {
                                // < 0说明录制处于暂停状态，只采集但不发送给编码器
                            }
                        } else {
                            //                        printf("can't get %d samples, ret_size:%d, cur_size:%d\n", audio_enc_frame_size_, ret_size,
                            //                               audio_resampler_get_fifo_size(audio_resampler_));
                            break;
                        }
                    } while (1);
                }
            }
        } else {
            fprintf(stderr, "other %d \n", packet.stream_index);
        }
        av_packet_unref(&packet);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    LOG(INFO) << "Audio capture thread leave";
}

void AudioCapturer::SetPause(bool pause)
{
    if(pause_ == pause) {
        LOG(WARNING) << "invalid operation";
        return;
    }
    pause_ = pause;
    if(pause_) {
        // 暂停录制
        AVPublishTime::GetInstance()->set_pause(true);
    } else {
        // 恢复录制
        AVPublishTime::GetInstance()->set_pause(false);
    }
}

bool AudioCapturer::initResample()
{
    // 重采样1， 采集转成 降噪
    //重采样设置选项----------------
    //输入的采样格式
    capture_sample_fmt_ = codec_ctx_->sample_fmt;
    //输入的采样率
    capture_sample_rate_ = codec_ctx_->sample_rate;
    //输入的声道布局
    capture_channel_layout_ = codec_ctx_->channel_layout;
    if (capture_channel_layout_ <= 0) {
        if (codec_ctx_->channels == 2) {
            capture_channel_layout_ = AV_CH_LAYOUT_STEREO;
        } else {
            capture_channel_layout_ = AV_CH_LAYOUT_MONO;
        }
    }
    // 重采样实例 用于采集后的格式转成降噪要的格式
    audio_resampler_params_t resampler_params;
    resampler_params.src_channel_layout = capture_channel_layout_;
    resampler_params.src_sample_fmt     = (enum AVSampleFormat)capture_sample_fmt_;
    resampler_params.src_sample_rate    =  capture_sample_rate_;
    resampler_params.dst_channel_layout = AV_CH_LAYOUT_STEREO;  //
    resampler_params.dst_sample_fmt     = (enum AVSampleFormat)noise_sample_fmt_;  // 降噪使用浮点数
    resampler_params.dst_sample_rate    = noise_sample_rate_;    // 降噪固定采用
    capture2noise_resampler_  = audio_resampler_alloc(resampler_params);
    if(!capture2noise_resampler_) {
        LOG(ERROR) << "audio_resampler_alloc failed";
        return false;
    }
    // 重采样2 降噪转成编码格式
    /// 由于ffmpeg编码aac需要输入FLTP格式的数据。
    //输入格式
    resampler_params.src_channel_layout = AV_CH_LAYOUT_STEREO;
    resampler_params.src_sample_fmt     = (enum AVSampleFormat)noise_sample_fmt_;
    resampler_params.src_sample_rate    =  noise_sample_rate_;
    // 输出格式
    resampler_params.dst_channel_layout = audio_enc_channel_layout_;  //
    resampler_params.dst_sample_fmt     = (enum AVSampleFormat)audio_enc_sample_fmt_;
    resampler_params.dst_sample_rate    = audio_enc_sample_rate_;
    noise2encode_resampler_  = audio_resampler_alloc(resampler_params);
    if(!noise2encode_resampler_) {
        LOG(ERROR) << "audio_resampler_alloc failed";
        return false;
    }
    return true;
}
}

