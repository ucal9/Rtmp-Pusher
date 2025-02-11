#include "bitratecontroller.h"
#include <QDebug>

BitrateController::BitrateController(QObject *parent)
    : QObject(parent)
    , checkTimer_(new QTimer(this))
    , currentBitrate_(8000)
    , minBitrate_(1000)
    , maxBitrate_(10000)
    , currentRTT_(0)
    , packetLoss_(0)
{
    connect(checkTimer_, &QTimer::timeout, 
            this, &BitrateController::onCheckNetworkStatus);
}

void BitrateController::start()
{
    checkTimer_->start(CHECK_INTERVAL);
}

void BitrateController::stop()
{
    checkTimer_->stop();
}

void BitrateController::setBitrateRange(int minBitrate, int maxBitrate)
{
    minBitrate_ = minBitrate;
    maxBitrate_ = maxBitrate;
}

void BitrateController::onCheckNetworkStatus()
{
    // 检查网络状态
    checkRTT();
    checkPacketLoss();
    
    // 计算最优码率
    int newBitrate = calculateOptimalBitrate();
    
    // 如果码率需要调整
    if (newBitrate != currentBitrate_) {
        currentBitrate_ = newBitrate;
        emit bitrateChanged(currentBitrate_);
    }
}

void BitrateController::checkRTT()
{
    // 这里需要集成实际的RTT检测
    // 可以使用RTMP的ping/pong机制
    // 或者使用自定义的心跳包
}

void BitrateController::checkPacketLoss()
{
    // 这里需要集成实际的丢包检测
    // 可以通过序列号检测或其他机制
}

int BitrateController::calculateOptimalBitrate()
{
    // 基于AIMD(Additive Increase/Multiplicative Decrease)算法
    int newBitrate = currentBitrate_;
    
    if (currentRTT_ > RTT_THRESHOLD || packetLoss_ > PACKET_LOSS_THRESHOLD) {
        // 网络拥塞，快速降低码率
        newBitrate = currentBitrate_ * 0.8;
    } else {
        // 网络良好，缓慢增加码率
        newBitrate = currentBitrate_ + 100;
    }
    
    // 确保码率在合理范围内
    newBitrate = qBound(minBitrate_, newBitrate, maxBitrate_);
    
    return newBitrate;
} 