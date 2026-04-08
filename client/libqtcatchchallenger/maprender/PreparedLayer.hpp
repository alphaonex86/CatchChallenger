#ifndef PREPAREDLAYER_H
#define PREPAREDLAYER_H

#include <QObject>
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include <QPointF>
#include <QElapsedTimer>

#include "MapVisualiserThread.hpp"
#include "../libcatchchallenger/ClientStructures.hpp"
#include "QMap_client.hpp"

class PreparedLayer : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT
public:
    explicit PreparedLayer(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,QGraphicsItem *parent = 0);
    explicit PreparedLayer(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const QPixmap &pixmap, QGraphicsItem *parent = 0);
    void hoverMoveEvent(QGraphicsSceneHoverEvent * event);
    void hoverEnterEvent(QGraphicsSceneHoverEvent * event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent * event);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event);
    void mousePressEvent(QGraphicsSceneMouseEvent * event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);
private:
    CATCHCHALLENGER_TYPE_MAPID mapIndex;
    QElapsedTimer clickDuration;
signals:
    void eventOnMap(CatchChallenger::MapEvent event,const CATCHCHALLENGER_TYPE_MAPID &mapIndex,COORD_TYPE x,COORD_TYPE y);
};

#endif // PREPAREDLAYER_H
