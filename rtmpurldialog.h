#ifndef RTMPURLDIALOG_H
#define RTMPURLDIALOG_H

#include <QDialog>

namespace Ui {
class RtmpUrlDialog;
}

class RtmpUrlDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RtmpUrlDialog(QWidget *parent = 0);
    ~RtmpUrlDialog();

private:
    Ui::RtmpUrlDialog *ui;
};

#endif // RTMPURLDIALOG_H
