#include "videocapturer.h"
#include "timeutil.h"
#include "avpublishtime.h"

#include <QtGlobal>
namespace LQF
{

// 声明 alloc_picture 函数
static AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height);

//Show Dshow Device
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
    LOG(INFO) << "========Device Option Info======";
    avformat_open_input(&pFormatCtx, "video=OBS Virtual Camera", iformat, &options);
    LOG(INFO) << "================================";
}

// 实现 alloc_picture 函数
static AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
{
    AVFrame *picture;
    int ret;
    picture = av_frame_alloc();
    if (!picture) {
        LOG(ERROR) << "Could not allocate frame data";
        return NULL;
    }
    picture->format = pix_fmt;
    picture->width  = width;
    picture->height = height;
    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(picture, 0);
    if (ret < 0) {
        LOG(ERROR) << "Could not av_frame_get_buffer";
        return NULL;
    }
    return picture;
}

VideoCapturer::VideoCapturer()
{
}

VideoCapturer::~VideoCapturer()
{
    Stop();
    if(video_frame_) {
        av_frame_free(&video_frame_);
    }
    if(yuv_frame_) {
        av_frame_free(&yuv_frame_);
    }
    if(codec_ctx_) {
        avcodec_free_context(&codec_ctx_);
    }
    if(ifmt_ctx_) {
        avformat_close_input(&ifmt_ctx_);
    }
}

RET_CODE VideoCapturer::Init(const Properties &properties)
{
    device_name_ = "video=OBS Virtual Camera";  // 恢复原来的设备名称
    video_enc_width_ = properties.GetProperty("video_enc_width", 0);
    video_enc_height_ = properties.GetProperty("video_enc_height", 0);
    video_enc_pix_fmt_ = properties.GetProperty("video_enc_pix_fmt", -1);
    capture_fps_ = properties.GetProperty("fps", 25);

    if(video_enc_width_ == 0
       || video_enc_height_ == 0
       || video_enc_pix_fmt_ == -1
       || capture_fps_ == 0) {
        LOG(ERROR) << "VideoCapturer init properties have null parameter, please check it";
        return RET_FAIL;
    }
    if(device_name_.empty()) {
        LOG(ERROR) << "VideoCapturer init failed, device_name is null";
        return RET_FAIL;
    }
    x_ = properties.GetProperty("x", 0);
    y_ = properties.GetProperty("y", 0);
    AVCodec *codec = nullptr;
    ifmt_ctx_ = avformat_alloc_context();

#ifdef Q_OS_WIN
    //Show Dshow Device
    show_dshow_device();
    //Show Device Options
    show_dshow_device_option();
    AVInputFormat *ifmt = av_find_input_format("dshow"); //使用dshow
    AVDictionary *param = NULL;
    char framerate[32] = {0};
    sprintf(framerate, "%d", capture_fps_);
    // 只保留基本的帧率设置
    if(avformat_open_input(&ifmt_ctx_, device_name_.c_str(), ifmt, &param) != 0) {
        LOG(ERROR) << "Couldn't open input stream video: " << device_name_;
        return RET_FAIL;
    }
#endif

    video_stream_ = -1;
    codec_ctx_   = nullptr;
    for(unsigned int i = 0; i < ifmt_ctx_->nb_streams; i++) {
        if(ifmt_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_ = static_cast<int>(i);
            break;
        }
    }
    if(video_stream_ == -1) {
        LOG(ERROR) << "Didn't find a video stream";
        return RET_FAIL;
    }
    //    pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    //find the decoder
    codec_ctx_ = avcodec_alloc_context3(nullptr);
    avcodec_parameters_to_context(codec_ctx_, ifmt_ctx_->streams[video_stream_]->codecpar);
    codec = (AVCodec *)avcodec_find_decoder(codec_ctx_->codec_id);
    if(codec == nullptr) {
        LOG(ERROR) << "video Codec not found";
        return RET_FAIL;
    }
    if(avcodec_open2(codec_ctx_, codec, nullptr) < 0) {
        LOG(ERROR) << "Could not open video codec";
        return RET_FAIL;
    }
    video_frame_ = av_frame_alloc();
    yuv_frame_ = av_frame_alloc();
    if (yuv_frame_) {
        yuv_frame_ = alloc_picture(AV_PIX_FMT_YUV420P, video_enc_width_, video_enc_height_);
        if (!yuv_frame_) {
            LOG(ERROR) << "Failed to allocate yuv frame";
            return RET_FAIL;
        }
    }
    return RET_OK;
}

void VideoCapturer::Loop()
{
    LOG(INFO) << "into loop";
    struct SwsContext *img_convert_ctx = nullptr;
    int64_t pts = 0;
    int64_t pre_pts = 0;

    if (codec_ctx_) {
        ///将数据转成YUV420P格式
        img_convert_ctx = sws_getContext(codec_ctx_->width, codec_ctx_->height, 
                                       (enum AVPixelFormat)codec_ctx_->pix_fmt,
                                       video_enc_width_, video_enc_height_, 
                                       (enum AVPixelFormat)video_enc_pix_fmt_,
                                       SWS_BICUBIC, nullptr, nullptr, nullptr);
        yuv_frame_ = alloc_picture(AV_PIX_FMT_YUV420P, video_enc_width_, video_enc_height_);
    }

    AVPacket packet;
    int64_t firstTime =  TimesUtil::GetTimeMillisecond();
    LOG(INFO) << "into loop while";
    while(true) {
        if(request_exit_) {
            break;
        }
        if (av_read_frame(ifmt_ctx_, &packet) < 0) {
            LOG(ERROR) << "read failed! ";
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }
        if(!is_first_frame_) {
            is_first_frame_ = true;
            LOG(INFO) <<  AVPublishTime::GetInstance()->getVInTag() << ":t"  << AVPublishTime::GetInstance()->getCurrenTime();
        }
        if(packet.stream_index == video_stream_) {
            if (avcodec_send_packet(codec_ctx_, &packet) != 0) {
                LOG(ERROR) << "input AVPacket to decoder failed!";
                av_packet_unref(&packet);
                continue;
            }
            while (0 == avcodec_receive_frame(codec_ctx_, video_frame_)) {
                /// 转换成YUV420
                /// 由于解码后的数据不一定是yuv420p，比如硬件解码后会是yuv420sp，因此这里统一转成yuv420p
                sws_scale(img_convert_ctx, (const uint8_t* const*)video_frame_->data, video_frame_->linesize, 0, codec_ctx_->height, yuv_frame_->data, yuv_frame_->linesize);
                pts = AVPublishTime::GetInstance()->get_video_pts();
                if(pts >= 0) {  // 时间戳大于等于0才发送给编码器
                    if(pre_pts >= pts) {                     
                        yuv_frame_->pts = pre_pts;      //TimesUtil::GetTimeMillisecond() - firstTime;
                        LOG(WARNING) << "video pts failed";
                    } else {
                        yuv_frame_->pts = pts;
                        pre_pts = pts;
                    }
                    //                    LOG(INFO) << "cap video pts: " << (float)(yuv_frame_->pts / 1000.0);
                    callback_get_frame_(yuv_frame_);
                } else {
                    //  <0 说明处于暂停状态，但采集继续保持
                }
            }
        } else {
            LOG(WARNING) << "other stream_index: " <<  packet.stream_index;
        }
        av_packet_unref(&packet);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}
}
