/*
* copyright(c) 部分源码引用第三方代码,第三方代码版权贵原作者所有,零声教育自研代码版权归湖南零声信息科技所有
* @file 	globalhelper.h
* @date 	2023-06-02 22:54
*
* @author 	itisyang
* @Contact	itisyang@gmail.com
*
* @modifier  Darren.Liao
* @Contact	326873713@qq.com
*
* @brief 	公共类，包括样式文件加载、参数保存和读取
* @note
*/

#ifndef GLOBALHELPER_H
#define GLOBALHELPER_H

#pragma execution_character_set("utf-8")

enum ERROR_CODE
{
    NoError = 0,
    ErrorFileInvalid
};



#include <QString>
#include <QPushButton>
#include <QDebug>
#include <QStringList>

class GlobalHelper      // 工具类
{
public:
    GlobalHelper();
	/**
	 * 获取样式表
	 * 
	 * @param	strQssPath 样式表文件路径
	 * @return	样式表
	 * @note 	
	 */
    static QString GetQssStr(QString strQssPath);

	/**
	 * 为按钮设置显示图标
	 * 
	 * @param	btn 按钮指针
	 * @param	iconSize 图标大小
	 * @param	icon 图标字符
	 */
    static void SetIcon(QPushButton* btn, int iconSize, QChar icon);


    static void SavePlaylist(QStringList& playList);    // 保存播放列表
    static void GetPlaylist(QStringList& playList);     // 获取播放列表
    static void SavePlayVolume(double& nVolume);        // 保存音量
    static void GetPlayVolume(double& nVolume);         // 获取音量


    // NS（Noise suppression） 设置降噪级别 -1代表无
    static void SaveAudioNs(int value);
    static void GetAudioNs(int &value);

    // 增强级别 0 ~200%, 保存的时候保存整数0~200
    static void SaveAudioAgc(int value);
    static void GetAudioAgc(int &value);

    // 视频保存目录
    static void SaveFileDir(QString &value);
    static void GetFileDir(QString &value);


    // 推流是否同时录制
    static void SaveLiveRecord(int value);
    static void GetLiveRecord(int &value);

    // 业务类型 0直播 1录制到本地 2推送本地文件进行直播
    static void SaveBusinessesType(int value);
    static void GetBusinessesType(int &value);


    // 保存缺省的麦克风设备
    static void SaveDefaultMicDevice(QString &value);
    static void GetDefaultMicDevice(QString &value);


    // 保存缺省的直播rtmp url
    static void SaveDefaultRtmpUrl(QString &value);
    static void GetDefaultRtmpUrl(QString &value);
    // 保存缺省的直播rtmp url的key
    static void SaveDefaultRtmpKey(QString &value);
    static void GetDefaultRtmpKey(QString &value);

    static QString GetAppVersion();
};

//必须加以下内容,否则编译不能通过,为了兼容C和C99标准
#ifndef INT64_C
#define INT64_C
#define UINT64_C
#endif

extern "C"{
#include "libavutil/avstring.h"
#include "libavutil/eval.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"
#include "libavutil/dict.h"
#include "libavutil/parseutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/avassert.h"
#include "libavutil/time.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "libavcodec/avfft.h"
#include "libswresample/swresample.h"
//#include "SDL.h"
}




#define MAX_SLIDER_VALUE 65536



#endif // GLOBALHELPER_H
