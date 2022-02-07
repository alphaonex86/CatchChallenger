#ifndef MAP_VISUALISER_H
#define MAP_VISUALISER_H

#include "MapItem.hpp"
#include "MapObjectItem.hpp"
#include "ObjectGroupItem.hpp"
#include "MapMark.hpp"

#include "../../general/base/GeneralStructures.hpp"
#include "../../general/base/CommonMap.hpp"
#include "../libqtcatchchallenger/DisplayStructures.hpp"
#include "../../general/base/Map_loader.hpp"

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
//#include <QGLWidget>

#include "MapVisualiserThread.hpp"

class MapVisualiser : public QGraphicsView
{
    Q_OBJECT

public:
    explicit MapVisualiser(const bool &debugTags=false, const bool &useCache=true, const bool &openGL=false);
    ~MapVisualiser();

    void setTargetFPS(int targetFPS);
    virtual void eventOnMap(CatchChallenger::MapEvent event,Map_full * tempMapObject,uint8_t x,uint8_t y);

    Map_full * getMap(const std::string &map) const;

    std::string current_map;
    std::unordered_map<std::string,Map_full *> all_map,old_all_map;
    std::unordered_map<std::string,QDateTime> old_all_map_time;
protected:
    Tiled::MapReader reader;
    QGraphicsScene *mScene;
    MapItem* mapItem;

    Tiled::Tileset * markPathFinding;
    int tagTilesetIndex;

    bool debugTags;
    std::string mLastError;

    uint8_t waitRenderTime;
    QTimer timerRender;
    QTime timeRender;
    uint16_t frameCounter;
    QTimer timerUpdateFPS;
    QTime timeUpdateFPS;

    Tiled::Layer *grass;
    Tiled::Layer *grassOver;
    MapMark *mark;

    MapVisualiserThread mapVisualiserThread;
    std::vector<std::string> asyncMap;
    std::unordered_map<uint16_t/*intervale*/,QTimer *> animationTimer;

    virtual void destroyMap(Map_full *map);
    static bool rectTouch(QRect r1,QRect r2);
protected slots:
    virtual void resetAll();
public slots:
    void loadOtherMap(const std::string &resolvedFileName);
    virtual void loadBotOnTheMap(Map_full *parsedMap, const uint32_t &botId, const uint8_t &x, const uint8_t &y,
                                 const std::string &lookAt, const std::string &skin);
    //virtual std::unordered_set<QString> loadMap(Map_full *map, const bool &display);
    virtual void removeUnusedMap();
    virtual void hideNotloadedMap();
private slots:
    void loadTeleporter(Map_full *map);
    void paintEvent(QPaintEvent * event);
    void updateFPS();
    void asyncDetectBorder(Map_full * tempMapObject);
    void applyTheAnimationTimer();
protected slots:
    void render();
    virtual bool asyncMapLoaded(const std::string &fileName,Map_full * tempMapObject);
    void passMapIntoOld();
signals:
    void loadOtherMapAsync(const std::string &fileName);
    void mapDisplayed(const std::string &fileName);
    void newFPSvalue(const unsigned int FPS);
};

#endif
