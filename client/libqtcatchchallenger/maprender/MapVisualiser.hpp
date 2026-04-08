#ifndef MAP_VISUALISER_H
#define MAP_VISUALISER_H

#include "MapItem.hpp"
#include "MapObjectItem.hpp"
#include "ObjectGroupItem.hpp"
#include "MapMark.hpp"

#include "../../general/base/lib.h"

#include <QGraphicsView>
#include <QGraphicsSimpleTextItem>
#include <QTimer>
#include <QList>
#include <QKeyEvent>
#include <QGraphicsItem>
#include <QRectF>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include <QSet>
#include <QString>
#include <QMultiMap>
#include <QHash>
#include <QTime>
#include <QDateTime>
#include <QElapsedTimer>
//#include <QGLWidget>

#include "MapVisualiserThread.hpp"
#include "QMap_client.hpp"

class DLL_PUBLIC MapVisualiser : public QGraphicsView
{
    Q_OBJECT

public:
    explicit MapVisualiser(const bool &debugTags=false, const bool &useCache=true, const bool &openGL=false);
    ~MapVisualiser();

    void setTargetFPS(int targetFPS);
    virtual void eventOnMap(CatchChallenger::MapEvent event,const CATCHCHALLENGER_TYPE_MAPID &mapIndex,COORD_TYPE x,COORD_TYPE y);

    CatchChallenger::QMap_client * getMap(const CATCHCHALLENGER_TYPE_MAPID &mapIndex) const;

    CATCHCHALLENGER_TYPE_MAPID current_map;
    //put to protected again:
    MapItem* mapItem;
protected:
    Tiled::MapReader reader;
    QGraphicsScene *mScene;

    Tiled::SharedTileset markPathFinding;
    int tagTilesetIndex;

    bool debugTags;
    std::string mLastError;

    uint8_t waitRenderTime;
    QTimer timerRender;
    QElapsedTimer timeRender;
    uint16_t frameCounter;
    QTimer timerUpdateFPS;
    QElapsedTimer timeUpdateFPS;

    Tiled::Layer *grass;
    Tiled::Layer *grassOver;
    MapMark *mark;

    MapVisualiserThread mapVisualiserThread;
    std::vector<CATCHCHALLENGER_TYPE_MAPID> asyncMap;
    std::unordered_map<uint16_t/*intervale*/,QTimer *> animationTimer;

    virtual void destroyMap(const CATCHCHALLENGER_TYPE_MAPID &mapIndex);
    static bool rectTouch(QRect r1,QRect r2);
protected slots:
    virtual void resetAll();
public slots:
    void loadOtherMap(const CATCHCHALLENGER_TYPE_MAPID &mapIndex);
    virtual void loadBotOnTheMap(const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const CATCHCHALLENGER_TYPE_BOTID &botId, const COORD_TYPE &x, const COORD_TYPE &y,const std::string &lookAt, const std::string &skin);
    //virtual std::unordered_set<QString> loadMap(const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const bool &display);
    virtual void removeUnusedMap();
    virtual void hideNotloadedMap();
private slots:
    void loadTeleporter(const CATCHCHALLENGER_TYPE_MAPID &mapIndex);
    void paintEvent(QPaintEvent * event);
    void updateFPS();
    void asyncDetectBorder(const CATCHCHALLENGER_TYPE_MAPID &mapIndex);
    void applyTheAnimationTimer();
protected slots:
    void render();
    virtual bool asyncMapLoaded(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,QMap_client * tempMapObject);
    void passMapIntoOld();
signals:
    void loadOtherMapAsync(const CATCHCHALLENGER_TYPE_MAPID &mapIndex);
    void mapDisplayed(const CATCHCHALLENGER_TYPE_MAPID &mapIndex);
    void newFPSvalue(const unsigned int FPS);
};

#endif
