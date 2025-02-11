#ifndef RECORDSETTINGDIALOG_H
#define RECORDSETTINGDIALOG_H

#include <QDialog>
#include <QMessageBox>

namespace Ui {
class RecordSettingDialog;
}

class RecordSettingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RecordSettingDialog(QWidget *parent = 0);
    ~RecordSettingDialog();

    static void scanAudioInputDeviceInfo(QList<QString> &audio_input_list);
    //
    static QString GetAudioDefaultMicDeviceName(QString &expect_name);
    static void scanVideoInputDeviceInfo(QList<QString> &video_input_list);

private slots:
    void on_openDirectory_clicked();

    void on_modifyDirctory_clicked();

    void on_micComboBox_currentIndexChanged(const QString &arg1);

    void on_comboBox_4_currentIndexChanged(int index);

    void onSelectImageClicked();
    void onFontSettingClicked();

private:
    Ui::RecordSettingDialog *ui;

    QString file_dir_path_;
    QFont watermarkFont_;
};

#endif // RECORDSETTINGDIALOG_H
