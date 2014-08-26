#ifndef TEMPORARYTILE_H
#define TEMPORARYTILE_H

#include <QObject>
#include <QTimer>

#include "../../tiled/tiled_mapobject.h"
#include "../../tiled/tiled_tile.h"
#include "../../tiled/tiled_tileset.h"

class TemporaryTile : public QObject
{
    Q_OBJECT
public:
    explicit TemporaryTile(Tiled::MapObject* object);
    ~TemporaryTile();
    void startAnimation(Tiled::Tile *tile,const quint32 &ms,const quint8 &count);
    static Tiled::Tile *empty;
private:
    Tiled::MapObject* object;
    int index;
    quint8 count;
    QTimer timer;
private:
    void updateTheTile();
};

#endif // TEMPORARYTILE_H
