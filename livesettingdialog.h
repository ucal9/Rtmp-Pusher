#ifndef LIVESETTINGDIALOG_H
#define LIVESETTINGDIALOG_H

#include <QDialog>
#include <QAbstractButton>

namespace Ui {
class LiveSettingDialog;
}

class LiveSettingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LiveSettingDialog(QWidget *parent = 0);
    ~LiveSettingDialog();
private slots:
    void on_buttonClicked(QAbstractButton *button);
private:
    Ui::LiveSettingDialog *ui;
};

#endif // LIVESETTINGDIALOG_H
