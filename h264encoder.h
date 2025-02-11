#ifndef H264ENCODER_H
#define H264ENCODER_H
#include "mediabase.h"
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#ifdef __cplusplus
};
#endif



namespace LQF
{
using std::string;
class H264Encoder
{
public:
    H264Encoder();
    /**
     * @brief Init
     * @param properties
     * @return
     */
    virtual int Init(const Properties &properties, AVRational tb);
    virtual ~H264Encoder();
    virtual RET_CODE Input(const AVFrame *frame);
    virtual RET_CODE Output(AVPacket *pkt);
    int get_sps(uint8_t *sps, int &sps_len);
    int get_pps(uint8_t *pps, int &pps_len);
    inline int get_width(){
        return ctx_->width;
    }
    inline int get_height(){
        return ctx_->height;
    }
    virtual AVRational get_time_base() { return ctx_->time_base;}
    double get_framerate(){
        return ctx_->framerate.num/ctx_->framerate.den;
    }
    inline int64_t get_bit_rate(){
        return ctx_->bit_rate;
    }
    inline uint8_t *get_sps_data() {
        return (uint8_t *)sps_.c_str();
    }
    inline int get_sps_size(){
        return sps_.size();
    }
    inline uint8_t *get_pps_data() {
        return (uint8_t *)pps_.c_str();
    }
    inline int get_pps_size(){
        return pps_.size();
    }
    inline int get_pix_fmt(){
        return ctx_->pix_fmt;
    }
    AVCodecContext* GetCodecContext() {
        return ctx_;
    }
private:
    int count;
    int framecnt;

    // 初始化参数
    string codec_name_;  //
    int width_;     // 宽
    int height_;    // 高
    int fps_; // 帧率
    int b_frames_;   // b帧数量
    int bitrate_;
    int gop_;
    bool annexb_;       // 默认不带star code
    int threads_;
    string profile_;
    string level_id_;

    string sps_;
    string pps_;

    //encoder message
    AVCodec* codec_ = NULL;
    AVDictionary *param = NULL;
    AVCodecContext* ctx_ = NULL;

    int64_t pts_ = 0;
};
}
#endif // H264ENCODER_H
