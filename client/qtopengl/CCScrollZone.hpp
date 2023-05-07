#ifndef CCSCROLLZONE_H
#define CCSCROLLZONE_H

#include <QGraphicsObject>

class CCScrollZone : public QGraphicsObject
{
public:
    CCScrollZone(QGraphicsItem *parent=nullptr);
    ~CCScrollZone();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QRectF boundingRect() const override;
    void setWidth(const int &w);
    void setHeight(const int &h);
    void setSize(const int &w,const int &h);
    int width() const;
    int height() const;
private:
    QRectF m_boundingRect;
    QPixmap *cache;
    int lastwidth,lastheight;
};

#endif // CCSCROLLZONE_H
