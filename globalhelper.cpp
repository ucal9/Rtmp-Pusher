#include <QFile>
#include <QDebug>
#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QStandardPaths>
#include "globalhelper.h"

// 这里的日志暂时保留qdebug，一般不会有逻辑错误，一般都是不够细心写错变量
// 保存路径
const QString PUSHER_CONFIG_BASEDIR = QDir::tempPath();

const QString PUSHER_CONFIG = "0voice_pusher_config.ini";

const QString APP_VERSION = "0.1.0";

GlobalHelper::GlobalHelper()
{
}

QString GlobalHelper::GetQssStr(QString strQssPath)
{
    QString strQss;
    QFile FileQss(strQssPath);
    if (FileQss.open(QIODevice::ReadOnly)) {
        strQss = FileQss.readAll();
        FileQss.close();
    } else {
        qDebug() << "读取样式表失败" << strQssPath;
    }
    return strQss;
}

void GlobalHelper::SetIcon(QPushButton* btn, int iconSize, QChar icon)
{
    QFont font;
    font.setFamily("FontAwesome");
    font.setPointSize(iconSize);
    btn->setFont(font);
    btn->setText(icon);
}

void GlobalHelper::SavePlaylist(QStringList& playList)
{
    //QString str_pusher_config_filename = QCoreApplication::applicationDirPath() + QDir::separator() + PLAYER_CONFIG;
    QString str_pusher_config_filename = PUSHER_CONFIG_BASEDIR + QDir::separator() + PUSHER_CONFIG;
    QSettings settings(str_pusher_config_filename, QSettings::IniFormat);
    settings.beginWriteArray("playlist");
    for (int i = 0; i < playList.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("movie", playList.at(i));
    }
    settings.endArray();
}

void GlobalHelper::GetPlaylist(QStringList& playList)
{
    //QString str_pusher_config_filename = QCoreApplication::applicationDirPath() + QDir::separator() + PLAYER_CONFIG;
    QString str_pusher_config_filename = PUSHER_CONFIG_BASEDIR + QDir::separator() + PUSHER_CONFIG;
    QSettings settings(str_pusher_config_filename, QSettings::IniFormat);
    int size = settings.beginReadArray("playlist");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        playList.append(settings.value("movie").toString());
    }
    settings.endArray();
}

void GlobalHelper::SavePlayVolume(double& nVolume)
{
    QString str_pusher_config_filename = PUSHER_CONFIG_BASEDIR + QDir::separator() + PUSHER_CONFIG;
    QSettings settings(str_pusher_config_filename, QSettings::IniFormat);
    settings.setValue("volume/size", nVolume);
}

void GlobalHelper::GetPlayVolume(double& nVolume)
{
    QString str_pusher_config_filename = PUSHER_CONFIG_BASEDIR + QDir::separator() + PUSHER_CONFIG;
    QSettings settings(str_pusher_config_filename, QSettings::IniFormat);
    QString str = settings.value("volume/size").toString();
    nVolume = settings.value("volume/size", nVolume).toDouble();
}

void GlobalHelper::SaveAudioNs(int value)
{
    QString str_pusher_config_filename = PUSHER_CONFIG_BASEDIR + QDir::separator() + PUSHER_CONFIG;
    QSettings settings(str_pusher_config_filename, QSettings::IniFormat);
    settings.setValue("audio/ns", value);
    qDebug() << "SaveAudioNs: " << value;
}

void GlobalHelper::GetAudioNs(int &value)
{
    QString str_pusher_config_filename = PUSHER_CONFIG_BASEDIR + QDir::separator() + PUSHER_CONFIG;
    QSettings settings(str_pusher_config_filename, QSettings::IniFormat);
    value = settings.value("audio/ns", -1).toInt(); // -1即是没有开启
    qDebug() << "GetAudioNs: " << value;
}

void GlobalHelper::SaveAudioAgc(int value)
{
    QString str_pusher_config_filename = PUSHER_CONFIG_BASEDIR + QDir::separator() + PUSHER_CONFIG;
    QSettings settings(str_pusher_config_filename, QSettings::IniFormat);
    settings.setValue("audio/agc", value);
    qDebug() << "SaveAudioAgc: " << value;
}

void GlobalHelper::GetAudioAgc(int &value)
{
    QString str_pusher_config_filename = PUSHER_CONFIG_BASEDIR + QDir::separator() + PUSHER_CONFIG;
    QSettings settings(str_pusher_config_filename, QSettings::IniFormat);
    value = settings.value("audio/agc", -1).toInt();    // -1即是没有开启
    qDebug() << "GetAudioNs: " << value;
}

void GlobalHelper::SaveFileDir(QString &value)
{
    QString str_pusher_config_filename = PUSHER_CONFIG_BASEDIR + QDir::separator() + PUSHER_CONFIG;
    QSettings settings(str_pusher_config_filename, QSettings::IniFormat);
    settings.setValue("directory/file", value);
    qDebug() << "SaveFileDir: " << value;
}

void GlobalHelper::GetFileDir(QString &value)
{
    QString str_pusher_config_filename = PUSHER_CONFIG_BASEDIR + QDir::separator() + PUSHER_CONFIG;
    QSettings settings(str_pusher_config_filename, QSettings::IniFormat);
    QString default_dir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    value = settings.value("directory/file", default_dir).toString();    //
    qDebug() << "GetFileDir: " << value;
}

void GlobalHelper::SaveLiveRecord(int value)
{
    QString str_pusher_config_filename = PUSHER_CONFIG_BASEDIR + QDir::separator() + PUSHER_CONFIG;
    QSettings settings(str_pusher_config_filename, QSettings::IniFormat);
    settings.setValue("business/liverec", value);
    qDebug() << "SaveLiveRecord: " << value;
}

void GlobalHelper::GetLiveRecord(int &value)
{
    QString str_pusher_config_filename = PUSHER_CONFIG_BASEDIR + QDir::separator() + PUSHER_CONFIG;
    QSettings settings(str_pusher_config_filename, QSettings::IniFormat);
    value = settings.value("business/liverec", 0).toInt();    // 0默认推流不录制
    qDebug() << "GetLiveRecord: " << value;
}

void GlobalHelper::SaveBusinessesType(int value)
{
    QString str_pusher_config_filename = PUSHER_CONFIG_BASEDIR + QDir::separator() + PUSHER_CONFIG;
    QSettings settings(str_pusher_config_filename, QSettings::IniFormat);
    settings.setValue("business/type", value);
    qDebug() << "SaveBusinessesType: " << value;
}

void GlobalHelper::GetBusinessesType(int &value)
{
    QString str_pusher_config_filename = PUSHER_CONFIG_BASEDIR + QDir::separator() + PUSHER_CONFIG;
    QSettings settings(str_pusher_config_filename, QSettings::IniFormat);
    value = settings.value("business/type", 0).toInt();    // 0默认推流不录制
    qDebug() << "GetBusinessesType: " << value;
}

void GlobalHelper::SaveDefaultMicDevice(QString &value)
{
    QString str_pusher_config_filename = PUSHER_CONFIG_BASEDIR + QDir::separator() + PUSHER_CONFIG;
    QSettings settings(str_pusher_config_filename, QSettings::IniFormat);
    settings.setValue("devices/mic", value);
    qDebug() << "SaveDefaultMicDevice: " << value;
}

void GlobalHelper::GetDefaultMicDevice(QString &value)
{
    QString str_pusher_config_filename = PUSHER_CONFIG_BASEDIR + QDir::separator() + PUSHER_CONFIG;
    QSettings settings(str_pusher_config_filename, QSettings::IniFormat);
    value = settings.value("devices/mic", "").toString();    //
    qDebug() << "GetDefaultMicDevice: " << value;
}


// 保存缺省的直播rtmp url
void GlobalHelper::SaveDefaultRtmpUrl(QString &value)
{
    QString str_pusher_config_filename = PUSHER_CONFIG_BASEDIR + QDir::separator() + PUSHER_CONFIG;
    QSettings settings(str_pusher_config_filename, QSettings::IniFormat);
    settings.setValue("live/rtmpurl", value);
    qDebug() << "SaveDefaultMicDevice: " << value;
}
void GlobalHelper::GetDefaultRtmpUrl(QString &value)
{
    QString str_pusher_config_filename = PUSHER_CONFIG_BASEDIR + QDir::separator() + PUSHER_CONFIG;
    QSettings settings(str_pusher_config_filename, QSettings::IniFormat);
    value = settings.value("live/rtmpurl", "").toString();    //
    qDebug() << "GetDefaultRtmpUrl: " << value;
}
// 保存缺省的直播rtmp url的key
void GlobalHelper::SaveDefaultRtmpKey(QString &value)
{
    QString str_pusher_config_filename = PUSHER_CONFIG_BASEDIR + QDir::separator() + PUSHER_CONFIG;
    QSettings settings(str_pusher_config_filename, QSettings::IniFormat);
    settings.setValue("live/rtmpkey", value);
    qDebug() << "SaveDefaultRtmpKey: " << value;
}
void GlobalHelper::GetDefaultRtmpKey(QString &value)
{
    QString str_pusher_config_filename = PUSHER_CONFIG_BASEDIR + QDir::separator() + PUSHER_CONFIG;
    QSettings settings(str_pusher_config_filename, QSettings::IniFormat);
    value = settings.value("live/rtmpkey", "").toString();    //
    qDebug() << "GetDefaultRtmpKey: " << value;
}

QString GlobalHelper::GetAppVersion()
{
    return APP_VERSION;
}

