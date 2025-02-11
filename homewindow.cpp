#include "homewindow.h"
#include "ui_homewindow.h"
#include <QDateTime>
#include <QApplication>
#include <QDesktopWidget>
#include <QDesktopServices>
#include <QUrl>
#include <QFileDialog>
#include <QInputDialog>
#include <QFontDialog>
#include <QLineEdit>
#include "globalhelper.h"
#include "audiooptimizerdialog.h"
#include "recordsettingdialog.h"
#include "livesettingdialog.h"
#include "easylogging++.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QSysInfo>

#pragma execution_character_set("utf-8")
HomeWindow::HomeWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::HomeWindow)
{
    ui->setupUi(this);
    initUi();
    setupEncoderUI();
    initEncoderConnections();
}

HomeWindow::~HomeWindow()
{
    delete ui;
}

bool HomeWindow::start(std::string url)
{
    if(business_type_ == 0) {
        enable_record_ = ui->checkBoxRecord->isChecked();
    }
    bool ret = false;
    ret =  startLiveOrRecord(url);
    if(!ret) {
        QString tips;
        tips.sprintf("录制失败，请查看日志");
        emit sig_showTips(Toast::ERROR, tips);
        return false;
    }
    return true;
}

bool HomeWindow::stop()
{
    stopTimer();
    push_work_->DeInit();
    delete push_work_;
    push_work_ = nullptr;
    ui->previewWidget->StopPlay();
    ui->stopBtn->setEnabled(false);
    ui->startOrPauseBtn->setText("开始");
    return true;
}

int HomeWindow::OutputVideo(const Frame *frame)
{
    // 调用显示控件
    return ui->previewWidget->Draw(frame);
}

void HomeWindow::initUi()
{
    //加载样式
    QString qss = GlobalHelper::GetQssStr("://res/qss/homewindow.css");
    setStyleSheet(qss);
    ui->avLabel->setOpenExternalLinks(true);
    ui->avLabel->setText(tr("<a style='color: red; text-decoration: none' href = https://ke.qq.com/course/468797#term_id=100561187>点击了解更多零声教育音视频实战项目"));
    ui->liveRadioButton->setCheckable(true);
    ui->stopBtn->setEnabled(false);
    ui->startOrPauseBtn->setEnabled(true);
    // 初始化直播是否同时录制
    int live_record = 0;
    GlobalHelper::GetLiveRecord(live_record);
    if(live_record == 1) {
        ui->checkBoxRecord->setChecked(true);
        enable_record_ = true;
    } else {
        ui->checkBoxRecord->setChecked(false);
        enable_record_ = false;
    }
    // 初始化获取业务类型
    GlobalHelper::GetBusinessesType(business_type_);
    if(business_type_ == 0) {
        enable_rtmp_live_ = true;
        ui->liveRadioButton->setChecked(true);
    }
    if(business_type_ == 1) {
        enable_record_ = true;
        ui->recRadioBtn->setChecked(true);
    }
    //初始化的时候当前时间为0
    do_updateTime(0);
    // toast提示不能直接在非ui线程显示，所以通过信号槽的方式触发提示
    // 自定义信号槽变量类型要注册，参考:https://blog.csdn.net/Larry_Yanan/article/details/127686354
    qRegisterMetaType<Toast::Level>("Toast::Level");
    connect(this, &HomeWindow::sig_showTips, this, &HomeWindow::on_showTips);
    //音频优化
    connect(ui->actionAudioOptimizer, &QAction::triggered, this, &HomeWindow::on_audioOptimizer);
    // 录制设置
    connect(ui->actionRecordSetting, &QAction::triggered, this, &HomeWindow::on_recordSetting);
    // 直播设置
    connect(ui->actionLiveSetting, &QAction::triggered, this, &HomeWindow::on_liveSetting);
    // 更新录制时间设置
    connect(this, &HomeWindow::sig_updateTime, this, &HomeWindow::on_updateTime);
    // 在构造函数中添加信号槽连接
    connect(ui->imageWatermarkBtn, &QPushButton::clicked, this, &HomeWindow::onImageWatermarkClicked);
    connect(ui->textWatermarkBtn, &QPushButton::clicked, this, &HomeWindow::onTextWatermarkClicked);
}

bool HomeWindow::startLiveOrRecord(string url)
{
    // 直播业务且rtmp url为空
    if(url.empty() && business_type_ == 0) {
        emit sig_showTips(Toast::ERROR, "URL为NULL");
        return false;
    }
    if(push_work_) {
        stop();
    }
    // 设置属性
    Properties properties;
    //是否录制
    properties.SetProperty("enable_record", enable_record_);
    record_file_name_ = getRecordFileName();        // 获取录制文件名和路径
    properties.SetProperty("record_file_name", record_file_name_);
    // audio采集设备名称
    QString expect_mic_name;
    GlobalHelper::GetDefaultMicDevice(expect_mic_name); // 获取保存的默认设备
    QString act_mic_name = RecordSettingDialog::GetAudioDefaultMicDeviceName(expect_mic_name);
    if(act_mic_name.isEmpty()) {
        emit sig_showTips(Toast::ERROR, "没有找到麦克风设备");
        return false;
    }
    if(act_mic_name != expect_mic_name) {
        emit sig_showTips(Toast::WARN, "更换麦克风设备为:" + act_mic_name);
    }
    act_mic_name = "audio=" + act_mic_name;   //记得加上这个前缀
    properties.SetProperty("audio_device_name", act_mic_name.toStdString());
    int audio_ns = -1;
    GlobalHelper::GetAudioNs(audio_ns);
    properties.SetProperty("audio_ns",  audio_ns);
    int audio_agc = 0;
    GlobalHelper::GetAudioAgc(audio_agc);
    properties.SetProperty("audio_agc",  audio_agc);
    // video采集设备名称
    properties.SetProperty("video_device_name", "video=screen-capture-recorder");
    // 音频编码属性 设置采样格式，码率，声道数量  固定使用aac编码
    properties.SetProperty("audio_sample_rate", 48000);
    properties.SetProperty("audio_channels", 2);
    properties.SetProperty("audio_bitrate", 128 * 1024);
    // 桌面录制属性 分辨率、帧率、码率、像素格式  固定使用h264编码
    properties.SetProperty("desktop_x", 0);
    properties.SetProperty("desktop_y", 0);
    QDesktopWidget *qDesktopWidget = QApplication::desktop();
    QRect qrect1 = qDesktopWidget->screen(0)->rect();
    int desktop_width = qrect1.width();
    int desktop_height = qrect1.height();
    properties.SetProperty("desktop_width", desktop_width); //屏幕分辨率
    properties.SetProperty("desktop_height", desktop_height);  // 屏幕分辨率
    properties.SetProperty("desktop_pixel_format", AV_PIX_FMT_YUV420P);
    properties.SetProperty("desktop_fps", 25);//测试模式时和yuv文件的帧率一致
    // 视频编码属性 视频的分辨率和可以桌面采集不一样
    properties.SetProperty("video_bitrate", 3 * 1024 * 1024); // 设置码率
    // rtmp推流
    properties.SetProperty("enable_rtmp", enable_rtmp_live_);
    properties.SetProperty("rtmp_url", url.c_str());
    // 添加编码器预设和调优选项
    properties.SetProperty("encoder_preset", presetCombo_->currentData().toString().toStdString());
    properties.SetProperty("encoder_tune", tuneCombo_->currentData().toString().toStdString());
    // 添加编码器配置
    properties.SetProperty("encoder_profile", profileCombo_->currentData().toString().toStdString());
    properties.SetProperty("encoder_rc_mode", rcCombo_->currentData().toString().toStdString());
    properties.SetProperty("encoder_qmin", qminSpinBox_->value());
    properties.SetProperty("encoder_qmax", qmaxSpinBox_->value());
    push_work_ = new PushWork();
    push_work_->AddVidePreviewCallback(std::bind(&HomeWindow::OutputVideo, this,
                                       std::placeholders::_1));
    if(push_work_->Init(properties) != RET_OK) {
        delete push_work_;
        push_work_ = nullptr;
        LogError("pushwork.Init failed");
        return false;
    }
    ui->previewWidget->StartPlay();
    ui->stopBtn->setEnabled(true);
    ui->startOrPauseBtn->setText("暂停");
    startTimer();
    return true;
}

string HomeWindow::getRecordFileName()
{
    QString file_dir;
    GlobalHelper::GetFileDir(file_dir);
    QDateTime time = QDateTime::currentDateTime();
    // 比如 20230513-161813-769.jpg
    QString dateTime = file_dir + "/" + time.toString("yyyyMMdd-hhmmss-zzz") + ".mp4";
    return dateTime.toStdString();
}

void HomeWindow::do_updateTime(long millisecond)
{
    int seconds;
    seconds = millisecond / 1000; // 将毫秒转为秒
    int hour = int(seconds / 3600);
    int min = int((seconds - hour * 3600) / 60);
    int sec = seconds % 60;
    //QString格式化arg前面自动补0
    QString str = QString("%1:%2:%3").arg(hour, 2, 10, QLatin1Char('0')).arg(min, 2, 10, QLatin1Char('0')).arg(sec, 2, 10, QLatin1Char('0'));
    ui->timeLablel->setText(str);
}

void HomeWindow::startTimer()
{
    if(timer_) {
        timer_->stop();
        delete timer_;
        timer_ = nullptr;
    }
    timer_ = new QTimer();
    timer_->setInterval(1000);  // 1秒触发一次
    connect(timer_, SIGNAL(timeout()), this, SLOT(on_timeOut()));
    timer_->start();
}

void HomeWindow::stopTimer()
{
    if(timer_) {
        timer_->stop();
        delete timer_;
        timer_ = nullptr;
    }
}

void HomeWindow::on_audioOptimizer()
{
    // 音频优化设置 主要是降噪 声音增强等设置
    AudioOptimizerDialog  audio_optimizer_dlg_;
    audio_optimizer_dlg_.exec();
}

void HomeWindow::on_recordSetting()
{
    RecordSettingDialog  record_setting_dlg;
    record_setting_dlg.exec();
}

void HomeWindow::on_liveSetting()
{
    LiveSettingDialog live_setting_dlg;
    live_setting_dlg.exec();
}

void HomeWindow::on_startOrPauseBtn_clicked()
{
    if(!push_work_) {
        //加载rtmp串流地址
        QString url;
        GlobalHelper::GetDefaultRtmpUrl(url);
        // 加载rtmp串流密匙
        QString key;
        GlobalHelper::GetDefaultRtmpKey(key);
        QString url_key =  url + key;
        start(url_key.toStdString());
    } else {
        if(business_type_ == 1) {       // 单纯的录播才运行暂停和恢复
            // 判断暂停 继续的问题
            bool pause = push_work_->GetPause();
            pause = !pause; // 反转
            push_work_->SetPause(pause);
            if(pause) {
                ui->startOrPauseBtn->setText("继续");
            } else {
                ui->startOrPauseBtn->setText("暂停");
            }
        } else {
            emit sig_showTips(Toast::WARN, "只有单纯的录制业务才允许暂停/继续");
        }
    }
}

void HomeWindow::on_stopBtn_clicked()
{
    if(push_work_) {
        stop();
    }
}

void HomeWindow::on_showTips(Toast::Level leve, QString tips)
{
    Toast::instance().show(leve, tips);
}

void HomeWindow::on_businessType()
{
    //    LOG(INFO) << "on_businessType " << ui->typeGroupBox->ch;
}

void HomeWindow::on_typeGroupBox_clicked()
{
}

void HomeWindow::on_typeGroupBox_toggled(bool arg1)
{
    LOG(INFO) << "on_businessType " << arg1;
}

void HomeWindow::on_fileRadioBtn_toggled(bool checked)
{
    // 目前不支持本地文件推流直播
}

void HomeWindow::on_liveRadioButton_clicked(bool checked)
{
    LOG(INFO) << "on_liveRadioButton_clicked " << checked;
    enable_rtmp_live_ = checked;
    business_type_ = 0; // 录制推流，是否录制到本地由enable_record_决定
    if(ui->checkBoxRecord->isChecked()) {
        enable_record_ = true;
    }
    GlobalHelper::SaveBusinessesType(business_type_);
    updateEncoderOptions();
}

void HomeWindow::on_recRadioBtn_clicked(bool checked)
{
    LOG(INFO) << "on_recRadioBtn_clicked " << checked;
    enable_record_ = checked;
    if(enable_record_) {
        enable_rtmp_live_ = false;  //如果选中单纯录制则不能推流
    }
    if(ui->checkBoxRecord->isChecked()) {
        enable_record_ = true;      // 只要该按钮勾选则允许录制
    }
    business_type_ = 1; //单纯录制到本地
    GlobalHelper::SaveBusinessesType(business_type_);
    updateEncoderOptions();
}

void HomeWindow::on_checkBoxRecord_clicked(bool checked)
{
    if(checked) {
        GlobalHelper::SaveLiveRecord(1);
    } else {
        GlobalHelper::SaveLiveRecord(0);
    }
}

void HomeWindow::on_updateTime(long millisecond)
{
    do_updateTime(millisecond);
}

void HomeWindow::on_timeOut()
{
    if(push_work_) {
        // 读取录制时间 然后触发触发信号
        int64_t time =  push_work_->GetTime();
        emit sig_updateTime(time);
    }
}

// 打开保存录制文件的目录
void HomeWindow::on_openRecordDir_clicked()
{
    QString file_dir_path;
    GlobalHelper::GetFileDir(file_dir_path);
    QString path = QString("file:///") + file_dir_path;
    bool is_open = QDesktopServices::openUrl(QUrl(path, QUrl::TolerantMode));
    LOG(INFO) << "file_dir_path open? -> " << is_open;
}

void HomeWindow::onImageWatermarkClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("选择水印图片"), "",
        tr("图片文件 (*.png *.jpg *.bmp)"));
        
    if (!fileName.isEmpty()) {
        // 处理图片水印
        // TODO: 实现水印添加逻辑
    }
}

void HomeWindow::onTextWatermarkClicked()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("文字水印"),
                                       tr("请输入水印文字:"), QLineEdit::Normal,
                                       "", &ok);
    if (ok && !text.isEmpty()) {
        QFont font = QFontDialog::getFont(&ok, QFont("Arial", 12), this, tr("选择字体"));
        if (ok) {
            // 处理文字水印
            // TODO: 实现水印添加逻辑
        }
    }
}

void HomeWindow::setupEncoderUI()
{
    // 创建编码器设置组
    encoderGroup_ = new QGroupBox("编码器设置", this);
    encoderGroup_->setMinimumHeight(720);  // 增加高度
    QGridLayout *encoderLayout = new QGridLayout(encoderGroup_);
    encoderLayout->setSpacing(35);  // 增加控件间距
    encoderLayout->setContentsMargins(40, 45, 40, 45);  // 增加边距

    // 设置每个控件的最小高度
    const int WIDGET_HEIGHT = 55;  // 增加控件高度
    const int ROW_HEIGHT = 80;    // 增加行高

    // 硬件加速选项
    hwAccelCheck_ = new QCheckBox("启用硬件加速", this);
    hwAccelCheck_->setMinimumHeight(WIDGET_HEIGHT);
    encoderLayout->addWidget(hwAccelCheck_, 0, 0, 1, 2);
    encoderLayout->setRowMinimumHeight(0, ROW_HEIGHT);
    
    // 编码器选择
    encoderCombo_ = new QComboBox(this);
    encoderCombo_->setMinimumHeight(35);
    QLabel* encoderLabel = new QLabel("编码器:", this);
    encoderLayout->addWidget(encoderLabel, 1, 0);
    encoderLayout->addWidget(encoderCombo_, 1, 1);
    encoderLayout->setRowMinimumHeight(1, 45);
    
    // 分辨率选择
    resolutionCombo_ = new QComboBox(this);
    resolutionCombo_->setMinimumHeight(35);
    resolutionCombo_->addItem("4K (3840x2160)", QSize(3840, 2160));
    resolutionCombo_->addItem("2K (2560x1440)", QSize(2560, 1440));
    resolutionCombo_->addItem("1080p (1920x1080)", QSize(1920, 1080));
    resolutionCombo_->addItem("720p (1280x720)", QSize(1280, 720));
    QLabel* resolutionLabel = new QLabel("分辨率:", this);
    encoderLayout->addWidget(resolutionLabel, 2, 0);
    encoderLayout->addWidget(resolutionCombo_, 2, 1);
    encoderLayout->setRowMinimumHeight(2, 45);
    
    // 码率设置
    bitrateSpinBox_ = new QSpinBox(this);
    bitrateSpinBox_->setMinimumHeight(35);
    bitrateSpinBox_->setRange(1000, 50000);
    bitrateSpinBox_->setValue(8000);
    bitrateSpinBox_->setSuffix(" Kbps");
    QLabel* bitrateLabel = new QLabel("码率:", this);
    encoderLayout->addWidget(bitrateLabel, 3, 0);
    encoderLayout->addWidget(bitrateSpinBox_, 3, 1);
    encoderLayout->setRowMinimumHeight(3, 45);
    
    // 帧率设置
    fpsSpinBox_ = new QSpinBox(this);
    fpsSpinBox_->setMinimumHeight(35);
    fpsSpinBox_->setRange(1, 120);
    fpsSpinBox_->setValue(30);
    fpsSpinBox_->setSuffix(" fps");
    QLabel* fpsLabel = new QLabel("帧率:", this);
    encoderLayout->addWidget(fpsLabel, 4, 0);
    encoderLayout->addWidget(fpsSpinBox_, 4, 1);
    encoderLayout->setRowMinimumHeight(4, 45);
    
    // GOP设置
    gopSpinBox_ = new QSpinBox(this);
    gopSpinBox_->setMinimumHeight(35);
    gopSpinBox_->setRange(1, 300);  // GOP范围1-300
    gopSpinBox_->setValue(fpsSpinBox_->value());  // 默认值设为和帧率一样
    gopSpinBox_->setSuffix(" frames");
    QLabel* gopLabel = new QLabel("GOP:", this);
    encoderLayout->addWidget(gopLabel, 5, 0);
    encoderLayout->addWidget(gopSpinBox_, 5, 1);
    encoderLayout->setRowMinimumHeight(5, 45);
    
    // 编码预设选择
    presetCombo_ = new QComboBox(this);
    presetCombo_->setMinimumHeight(35);
    presetCombo_->addItem("ultrafast", "ultrafast");
    presetCombo_->addItem("superfast", "superfast");
    presetCombo_->addItem("veryfast", "veryfast");
    presetCombo_->addItem("faster", "faster");
    presetCombo_->addItem("fast", "fast");
    presetCombo_->addItem("medium", "medium");
    presetCombo_->addItem("slow", "slow");
    presetCombo_->addItem("slower", "slower");
    presetCombo_->addItem("veryslow", "veryslow");
    QLabel* presetLabel = new QLabel("编码预设:", this);
    encoderLayout->addWidget(presetLabel, 6, 0);
    encoderLayout->addWidget(presetCombo_, 6, 1);
    encoderLayout->setRowMinimumHeight(6, 45);
    
    // 编码调优选择
    tuneCombo_ = new QComboBox(this);
    tuneCombo_->setMinimumHeight(35);
    tuneCombo_->addItem("zerolatency (最低延迟)", "zerolatency");
    tuneCombo_->addItem("film (电影内容)", "film");
    tuneCombo_->addItem("animation (动画内容)", "animation");
    tuneCombo_->addItem("grain (保留噪点)", "grain");
    tuneCombo_->addItem("stillimage (静态图像)", "stillimage");
    tuneCombo_->addItem("psnr (优化PSNR)", "psnr");
    tuneCombo_->addItem("ssim (优化SSIM)", "ssim");
    QLabel* tuneLabel = new QLabel("优化选项:", this);
    encoderLayout->addWidget(tuneLabel, 7, 0);
    encoderLayout->addWidget(tuneCombo_, 7, 1);
    encoderLayout->setRowMinimumHeight(7, 45);
    
    // 编码配置选择
    profileCombo_ = new QComboBox(this);
    profileCombo_->setMinimumHeight(WIDGET_HEIGHT);
    profileCombo_->addItem("Baseline (低延迟)", "baseline");
    profileCombo_->addItem("Main (平衡)", "main");
    profileCombo_->addItem("High (高质量)", "high");
    QLabel* profileLabel = new QLabel("编码配置:", this);
    encoderLayout->addWidget(profileLabel, 8, 0);
    encoderLayout->addWidget(profileCombo_, 8, 1);
    encoderLayout->setRowMinimumHeight(8, ROW_HEIGHT);

    // 码率控制模式
    rcCombo_ = new QComboBox(this);
    rcCombo_->setMinimumHeight(WIDGET_HEIGHT);
    rcCombo_->addItem("CBR (固定码率)", "cbr");
    rcCombo_->addItem("VBR (可变码率)", "vbr");
    rcCombo_->addItem("CRF (固定质量)", "crf");
    QLabel* rcLabel = new QLabel("码率控制:", this);
    encoderLayout->addWidget(rcLabel, 9, 0);
    encoderLayout->addWidget(rcCombo_, 9, 1);
    encoderLayout->setRowMinimumHeight(9, ROW_HEIGHT);

    // 量化参数范围
    QWidget* qpWidget = new QWidget(this);
    QHBoxLayout* qpLayout = new QHBoxLayout(qpWidget);
    qminSpinBox_ = new QSpinBox(this);
    qmaxSpinBox_ = new QSpinBox(this);
    qminSpinBox_->setRange(0, 51);
    qmaxSpinBox_->setRange(0, 51);
    qminSpinBox_->setValue(10);
    qmaxSpinBox_->setValue(51);
    qminSpinBox_->setMinimumHeight(WIDGET_HEIGHT);
    qmaxSpinBox_->setMinimumHeight(WIDGET_HEIGHT);
    qpLayout->addWidget(qminSpinBox_);
    qpLayout->addWidget(new QLabel("~"));
    qpLayout->addWidget(qmaxSpinBox_);
    QLabel* qpLabel = new QLabel("量化范围:", this);
    encoderLayout->addWidget(qpLabel, 10, 0);
    encoderLayout->addWidget(qpWidget, 10, 1);
    encoderLayout->setRowMinimumHeight(10, ROW_HEIGHT);
    
    // 设置标签对齐方式和样式
    QList<QLabel*> labels = {encoderLabel, resolutionLabel, bitrateLabel, fpsLabel, gopLabel, presetLabel, tuneLabel, profileLabel, rcLabel, qpLabel};
    for (QLabel* label : labels) {
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        label->setMinimumWidth(180);
        label->setStyleSheet("QLabel { font-family: 'Microsoft YaHei', '微软雅黑'; font-size: 9pt; }");
    }
    
    // 设置下拉框和输入框的样式
    QString inputStyle = "QComboBox, QSpinBox { font-family: 'Microsoft YaHei', '微软雅黑'; font-size: 9pt; padding: 5px; }";
    encoderCombo_->setStyleSheet(inputStyle);
    resolutionCombo_->setStyleSheet(inputStyle);
    bitrateSpinBox_->setStyleSheet(inputStyle);
    fpsSpinBox_->setStyleSheet(inputStyle);
    gopSpinBox_->setStyleSheet(inputStyle);
    hwAccelCheck_->setStyleSheet("QCheckBox { font-family: 'Microsoft YaHei', '微软雅黑'; font-size: 9pt; }");
    
    // 将编码器设置组添加到专门的容器中
    if (ui->encoderContainerLayout) {
        ui->encoderContainerLayout->addWidget(encoderGroup_);
    }
    
    // 初始化编码器选项
    updateEncoderOptions();

    // 调整分组框标题样式
    encoderGroup_->setStyleSheet("QGroupBox { font-family: 'Microsoft YaHei', '微软雅黑'; font-size: 10pt; padding-top: 15px; margin-top: 15px; }");
}

void HomeWindow::initEncoderConnections()
{
    connect(hwAccelCheck_, &QCheckBox::toggled, this, &HomeWindow::onHWAccelToggled);
}

void HomeWindow::onHWAccelToggled(bool checked)
{
    updateEncoderOptions();
}

void HomeWindow::updateEncoderOptions()
{
    // 保存当前选择的编码器名称
    QString currentEncoder = encoderCombo_->currentData().toString();
    QString currentEncoderName = encoderCombo_->currentText();
    
    // 清空现有选项
    encoderCombo_->clear();

    // 如果是直播模式，只允许使用 H.264
    if (ui->liveRadioButton->isChecked()) {
        if (hwAccelCheck_->isChecked()) {
            // 硬件编码器 - 只用 H.264
            if (QSysInfo::windowsVersion() != QSysInfo::WV_None) {
                encoderCombo_->addItem("NVIDIA NVENC", "h264_nvenc");
                encoderCombo_->addItem("Intel QuickSync", "h264_qsv");
                encoderCombo_->addItem("AMD AMF", "h264_amf");
            } else {
                encoderCombo_->addItem("VAAPI", "h264_vaapi");
            }
        } else {
            encoderCombo_->addItem("H.264 (x264)", "libx264");
        }

        // 如果之前的编码器不是 H.264，显示提示
        if (!currentEncoder.isEmpty() && !currentEncoder.contains("264")) {
            QString tips = QString("直播模式下只支持 H.264 编码器，已从 %1 自动切换").arg(currentEncoderName);
            emit sig_showTips(Toast::INFO, tips);
        }

        // 如果是直播模式且分辨率大于1080p，自动降级
        QSize currentRes = resolutionCombo_->currentData().toSize();
        if (currentRes.height() > 1080) {
            // 自动切换到1080p
            int index = resolutionCombo_->findData(QSize(1920, 1080));
            if (index >= 0) {
                resolutionCombo_->setCurrentIndex(index);
                emit sig_showTips(Toast::INFO, "直播模式下已自动将分辨率降至1080p以确保稳定性");
            }
        }
    } else {
        // 录制模式，提供所有编码器选项
        if (hwAccelCheck_->isChecked()) {
            // 硬件编码器
            if (QSysInfo::windowsVersion() != QSysInfo::WV_None) {
                encoderCombo_->addItem("NVIDIA NVENC H.264", "h264_nvenc");
                encoderCombo_->addItem("NVIDIA NVENC HEVC", "hevc_nvenc");
                encoderCombo_->addItem("Intel QuickSync H.264", "h264_qsv");
                encoderCombo_->addItem("Intel QuickSync HEVC", "hevc_qsv");
                encoderCombo_->addItem("AMD AMF H.264", "h264_amf");
                encoderCombo_->addItem("AMD AMF HEVC", "hevc_amf");
            } else {
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
    }

    // 恢复之前选择的编码器（如果可用）
    if (!currentEncoder.isEmpty()) {
        int index = encoderCombo_->findData(currentEncoder);
        if (index >= 0) {
            encoderCombo_->setCurrentIndex(index);
        } else if (ui->liveRadioButton->isChecked()) {
            // 如果在直播模式下找不到之前的编码器，选择默认的 H.264
            encoderCombo_->setCurrentIndex(0);
        }
    }
}

QString HomeWindow::getSelectedEncoder() const
{
    return encoderCombo_->currentData().toString();
}

bool HomeWindow::isHWAccelEnabled() const
{
    return hwAccelCheck_->isChecked();
}
