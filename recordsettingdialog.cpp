#include "recordsettingdialog.h"
#include "ui_recordsettingdialog.h"
#include <QFileDialog>
#include <QDesktopServices>
#include <QDebug>
#include <QAudioDeviceInfo>
#include <QCameraInfo>
#include <QMessageBox>
#include <QFontDialog>
#include "globalhelper.h"
#include "easylogging++.h"

RecordSettingDialog::RecordSettingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RecordSettingDialog)
{
    ui->setupUi(this);
    // 去掉问号，只保留关闭
    setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);
    //加载样式
    QString qss = GlobalHelper::GetQssStr("://res/qss/homewindow.css");
    setStyleSheet(qss);
    GlobalHelper::GetFileDir(file_dir_path_);
    ui->dirLineEdit->setText(file_dir_path_);
    QList<QString> audio_input_list;
    scanAudioInputDeviceInfo(audio_input_list);
    int audio_mic_default_index = -1;
    QString audio_mic_default_name;
    GlobalHelper::GetDefaultMicDevice(audio_mic_default_name);
    for(int i = 0; i < audio_input_list.size(); i++) {
        ui->micComboBox->addItem(audio_input_list[i]);
        if(audio_mic_default_name == audio_input_list[i]) {
            audio_mic_default_index = i;    // 获取默认mic设备名的位置
        }
    }
    if(audio_mic_default_index != -1) {
        ui->micComboBox->setCurrentIndex(audio_mic_default_index);    // 选择默认位置mic
    } else if(audio_input_list.size() > 0) {
        ui->micComboBox->setCurrentIndex(0);    // 扫描列表里的设备名和默认保存的不一致则使用扫描的第一个设备
        GlobalHelper::SaveDefaultMicDevice(audio_input_list[0]);
    }
    QList<QString> video_input_list;
    scanVideoInputDeviceInfo(video_input_list);
    QString audio_default_input;
    GetAudioDefaultMicDeviceName(audio_default_input);

    // 连接信号槽
    connect(ui->selectImageBtn, &QPushButton::clicked, this, &RecordSettingDialog::onSelectImageClicked);
    connect(ui->fontSettingBtn, &QPushButton::clicked, this, &RecordSettingDialog::onFontSettingClicked);
}

RecordSettingDialog::~RecordSettingDialog()
{
    delete ui;
}

void RecordSettingDialog::scanAudioInputDeviceInfo(QList<QString> &audio_input_list)
{
    audio_input_list.clear();
    QList<QAudioDeviceInfo> m_inputDevicesInfo = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for (int i = 0; i < m_inputDevicesInfo.size(); i++) {
        QString str = m_inputDevicesInfo[i].deviceName();
        audio_input_list.push_back(str);
        LOG(INFO) << "audio input device name:" << str.toStdString() ; //麦克风
    }
}

QString RecordSettingDialog::GetAudioDefaultMicDeviceName(QString &expect_name)
{
    QList<QString> audio_input_list;
    scanAudioInputDeviceInfo(audio_input_list);
    if(audio_input_list.size() == 0) {
        return "";      // 返回空的设备名
    }
    int i = 0;
    for(i = 0; i < audio_input_list.size(); i++) {
        if(expect_name == audio_input_list[i]) {
            break;
        }
    }
    if(i < audio_input_list.size()) {       // 说明是读取到期望的缺省名字
        return expect_name;
    }
    GlobalHelper::SaveDefaultMicDevice(audio_input_list[0]);    // 将设备列表第一个设置为默认
    return audio_input_list[0];
}

void RecordSettingDialog::scanVideoInputDeviceInfo(QList<QString> &video_input_list)
{
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    for (int i = 0; i < cameras.size(); i++) {
        LOG(INFO) << "VideoDeviceManager::VideoDeviceManager: input device " <<  i << ":" <<  cameras[i].deviceName().toStdString();
    }
    foreach (const QCameraInfo &cameraInfo, cameras) {
        QString str = cameraInfo.description();
        LOG(INFO) << "video input device name:" << str.toStdString() ; //摄像头名字
        video_input_list.push_back(str);
    }
}

void RecordSettingDialog::on_openDirectory_clicked()
{
    QString path = QString("file:///") + file_dir_path_;
    bool is_open = QDesktopServices::openUrl(QUrl(path, QUrl::TolerantMode));
    LOG(INFO) << "file_dir_path_ open? -> " << is_open;
}

void RecordSettingDialog::on_modifyDirctory_clicked()
{
    //打开对应的目录
    QString dir = QFileDialog::getExistingDirectory(nullptr, "打开视频目录", file_dir_path_,
                  QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    LOG(INFO) << file_dir_path_.toStdString() <<  " change to " << dir.toStdString();
    file_dir_path_ = dir;
    GlobalHelper::SaveFileDir(file_dir_path_);
}

void RecordSettingDialog::on_micComboBox_currentIndexChanged(const QString &arg1)
{
    LOG(INFO) << "on_micComboBox_currentIndexChanged:" << arg1.toStdString() ; //麦克风
    QString mic_name = arg1;
    GlobalHelper::SaveDefaultMicDevice(mic_name);
}

void RecordSettingDialog::on_comboBox_4_currentIndexChanged(int index)
{
    // 获取当前选择的分辨率
    QString resolution = ui->comboBox_4->currentText();
    
    // 如果是直播模式且选择了4K/2K分辨率，显示警告并自动降级到1080p
    int businessType;
    GlobalHelper::GetBusinessesType(businessType);
    if(businessType == 0) { // 直播模式
        if(resolution.contains("4K") || resolution.contains("2K")) {
            QMessageBox::warning(this, "提示", 
                "直播模式下不建议使用4K/2K分辨率，已自动调整为1080p。\n"
                "原因：高分辨率直播需要很大的网络带宽，可能导致直播卡顿。");
            
            // 自动切换到1080p
            for(int i = 0; i < ui->comboBox_4->count(); i++) {
                if(ui->comboBox_4->itemText(i).contains("1920x1080")) {
                    ui->comboBox_4->setCurrentIndex(i);
                    break;
                }
            }
        }
    }

    // 根据分辨率自动调整推荐码率
    int recommendedBitrate = 0;
    if(resolution.contains("3840x2160")) { // 4K
        recommendedBitrate = 35000; // 35Mbps
    }
    else if(resolution.contains("2560x1440")) { // 2K
        recommendedBitrate = 20000; // 20Mbps  
    }
    else if(resolution.contains("1920x1080")) { // 1080p
        recommendedBitrate = 8000;  // 8Mbps
    }
    else if(resolution.contains("1280x720")) { // 720p
        recommendedBitrate = 5000;  // 5Mbps
    }
    else {
        recommendedBitrate = 2500;  // 2.5Mbps
    }

    // 更新码率设置
    for(int i = 0; i < ui->comboBox_2->count(); i++) {
        QString text = ui->comboBox_2->itemText(i);
        if(text.contains(QString::number(recommendedBitrate))) {
            ui->comboBox_2->setCurrentIndex(i);
            break;
        }
    }
}
void RecordSettingDialog::onSelectImageClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("选择水印图片"), "",
        tr("图片文件 (*.png *.jpg *.bmp)"));
        
    if (!fileName.isEmpty()) {
        ui->imageWatermarkPath->setText(fileName);
    }
}

void RecordSettingDialog::onFontSettingClicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, QFont("Arial", 12), this, tr("选择字体"));
    if (ok) {
        watermarkFont_ = font;
    }
}

