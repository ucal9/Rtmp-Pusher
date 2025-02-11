#include "recorderdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QSysInfo>

RecorderDialog::RecorderDialog(QWidget *parent)
    : QDialog(parent)
    , recorder_(new Recorder())
{
    setupUI();
    initConnections();
}

void RecorderDialog::setupUI()
{
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 视频设置组
    QGroupBox *videoGroup = new QGroupBox("视频设置", this);
    QGridLayout *videoLayout = new QGridLayout(videoGroup);

    // 分辨率选择
    resolutionCombo_ = new QComboBox(this);
    resolutionCombo_->addItem("4K (3840x2160)", QSize(3840, 2160));
    resolutionCombo_->addItem("2K (2560x1440)", QSize(2560, 1440));
    resolutionCombo_->addItem("1080p (1920x1080)", QSize(1920, 1080));
    resolutionCombo_->addItem("720p (1280x720)", QSize(1280, 720));
    resolutionCombo_->addItem("480p (854x480)", QSize(854, 480));
    videoLayout->addWidget(new QLabel("分辨率:"), 0, 0);
    videoLayout->addWidget(resolutionCombo_, 0, 1);

    // 码率设置
    bitrateSpinBox_ = new QSpinBox(this);
    bitrateSpinBox_->setRange(1000, 50000);
    bitrateSpinBox_->setValue(8000);
    bitrateSpinBox_->setSuffix(" Kbps");
    videoLayout->addWidget(new QLabel("码率:"), 2, 0);
    videoLayout->addWidget(bitrateSpinBox_, 2, 1);

    // 帧率设置
    fpsSpinBox_ = new QSpinBox(this);
    fpsSpinBox_->setRange(1, 120);
    fpsSpinBox_->setValue(30);
    fpsSpinBox_->setSuffix(" fps");
    videoLayout->addWidget(new QLabel("帧率:"), 3, 0);
    videoLayout->addWidget(fpsSpinBox_, 3, 1);

    // 硬件加速选项
    hwAccelCheck_ = new QCheckBox("启用硬件加速", this);
    videoLayout->addWidget(hwAccelCheck_, 4, 0, 1, 2);

    mainLayout->addWidget(videoGroup);

    // 保存路径设置
    QGroupBox *pathGroup = new QGroupBox("保存设置", this);
    QHBoxLayout *pathLayout = new QHBoxLayout(pathGroup);
    pathEdit_ = new QLineEdit(this);
    browseButton_ = new QPushButton("浏览", this);
    pathLayout->addWidget(pathEdit_);
    pathLayout->addWidget(browseButton_);
    mainLayout->addWidget(pathGroup);

    // 控制按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    startButton_ = new QPushButton("开始录制", this);
    stopButton_ = new QPushButton("停止录制", this);
    stopButton_->setEnabled(false);
    buttonLayout->addWidget(startButton_);
    buttonLayout->addWidget(stopButton_);
    mainLayout->addLayout(buttonLayout);

    setWindowTitle("视频录制设置");
    resize(400, 300);
}

void RecorderDialog::initConnections()
{
    connect(browseButton_, &QPushButton::clicked, this, &RecorderDialog::onBrowsePath);
    connect(startButton_, &QPushButton::clicked, this, &RecorderDialog::onStartRecord);
    connect(stopButton_, &QPushButton::clicked, this, &RecorderDialog::onStopRecord);
    connect(resolutionCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RecorderDialog::onResolutionChanged);
    connect(hwAccelCheck_, &QCheckBox::toggled, this, &RecorderDialog::onHWAccelToggled);
}

void RecorderDialog::onStartRecord()
{
    if (pathEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "警告", "请选择保存路径");
        return;
    }

    // 获取当前设置
    QSize resolution = resolutionCombo_->currentData().toSize();
    int bitrate = bitrateSpinBox_->value() * 1000; // 转换为bps
    int fps = fpsSpinBox_->value();
    bool useHWAccel = hwAccelCheck_->isChecked();
    QString savePath = pathEdit_->text();

    // 配置录制参数
    Recorder::Properties properties;
    properties.SetProperty("width", resolution.width());
    properties.SetProperty("height", resolution.height());
    properties.SetProperty("bitrate", bitrate);
    properties.SetProperty("fps", fps);
    properties.SetProperty("enable_hwaccel", useHWAccel);
    properties.SetProperty("save_path", savePath.toStdString());

    if (recorder_->Init(properties) != RET_OK) {
        QMessageBox::critical(this, "错误", "初始化录制器失败");
        return;
    }

    recorder_->Start();
    startButton_->setEnabled(false);
    stopButton_->setEnabled(true);
    // 禁用设置项
    resolutionCombo_->setEnabled(false);
    bitrateSpinBox_->setEnabled(false);
    fpsSpinBox_->setEnabled(false);
    hwAccelCheck_->setEnabled(false);
    pathEdit_->setEnabled(false);
    browseButton_->setEnabled(false);
}

void RecorderDialog::onStopRecord()
{
    recorder_->DeInit();
    
    startButton_->setEnabled(true);
    stopButton_->setEnabled(false);
    // 启用设置项
    resolutionCombo_->setEnabled(true);
    bitrateSpinBox_->setEnabled(true);
    fpsSpinBox_->setEnabled(true);
    hwAccelCheck_->setEnabled(true);
    pathEdit_->setEnabled(true);
    browseButton_->setEnabled(true);
}

void RecorderDialog::onBrowsePath()
{
    QString path = QFileDialog::getSaveFileName(
        this,
        "选择保存路径",
        QDir::homePath(),
        "MP4文件 (*.mp4);;所有文件 (*.*)"
    );
    if (!path.isEmpty()) {
        pathEdit_->setText(path);
    }
}

void RecorderDialog::onResolutionChanged(int index)
{
    QSize resolution = resolutionCombo_->currentData().toSize();
    
    // 根据分辨率自动调整推荐码率
    int recommendedBitrate = 0;
    if(resolution.width() >= 3840) { // 4K
        recommendedBitrate = 35000; // 35Mbps
    }
    else if(resolution.width() >= 2560) { // 2K
        recommendedBitrate = 20000; // 20Mbps  
    }
    else if(resolution.width() >= 1920) { // 1080p
        recommendedBitrate = 8000;  // 8Mbps
    }
    else if(resolution.width() >= 1280) { // 720p
        recommendedBitrate = 5000;  // 5Mbps
    }
    else {
        recommendedBitrate = 2500;  // 2.5Mbps
    }

    bitrateSpinBox_->setValue(recommendedBitrate);
}

void RecorderDialog::onHWAccelToggled(bool checked)
{
}

void RecorderDialog::updateEncoderOptions()
{
    // ... 移除整个函数 ...
} 