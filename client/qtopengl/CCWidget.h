#ifndef CCWidget_H
#define CCWidget_H

#include <QGraphicsWidget>

class CCWidget : public QGraphicsWidget
{
public:
    CCWidget(QWidget *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    unsigned int currentBorderSize() const;
private:
    QPixmap tr,tm,tl,mr,mm,ml,br,bm,bl;
    int oldratio;
    unsigned int imageBorderSize() const;
};

#endif // CCWidget_H
