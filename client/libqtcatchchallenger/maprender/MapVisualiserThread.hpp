#ifndef MAPVISUALISERTHREAD_H
#define MAPVISUALISERTHREAD_H

#ifndef NOTHREADS
#include <QThread>
#else
#include <QObject>
#endif
#include <QSet>
#include <QString>
#include <QHash>
#include <QRegularExpression>

#include <mapreader.h>
#include <tileset.h>
#include <tile.h>

#include "MapVisualiserOrder.hpp"
#include "../../general/base/lib.h"

class DLL_PUBLIC MapVisualiserThread
        #ifndef NOTHREADS
        : public QThread
        #else
        : public QObject
        #endif
        , public MapVisualiserOrder
{
    Q_OBJECT
public:
    std::unordered_map<Tiled::Tile *,TriggerAnimationContent> tileToTriggerAnimationContent;

    explicit MapVisualiserThread();
    ~MapVisualiserThread();
    std::string mLastError;
    bool debugTags;
    Tiled::Tileset * tagTileset;
    int tagTilesetIndex;
    volatile bool stopIt;
    bool hideTheDoors;
    std::string error();
signals:
    void asyncMapLoaded(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,QMap_client *parsedMap);
public slots:
    void loadOtherMapAsync(const CATCHCHALLENGER_TYPE_MAPID &mapIndex);
    QMap_client * loadOtherMap(const CATCHCHALLENGER_TYPE_MAPID &mapIndex);
    //drop and remplace by Map_loader info
    bool loadOtherMapClientPart(QMap_client *parsedMap);
    bool loadOtherMapMetaData(QMap_client *parsedMap);
    void loadBotFile();
public slots:
    virtual void resetAll();
private:
    Tiled::MapReader reader;
    std::unordered_map<std::string/*name*/,std::unordered_map<CATCHCHALLENGER_TYPE_BOTID/*bot id*/,CatchChallenger::Bot> > botFiles;
    std::string language;
};

#endif // MAPVISUALISERTHREAD_H
