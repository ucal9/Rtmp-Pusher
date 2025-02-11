#pragma once
#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>
#include "recorder.h"

class RecorderDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RecorderDialog(QWidget *parent = nullptr);
    ~RecorderDialog();

private slots:
    void onStartRecord();
    void onStopRecord();
    void onBrowsePath();
    void onResolutionChanged(int index);
    void onHWAccelToggled(bool checked);

private:
    void setupUI();
    void initConnections();
    void updateEncoderOptions();

    // UI控件
    QComboBox *resolutionCombo_;     // 分辨率选择
    QSpinBox *bitrateSpinBox_;       // 码率设置
    QSpinBox *fpsSpinBox_;          // 帧率设置
    QCheckBox *hwAccelCheck_;        // 硬件加速开关
    QLineEdit *pathEdit_;            // 保存路径
    QPushButton *browseButton_;      // 浏览按钮
    QPushButton *startButton_;       // 开始录制
    QPushButton *stopButton_;        // 停止录制

    Recorder *recorder_;             // 录制器实例
}; 