#ifndef CCWidget_H
#define CCWidget_H

#include <QProgressBar>

class CCWidget : public QProgressBar
{
public:
    CCWidget(QWidget *parent = nullptr);
    void paintEvent(QPaintEvent *) override;
private:
    QPixmap tr,tm,tl,mr,mm,ml,br,bm,bl;
    int oldratio;
};

#endif // CCWidget_H
