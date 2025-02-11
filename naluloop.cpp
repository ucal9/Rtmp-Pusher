#include "naluloop.h"
#include "easylogging++.h"
namespace LQF
{
NaluLoop::NaluLoop(int queue_nalu_len):
    max_nalu_(queue_nalu_len), Looper()
{}

//队列里面既有音频，也有视频
void NaluLoop::addmsg(LooperMessage *msg, bool flush)
{
    queue_mutex_.lock();
    if (flush) {
        msg_queue_.clear();
    }
    if(msg_queue_.size() >= max_nalu_) { //移除消息,直到下一个I帧，或者队列为空
        while(msg_queue_.size() > 0) {
            LooperMessage *tempMsg =  msg_queue_.front();
            // 从I帧开始, 5代码I帧 nalu type
            if(tempMsg->what == RTMP_BODY_VID_RAW && ((NaluStruct*)tempMsg->obj)->type == 5) {
                LOG(WARNING) << "drop msg, now have " << msg_queue_.size() << "frame";
                break;
            }
            msg_queue_.pop_front();
            delete tempMsg->obj;
            delete tempMsg;
        }
    }
    msg_queue_.push_back(msg);
    if(msg_queue_.size() > max_nalu_ / 3) {
        LOG(WARNING) << "have " << msg_queue_.size() << " msg cache, it will cause delay";
    }
    queue_mutex_.unlock();
    head_data_available_->post();
}
}
