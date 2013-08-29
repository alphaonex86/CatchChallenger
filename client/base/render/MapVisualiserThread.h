#ifndef MAPVISUALISERTHREAD_H
#define MAPVISUALISERTHREAD_H

#include <QThread>
#include <QSet>
#include <QString>
#include <QHash>

#include <tiled/isometricrenderer.h>
#include <tiled/map.h>
#include <tiled/mapobject.h>
#include <tiled/mapreader.h>
#include <tiled/objectgroup.h>
#include <tiled/orthogonalrenderer.h>
#include <tiled/tilelayer.h>
#include <tiled/tileset.h>
#include <tiled/tile.h>

#include "../../general/base/GeneralStructures.h"
#include "../../general/base/Map.h"
#include "../../client/base/Map_client.h"
#include "../../client/base/DisplayStructures.h"
#include "../../general/base/Map_loader.h"

class MapVisualiserThread : public QThread
{
    Q_OBJECT
public:
    struct Map_animation
    {
        quint8 frames;
        quint8 count;
        QList<Tiled::MapObject *> animatedObject;
    };
    struct Map_full
    {
        CatchChallenger::Map_client logicalMap;
        Tiled::Map * tiledMap;
        Tiled::MapRenderer * tiledRender;
        Tiled::ObjectGroup * objectGroup;
        QHash<quint16,Map_animation> animatedObject;
        int objectGroupIndex;
        int x,y;//needed for the async load
        int x_pixel,y_pixel;
        bool displayed;
    };
    explicit MapVisualiserThread();
    ~MapVisualiserThread();
    QString mLastError;
    bool debugTags;
    Tiled::Tileset * tagTileset;
    int tagTilesetIndex;
    volatile bool stopIt;
    bool hideTheDoors;
    QString error();
signals:
    void asyncMapLoaded(const QString &fileName,MapVisualiserThread::Map_full *parsedMap);
public slots:
    void loadOtherMapAsync(const QString &fileName);
    Map_full * loadOtherMap(const QString &fileName);
    //drop and remplace by Map_loader info
    bool loadOtherMapClientPart(MapVisualiserThread::Map_full *parsedMap);
    void loadBotFile(const QString &fileName);
public slots:
    virtual void resetAll();
private:
    Tiled::MapReader reader;
    QHash<QString/*name*/,QHash<quint8/*bot id*/,CatchChallenger::Bot> > botFiles;
};

#endif // MAPVISUALISERTHREAD_H
