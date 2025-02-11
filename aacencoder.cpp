#include "aacencoder.h"

/**
 * @brief AACEncoder::AACEncoder
 * @param properties
 *      key value
 *      sample_fmt 默认AV_SAMPLE_FMT_FLTP
 *      samplerate 默认 48000
 *      ch_layout  默认AV_CH_LAYOUT_STEREO
 *      bitrate    默认out_samplerate*3
 */
AACEncoder::AACEncoder()
{
}
/**
 * @brief 析构
 */
AACEncoder::~AACEncoder()
{
    if (ctx_) {
        avcodec_free_context(&ctx_);
    }
    if (frame_) {
        av_frame_free(&frame_);
    }
}

RET_CODE AACEncoder::Init(const Properties &properties, AVRational tb)
{
    // 获取参数
    sample_rate_ = properties.GetProperty("sample_rate", 48000);
    bitrate_ = properties.GetProperty("bitrate", 128 * 1024);
    channels_ = properties.GetProperty("channels", 2);
    channel_layout_ = properties.GetProperty("channel_layout",
                      (int)av_get_default_channel_layout(channels_));
    int ret;
    type_ = AudioCodec::AAC;
    // 读取默认的aac encoder
    codec_ = (AVCodec *)avcodec_find_encoder(AV_CODEC_ID_AAC);
    if(!codec_) {
        LOG(ERROR) << "AAC: No encoder found";
        return RET_ERR_MISMATCH_CODE;
    }
    ctx_ = avcodec_alloc_context3(codec_);
    if (ctx_ == NULL) {
        LOG(ERROR) << "AAC: could not allocate context.";
        return RET_ERR_OUTOFMEMORY;
    }
    //Set params
    ctx_->channels		= channels_;
    ctx_->channel_layout	= channel_layout_;
    ctx_->sample_fmt		= AV_SAMPLE_FMT_FLTP;
    ctx_->sample_rate	= sample_rate_;
    ctx_->bit_rate		= bitrate_;
    ctx_->thread_count	= 1;
    ctx_->time_base = tb;
    //Allow experimental codecs
    ctx_->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    if (avcodec_open2(ctx_, codec_, NULL) < 0) {
        LOG(ERROR) << "AAC: could not open codec";
        return RET_FAIL;
    }
    frame_byte_size_ = av_get_bytes_per_sample(ctx_->sample_fmt)
                       * ctx_->channels * ctx_->frame_size;
    //Create frame
    frame_ = av_frame_alloc();
    //Set defaults
    frame_->nb_samples     = ctx_->frame_size;
    frame_->format         = ctx_->sample_fmt;
    frame_->channel_layout = ctx_->channel_layout;
    frame_->sample_rate    = ctx_->sample_rate;
    //分配data buf
    ret = av_frame_get_buffer(frame_, 0);
    if(ret < 0) {
        LOG(ERROR) << "av_frame_get_buffer failed";
    }
    LOG(INFO) << "AAC: Encoder open with frame sample size: " <<   ctx_->frame_size;
    return RET_OK;
}


RET_CODE AACEncoder::Input(const AVFrame *frame)
{
    int ret = avcodec_send_frame(ctx_, frame);
    if(AVERROR(EAGAIN) == ret) {
        LOG(WARNING) << "please receive audio packet then send frame";
        return RET_ERR_EAGAIN;
    } else if(AVERROR_EOF == ret) {
        LOG(WARNING) <<  "if you wan continue use it, please new one audio decoder again";
        return RET_OK;
    }
    if(ret < 0) {
        return RET_FAIL;
    }
    return RET_OK;
}

RET_CODE AACEncoder::Output(AVPacket *pkt)
{
    int ret = avcodec_receive_packet(ctx_, pkt);
    if(ret != 0) {
        if(AVERROR(EAGAIN) == ret) {
            // LOG(WARNING) << "output audio is not available in the current state - user must try to send input";
            return RET_ERR_EAGAIN;
        } else if(AVERROR_EOF == ret) {
            LOG(WARNING) << "the audio encoder has been fully flushed, and there will be no more output packets";
            return RET_ERR_EOF;
        }
        return RET_FAIL;
    }
    return RET_OK;
}

