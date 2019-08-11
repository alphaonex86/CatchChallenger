#ifndef CCWidget_H
#define CCWidget_H

#include <QWidget>

class CCWidget : public QWidget
{
public:
    CCWidget(QWidget *parent = nullptr);
    void paintEvent(QPaintEvent *) override;
    unsigned int currentBorderSize() const;
private:
    QPixmap tr,tm,tl,mr,mm,ml,br,bm,bl;
    int oldratio;
    unsigned int imageBorderSize() const;
};

#endif // CCWidget_H
