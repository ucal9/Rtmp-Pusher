# 1 项目简介
零声录制推流教学软件，目前功能包括：

- 录制屏幕

- 录制麦克风 ，目前支持启动软件时自动检测

- rtmp推流同时保存到本地

- 支持纯录制的时候暂停/继续切换

- 录制保存到本地

- 支持降噪，目前支持16khz

- 支持修改视频保存目录

- 支持设置参数保存

- 自动获取屏幕分辨率，目前编码分辨率也是用的屏幕分辨率

- **支持Windows平台**
  - **使用dshow，屏幕录制需要安装**Screen Capturer Recorder https://github.com/rdp/screen-capture-recorder-to-video-windows-free/releases    ![](https://avmedia.0voice.com/zb_users/upload/2023/06/202306031451523640378.png)

    Setup.Screen.Capturer.Recorder.v0.13.2.exe已经传到工程目录下。

  - 对应的dll在dll目录。
  
- **支持Ubuntu平台（需要根据自己ffmpeg的库文件配置0voice_pusher.pro，我是用ffmpeg6.0的库测试的，4.2版本应该也没有问题(因为Win10也用的4.2版本)）：**
  - 麦克风采集使用alsa，直接写死了 hw:1，具体见AudioCapturer::Init函数

  - 屏幕录制使用x11grab，直接写死了录制整个屏幕VideoCapturer::Init

- 目前ui界面只适配了1920*1080的屏幕



**待完善功能**（优先级按顺序）：

- 支持mac

- 支持图片水印
- 支持系统声音
- 支持麦克风+系统声音叠加
- 支持摄像头
- 支持屏幕和摄像头叠加
- 支持本地文件推流直播
- 支持同时推送多路rtmp直播
- ui适配



# 2 操作说明

## 2.1 选择默认的麦克风

在主界面 左上角点击  设置 -> 录屏设置 -> 麦克风列表 选择默认的麦克风。

注意：如果没有麦克风则麦克风列表为NULL

![image-20230525202755302](https://avmedia.0voice.com/zb_users/upload/2023/05/202305252055105422816.png)

注意：目前麦克风列表只对 Windows平台生效，Ubuntu直接写死了hw:1的设备。



## 2.2 选择视频保存目录

在主界面 左上角点击  设置 -> 录屏设置 

![image-20230525203921358](https://avmedia.0voice.com/zb_users/upload/2023/05/202305252055282578296.png)



## 2.3 直播推流

![image-20230525204216819](https://avmedia.0voice.com/zb_users/upload/2023/05/202305252055408325346.png)



这个已经转到

设置->直播设置 里



## 2.4 单纯录制

参考直播推流界面，只是业务类型 选择录制。

录制保存视频的路径 参考 《2.2 选择视频保存目录》，文件名自动根据年月日 月 日 时分秒 生成。







# 3 部分功能实现原理

## 3.1 自动加载音频、视频设备列表

### 麦克风列表获取

```c
void RecordSettingDialog::scanAudioInputDeviceInfo(QList<QString> audio_input_list)
{
    audio_input_list.clear();
    QList<QAudioDeviceInfo> m_inputDevicesInfo = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for (int i = 0; i < m_inputDevicesInfo.size(); i++) {
        QString str = m_inputDevicesInfo[i].deviceName();
        audio_input_list.push_back(str);
        qDebug()<<"audio input device name:"<<str ;//麦克风
    }
}
```



### 摄像头列表获取（包括屏幕）

```c
void RecordSettingDialog::scanVideoInputDeviceInfo(QList<QString> video_input_list)
{
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    for (int i = 0; i < cameras.size(); i++) {
        qDebug("VideoDeviceManager::VideoDeviceManager: input device #%d: %s", i, qPrintable(cameras[i].deviceName()));
    }

    foreach (const QCameraInfo &cameraInfo, cameras) {
        QString str = cameraInfo.description();
        qDebug()<<"video input device name:"<<str ;//摄像头名字
        video_input_list.push_back(str);
    }
}
```



### 选择策略

- 每次开机扫码麦克风、摄像头列表 ；
- 如果保存的麦克风、摄像头名字有在扫码的列表里，则使用默认的麦克风、摄像头；
- 如果麦克风、摄像头不在扫码列表里，则使用扫描到的第一个设备作为输入。



进一步考虑：可以考虑添加拔插检测事件，这里大家自己去实现。



## 3.2 视频合成功能

在保存录制的音频+视频->合成mp4文件时，**需要注意av_interleaved_write_packet线程安全的问题**，需要加锁避免多线程同时调用av_interleaved_write_packet。

如果多个线程同时使用同一个AVFormatContext来执行`av_interleaved_write_packet`操作，则可能存在线程安全问题。



## 3.3 录制暂停功能

**注意：只有在纯录制的情况下允许暂停， 在rtmp直播状态下不允许暂停。**

时间戳考虑：音频采集、视频采集暂停和恢复都以音频为基准

- 在AVPublishTime类增加暂停接口

  - set_pause 为true的时候只是单纯设置pause_， 为false的时候要恢复时钟

  ```
  void set_pause(bool pause) {
      std::lock_guard<std::mutex> lock(mutex_);
      if(pause_ == pause) {
          return;
      }
      pause_ = pause;
      if(!pause_) {  // 不暂停则要处理
          offset_time_ = (int64_t)audio_pre_pts_; // 获取直接的audio pts值
          start_time_ = getCurrentTimeMsec(); // 重置起始时间
      }
  }
  ```

  - 音频采集、视频采集其时间戳在读取的时候 
    - 如果暂停返回-1
    - 如果非暂停状态则为：int64_t pts = getCurrentTimeMsec() - start_time_ + offset_time_;   这里的offset_time_实际是暂停前audio的pts值

- 在PushWork增加暂停接口

  - ```
    void SetPause(bool pause);  // 设置暂停
    bool GetPause();			// 获取暂停的状态
    ```

  - ```
    void PushWork::SetPause(bool pause)
    {
        if(pause_ == pause) {
            return;     // 操作不起作用
        }
        pause_ = pause;
        AVPublishTime::GetInstance()->set_pause(pause_);		// 实际是设置时钟
    }
    ```



## 3.4 直播设置

1. **设置RTMP推流的url和密匙**

livesettingdialog.xx文件

直播设置对话框只管修改 rtmp的url和密匙，主程序从保存的文件里读取对应的url和密匙

```
void LiveSettingDialog::on_buttonClicked(QAbstractButton *button)
{
    if(button == (QAbstractButton *)(ui->buttonBox->button(QDialogButtonBox::Ok)))
    {
        // 保存参数
        QString url = ui->urlStreamLineEdit->text();
        QString key = ui->urlKeyLineEdit->text();
        GlobalHelper::SaveDefaultRtmpUrl(url);
        GlobalHelper::SaveDefaultRtmpKey(key);
    }
    ......
}
```

2. **读取RTMP推流的url**

```
 void HomeWindow::on_startOrPauseBtn_clicked() {
        ......
   	//加载rtmp串流地址
    QString url;
    GlobalHelper::GetDefaultRtmpUrl(url);
    // 加载rtmp串流密匙
    QString key;
    GlobalHelper::GetDefaultRtmpKey(key);
    QString url_key =  url+key;
    start(url_key.toStdString());
		..........
 }

```



## 3.5 自动增益AGC



### 相关函数说明

WebRtcAgc_Create

WebRtcAgc_Free

WebRtcAgc_config_t gain_config;

 gain_config.targetLevelDbfs = 1;
    gain_config.compressionGaindB = 20;
    gain_config.limiterEnable = kAgcTrue;

WebRtcAgc_Init

WebRtcAgc_set_config



#### WebRtcAgc_AddFarend

```
/*
 * This function processes a 10 ms frame of far-end speech to determine
 * if there is active speech. The length of the input speech vector must be
 * given in samples (80 when FS=8000, and 160 when FS=16000, FS=32000 or
 * FS=48000).
 *
 * Input:
 *      - agcInst           : AGC instance.
 *      - inFar             : Far-end input speech vector
 *      - samples           : Number of samples in input vector
 *
 * Return value:
 *                          :  0 - Normal operation.
 *                          : -1 - Error
 */
```



#### WebRtcAgc_VirtualMic

```
/*
 * This function replaces the analog microphone with a virtual one.
 * It is a digital gain applied to the input signal and is used in the
 * agcAdaptiveDigital mode where no microphone level is adjustable. The length
 * of the input speech vector must be given in samples (80 when FS=8000, and 160
 * when FS=16000, FS=32000 or FS=48000).
 *
 * Input:
 *      - agcInst           : AGC instance.
 *      - inMic             : Microphone input speech vector for each band
 *      - num_bands         : Number of bands in input vector
 *      - samples           : Number of samples in input vector
 *      - micLevelIn        : Input level of microphone (static)
 *
 * Output:
 *      - inMic             : Microphone output after processing (L band)
 *      - inMic_H           : Microphone output after processing (H band)
 *      - micLevelOut       : Adjusted microphone level after processing
 *
 * Return value:
 *                          :  0 - Normal operation.
 *                          : -1 - Error
 */
int WebRtcAgc_VirtualMic(void *agcInst,
                         int16_t *const *inMic,
                         size_t num_bands,
                         size_t samples,
                         int32_t micLevelIn,
                         int32_t *micLevelOut);
```



#### WebRtcAgc_Process

```
/*
 * This function processes a 10 ms frame and adjusts (normalizes) the gain both
 * analog and digitally. The gain adjustments are done only during active
 * periods of speech. The length of the speech vectors must be given in samples
 * (80 when FS=8000, and 160 when FS=16000, FS=32000 or FS=48000). The echo
 * parameter can be used to ensure the AGC will not adjust upward in the
 * presence of echo.
 *
 * This function should be called after processing the near-end microphone
 * signal, in any case after any echo cancellation.
 *
 * Input:
 *      - agcInst           : AGC instance
 *      - inNear            : Near-end input speech vector for each band
 *      - num_bands         : Number of bands in input/output vector
 *      - samples           : Number of samples in input/output vector
 *      - inMicLevel        : Current microphone volume level
 *      - echo              : Set to 0 if the signal passed to add_mic is
 *                            almost certainly free of echo; otherwise set
 *                            to 1. If you have no information regarding echo
 *                            set to 0.
 *
 * Output:
 *      - outMicLevel       : Adjusted microphone volume level
 *      - out               : Gain-adjusted near-end speech vector
 *                          : May be the same vector as the input.
 *      - saturationWarning : A returned value of 1 indicates a saturation event
 *                            has occurred and the volume cannot be further
 *                            reduced. Otherwise will be set to 0.
 *
 * Return value:
 *                          :  0 - Normal operation.
 *                          : -1 - Error
 */
int WebRtcAgc_Process(void *agcInst,
                      const int16_t *const *inNear,
                      size_t num_bands,
                      size_t samples,
                      int16_t *const *out,
                      int32_t inMicLevel,
                      int32_t *outMicLevel,
                      int16_t echo,
                      uint8_t *saturationWarning);
```



### 参考扩展

[详解 WebRTC 高音质低延时的背后 — AGC（自动增益控制）]: https://developer.aliyun.com/article/784304#



# 4 说明

- 本项目主要应用于教育，重点在于功能的实现，大家在重写代码的时候可以根据自己的风格和习惯进行重构。
- 对于ui界面，主要也是为了配合功能，所以ui界面没有花太多时间做适配。
- 这个项目长期迭代，随着功能的增加代码也会越来越复杂，**对于项目不理解的内容大家多找老师沟通。**



# 5 参考

- 宽容是对人的一种尊重，严于律己是对自己的一份负责。

- 现在网上抡键盘一顿输出是最容易的，所以不要和键盘侠理论。

- [Astyle - Qt中配置代码格式化工具（附：最佳教科书格式） - Citrusliu - 博客园 (cnblogs.com)](https://www.cnblogs.com/citrus/p/15122820.html)

- [【QT】史上最全最详细的QSS样式表用法及用例说明_qt qss_半醒半醉日复日，花落花开年复年的博客-CSDN博客](https://blog.csdn.net/WL0616/article/details/129118087)

- 降噪算法：[WebRtc_noise_suppression](https://github.com/jagger2048/WebRtc_noise_suppression/blob/master/readme_cn.md)

- [单独编译和使用webrtc音频回声消除模块(附完整源码+测试音频文件) - 繁星jemini - 博客园 (cnblogs.com)](https://www.cnblogs.com/mod109/p/5827918.html)

- [单独编译和使用webrtc音频降噪模块(附完整源码+测试音频文件) - 繁星jemini - 博客园 (cnblogs.com)](https://www.cnblogs.com/mod109/p/5469799.html)
- [Ubuntu下Qt创建窗口菜单栏menubar隐藏？ - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/92837638)
- 降噪和自动增益源码原理参考：
  - [Android集成webrtc降噪和增益模块, ns_core函数简析_android webrtcns_process_soso密斯密斯的博客-CSDN博客](https://blog.csdn.net/qq_38366777/article/details/107877262)
  - [详解 WebRTC 高音质低延时的背后 — AGC（自动增益控制）-阿里云开发者社区 (aliyun.com)](https://developer.aliyun.com/article/784304)

