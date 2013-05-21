#include "MapItem.h"
#include "MapObjectItem.h"
#include "ObjectGroupItem.h"

#include "../../general/base/GeneralStructures.h"
#include "../../general/base/Map.h"
#include "../../client/base/Map_client.h"
#include "../../client/base/DisplayStructures.h"
#include "../../general/base/Map_loader.h"

#include "../../general/libtiled/isometricrenderer.h"
#include "../../general/libtiled/map.h"
#include "../../general/libtiled/mapobject.h"
#include "../../general/libtiled/mapreader.h"
#include "../../general/libtiled/objectgroup.h"
#include "../../general/libtiled/orthogonalrenderer.h"
#include "../../general/libtiled/tilelayer.h"
#include "../../general/libtiled/tileset.h"
#include "../../general/libtiled/tile.h"

#ifndef MAP_VISUALISER_H
#define MAP_VISUALISER_H

#include <QGraphicsView>
#include <QGraphicsSimpleTextItem>
#include <QTimer>
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
#include <QGLWidget>

class MapVisualiser : public QGraphicsView
{
    Q_OBJECT

public:
    explicit MapVisualiser(const bool &debugTags=false,const bool &useCache=true,const bool &OpenGL=false);
    ~MapVisualiser();
    bool RectTouch(QRect r1,QRect r2);

    bool showFPS();
    void setShowFPS(const bool &showFPS);
    void setTargetFPS(int targetFPS);

    QHash<QString/*name*/,QHash<quint8/*bot id*/,CatchChallenger::Bot> > botFiles;
    struct Map_full
    {
        CatchChallenger::Map_client logicalMap;
        Tiled::Map * tiledMap;
        Tiled::MapRenderer * tiledRender;
        Tiled::ObjectGroup * objectGroup;
        int objectGroupIndex;
    };
    Map_full * getMap(QString map);

    Map_full *current_map;
    QHash<QString,Map_full *> all_map;

protected:
    Tiled::MapReader reader;
    QGraphicsScene *mScene;
    MapItem* mapItem;

    Tiled::Tileset * tagTileset;
    int tagTilesetIndex;

    /** map loaded (displayed or not), because it's in immediate range
     * The current map is resolved on it, and the current player can go on it (teleporter or border) */
    QSet<QString> mapUsed;
    /** \brief to have only the near/displayed map
     * Then only for the current player */
    QSet<QString> displayed_map;

    bool debugTags;
    QString mLastError;

    quint8 waitRenderTime;
    QTimer timerRender;
    QTime timeRender;
    quint16 frameCounter;
    QTimer timerUpdateFPS;
    QTime timeUpdateFPS;
    QGraphicsSimpleTextItem *FPSText;
    bool mShowFPS;

    Tiled::Layer *grass;
    Tiled::Layer *grassOver;

    virtual void destroyMap(Map_full *map);
protected slots:
    virtual void resetAll();
public slots:
    QString loadOtherMap(const QString &fileName);
    void loadOtherMapClientPart(Map_full *parsedMap);
    virtual void loadBotOnTheMap(Map_full *parsedMap,const quint8 &x,const quint8 &y,const QString &lookAt,const QString &skin);
    void loadBotFile(const QString &fileName);
    virtual QSet<QString> loadMap(Map_full *map, const bool &display);
    virtual void removeUnusedMap();
private slots:
    QSet<QString> loadTeleporter(Map_full *map);
    QSet<QString> loadNearMap(const QString &fileName, const bool &display, const qint32 &x=0, const qint32 &y=0, const qint32 &x_pixel=0, const qint32 &y_pixel=0,const QSet<QString> &previousLoadedNearMap=QSet<QString>());
    void paintEvent(QPaintEvent * event);
    void updateFPS();
protected slots:
    void render();
};

#endif
