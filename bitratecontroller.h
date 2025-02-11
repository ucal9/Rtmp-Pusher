#ifndef BITRATECONTROLLER_H
#define BITRATECONTROLLER_H

#include <QObject>
#include <QTimer>

class BitrateController : public QObject
{
    Q_OBJECT
public:
    explicit BitrateController(QObject *parent = nullptr);
    
    void start();
    void stop();
    
    // 设置目标码率范围
    void setBitrateRange(int minBitrate, int maxBitrate);
    
signals:
    // 发出码率调整建议
    void bitrateChanged(int newBitrate);
    
private slots:
    void onCheckNetworkStatus();
    
private:
    // 网络状态检测
    void checkRTT();
    void checkPacketLoss();
    
    // 码率调整算法
    int calculateOptimalBitrate();
    
private:
    QTimer* checkTimer_;
    int currentBitrate_;
    int minBitrate_;
    int maxBitrate_;
    
    // 网络状态指标
    double currentRTT_;
    double packetLoss_;
    
    // 调整参数
    const int CHECK_INTERVAL = 2000;  // 检测间隔(ms)
    const double RTT_THRESHOLD = 100.0;  // RTT阈值(ms)
    const double PACKET_LOSS_THRESHOLD = 0.02;  // 丢包率阈值
};

#endif // BITRATECONTROLLER_H 