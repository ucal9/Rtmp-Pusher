#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QGridLayout>
#include <QTabWidget>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onHWAccelToggled(bool checked);
    void updateEncoderOptions();
    void onStartPush();
    void onStopPush();
    void onStartRecord();
    void onStopRecord();

private:
    void setupUI();
    void initConnections();
    void createPushTab();    // 创建推流标签页
    void createRecordTab();  // 创建录制标签页
    
    // 主要控件
    QWidget *centralWidget_;
    QTabWidget *tabWidget_;
    
    // 推流标签页控件
    QWidget *pushTab_;
    QLineEdit *rtmpUrlEdit_;    // RTMP地址输入框
    QPushButton *startPushBtn_;
    QPushButton *stopPushBtn_;
    
    // 录制标签页控件
    QWidget *recordTab_;
    QLineEdit *savePathEdit_;   // 保存路径输入框
    QPushButton *browseBtn_;
    QPushButton *startRecordBtn_;
    QPushButton *stopRecordBtn_;
    
    // 编码器相关控件
    QGroupBox *encoderGroup_;
    QComboBox *encoderCombo_;    // 编码器选择
    QCheckBox *hwAccelCheck_;    // 硬件加速开关
    QComboBox *resolutionCombo_; // 分辨率选择
    QSpinBox *bitrateSpinBox_;   // 码率设置
    QSpinBox *fpsSpinBox_;      // 帧率设置
};

#endif // MAINWINDOW_H 