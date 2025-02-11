#include "recorder.h"
#include "easylogging++.h"
#include <cmath>  // for sqrtf
Recorder::Recorder()
{
}

Recorder::~Recorder()
{
    DeInit();       // 释放资源
}


//通过属性方式方便以后扩展参数
RET_CODE Recorder::Init(const Properties &properties)
{
    url_ = properties.GetProperty("url", "");
    if(url_ == "") {
        LOG(ERROR) << "url is null";
        return RET_FAIL;
    }
    int ret = 0;
    // 分配AVFormatContext
    ret = avformat_alloc_output_context2(&fmt_ctx_, NULL, NULL, url_.c_str());
    if(ret < 0) {
        av_strerror(ret, str_error, sizeof(str_error) - 1);
        LOG(ERROR) << "avformat_alloc_output_context2 failed: " << str_error;
        return RET_FAIL;
    }
    return RET_OK;
}

void Recorder::DeInit()       // 这个函数重复调用没有问题
{
    if (fmt_ctx_ && (!(fmt_ctx_->oformat->flags & AVFMT_NOFILE))) {
        /* Close the output file. */
        avio_closep(&fmt_ctx_->pb);
        LOG(INFO) << "avio_closep";
    }
    if(fmt_ctx_) {
        avformat_free_context(fmt_ctx_);
        LOG(INFO) << "avformat_free_context fmt_ctx_";
        fmt_ctx_ = NULL;
    }
}

RET_CODE Recorder::WritePacket(AVPacket *pkt, MediaType media_type)
{
    //    如果多个线程同时使用同一个AVFormatContext来执行av_interleaved_write_packet操作，则可能存在线程安全问题。
    std::lock_guard<std::mutex> lock(mutex_);
    //直接写入，本地文件比较块，这里就不用消息队列了
    AVRational dst_time_base;
    AVRational src_time_base = {1, 1000};      // 我们采集、编码 时间戳单位都是ms
    if(E_VIDEO_TYPE == media_type) {
        src_time_base = video_tb_;
        pkt->stream_index = video_index_;
        dst_time_base = video_stream_->time_base;
    } else if(E_AUDIO_TYPE == media_type) {
        src_time_base = audio_tb_;
        pkt->stream_index = audio_index_;
        dst_time_base = audio_stream_->time_base;
    } else {
        LOG(ERROR) << "unknown mediatype: " << media_type;
        return RET_FAIL;
    }
    av_packet_rescale_ts(pkt, src_time_base, dst_time_base);
    if(E_VIDEO_TYPE == media_type) {
        //        LOG(INFO) << "file video dts: " << pkt->dts* av_q2d(dst_time_base);
        //        LOG(INFO) << "file video pts: " << pkt->pts* av_q2d(dst_time_base);
    } else if(E_AUDIO_TYPE == media_type) {
        //        LOG(INFO) << "file audio dts: " << pkt->dts* av_q2d(dst_time_base);
    }
    int ret = av_interleaved_write_frame(fmt_ctx_, pkt);
    if(ret < 0) {
        av_strerror(ret, str_error, sizeof(str_error) - 1);
        if(E_VIDEO_TYPE == media_type) {
            LOG(ERROR) << "av_write_frame video failed: " << str_error;       // 出错没有回调给PushWork
        } else {
            LOG(ERROR) << "av_write_frame audio failed: " << str_error;
        }
        return RET_FAIL;
    }
    return RET_OK;
}

RET_CODE Recorder::WriteHeader()
{
    //    if(!audio_stream_ && !video_stream_) {
    //        LOG(ERROR) << "audio_stream_ or video_stream_ is null");
    //        return RET_FAIL;
    //    }
    /* open the output file, if needed */
    if (!(fmt_ctx_->oformat->flags & AVFMT_NOFILE)) {
        int ret = avio_open(&fmt_ctx_->pb, url_.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            av_strerror(ret, str_error, sizeof(str_error) - 1);
            LOG(ERROR) <<  "Could not open: " << url_ << ", ret:" << str_error;
            return RET_FAIL;
        }
    }
    av_dump_format(fmt_ctx_, 0, url_.c_str(), 1);
    int ret = avformat_write_header(fmt_ctx_, NULL);
    if(ret < 0) {
        av_strerror(ret, str_error, sizeof(str_error) - 1);
        LOG(ERROR) << "av_opt_set failed: " << str_error;
        return RET_FAIL;
    }
    LOG(INFO) << "avformat_write_header ok";
    return RET_OK;
}

RET_CODE Recorder::WriteTrailer()
{
    int ret = av_write_trailer(fmt_ctx_);
    if(ret < 0) {
        av_strerror(ret, str_error, sizeof(str_error) - 1);
        LOG(ERROR) << "av_write_trailer failed: " << str_error;
        return RET_FAIL;
    }
    LOG(INFO) << "av_write_trailer ok";
    return RET_OK;
}


RET_CODE Recorder::ConfigVideoStream(AVCodecContext *ctx)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if(!fmt_ctx_) {
        LOG(ERROR) << "fmt_ctx is null";
        return RET_FAIL;
    }
    if(!ctx) {
        LOG(ERROR) << "ctx is null";
        return RET_FAIL;
    }
    // 添加视频流
    AVStream *vs = avformat_new_stream(fmt_ctx_, NULL);
    if(!vs) {
        LOG(ERROR) << "avformat_new_stream failed";
        return RET_FAIL;
    }
    //    vs->codecpar->codec_tag = 0;
    // 从编码器拷贝信息
    avcodec_parameters_from_context(vs->codecpar, ctx);
    av_dump_format(fmt_ctx_, 0, url_.c_str(), 1);
    video_ctx_ = (AVCodecContext *) ctx;
    video_stream_ = vs;
    video_index_ = vs->index;       // 整个索引非常重要 fmt_ctx_根据index判别 音视频包
    video_tb_ = ctx->time_base;
    return RET_OK;
}

RET_CODE Recorder::ConfigAudioStream(AVCodecContext *ctx)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if(!fmt_ctx_) {
        LOG(ERROR) << "fmt_ctx is null";
        return RET_FAIL;
    }
    if(!ctx) {
        LOG(ERROR) << "ctx is null";
        return RET_FAIL;
    }
    // 添加视频流
    AVStream *as = avformat_new_stream(fmt_ctx_, NULL);
    if(!as) {
        LOG(ERROR) << "avformat_new_stream failed";
        return RET_FAIL;
    }
    //    as->codecpar->codec_tag = 0;
    // 从编码器拷贝信息
    avcodec_parameters_from_context(as->codecpar, ctx);
    av_dump_format(fmt_ctx_, 0, url_.c_str(), 1);
    audio_ctx_ = (AVCodecContext *) ctx;
    audio_stream_ = as;
    audio_index_ = as->index;       // 整个索引非常重要 fmt_ctx_根据index判别 音视频包
    audio_tb_ = ctx->time_base;
    return RET_OK;
}

void Recorder::AddVideoPreviewCallback(std::function<int(const Frame*)> callback)
{
    preview_callback_ = callback;
}

void Recorder::OnVideoFrame(AVFrame* frame)
{
    if (preview_callback_) {
        Frame previewFrame;
        previewFrame.frame = frame;
        previewFrame.serial = 0;
        previewFrame.pts = 0;
        previewFrame.duration = 0;
        previewFrame.pos = 0;
        previewFrame.width = frame->width;
        previewFrame.height = frame->height;
        previewFrame.format = frame->format;
        previewFrame.sar = frame->sample_aspect_ratio;
        previewFrame.uploaded = 0;
        previewFrame.flip_v = 0;
        
        preview_callback_(&previewFrame);
    }
    
    // 继续处理编码等...
}
