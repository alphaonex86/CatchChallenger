#ifndef PREPAREDLAYER_H
#define PREPAREDLAYER_H

#include <QObject>
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include <QPointF>
#include <QTime>

#include "MapVisualiserThread.h"
#include "../ClientStructures.h"

class PreparedLayer : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT
public:
    explicit PreparedLayer(MapVisualiserThread::Map_full * tempMapObject,QGraphicsItem *parent = 0);
    explicit PreparedLayer(MapVisualiserThread::Map_full * tempMapObject,const QPixmap &pixmap, QGraphicsItem *parent = 0);
    void hoverMoveEvent(QGraphicsSceneHoverEvent * event);
    void hoverEnterEvent(QGraphicsSceneHoverEvent * event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent * event);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event);
    void mousePressEvent(QGraphicsSceneMouseEvent * event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);
private:
    MapVisualiserThread::Map_full * tempMapObject;
    QTime clickDuration;
signals:
    void eventOnMap(CatchChallenger::MapEvent event,MapVisualiserThread::Map_full * tempMapObject,uint8_t x,uint8_t y);
};

#endif // PREPAREDLAYER_H
