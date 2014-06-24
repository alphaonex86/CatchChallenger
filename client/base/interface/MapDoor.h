#ifndef MAPDOOR_H
#define MAPDOOR_H

#include <QObject>
#include <QTimer>

#include "../../tiled/tiled_mapobject.h"
#include "../../tiled/tiled_tile.h"
#include "../../tiled/tiled_tileset.h"

class MapDoor : public QObject
{
    Q_OBJECT
public:
    explicit MapDoor(Tiled::MapObject* object, const quint8 &framesCount, const quint16 &ms);
    ~MapDoor();
    void startOpen();
    void startClose();
private:
    Tiled::MapObject* object;
    Tiled::Cell cell;
    const Tiled::Tile* baseTile;
    const quint8 framesCount;
    const quint16 ms;
    struct EventCall
    {
        QTimer *timer;
        quint8 frame;//0 to framesCount*2 -> open + close
    };
    QList<EventCall> events;
private slots:
    void timerFinish();
};

#endif // MAPDOOR_H
