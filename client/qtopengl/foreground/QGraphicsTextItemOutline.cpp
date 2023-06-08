#include "QGraphicsTextItemOutline.hpp"
#include <QPainter>
#include <QPen>

QGraphicsTextItemOutline::QGraphicsTextItemOutline(QGraphicsItem *parent) :
    QGraphicsTextItem(parent)
{
}

void QGraphicsTextItemOutline::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget )
{
    //painter->strokePath(painter->clipPath(),QPen(QColor(255,0,0)));
    QPen p=painter->pen();
    p.setColor(QColor(255,0,0));
    painter->setPen(p);
    QGraphicsTextItem::paint(painter,option,widget);
}
