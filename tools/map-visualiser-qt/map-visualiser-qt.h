#include "mapobject.h"
#include "objectgroup.h"

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

namespace Tiled {
class Map;
class MapRenderer;
}

#define STEPPERSO 1

class MapItem : public QGraphicsItem
{
public:
    MapItem(QGraphicsItem *parent = 0);
    void addMap(Tiled::Map *map, Tiled::MapRenderer *renderer);
    void removeMap(Tiled::Map *map);
    void setMapPosition(Tiled::Map *map, qint16 x, qint16 y);
    QRectF boundingRect() const;
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
private:
    struct Map_graphics_link
    {
        QGraphicsItem * graphic;
        Tiled::Layer *layer;
    };
    QMultiMap<Tiled::Map *,Map_graphics_link> displayed_layer;
    QSet<Tiled::Map *> displayed_map;
};

class MapVisualiserQt : public QGraphicsView
{
    Q_OBJECT

public:
    explicit MapVisualiserQt(QWidget *parent = 0);
    ~MapVisualiserQt();
    void keyPressEvent(QKeyEvent * event);
    void keyReleaseEvent(QKeyEvent *event);
    void keyPressParse();
    void viewMap(const QString &fileName);
    bool RectTouch(QRect r1,QRect r2);
private:
    struct Map_full
    {
        Pokecraft::Map_client logicalMap;
        Tiled::Map * tiledMap;
        Tiled::MapRenderer * tiledRender;
        Tiled::ObjectGroup * objectGroup;
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
private slots:
    QString loadOtherMap(const QString &fileName);
    void loadCurrentMap();
    void loadNearMap(const QString &fileName, const qint32 &x=0, const qint32 &y=0);
    void unloadCurrentMap();
    void loadPlayerFromCurrentMap();
    void unloadPlayerFromCurrentMap();
    void moveStepSlot(bool justUpdateTheTile=false);
    void transformLookToMove();
    void blinkDynaLayer();
};

#endif
