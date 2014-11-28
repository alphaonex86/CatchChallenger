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

#include "../../../general/base/GeneralStructures.h"
#include "../../../general/base/CommonMap.h"
#include "../../../general/base/GeneralVariable.h"
#include "../../../client/base/Map_client.h"
#include "../../../client/base/DisplayStructures.h"
#include "../../../general/base/Map_loader.h"
#include "../interface/MapDoor.h"
#include "../interface/TriggerAnimation.h"
#include "MapVisualiserOrder.h"

class MapVisualiserThread : public QThread, public MapVisualiserOrder
{
    Q_OBJECT
public:
    QHash<Tiled::Tile *,TriggerAnimationContent> tileToTriggerAnimationContent;

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
    bool loadOtherMapMetaData(MapVisualiserThread::Map_full *parsedMap);
    void loadBotFile(const QString &fileName);
public slots:
    virtual void resetAll();
private:
    QHash<QString,MapVisualiserThread::Map_full> mapCache;
    Tiled::MapReader reader;
    QHash<QString/*name*/,QHash<quint32/*bot id*/,CatchChallenger::Bot> > botFiles;
    QString language;
};

#endif // MAPVISUALISERTHREAD_H
