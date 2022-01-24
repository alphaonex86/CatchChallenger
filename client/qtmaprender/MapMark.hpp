#ifndef MAPMARKET_H
#define MAPMARKET_H

#include <QObject>
#include <QTimer>
#include "../tiled/tiled_mapobject.hpp"
#include "../tiled/tiled_tile.hpp"
#include "../tiled/tiled_tileset.hpp"
#include "../tiled/tiled_objectgroup.hpp"

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
