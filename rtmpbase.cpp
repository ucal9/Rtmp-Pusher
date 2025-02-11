#include "rtmpbase.h"

#include <stdio.h>
#include "librtmp/rtmp_sys.h"
//#include "librtmp/log.h"
#include "easylogging++.h"
#define DEF_TIMEOUT     30      /* seconds */
#define DEF_BUFTIME     (10 * 60 * 60 * 1000)   /* 10 hours default */
#ifdef _WIN32
    #pragma comment(lib, "ws2_32.lib")
#endif
namespace LQF
{

bool RTMPBase::initRtmp()
{
    bool ret_code = true;
#ifdef WIN32
    WORD version;
    WSADATA wsaData;
    version = MAKEWORD(1, 1);
    ret_code = (WSAStartup(version, &wsaData) == 0) ? true : false;
#endif
    LOG(INFO) << "at rtmp object create";
    rtmp_ = RTMP_Alloc();
    RTMP_Init(rtmp_);
    return ret_code;
}

RTMPBase::RTMPBase():
    rtmp_obj_type_(RTMP_BASE_TYPE_UNKNOW),
    enable_video_(true),
    enable_audio_(true)
{
    initRtmp();
}

RTMPBase::RTMPBase(RTMP_BASE_TYPE rtmp_obj_type):
    rtmp_obj_type_(rtmp_obj_type),
    enable_video_(true),
    enable_audio_(true)
{
    initRtmp();
}

RTMPBase::RTMPBase(RTMP_BASE_TYPE rtmp_obj_type, std::string& url):
    rtmp_obj_type_(rtmp_obj_type),
    url_(url),
    enable_video_(true),
    enable_audio_(true)
{
    initRtmp();
}

RTMPBase::RTMPBase(std::string& url, bool is_recv_audio, bool is_recv_video):
    rtmp_obj_type_(RTMP_BASE_TYPE_PLAY),
    url_(url),
    enable_video_(is_recv_video),
    enable_audio_(is_recv_audio)
{
    initRtmp();
}

RTMPBase::~RTMPBase()
{
    if (IsConnect()) {
        Disconnect();
    }
    RTMP_Free(rtmp_);
    rtmp_ = NULL;
#ifdef WIN32
    WSACleanup();
#endif // WIN32
}

void RTMPBase::SetConnectUrl(std::string& url)
{
    url_ = url;
}

bool RTMPBase::SetReceiveAudio(bool is_recv_audio)
{
    if(is_recv_audio == enable_audio_) {
        return true;
    }
    if(IsConnect()) {
        LOG(INFO) << "RTMP_SendReceiveAudio";
        if(RTMP_SendReceiveAudio(rtmp_, is_recv_audio)) {
            enable_audio_ = is_recv_audio;
            return true;
        }
    } else {
        enable_audio_ = is_recv_audio;
    }
    return false;
}

bool RTMPBase::SetReceiveVideo(bool is_recv_video)
{
    if(is_recv_video == enable_video_) {
        return true;
    }
    if(IsConnect()) {
        if(RTMP_SendReceiveVideo(rtmp_, is_recv_video)) {
            enable_video_ == is_recv_video;
            return true;
        }
    } else {
        enable_video_ = is_recv_video;
    }
    return false;
}

bool RTMPBase::IsConnect()
{
    return RTMP_IsConnected(rtmp_);
}

void RTMPBase::Disconnect()
{
    RTMP_Close(rtmp_);
}

bool RTMPBase::Connect()
{
    //断线重连必须执行次操作，才能重现连上（比较疑惑）
    RTMP_Free(rtmp_);
    rtmp_ = RTMP_Alloc();
    RTMP_Init(rtmp_);
    LOG(INFO) << "base begin connect";
    //set connection timeout,default 30s
    rtmp_->Link.timeout = 10;
    if (RTMP_SetupURL(rtmp_, (char*)url_.c_str()) < 0) {
        LOG(ERROR) << "RTMP_SetupURL failed!";
        return FALSE;
    }
    rtmp_->Link.lFlags |= RTMP_LF_LIVE;           //  then we can't seek/resume
    RTMP_SetBufferMS(rtmp_, 3600 * 1000); //1hour
    if(rtmp_obj_type_ == RTMP_BASE_TYPE_PUSH) {
        RTMP_EnableWrite(rtmp_);    /*设置可写,即发布流,这个函数必须在连接前使用,否则无效*/
    }
    if (!RTMP_Connect(rtmp_, NULL)) {
        LOG(INFO) << "RTMP_Connect failed!";
        return FALSE;
    }
    if (!RTMP_ConnectStream(rtmp_, 0)) {
        LOG(INFO) << "RTMP_ConnectStream failed";
        return FALSE;
    }
    //判断是否打开音视频,默认打开
    if(rtmp_obj_type_ == RTMP_BASE_TYPE_PUSH) {
        if(!enable_video_) {
            RTMP_SendReceiveVideo(rtmp_, enable_video_);
        }
        if(!enable_audio_) {
            RTMP_SendReceiveAudio(rtmp_, enable_audio_);
        }
    }
    return true;
}

bool RTMPBase::Connect(std::string url)
{
    url_ = url;
    return Connect();
}

uint32_t  RTMPBase::GetSampleRateByFreqIdx(uint8_t freq_idx )
{
    uint32_t freq_idx_table[] = {96000, 88200, 64000, 48000, 44100, 32000,
                                 24000, 22050, 16000, 12000, 11025, 8000, 7350
                                };
    if(freq_idx < 13) {
        return freq_idx_table[freq_idx];
    }
    LOG(ERROR) << "freq_idx: " << freq_idx << " is error";
    return 44100;
}
}
