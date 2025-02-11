#ifndef VIDEOCAPTURER_H
#define VIDEOCAPTURER_H

#include <functional>
#include "commonlooper.h"
#include "mediabase.h"
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#ifdef __cplusplus
}
#endif

namespace LQF {
using std::function;
class VideoCapturer: public CommonLooper
{
public:
    VideoCapturer();
    virtual ~VideoCapturer();
    /**
     * @brief Init
     * @param "x", x起始位置，缺省为0
     *          "y", y起始位置，缺省为0
     *          "width", 宽度，缺省为屏幕宽带
     *          "height", 高度，缺省为屏幕高度
     *          "format", 像素格式，AVPixelFormat对应的值，缺省为AV_PIX_FMT_YUV420P
     *          "fps", 帧数，缺省为25
     * @return
     */
    RET_CODE Init(const Properties& properties);

    virtual void Loop();
    void AddCallback(function<void(AVFrame *)> callback)
    {
        callback_get_frame_ = callback;
    }
private:
    int video_test_ = 0;

    int x_ = 0;
    int y_ = 0;
    int video_enc_width_ = 0;
    int video_enc_height_ = 0;
    int video_enc_pix_fmt_ = 0;
    int capture_fps_;


    function<void(AVFrame*)> callback_get_frame_ = NULL;

    AVFormatContext	*ifmt_ctx_ = nullptr;   // 其他的资源也要释放
    int             video_stream_ = -1;
    AVCodecContext	*codec_ctx_ = nullptr;

    AVFrame	*video_frame_ = nullptr;
    AVFrame *yuv_frame_ = nullptr;

    std::string device_name_;
    bool is_first_frame_ = false;
};
}


#endif // VIDEOCAPTURER_H
