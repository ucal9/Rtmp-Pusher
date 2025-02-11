#include "rtmppusher.h"
#include "aacrtmppackager.h"
#include "timeutil.h"
#include "avpublishtime.h"
#include "easylogging++.h"
#include <chrono>
#include <thread>
namespace LQF
{
char * put_byte(char *output, uint8_t nVal)
{
    output[0] = nVal;
    return output + 1;
}
char * put_be16(char *output, uint16_t nVal)
{
    output[1] = nVal & 0xff;
    output[0] = nVal >> 8;
    return output + 2;
}
char * put_be24(char *output, uint32_t nVal)
{
    output[2] = nVal & 0xff;
    output[1] = nVal >> 8;
    output[0] = nVal >> 16;
    return output + 3;
}
char * put_be32(char *output, uint32_t nVal)
{
    output[3] = nVal & 0xff;
    output[2] = nVal >> 8;
    output[1] = nVal >> 16;
    output[0] = nVal >> 24;
    return output + 4;
}
char *  put_be64(char *output, uint64_t nVal)
{
    output = put_be32(output, nVal >> 32);
    output = put_be32(output, nVal);
    return output;
}
char * put_amf_string(char *c, const char *str)
{
    uint16_t len = strlen(str);
    c = put_be16(c, len);
    memcpy(c, str, len);
    return c + len;
}
char * put_amf_double(char *c, double d)
{
    *c++ = AMF_NUMBER;  /* type: Number */
    {
        unsigned char *ci, *co;
        ci = (unsigned char *)&d;
        co = (unsigned char *)c;
        co[0] = ci[7];
        co[1] = ci[6];
        co[2] = ci[5];
        co[3] = ci[4];
        co[4] = ci[3];
        co[5] = ci[2];
        co[6] = ci[1];
        co[7] = ci[0];
    }
    return c + 8;
}

void RTMPPusher::handle(int what, void *data)
{
    //要加是否断开连接逻辑
    if(!IsConnect()) {
        LOG(WARNING) << "开始断线重连";
        if(!Connect()) {
            LOG(ERROR) << "重连失败";
            delete data;
            return;
        }
    }
    switch(what) {
        case RTMP_BODY_METADATA: {
            if(!is_first_metadata_) {
                is_first_metadata_ = true;
                LOG(INFO) << AVPublishTime::GetInstance()->getMetadataTag() << ":t" << AVPublishTime::GetInstance()->getCurrenTime();
            }
            FLVMetadataMsg *metadata = (FLVMetadataMsg*)data;
            if(!SendMetadata(metadata)) {
                LOG(ERROR) << "SendMetadata failed";
            }
            delete metadata;
            break;
        }
        case RTMP_BODY_VID_CONFIG: {
            if(!is_first_video_sequence_) {
                is_first_video_sequence_ = true;
                LOG(INFO) << AVPublishTime::GetInstance()->getAvcHeaderTag() << ":t" << AVPublishTime::GetInstance()->getCurrenTime();
            }
            VideoSequenceHeaderMsg *vid_cfg_msg = (VideoSequenceHeaderMsg*)data;
            if(!sendH264SequenceHeader(vid_cfg_msg)) {
                LOG(ERROR) << "sendH264SequenceHeader failed";
            }
            delete vid_cfg_msg;
            break;
        }
        case RTMP_BODY_VID_RAW: {
            if(!is_first_video_frame_) {
                is_first_video_frame_ = true;
                LOG(INFO) << AVPublishTime::GetInstance()->getAvcFrameTag() << ":t" << AVPublishTime::GetInstance()->getCurrenTime();
            }
            NaluStruct* nalu = (NaluStruct*)data;
            if(sendH264Packet((char*)nalu->data, nalu->size, (nalu->type == 0x05) ? true : false,
                              nalu->pts)) {
                //LOG(INFO) << "send pack ok";
            } else {
                LOG(WARNING) << "at handle send h264 pack failed";
            }
            delete nalu;                       //注意要用new 会调用析构函数，释放内部空间
            break;
        }
        case RTMP_BODY_AUD_SPEC: {
            if(!is_first_audio_sequence_) {
                is_first_audio_sequence_ = true;
                LOG(INFO) << AVPublishTime::GetInstance()->getAacHeaderTag() << ":t" << AVPublishTime::GetInstance()->getCurrenTime();
            }
            AudioSpecMsg* audio_spec = (AudioSpecMsg*)data;
            uint8_t aac_spec_[4];
            aac_spec_[0] = 0xAF;
            aac_spec_[1] = 0x0;     // 0 = aac sequence header
            AACRTMPPackager::GetAudioSpecificConfig(&aac_spec_[2], audio_spec->profile_,
                                                    audio_spec->sample_rate_, audio_spec->channels_);
            SendAudioSpecificConfig((char *)aac_spec_, 4);
            break;
        }
        case RTMP_BODY_AUD_RAW: {
            if(!is_first_audio_frame_) {
                is_first_audio_frame_ = true;
                LOG(INFO) << AVPublishTime::GetInstance()->getAacDataTag() << ":t" << AVPublishTime::GetInstance()->getCurrenTime();
            }
            AudioRawMsg* audio_raw = (AudioRawMsg*)data;
            if(sendPacket(RTMP_PACKET_TYPE_AUDIO, (unsigned char*)audio_raw->data,
                          audio_raw->size, audio_raw->pts)) {
            } else {
                LOG(INFO) << "at handle send audio pack failed";
            }
            delete audio_raw;                       //注意要用new 会调用析构函数，释放内部空间
            break;
        }
        default:
            break;
    }
}

RTMPPusher::RTMPPusher(): RTMPBase(RTMP_BASE_TYPE_PUSH), NaluLoop(30)
{
    LOG(INFO) << "RTMPPusher create!";

    Properties vid_properties;
    // 降低分辨率
    vid_properties.SetProperty("video_width", 1280);  
    vid_properties.SetProperty("video_height", 720);
    // 降低码率
    vid_properties.SetProperty("video_bitrate", 2000000); // 2Mbps
    // 降低帧率
    vid_properties.SetProperty("video_fps", 25);

    // x264 特定参数
    vid_properties.SetProperty("preset", "veryfast");
    vid_properties.SetProperty("tune", "zerolatency");
    vid_properties.SetProperty("profile", "baseline");
    vid_properties.SetProperty("x264-params", "nal-hrd=cbr:force-cfr=1");
}


bool RTMPPusher::SendMetadata(FLVMetadataMsg *metadata)
{
    if (metadata == NULL) {
        return false;
    }
    char body[1024] = { 0 };
    char * p = (char *)body;
    p = put_byte(p, AMF_STRING);
    p = put_amf_string(p, "@setDataFrame");
    p = put_byte(p, AMF_STRING);
    p = put_amf_string(p, "onMetaData");
    p = put_byte(p, AMF_OBJECT);
    p = put_amf_string(p, "copyright");
    p = put_byte(p, AMF_STRING);
    p = put_amf_string(p, "firehood");
    if(metadata->has_video) {
        p = put_amf_string(p, "width");
        p = put_amf_double(p, metadata->width);
        p = put_amf_string(p, "height");
        p = put_amf_double(p, metadata->height);
        p = put_amf_string(p, "framerate");
        p = put_amf_double(p, metadata->framerate);
        p = put_amf_string(p, "videodatarate");
        p = put_amf_double(p, metadata->videodatarate);
        p = put_amf_string(p, "videocodecid");
        p = put_amf_double(p, FLV_CODECID_H264);
    }
    if(metadata->has_audio) {
        p = put_amf_string(p, "audiodatarate");
        p = put_amf_double(p, (double)metadata->audiodatarate);
        p = put_amf_string(p, "audiosamplerate");
        p = put_amf_double(p, (double)metadata->audiosamplerate);
        p = put_amf_string(p, "audiosamplesize");
        p = put_amf_double(p, (double)metadata->audiosamplesize);
        p = put_amf_string(p, "stereo");
        p = put_amf_double(p, (double)metadata->channles);
        p = put_amf_string(p, "audiocodecid");
        p = put_amf_double(p, (double)FLV_CODECID_AAC);
    }
    p = put_amf_string(p, "");
    p = put_byte(p, AMF_OBJECT_END);
    return sendPacket(RTMP_PACKET_TYPE_INFO, (unsigned char*)body, p - body, 0);
}

bool RTMPPusher::sendH264SequenceHeader(VideoSequenceHeaderMsg *seq_header)
{
    if (seq_header == NULL) {
        return false;
    }
    uint8_t body[1024] = { 0 };
    int i = 0;
    body[i++] = 0x17; // 1:keyframe  7:AVC
    body[i++] = 0x00; // AVC sequence header
    body[i++] = 0x00;
    body[i++] = 0x00;
    body[i++] = 0x00; // fill in 0;   0
    // AVCDecoderConfigurationRecord.
    body[i++] = 0x01;               // configurationVersion
    body[i++] = seq_header->sps_[1]; // AVCProfileIndication
    body[i++] = seq_header->sps_[2]; // profile_compatibility
    body[i++] = seq_header->sps_[3]; // AVCLevelIndication
    body[i++] = 0xff;               // lengthSizeMinusOne
    // sps nums
    body[i++] = 0xE1;                 //&0x1f
    // sps data length
    body[i++] = (seq_header->sps_size_ >> 8) & 0xff;;
    body[i++] = seq_header->sps_size_ & 0xff;
    // sps data
    memcpy(&body[i], seq_header->sps_, seq_header->sps_size_);
    i = i + seq_header->sps_size_;
    // pps nums
    body[i++] = 0x01; //&0x1f
    // pps data length
    body[i++] = (seq_header->pps_size_ >> 8) & 0xff;;
    body[i++] = seq_header->pps_size_ & 0xff;
    // sps data
    memcpy(&body[i], seq_header->pps_, seq_header->pps_size_);
    i = i + seq_header->pps_size_;
    time_ = TimesUtil::GetTimeMillisecond();
    //    time_ = Tim
    return sendPacket(RTMP_PACKET_TYPE_VIDEO, (unsigned char*)body, i, 0);
}

bool RTMPPusher::SendAudioSpecificConfig(char* data, int length)
{
    if(data == NULL) {
        return false;
    }
    RTMPPacket packet;
    RTMPPacket_Reset(&packet);
    RTMPPacket_Alloc(&packet, 4);
    packet.m_body[0] = data[0];
    packet.m_body[1] = data[1];
    packet.m_body[2] = data[2];
    packet.m_body[3] = data[3];
    packet.m_headerType  = RTMP_PACKET_SIZE_LARGE;
    packet.m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet.m_nChannel   = RTMP_AUDIO_CHANNEL;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = rtmp_->m_stream_id;
    packet.m_nBodySize  = 4;
    //调用发送接口 发送一个消息
    int nRet = RTMP_SendPacket(rtmp_, &packet, 0);
    if (nRet != TRUE) {
        LOG(ERROR) << "RTMP_SendPacket fail: " << nRet;
    }
    RTMPPacket_Free(&packet);//释放内存
    return (nRet == TRUE ? true : false);
}

bool RTMPPusher::sendH264Packet(char *data, int size, bool is_keyframe, unsigned int timestamp)
{
    if (data == NULL && size < 11) {
        return false;
    }
    // 检查数据大小是否合理
    if (size > 1024 * 1024) {  // 如果单个包超过1MB
        LOG(WARNING) << "Packet size too large: " << size;
        return false;
    }

    unsigned char *body = new unsigned char[size + 9];
    int i = 0;
    if (is_keyframe) {
        body[i++] = 0x17;// 1:Iframe  7:AVC
        static int keyframe_count = 0;
        if (++keyframe_count % 30 == 0) {  // 每30个关键帧记录一次
            LOG(INFO) << "Keyframe sent, timestamp: " << timestamp;
        }
    } else {
        body[i++] = 0x27;// 2:Pframe  7:AVC
    }
    
    // 添加发送速率控制
    static int64_t last_send_time = 0;
    int64_t current_time = TimesUtil::GetTimeMillisecond();
    
    if (last_send_time > 0) {
        int64_t elapsed = current_time - last_send_time;
        if (elapsed < 20) {  // 如果距离上次发送不足20ms
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }
    last_send_time = current_time;

    body[i++] = 0x01;// AVC NALU
    body[i++] = 0x00;
    body[i++] = 0x00;
    body[i++] = 0x00;
    // NALU size
    body[i++] = size >> 24;
    body[i++] = size >> 16;
    body[i++] = size >> 8;
    body[i++] = size & 0xff;;
    // NALU data
    memcpy(&body[i], data, size);
    bool bRet = sendPacket(RTMP_PACKET_TYPE_VIDEO, body, i + size, timestamp);
    delete[] body;
    return bRet;
}

int RTMPPusher::sendPacket(unsigned int packet_type, unsigned char *data,
                           unsigned int size, unsigned int timestamp)
{
    if (rtmp_ == NULL) {
        return FALSE;
    }
    RTMPPacket packet;
    RTMPPacket_Reset(&packet);
    RTMPPacket_Alloc(&packet, size);
    packet.m_packetType = packet_type;
    if(packet_type == RTMP_PACKET_TYPE_AUDIO) {
        //        packet.m_nChannel = RTMP_AUDIO_CHANNEL;
        //        LOG(INFO) << "audio packet timestamp:%u, delta:%u", timestamp, timestamp - audio_pre_timestamp);
        audio_pre_timestamp = timestamp;
    } else if(packet_type == RTMP_PACKET_TYPE_VIDEO) {
        packet.m_nChannel = RTMP_VIDEO_CHANNEL;
        //        LOG(INFO) << "video packet timestamp:%u, delta:%u", timestamp, timestamp - video_pre_timestamp);
        video_pre_timestamp  = timestamp;
    } else {
        //        packet.m_nChannel = RTMP_NETWORK_CHANNEL;
    }
    packet.m_nChannel = RTMP_AUDIO_CHANNEL;     // 这里参考obs，音频、视频使用同样的channel
    packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet.m_nTimeStamp = timestamp;
    packet.m_nInfoField2 = rtmp_->m_stream_id;
    packet.m_nBodySize = size;
    memcpy(packet.m_body, data, size);
    int nRet = RTMP_SendPacket(rtmp_, &packet, 0);
    if (nRet != 1) {
        LOG(INFO) << "RTMP_SendPacket fail: " << nRet;
    }
    RTMPPacket_Free(&packet);
    return nRet;
}
}

