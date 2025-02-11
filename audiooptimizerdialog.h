#ifndef AUDIOOPTIMIZERDIALOG_H
#define AUDIOOPTIMIZERDIALOG_H

#include <QDialog>
#include <QSizePolicy>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>

namespace Ui {
class AudioOptimizerDialog;
}

class AudioOptimizerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AudioOptimizerDialog(QWidget *parent = 0);
    ~AudioOptimizerDialog();

private slots:
    void on_noiseComboBox_currentIndexChanged(int index);

    void on_agcComboBox_currentIndexChanged(int index);

private:
    void initUi();
    
    Ui::AudioOptimizerDialog *ui;

    int audio_ns_level_ = -1;
    int audio_agc_level_ = 0;
};

#endif // AUDIOOPTIMIZERDIALOG_H
