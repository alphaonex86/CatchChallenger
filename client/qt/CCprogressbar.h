#ifndef CCprogressbar_H
#define CCprogressbar_H

#include <QProgressBar>

class CCprogressbar : public QProgressBar
{
public:
    CCprogressbar(QWidget *parent = nullptr);
    void paintEvent(QPaintEvent *) override;
private:
    QPixmap backgroundLeft,backgroundMiddle,backgroundRight;
    QPixmap barLeft,barMiddle,barRight;
};

#endif // CCprogressbar_H
