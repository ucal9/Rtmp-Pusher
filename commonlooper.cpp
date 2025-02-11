#include "commonlooper.h"
#include "easylogging++.h"

namespace LQF
{
void* CommonLooper::trampoline(void* p)
{
    LOG(INFO) << "at CommonLooper trampoline";
    ((CommonLooper*)p)->Loop();
    return NULL;
}

CommonLooper::CommonLooper()
{
    request_exit_ = false;
}

RET_CODE CommonLooper::Start()
{
    LOG(INFO) << "at CommonLooper create";
    worker_ = new std::thread(trampoline, this);
    if(worker_ == NULL) {
        LOG(ERROR) << "new std::thread failed";
        return RET_FAIL;
    }
    running_ = true;
    return RET_OK;
}


CommonLooper::~CommonLooper()
{
    if (running_) {
        LOG(INFO) << "CommonLooper deleted while still running. Some messages will not be processed";
        Stop();
    }
}


void CommonLooper::Stop()
{
    request_exit_ = true;
    if(worker_) {
        worker_->join();
        delete worker_;
        worker_ = NULL;
    }
    running_ = false;
}

}
