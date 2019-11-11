#ifndef CCWidget_H
#define CCWidget_H

#include <QGraphicsWidget>

class CCWidget : public QGraphicsItem
{
public:
    CCWidget(QGraphicsItem *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    unsigned int currentBorderSize() const;
    QRectF boundingRect() const override;
    void setWidth(const int &w);
    void setHeight(const int &h);
    int width() const;
    int height() const;
private:
    QPixmap tr,tm,tl,mr,mm,ml,br,bm,bl;
    int oldratio;
    unsigned int imageBorderSize() const;
    int m_w,m_h;
};

#endif // CCWidget_H
