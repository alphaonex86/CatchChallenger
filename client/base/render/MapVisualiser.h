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
#include <QGraphicsSimpleTextItem>
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
#include <QGLWidget>

class MapVisualiser : public QGraphicsView
{
    Q_OBJECT

public:
    explicit MapVisualiser(QWidget *parent = 0,const bool &centerOnPlayer=true,const bool &debugTags=false,const bool &useCache=true,const bool &OpenGL=false);
    ~MapVisualiser();
    void keyPressEvent(QKeyEvent * event);
    void keyReleaseEvent(QKeyEvent *event);
    bool viewMap(const QString &fileName);
    bool RectTouch(QRect r1,QRect r2);

    bool showFPS();
    void setShowFPS(const bool &showFPS);
    void setTargetFPS(int targetFPS);

    struct Map_full
    {
        Pokecraft::Map_client logicalMap;
        Tiled::Map * tiledMap;
        Tiled::MapRenderer * tiledRender;
        Tiled::ObjectGroup * objectGroup;
        int objectGroupIndex;
    };
    Map_full *current_map;
    QHash<QString,Map_full *> other_map;
    QSet<QString> displayed_map;//the map really show
private:
    Tiled::MapReader reader;
    QGraphicsScene *mScene;
    MapItem* mapItem;

    Tiled::Tileset * tagTileset;
    int tagTilesetIndex;
    QTimer blink_dyna_layer;

    QSet<QString> loadedNearMap;//temp variable to have only the near map

    bool centerOnPlayer;
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
public slots:
    QString loadOtherMap(const QString &fileName);
    void loadCurrentMap();
private slots:
    void loadNearMap(const QString &fileName, const qint32 &x=0, const qint32 &y=0);

    void blinkDynaLayer();

    void render();
    void paintEvent(QPaintEvent * event);
    void updateFPS();
signals:
    void newKeyPressEvent(QKeyEvent * event);
    void newKeyReleaseEvent(QKeyEvent *event);
};

#endif
