#include "rtmpurldialog.h"
#include "ui_rtmpurldialog.h"

RtmpUrlDialog::RtmpUrlDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RtmpUrlDialog)
{
    ui->setupUi(this);
}

RtmpUrlDialog::~RtmpUrlDialog()
{
    delete ui;
}
