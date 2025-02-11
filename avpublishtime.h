#ifndef AVTIMEBASE_H
#define AVTIMEBASE_H
#include <stdint.h>
#include <math.h>
#ifdef _WIN32
    #include <winsock2.h>
    #include <time.h>
#else
    #include <sys/time.h>
#endif
#include <mutex>
#include "easylogging++.h"
class AVPublishTime
{
public:
    typedef enum PTS_STRATEGY {
        PTS_RECTIFY = 0,        // 缺省类型，pts的间隔尽量保持帧间隔
        PTS_REAL_TIME           // 实时pts
    } PTS_STRATEGY;
public:
    static AVPublishTime* GetInstance()
    {
        if ( s_publish_time == NULL ) {
            s_publish_time = new AVPublishTime();
        }
        return s_publish_time;
    }

    AVPublishTime()
    {
        start_time_ = getCurrentTimeMsec();
    }

    void Rest()
    {
        start_time_ = getCurrentTimeMsec();
    }

    void set_pause(bool pause)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if(pause_ == pause) {
            return;
        }
        pause_ = pause;
        if(!pause_) {  // 不暂停则要处理
            offset_time_ = (int64_t)audio_pre_pts_;
            start_time_ = getCurrentTimeMsec(); // 重置起始时间
        }
    }

    // 以音频为时间基准
    int64_t get_time()
    {
        return (int64_t)audio_pre_pts_;
    }

    // 单位毫秒
    void set_offset_time(int64_t offset_time)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        offset_time_ = offset_time;
    }

    void set_audio_frame_duration(const double frame_duration)
    {
        audio_frame_duration_ = frame_duration;
        audio_frame_threshold_ = (int64_t)(frame_duration * 2);
    }

    void set_video_frame_duration(const double frame_duration)
    {
        video_frame_duration_ = frame_duration;
        video_frame_threshold_ = (int64_t)(frame_duration * 2);
    }

    int64_t get_audio_pts()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if(pause_) {    // 暂停的时候返回-1
            return -1;
        }
        int64_t pts = getCurrentTimeMsec() - start_time_ + offset_time_;
        if(PTS_RECTIFY == audio_pts_strategy_) {
            int64_t diff = (int64_t)abs(pts - (int64_t)(audio_pre_pts_ + audio_frame_duration_));
            if(diff < audio_frame_threshold_) {
                // 误差在阈值范围内, 保持帧间隔
                audio_pre_pts_ += audio_frame_duration_; //帧间隔累加，浮点数
                //                LOG(INFO) << "get_audio_pts1: " << diff << ", RECTIFY: " << audio_pre_pts_;
                return (int64_t)audio_pre_pts_;
            }
            audio_pre_pts_ = (double)pts; // 误差超过阈值，重新调整pts
            //            LOG(INFO) << "get_audio_pts2: " << diff << ", RECTIFY: " << audio_pre_pts_;
            return pts;
        } else {
            audio_pre_pts_ = (double)pts;
            LOG(INFO) << "get_audio_pts REAL_TIME" <<  audio_pre_pts_;
            return pts;
        }
    }

    int64_t get_video_pts()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if(pause_) {    // 暂停的时候返回-1
            return -1;
        }
        int64_t pts = getCurrentTimeMsec() - start_time_ + offset_time_;
        if(PTS_RECTIFY == video_pts_strategy_) {
            int64_t diff = (int64_t)abs(pts - (int64_t)(video_pre_pts_ + video_frame_duration_));
            if(diff < video_frame_threshold_) {
                // 误差在阈值范围内, 保持帧间隔
                video_pre_pts_ += video_frame_duration_;
                //                LOG(INFO) << "get_video_pts1: " << diff << ", RECTIFY: " << video_pre_pts_;
                return (int64_t)video_pre_pts_;
            }
            video_pre_pts_ = (double)pts; // 误差超过阈值，重新调整pts
            LOG(INFO) << "get_video_pts2: " << diff << ", RECTIFY: " << video_pre_pts_;
            return pts;
        } else {
            video_pre_pts_ = (double)pts;
            //            LOG(INFO) << "get_video_pts REAL_TIME: " <<  video_pre_pts_;
            return pts;
        }
        return pts;
    }


    void set_audio_pts_strategy(PTS_STRATEGY pts_strategy)
    {
        audio_pts_strategy_ = pts_strategy;
    }
    void set_video_pts_strategy(PTS_STRATEGY pts_strategy)
    {
        video_pts_strategy_ = pts_strategy;
    }

    int64_t getCurrenTime()
    {
        int64_t t = getCurrentTimeMsec() - start_time_ + offset_time_;
        return  t;
    }
    // 各个关键点的时间戳
    inline const char *getKeyTimeTag()
    {
        return "keytime";
    }
    // rtmp位置关键点
    inline const char *getRtmpTag()
    {
        return "keytime:rtmp_publish";
    }

    // 发送metadata
    inline const char *getMetadataTag()
    {
        return "keytime:metadata";
    }
    // aac sequence header
    inline const char *getAacHeaderTag()
    {
        return "keytime:aacheader";
    }
    // aac raw data
    inline const char *getAacDataTag()
    {
        return "keytime:aacdata";
    }
    // avc sequence header
    inline const char *getAvcHeaderTag()
    {
        return "keytime:avcheader";
    }

    // 第一个i帧
    inline const char *getAvcIFrameTag()
    {
        return "keytime:avciframe";
    }
    // 第一个非i帧
    inline const char *getAvcFrameTag()
    {
        return "keytime:avcframe";
    }
    // 音视频解码
    inline const char *getAcodecTag()
    {
        return "keytime:acodec";
    }
    inline const char *getVcodecTag()
    {
        return "keytime:vcodec";
    }
    // 音视频捕获
    inline const char *getAInTag()
    {
        return "keytime:ain";
    }
    inline const char *getVInTag()
    {
        return "keytime:vin";
    }
private:
    int64_t getCurrentTimeMsec()
    {
#ifdef _WIN32
        struct timeval tv;
        time_t clock;
        struct tm tm;
        SYSTEMTIME wtm;
        GetLocalTime(&wtm);
        tm.tm_year = wtm.wYear - 1900;
        tm.tm_mon = wtm.wMonth - 1;
        tm.tm_mday = wtm.wDay;
        tm.tm_hour = wtm.wHour;
        tm.tm_min = wtm.wMinute;
        tm.tm_sec = wtm.wSecond;
        tm.tm_isdst = -1;
        clock = mktime(&tm);
        tv.tv_sec = clock;
        tv.tv_usec = wtm.wMilliseconds * 1000;
        return ((unsigned long long)tv.tv_sec * 1000 + ( long)tv.tv_usec / 1000);
#else
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return ((unsigned long long)tv.tv_sec * 1000 + (long)tv.tv_usec / 1000);
#endif
    }
    std::mutex mutex_;
    bool pause_ = false;
    int64_t start_time_ = 0;
    int64_t offset_time_ = 0; // 用于录制暂停和恢复后都以

    PTS_STRATEGY audio_pts_strategy_ = PTS_RECTIFY;
    double audio_frame_duration_ = 21.3333;  // 默认按aac 1024 个采样点, 48khz计算
    int64_t audio_frame_threshold_ = (int64_t)(audio_frame_duration_ * 2);
    double audio_pre_pts_ = 0;

    PTS_STRATEGY video_pts_strategy_ = PTS_REAL_TIME;
    double video_frame_duration_ = 40;  // 默认是25帧计算
    int64_t video_frame_threshold_ = (int64_t)(video_frame_duration_ * 2);
    double video_pre_pts_ = 0;

    static AVPublishTime * s_publish_time;
};



#endif // AVTIMEBASE_H
