#include "toast.h"

#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QEvent>

class ToastDlg: public QDialog
{
private:
    QLabel* mLabel;
    QLabel* mCloseBtn;
protected:
    bool eventFilter(QObject *obj, QEvent *ev) override
    {
        if (obj == mCloseBtn)
        {
            if (ev->type() == QEvent::MouseButtonRelease)
            {
                accept();
            }
        }
        return QObject::eventFilter(obj, ev);
    }
public:
    ToastDlg()
    {
        auto layout = new QHBoxLayout;//水平布局

        mLabel = new QLabel;
        mLabel->setStyleSheet("color: white; background:transparent");//red
        layout->addWidget(mLabel, 1);//stretch = 1

        mCloseBtn = new QLabel;
        //mCloseBtn->setPixmap(QPixmap(":/res/img/close.png"));
        mCloseBtn->installEventFilter(this);
        mCloseBtn->setStyleSheet("background:transparent");
        layout->addWidget(mCloseBtn);

        setLayout(layout);
        setWindowFlag(Qt::FramelessWindowHint);//生成一个无边界窗口。用户不能通过窗口系统移动或调整无边界窗口的大小。
        setAttribute(Qt::WA_ShowWithoutActivating,true);//Show the widget without making it active.
        //setAttribute(Qt::WA_TranslucentBackground, true);   // 背景透明
    }

    void show(Toast::Level level, const QString& text)
    {
        QPalette p = palette();
        //QColor(int r, int g, int b, int a = 255)
        //Constructs a color with the RGB value r, g, b, and the alpha-channel (transparency) value of a.
        p.setColor(QPalette::Window, QColor(0,0,0,200));
        if (level == Toast::INFO)
        {
            p.setColor(QPalette::Window, QColor(0,0,0,200));//黑色
        }
        else if (level == Toast::WARN)
        {
            p.setColor(QPalette::Window, QColor(0,0,255,200));
        }
        else//ERROR
        {
            p.setColor(QPalette::Window, QColor(255,0,0,200));
        }
        setPalette(p);//set widget's palette

        mLabel->setText(text);
        setWindowFlag(Qt::WindowStaysOnTopHint);//通知窗口系统该窗口应保持在所有其他窗口的顶部。
        QDialog::show();
    }
};//~class ToastDlg end


Toast::Toast()//构造函数
{
    dlg_ = new ToastDlg;
}

//返回一个实例(instance)
Toast &Toast::instance()
{
    static Toast thiz;//这种实例化方法会自动回收内存
    return thiz;
}

void Toast::show(Toast::Level level, const QString &text)
{
    dlg_->show(level, text);//ToastDlg.show方法
    if (timer_id_ != 0)//int mTimerId
    {
        //如果之前已经开启了一个定时器，先把他关掉
        killTimer(timer_id_);
    }
    timer_id_ = startTimer(2000);//启动定时器，每2s触发定时器事件，直到调用killTimer
}

//重写定时器事件回调函数
void Toast::timerEvent(QTimerEvent *event)
{
    killTimer(timer_id_);
    timer_id_ = 0;
    dlg_->accept();//隐藏模态对话框
    //mDlg->hide();
}
