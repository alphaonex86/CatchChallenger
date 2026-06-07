#ifndef MAPMARKET_H
#define MAPMARKET_H

#include <QObject>
#include <QTimer>
#include <mapobject.h>

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
    //detach m_mapObject from its ObjectGroupItem and null it — safe even when the
    //map (and its objectGroupLink entry) was already unloaded under the marker.
    void detachObject();
    Tiled::MapObject *m_mapObject;
    QTimer timer;
};

#endif // MAPMARKET_H
