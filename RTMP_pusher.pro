#-------------------------------------------------
#
# Project created by QtCreator 2023-05-15T16:00:48
#
#-------------------------------------------------

QT       += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = 0voice_pusher
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
RC_ICONS = live.ico
# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        homewindow.cpp \
    aacencoder.cpp \
    aacrtmppackager.cpp \
    audiocapturer.cpp \
    audioresampler.cpp \
    avpublishtime.cpp \
    commonlooper.cpp \
    h264encoder.cpp \
    looper.cpp \
    mediabase.cpp \
    naluloop.cpp \
    pushwork.cpp \
    rtmpbase.cpp \
    rtmppusher.cpp \
    videocapturer.cpp \
    librtmp/amf.c \
    librtmp/hashswf.c \
    librtmp/log.c \
    librtmp/parseurl.c \
    librtmp/rtmp.c \
    displaywind.cpp \
    globalhelper.cpp \
    toast.cpp \
    noise_suppression.c \
    recorder.cpp \
    audiooptimizerdialog.cpp \
    recordsettingdialog.cpp \
    rtmpurldialog.cpp \
    livesettingdialog.cpp \
    agc/analog_agc.c \
    agc/digital_agc.c \
    common_audio/delay_estimator.cc \
    common_audio/delay_estimator_wrapper.cc \
    common_audio/dot_product_with_scale.cc \
    common_audio/complex_bit_reverse.c \
    common_audio/complex_fft.c \
    common_audio/copy_set_operations.c \
    common_audio/division_operations.c \
    common_audio/energy.c \
    common_audio/fft4g.c \
    common_audio/get_scaling_square.c \
    common_audio/min_max_operations.c \
    common_audio/randomization_functions.c \
    common_audio/real_fft.c \
    common_audio/resample_by_2.c \
    common_audio/ring_buffer.c \
    common_audio/spl_sqrt.c \
    common_audio/spl_sqrt_floor.c \
    common_audio/sqrt_of_one_minus_x_squared.c \
    common_audio/vector_scaling_operations.c \
    volumeadjuster.cpp \
    log/easylogging++.cc

HEADERS += \
        homewindow.h \
    aacencoder.h \
    aacrtmppackager.h \
    audiocapturer.h \
    audioresampler.h \
    avpublishtime.h \
    avpublishtime.h \
    codecs.h \
    commonlooper.h \
    h264encoder.h \
    imagescaler.h \
    looper.h \
    mediabase.h \
    naluloop.h \
    pushwork.h \
    rtmpbase.h \
    rtmppackager.h \
    rtmppusher.h \
    semaphore.h \
    timeutil.h \
    videocapturer.h \
    librtmp/amf.h \
    librtmp/bytes.h \
    librtmp/dh.h \
    librtmp/dhgroups.h \
    librtmp/handshake.h \
    librtmp/http.h \
    librtmp/log.h \
    librtmp/rtmp.h \
    librtmp/rtmp_sys.h \
    ff_ffplay_def.h \
    displaywind.h \
    globalhelper.h \
    toast.h \
    noise_suppression.h \
    recorder.h \
    audiooptimizerdialog.h \
    recordsettingdialog.h \
    rtmpurldialog.h \
    livesettingdialog.h \
    agc/analog_agc.h \
    agc/digital_agc.h \
    common_audio/arch.h \
    common_audio/compile_assert.h \
    common_audio/compile_assert_c.h \
    common_audio/complex_fft_tables.h \
    common_audio/cpu_features_wrapper.h \
    common_audio/delay_estimator.h \
    common_audio/delay_estimator_internal.h \
    common_audio/delay_estimator_wrapper.h \
    common_audio/dot_product_with_scale.h \
    common_audio/fft4g.h \
    common_audio/real_fft.h \
    common_audio/ring_buffer.h \
    common_audio/safe_conversions.h \
    common_audio/safe_conversions_impl.h \
    common_audio/signal_processing_library.h \
    common_audio/spl_inl.h \
    common_audio/spl_inl_armv7.h \
    common_audio/spl_sqrt_floor.h \
    volumeadjuster.h \
    log/easylogging++.h

FORMS += \
        homewindow.ui \
    displaywind.ui \
    audiooptimizerdialog.ui \
    recordsettingdialog.ui \
    rtmpurldialog.ui \
    livesettingdialog.ui

win32 {
INCLUDEPATH += $$PWD/ffmpeg-4.2.1-win32-dev/include
INCLUDEPATH += $$PWD/SDL2/include
INCLUDEPATH += $$PWD/log
LIBS += $$PWD/ffmpeg-4.2.1-win32-dev/lib/avformat.lib   \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/avcodec.lib    \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/avdevice.lib   \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/avfilter.lib   \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/avutil.lib     \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/postproc.lib   \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/swresample.lib \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/swscale.lib    \
        $$PWD/SDL2/lib/x86/SDL2.lib \
        $$PWD/SDL2/lib/x86/Ole32.lib
#LIBS += libws2_32
LIBS += -lws2_32
#        "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x86/Ole32.Lib"
#LIBS += -lOle32
}
QMAKE_CFLAGS_ISYSTEM = -I

#Linux系统
unix {
INCLUDEPATH += $$PWD/log
INCLUDEPATH +=/usr/include
INCLUDEPATH += /home/lqf/ffmpeg6.0_build/include
LIBS += -L/home/lqf/ffmpeg6.0_build/lib -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc -lswscale -lswresample
INCLUDEPATH += /usr/include/
#LIBS += -L/usr/lib/x86_64-linux-gnu -lSDL2
LIBS += /usr/lib/x86_64-linux-gnu/libSDL2.so
}

DISTFILES += \
    res/qss/homewindow.css

RESOURCES += \
    res.qrc
