#include "mapobject.h"
#include "objectgroup.h"
#include "MapItem.h"
#include "MapObjectItem.h"
#include "ObjectGroupItem.h"

#include "../../general/base/GeneralStructures.h"
#include "../../general/base/Map.h"
#include "../../client/base/Map_client.h"
#include "../../general/base/Map_loader.h"

#include "isometricrenderer.h"
#include "map.h"
#include "mapobject.h"
#include "mapreader.h"
#include "objectgroup.h"
#include "orthogonalrenderer.h"
#include "tilelayer.h"
#include "tileset.h"

#ifndef MAP_VISUALISER_H
#define MAP_VISUALISER_H

#include <QGraphicsView>
#include <QTimer>
#include <QKeyEvent>
#include <QGraphicsItem>
#include <QRectF>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include <QSet>
#include <QMultiMap>
#include <QHash>

class MapVisualiser : public QGraphicsView
{
    Q_OBJECT

public:
    explicit MapVisualiser(QWidget *parent = 0,const bool &centerOnPlayer=true,const bool &debugTags=false,const quint8 &targetFPS=30,const bool &useCache=true);
    ~MapVisualiser();
    void keyPressEvent(QKeyEvent * event);
    void keyReleaseEvent(QKeyEvent *event);
    void keyPressParse();
    void viewMap(const QString &fileName);
    bool RectTouch(QRect r1,QRect r2);
    void displayTheDebugMap();
private:
    struct Map_full
    {
        Pokecraft::Map_client logicalMap;
        Tiled::Map * tiledMap;
        Tiled::MapRenderer * tiledRender;
        Tiled::ObjectGroup * objectGroup;
        int objectGroupIndex;
    };

    Tiled::MapReader reader;
    QGraphicsScene *mScene;
    MapItem* mapItem;

    Tiled::MapObject * playerMapObject;
    Tiled::Tileset * playerTileset;
    Tiled::Tileset * tagTileset;
    int tagTilesetIndex;
    int moveStep;
    Pokecraft::Direction direction;
    quint8 xPerso,yPerso;
    bool inMove;

    QTimer timer;
    QTimer moveTimer;
    QTimer lookToMove;
    QSet<int> keyPressed;

    QTimer blink_dyna_layer;

    Map_full *current_map;
    QHash<QString,Map_full *> other_map;
    QSet<QString> displayed_map;//the map really show

    QSet<QString> loadedNearMap;//temp variable to have only the near map

    bool centerOnPlayer;
    bool debugTags;

    quint8 waitRenderTime;
    QTimer timerRender;
private slots:
    QString loadOtherMap(const QString &fileName);
    void loadCurrentMap();
    void loadNearMap(const QString &fileName, const qint32 &x=0, const qint32 &y=0);
    void unloadCurrentMap();
    void loadPlayerFromCurrentMap();
    void unloadPlayerFromCurrentMap();
    void moveStepSlot();
    void transformLookToMove();
    void blinkDynaLayer();

    void render();
};

#endif
