#ifndef MAPVISUALISERTHREAD_H
#define MAPVISUALISERTHREAD_H

#include <QThread>
#include <QSet>
#include <QString>
#include <QHash>
#include <QRegularExpression>

#include "../../tiled/tiled_isometricrenderer.h"
#include "../../tiled/tiled_map.h"
#include "../../tiled/tiled_mapobject.h"
#include "../../tiled/tiled_mapreader.h"
#include "../../tiled/tiled_objectgroup.h"
#include "../../tiled/tiled_orthogonalrenderer.h"
#include "../../tiled/tiled_tilelayer.h"
#include "../../tiled/tiled_tileset.h"
#include "../../tiled/tiled_tile.h"

#include "../../general/base/GeneralStructures.h"
#include "../../general/base/CommonMap.h"
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
        int relative_x,relative_y;//needed for the async load
        int relative_x_pixel,relative_y_pixel;
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
    QRegularExpression regexMs;
    QRegularExpression regexFrames;
    QHash<QString,MapVisualiserThread::Map_full> mapCache;
    Tiled::MapReader reader;
    QHash<QString/*name*/,QHash<quint32/*bot id*/,CatchChallenger::Bot> > botFiles;
    QString language;
};

#endif // MAPVISUALISERTHREAD_H
