#ifndef HOMEWINDOW_H
#define HOMEWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <iostream>
#include "pushwork.h"
#include "displaywind.h"
#include "toast.h"
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QGroupBox>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>

//using namespace std;
using namespace LQF;
using std::string;

namespace Ui {
class HomeWindow;
}

class HomeWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit HomeWindow(QWidget *parent = 0);
    ~HomeWindow();

    bool start(string  url);
    bool pause();
    bool resume();
    bool stop();
    int OutputVideo(const Frame *frame);
private:
    void initUi();
    bool startLiveOrRecord(string  url);

    std::string getRecordFileName();

     void do_updateTime(long millisecond);

     void startTimer();
     void stopTimer();
signals:
    // 发送要显示的提示信息
    void sig_showTips(Toast::Level leve, QString tips);
    void sig_updateTime(long millisecond);
private slots:
    void on_audioOptimizer();
    void on_recordSetting();
    void on_liveSetting();
    void on_startOrPauseBtn_clicked();
    void on_stopBtn_clicked();
    void on_showTips(Toast::Level leve, QString tips);
    void on_businessType();
    void on_typeGroupBox_clicked();

    void on_typeGroupBox_toggled(bool arg1);

    void on_fileRadioBtn_toggled(bool checked);

    void on_liveRadioButton_clicked(bool checked);

    void on_recRadioBtn_clicked(bool checked);

    void on_checkBoxRecord_clicked(bool checked);

    // 更新录制时长
    void on_updateTime(long millisecond);
    void on_timeOut();
    void on_openRecordDir_clicked();

    void onImageWatermarkClicked();
    void onTextWatermarkClicked();

    void onHWAccelToggled(bool checked);
    void updateEncoderOptions();
    QString getSelectedEncoder() const;
    bool isHWAccelEnabled() const;

private:
    int business_type_ = 0; //业务类型，0:直播推送（采集视频+声音推送）; 1:录制(采集视频+声音合成到本地) 2:文件推送，从本地读取文件推送(比如mp4);
    Ui::HomeWindow *ui;
    PushWork *push_work_ = nullptr;
    int push_state_ = 0;    // 0 初始化，1 推送或录制中, 2暂停，3停止，0初始化

    bool enable_record_ = false;        // 录制 只在录制功能以及直播的时候能开启，文件推送不会触发录制功能
    bool enable_rtmp_live_ = false;     // rtmp推流直播，可以同时开启 enable_record_录制功能
    bool enable_file_live_ = false;     // 文件推送播放，不会触发录制功能

    std::string record_file_name_;
    QTimer *timer_ = nullptr;   // 定时1秒读取当前播放时长

    // 编码器相关控件
    QGroupBox *encoderGroup_;
    QComboBox *encoderCombo_;    // 编码器选择
    QCheckBox *hwAccelCheck_;    // 硬件加速开关
    QComboBox *resolutionCombo_; // 分辨率选择
    QSpinBox *bitrateSpinBox_;   // 码率设置
    QSpinBox *fpsSpinBox_;      // 帧率设置
    QSpinBox *gopSpinBox_;      // GOP设置
    QComboBox *presetCombo_;    // 编码预设
    QComboBox *tuneCombo_;      // 编码调优
    QComboBox *profileCombo_;   // 编码配置(Baseline/Main/High)
    QSpinBox *qminSpinBox_;     // 最小量化参数
    QSpinBox *qmaxSpinBox_;     // 最大量化参数
    QComboBox *rcCombo_;        // 码率控制模式(CBR/VBR/CRF)

    void setupEncoderUI();
    void initEncoderConnections();
};

#endif // HOMEWINDOW_H
