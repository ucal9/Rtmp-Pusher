#include "Looper.h"
#include "timeutil.h"
#include "avpublishtime.h"
namespace LQF
{
void* Looper::trampoline(void* p)
{
    LOG(INFO) << "at Looper trampoline";
    ((Looper*)p)->loop();
    return NULL;
}

Looper::Looper(const int deque_max_size)
    : deque_max_size_(deque_max_size)
{
    LOG(INFO) << "at Looper create";
    head_data_available_ = new Semaphore(0);
    worker_ = new std::thread(trampoline, this);
    running_ = true;
}


Looper::~Looper()
{
    if (running_) {
        LOG(INFO) << "Looper deleted while still running. Some messages will not be processed";
        Stop();
    }
}

//
void Looper::Post(int what, void *data, bool flush)
{
    LooperMessage *msg = new LooperMessage();
    msg->what = what;
    msg->obj = (MsgBaseObj *)data;
    msg->quit = false;
    addmsg(msg, flush);
}

void Looper::addmsg(LooperMessage *msg, bool flush)
{
    // 获取锁，发数据
    int64_t t1 = TimesUtil::GetTimeMillisecond();
    queue_mutex_.lock();
    if (flush || msg_queue_.size() > deque_max_size_) {
        LOG(WARNING) << "flush queue,what: " << msg->what << ", size: " << msg_queue_.size()
                     << ", t:" << AVPublishTime::GetInstance()->getCurrenTime();
        while(msg_queue_.size() > 0) {
            LooperMessage * tmp_msg = msg_queue_.front();
            msg_queue_.pop_front();
            delete tmp_msg->obj;
            delete tmp_msg;
        }
    }
    msg_queue_.push_back(msg);
    queue_mutex_.unlock();
    // 发送数据进行通知
    if(1 == msg->what) {
        LOG(INFO) << "post msg what: " << msg->what << ", size: " << msg_queue_.size()
                  << ", t:" << AVPublishTime::GetInstance()->getCurrenTime();
    }
    head_data_available_->post();
    //    LOG(INFO) << "Looper post");
    int64_t t2 = TimesUtil::GetTimeMillisecond();
    if(t2 - t1 > 10) {
        LOG(WARNING) << "t2 - t1 = " << t2 - t1;
    }
}

void Looper::loop()
{
    LOG(INFO) << "into loop";
    LooperMessage *msg;
    while(true) {
        queue_mutex_.lock();
        int size = msg_queue_.size();
        if(size > 0) {
            msg = msg_queue_.front();
            msg_queue_.pop_front();
            queue_mutex_.unlock();
            //quit 退出
            if (msg->quit) {
                break;
            }
            if(msg->what == 1) {
                //                LOG(INFO) << "processing into msg " <<  msg->what << ", size: " << size << ", t:%u" << AVPublishTime::GetInstance()->getCurrenTime();
            }
            handle(msg->what, msg->obj);
            if(msg->what == 1) {
                //                LOG(INFO) << "processing leave msg " <<  msg->what << ", size: " << size << ", t:%u" << AVPublishTime::GetInstance()->getCurrenTime();
            }
            delete msg;
        } else {
            queue_mutex_.unlock();
            //            if(msg->what == 1)
            //            LOG(INFO) << "sleep msg, t: " << AVPublishTime::GetInstance()->getCurrenTime();
            head_data_available_->wait();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    delete msg->obj;
    delete msg;
    while(msg_queue_.size() > 0) {
        msg = msg_queue_.front();
        msg_queue_.pop_front();
        delete msg->obj;
        delete msg;
    }
}

void Looper::Stop()
{
    if(running_) {
        LOG(INFO) << "Stop";
        LooperMessage *msg = new LooperMessage();
        msg->what = 0;
        msg->obj = NULL;
        msg->quit = true;
        addmsg(msg, true);
        if(worker_) {
            worker_->join();
            delete worker_;
            worker_ = NULL;
        }
        if(head_data_available_) {
            delete head_data_available_;
        }
        running_ = false;
    }
}

void Looper::handle(int what, void* obj)
{
    LOG(INFO) << "dropping msg what: " << what;
}
}
