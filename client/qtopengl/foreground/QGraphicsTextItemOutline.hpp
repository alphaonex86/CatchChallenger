#ifndef QGraphicsTextItemOutline_HPP
#define QGraphicsTextItemOutline_HPP

#include <QGraphicsTextItem>

class QGraphicsTextItemOutline : public QGraphicsTextItem
{
    Q_OBJECT
public:
    QGraphicsTextItemOutline(QGraphicsItem *parent = nullptr);
    void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget );
};

#endif // QGraphicsTextItemOutline_HPP
