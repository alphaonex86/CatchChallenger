#ifndef PREPAREDLAYER_H
#define PREPAREDLAYER_H

#include <QObject>
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include <QPointF>
#include <QTime>

#include "MapVisualiserThread.hpp"
#include "../../qt/ClientStructures.hpp"

class PreparedLayer : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT
public:
    explicit PreparedLayer(Map_full * tempMapObject,QGraphicsItem *parent = 0);
    explicit PreparedLayer(Map_full * tempMapObject,const QPixmap &pixmap, QGraphicsItem *parent = 0);
    void hoverMoveEvent(QGraphicsSceneHoverEvent * event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent * event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent * event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent * event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent * event) override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
private:
    Map_full * tempMapObject;
    QTime clickDuration;
    QPixmap cache;
signals:
    void eventOnMap(CatchChallenger::MapEvent event,Map_full * tempMapObject,uint8_t x,uint8_t y);
};

#endif // PREPAREDLAYER_H
