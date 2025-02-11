#ifndef FF_FFPLAY_DEF_H
#define FF_FFPLAY_DEF_H
extern "C" {
#include "libavutil/avstring.h"
#include "libavutil/eval.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"
#include "libavutil/dict.h"
#include "libavutil/parseutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/avassert.h"
#include "libavutil/time.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "libavcodec/avfft.h"
#include "libswresample/swresample.h"
}


/* Common struct for handling all types of decoded data and allocated render buffers. */
// 用于缓存解码后的数据
typedef struct Frame {
    AVFrame		*frame;         // 指向数据帧
    int		serial;             // 帧序列，在seek的操作时serial会变化
    double		pts;            // 时间戳，单位为秒
    double		duration;       // 该帧持续时间，单位为秒
    int64_t pos;
    int		width;              // 图像宽度
    int		height;             // 图像高读
    int		format;             // 对于图像为(enum AVPixelFormat)
    AVRational sar;
    int uploaded;
    int flip_v;
} Frame;


#endif // FF_FFPLAY_DEF_H
