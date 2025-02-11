#include "livesettingdialog.h"
#include "ui_livesettingdialog.h"

#include "globalhelper.h"
#include "easylogging++.h"
LiveSettingDialog::LiveSettingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LiveSettingDialog)
{
    ui->setupUi(this);
    //加载样式
    QString qss = GlobalHelper::GetQssStr("://res/qss/homewindow.css");
    setStyleSheet(qss);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("保存"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("取消"));
    //加载rtmp串流地址
    QString url =  ui->urlStreamLineEdit->text();
    GlobalHelper::GetDefaultRtmpUrl(url);
    ui->urlStreamLineEdit->setText(url);
    // 加载rtmp串流密匙
    QString key;
    GlobalHelper::GetDefaultRtmpKey(key);
    ui->urlKeyLineEdit->setText(key);
    connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(on_buttonClicked(QAbstractButton*)));
}

LiveSettingDialog::~LiveSettingDialog()
{
    delete ui;
}


void LiveSettingDialog::on_buttonClicked(QAbstractButton *button)
{
    if(button == (QAbstractButton *)(ui->buttonBox->button(QDialogButtonBox::Ok))) {
        // 保存参数
        LOG(INFO) << "(Button OK has been clicked.)";
        QString url = ui->urlStreamLineEdit->text();
        QString key = ui->urlKeyLineEdit->text();
        GlobalHelper::SaveDefaultRtmpUrl(url);
        GlobalHelper::SaveDefaultRtmpKey(key);
    } else if(button == (QAbstractButton *)(ui->buttonBox->button(QDialogButtonBox::Cancel))) {
        // 没有任何处理
        LOG(INFO) << "(Button Cancle has been clicked.)";
    }
}
