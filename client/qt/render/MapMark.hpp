#ifndef MAPMARKET_H
#define MAPMARKET_H

#include <QObject>
#include <QTimer>
#include "../tiled/tiled_mapobject.h"
#include "../tiled/tiled_tile.h"
#include "../tiled/tiled_tileset.h"
#include "../tiled/tiled_objectgroup.h"

class MapMark : public QObject
{
    Q_OBJECT
public:
    explicit MapMark(Tiled::MapObject *m_mapObject);
    ~MapMark();
    Tiled::MapObject *mapObject();
private slots:
    void updateTheFrame();
private:
    Tiled::MapObject *m_mapObject;
    QTimer timer;
};

#endif // MAPMARKET_H
