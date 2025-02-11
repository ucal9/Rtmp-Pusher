#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QSysInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    initConnections();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    // 创建中央窗口部件
    centralWidget_ = new QWidget(this);
    setCentralWidget(centralWidget_);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget_);
    
    // 创建编码器设置组
    encoderGroup_ = new QGroupBox("编码器设置", this);
    QGridLayout *encoderLayout = new QGridLayout(encoderGroup_);
    
    // 硬件加速选项
    hwAccelCheck_ = new QCheckBox("启用硬件加速", this);
    encoderLayout->addWidget(hwAccelCheck_, 0, 0, 1, 2);
    
    // 编码器选择
    encoderCombo_ = new QComboBox(this);
    encoderLayout->addWidget(new QLabel("编码器:"), 1, 0);
    encoderLayout->addWidget(encoderCombo_, 1, 1);
    
    // 分辨率选择
    resolutionCombo_ = new QComboBox(this);
    resolutionCombo_->addItem("4K (3840x2160)", QSize(3840, 2160));
    resolutionCombo_->addItem("2K (2560x1440)", QSize(2560, 1440));
    resolutionCombo_->addItem("1080p (1920x1080)", QSize(1920, 1080));
    resolutionCombo_->addItem("720p (1280x720)", QSize(1280, 720));
    encoderLayout->addWidget(new QLabel("分辨率:"), 2, 0);
    encoderLayout->addWidget(resolutionCombo_, 2, 1);
    
    // 码率设置
    bitrateSpinBox_ = new QSpinBox(this);
    bitrateSpinBox_->setRange(1000, 50000);
    bitrateSpinBox_->setValue(8000);
    bitrateSpinBox_->setSuffix(" Kbps");
    encoderLayout->addWidget(new QLabel("码率:"), 3, 0);
    encoderLayout->addWidget(bitrateSpinBox_, 3, 1);
    
    // 帧率设置
    fpsSpinBox_ = new QSpinBox(this);
    fpsSpinBox_->setRange(1, 120);
    fpsSpinBox_->setValue(30);
    fpsSpinBox_->setSuffix(" fps");
    encoderLayout->addWidget(new QLabel("帧率:"), 4, 0);
    encoderLayout->addWidget(fpsSpinBox_, 4, 1);
    
    mainLayout->addWidget(encoderGroup_);
    
    // 创建标签页
    tabWidget_ = new QTabWidget(this);
    createPushTab();
    createRecordTab();
    mainLayout->addWidget(tabWidget_);
    
    // 初始化编码器选项
    updateEncoderOptions();
    
    // 设置窗口属性
    setWindowTitle("视频推流/录制");
    resize(600, 400);
}

void MainWindow::createPushTab()
{
    pushTab_ = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(pushTab_);
    
    // RTMP地址输入
    QGroupBox *urlGroup = new QGroupBox("推流地址", pushTab_);
    QHBoxLayout *urlLayout = new QHBoxLayout(urlGroup);
    rtmpUrlEdit_ = new QLineEdit(pushTab_);
    rtmpUrlEdit_->setPlaceholderText("请输入RTMP推流地址");
    urlLayout->addWidget(rtmpUrlEdit_);
    layout->addWidget(urlGroup);
    
    // 推流控制按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    startPushBtn_ = new QPushButton("开始推流", pushTab_);
    stopPushBtn_ = new QPushButton("停止推流", pushTab_);
    stopPushBtn_->setEnabled(false);
    btnLayout->addWidget(startPushBtn_);
    btnLayout->addWidget(stopPushBtn_);
    layout->addLayout(btnLayout);
    
    tabWidget_->addTab(pushTab_, "推流");
}

void MainWindow::createRecordTab()
{
    recordTab_ = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(recordTab_);
    
    // 保存路径设置
    QGroupBox *pathGroup = new QGroupBox("保存设置", recordTab_);
    QHBoxLayout *pathLayout = new QHBoxLayout(pathGroup);
    savePathEdit_ = new QLineEdit(recordTab_);
    browseBtn_ = new QPushButton("浏览", recordTab_);
    pathLayout->addWidget(savePathEdit_);
    pathLayout->addWidget(browseBtn_);
    layout->addWidget(pathGroup);
    
    // 录制控制按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    startRecordBtn_ = new QPushButton("开始录制", recordTab_);
    stopRecordBtn_ = new QPushButton("停止录制", recordTab_);
    stopRecordBtn_->setEnabled(false);
    btnLayout->addWidget(startRecordBtn_);
    btnLayout->addWidget(stopRecordBtn_);
    layout->addLayout(btnLayout);
    
    tabWidget_->addTab(recordTab_, "录制");
}

void MainWindow::initConnections()
{
    // 编码器设置相关
    connect(hwAccelCheck_, &QCheckBox::toggled, this, &MainWindow::onHWAccelToggled);
    
    // 推流相关
    connect(startPushBtn_, &QPushButton::clicked, this, &MainWindow::onStartPush);
    connect(stopPushBtn_, &QPushButton::clicked, this, &MainWindow::onStopPush);
    
    // 录制相关
    connect(browseBtn_, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getSaveFileName(
            this,
            "选择保存路径",
            QDir::homePath(),
            "MP4文件 (*.mp4);;所有文件 (*.*)"
        );
        if (!path.isEmpty()) {
            savePathEdit_->setText(path);
        }
    });
    connect(startRecordBtn_, &QPushButton::clicked, this, &MainWindow::onStartRecord);
    connect(stopRecordBtn_, &QPushButton::clicked, this, &MainWindow::onStopRecord);
}

void MainWindow::onHWAccelToggled(bool checked)
{
    updateEncoderOptions();
}

void MainWindow::updateEncoderOptions()
{
    // 保存当前选择的编码器名称
    QString currentEncoder = encoderCombo_->currentData().toString();
    
    // 清空现有选项
    encoderCombo_->clear();

    // 根据是否启用硬件加速添加不同的编码器选项
    if (hwAccelCheck_->isChecked()) {
        // 检测系统支持的硬件编码器
        if (QSysInfo::windowsVersion() != QSysInfo::WV_None) {
            // Windows 平台
            encoderCombo_->addItem("NVIDIA NVENC H.264", "h264_nvenc");
            encoderCombo_->addItem("NVIDIA NVENC HEVC", "hevc_nvenc");
            encoderCombo_->addItem("Intel QuickSync H.264", "h264_qsv");
            encoderCombo_->addItem("Intel QuickSync HEVC", "hevc_qsv");
            encoderCombo_->addItem("AMD AMF H.264", "h264_amf");
            encoderCombo_->addItem("AMD AMF HEVC", "hevc_amf");
        } else {
            // Linux 平台
            encoderCombo_->addItem("VAAPI H.264", "h264_vaapi");
            encoderCombo_->addItem("VAAPI HEVC", "hevc_vaapi");
        }
    } else {
        // 软件编码器
        encoderCombo_->addItem("H.264 (x264)", "libx264");
        encoderCombo_->addItem("H.265/HEVC (x265)", "libx265");
        encoderCombo_->addItem("VP8", "libvpx");
        encoderCombo_->addItem("VP9", "libvpx-vp9");
    }

    // 恢复之前选择的编码器
    if (!currentEncoder.isEmpty()) {
        int index = encoderCombo_->findData(currentEncoder);
        if (index >= 0) {
            encoderCombo_->setCurrentIndex(index);
        }
    }
}

// 在开始录制或推流时获取编码器设置
QString MainWindow::getSelectedEncoder() const
{
    return encoderCombo_->currentData().toString();
}

bool MainWindow::isHWAccelEnabled() const
{
    return hwAccelCheck_->isChecked();
} 