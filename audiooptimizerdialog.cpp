#include "audiooptimizerdialog.h"
#include "ui_audiooptimizerdialog.h"
#include "globalhelper.h"



int getAgcIndex(int agc_level)
{
    switch (agc_level) {
    case 0:
        return 0;
    case 25:
        return 1;
    case 50:
        return 2;
    case 75:
        return 3;
    case 100:
        return 4;
    case 125:
        return 5;
    case 150:
        return 6;
    case 175:
        return 7;
    case 200:
        return 8;
    default:
        return 0;
    }
    return 0;
}

AudioOptimizerDialog::AudioOptimizerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AudioOptimizerDialog)
{
    ui->setupUi(this);
    // 去掉问号，只保留关闭
    setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);


    //加载样式
    QString qss = GlobalHelper::GetQssStr("://res/qss/homewindow.css");
    setStyleSheet(qss);

    GlobalHelper::GetAudioNs(audio_ns_level_);
    ui->noiseComboBox->setCurrentIndex(audio_ns_level_+1);

    GlobalHelper::GetAudioAgc(audio_agc_level_);
    ui->agcComboBox->setCurrentIndex(getAgcIndex(audio_agc_level_));
}

AudioOptimizerDialog::~AudioOptimizerDialog()
{
    delete ui;
}

void AudioOptimizerDialog::on_noiseComboBox_currentIndexChanged(int index)
{
    switch (index) {
    case 0:
        audio_ns_level_ = -1;   // 关闭
        break;
    case 1:
        audio_ns_level_ = 0;    // 低
        break;
    case 2:
        audio_ns_level_ = 1;  //中
        break;
    case 3:
        audio_ns_level_ = 2; // 高
        break;
    case 4:
        audio_ns_level_ = 3; // 超高
        break;
    default:
        audio_ns_level_ = -1;
        break;
    }
    GlobalHelper::SaveAudioNs(audio_ns_level_);
}

void AudioOptimizerDialog::on_agcComboBox_currentIndexChanged(int index)
{
    switch (index) {
    case 0:
        audio_agc_level_ = 0;
        break;
    case 1:
        audio_agc_level_ = 25;
        break;
    case 2:
        audio_agc_level_ = 50;
        break;
    case 3:
        audio_agc_level_ = 75;
        break;
    case 4:
        audio_agc_level_ = 100;
        break;
    case 5:
        audio_agc_level_ = 125;
        break;
    case 6:
        audio_agc_level_ = 150;
        break;
    case 7:
        audio_agc_level_ = 175;
        break;
    case 8:
        audio_agc_level_ = 200;
        break;
    default:
        audio_agc_level_ = 0;
        break;
    }
    GlobalHelper::SaveAudioAgc(audio_agc_level_);
}
