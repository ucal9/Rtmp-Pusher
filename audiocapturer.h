#ifndef AUDIOCAPTURER_H
#define AUDIOCAPTURER_H
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
#include "agc/gain_control.h"
#ifdef __cplusplus
}
#endif
#include "audioresampler.h"
#include "volumeadjuster.h"

#define USE_DSHOW 1

#include "noise_suppression.h"

namespace LQF
{
using std::function;

enum nsLevel {
    kDisable = -1,
    kLow,       //低     0
    kModerate,  // 中    1
    kHigh,      //高     2
    kVeryHigh   //非常高  3
};


class AudioCapturer : public CommonLooper
{
public:
    AudioCapturer();
    virtual ~AudioCapturer();
    /**
     * @brief Init
     * @param "audio_test": 缺省为0，为1时数据读取本地文件进行播放
     *        "input_pcm_name": 测试模式时读取的文件路径
     *        "sample_rate": 采样率
     *        "channels": 采样通道
     *        "sample_fmt": 采样格式
     * @return
     */
    RET_CODE Init(const Properties& properties);

    bool initResample();
    virtual void Loop();
    void AddCallback(function<void(AVFrame *)> callback)
    {
        callback_get_frame_ = callback;
    }

    void SetPause(bool pause);

private:
    bool pause_ = false;
    // 初始化参数
    int audio_test_ = 0;

    // PCM file只是用来测试, 写死为s16格式 2通道 采样率48Khz
    // 1帧1024采样点持续的时间21.333333333333333333333333333333ms

    int64_t pcm_start_time_ = 0;    // 起始时间
    double pcm_total_duration_ = 0;        // PCM读取累计的时间
    FILE *pcm_fp_ = nullptr;


    function<void(AVFrame *)> callback_get_frame_ = nullptr;

    bool is_first_frame_ = false;

    // 编码参数
    int audio_enc_sample_rate_ = 0;
    int audio_enc_sample_fmt_ = 0;
    uint64_t audio_enc_channel_layout_ = 0;
    int audio_enc_frame_size_ = 0;

    // 采样参数
    int capture_sample_rate_ = 16000;
    int capture_sample_fmt_ = 0;
    uint64_t capture_channel_layout_ = 0;
    int capture_frame_size_ = 0;




    // 重采样相关
    float audio_pts_ = 0;

    // pcm转换
//    capture_frame_(以实际采样点数为准)-> noise2encode_resampler_ -> noise_frame_(固定获取320采样点) -> noise2encode_resampler_ -> encode_frame_(固定获取1024采样点)

    audio_resampler_t  *capture2noise_resampler_ = nullptr;  // capture_frame_ ->
    // 音频采集相关
    std::string device_name_;
    AVFormatContext	*ifmt_ctx_ = nullptr;
    int             audio_stream_ = -1;
    AVCodecContext	*codec_ctx_ = nullptr;
    AVFrame	*capture_frame_ = nullptr;      // 保存解码后的数据



    // 降噪相关的处理
    // 降噪需要的参数
    nsLevel audio_ns_level_ = kDisable; // 默认关闭
    int noise_sample_rate_ = 16000;
    int noise_sample_fmt_ = (int)AV_SAMPLE_FMT_S16P;
    AVFrame	*noise_frame_ = nullptr;                             // 保存做回音消除后的数据
    audio_resampler_t  *noise2encode_resampler_ = nullptr;     // 用于降噪格式的转换， 输入32khz，输出32khz
    int noise_frame_size_ = 160;

   int audio_agc_level_ = 0;  //默认关闭


    ///以下变量用于音频重采样
    /// 由于ffmpeg解码出来后的pcm数据有可能是带平面的pcm，因此这里统一做重采样处理，
    /// 重采样成44100的16 bits 双声道数据(AV_SAMPLE_FMT_S16)
    AVFrame *encode_frame_ = nullptr;       // 保存要送给编码的数据（从noise_resampler_读取）
    char err2str_[512] = {0};

    // 降噪相关
    NsHandle *ns_l_handle_ = nullptr;
    NsHandle *ns_r_handle_ = nullptr;

    // 自动增益相关
    void *gain_control_ = nullptr;

    FILE *f_agc_before_ = nullptr;
    FILE *f_agc_after_ = nullptr;

    // 音量调整
    VolumeAdjuster *volume_adjuster_=nullptr;
};
}
#endif // AUDIOCAPTURER_H
