#ifndef RECORDER_H
#define RECORDER_H
#include "mediabase.h"
#include "commonlooper.h"
#include <mutex>
#include <functional>
#include "ff_ffplay_def.h"  // 包含 Frame 结构体定义

extern "C" {
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavcodec/avcodec.h"
#include "libavutil/opt.h"
}

using namespace LQF;
class Recorder
{
public:
    Recorder();
    virtual ~Recorder();
    RET_CODE Init(const Properties& properties);
    void DeInit();
    RET_CODE WritePacket(AVPacket *pkt, MediaType media_type);
    // 连接服务器，如果连接成功则启动线程
    RET_CODE WriteHeader();
    RET_CODE WriteTrailer();

    // 如果有视频成分
    RET_CODE ConfigVideoStream(AVCodecContext *ctx);
    // 如果有音频成分
    RET_CODE ConfigAudioStream(AVCodecContext *ctx);

    void AddVideoPreviewCallback(std::function<int(const Frame*)> callback);

private:
    // 整个输出流的上下文
    AVFormatContext *fmt_ctx_ = NULL;
    // 视频编码器上下文
    AVCodecContext *video_ctx_ = NULL;
    // 音频频编码器上下文
    AVCodecContext *audio_ctx_ = NULL;

    // 编码器的时间基
    AVRational audio_tb_ = {1, 1000};   // 音频时间基，设置为毫秒
    AVRational video_tb_ = {1, 1000};   // 视频时间基，设置为毫秒

    // 流成分
    AVStream *video_stream_ = NULL;
    int video_index_ = -1;
    AVStream *audio_stream_ = NULL;
    int audio_index_ = -1;

    std::string url_ = "";

    char str_error[256] = {0};
    std::mutex mutex_;

    std::function<int(const Frame*)> preview_callback_;

    void OnVideoFrame(AVFrame* frame);  // 添加声明
};

#endif // RECORDER_H
