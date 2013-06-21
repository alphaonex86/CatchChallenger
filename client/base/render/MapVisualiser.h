#ifndef MAP_VISUALISER_H
#define MAP_VISUALISER_H

#include "MapItem.h"
#include "MapObjectItem.h"
#include "ObjectGroupItem.h"

#include "../../general/base/GeneralStructures.h"
#include "../../general/base/Map.h"
#include "../../client/base/Map_client.h"
#include "../../client/base/DisplayStructures.h"
#include "../../general/base/Map_loader.h"

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

#include "MapVisualiserThread.h"

class MapVisualiser : public QGraphicsView
{
    Q_OBJECT

public:
    explicit MapVisualiser(const bool &debugTags=false,const bool &useCache=true,const bool &OpenGL=false);
    ~MapVisualiser();

    bool showFPS();
    void setShowFPS(const bool &showFPS);
    void setTargetFPS(int targetFPS);

    MapVisualiserThread::Map_full * getMap(QString map);

    MapVisualiserThread::Map_full *current_map;
    QHash<QString,MapVisualiserThread::Map_full *> all_map;
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

    MapVisualiserThread mapVisualiserThread;

    virtual void destroyMap(MapVisualiserThread::Map_full *map);
protected slots:
    virtual void resetAll();
public slots:
    QString loadOtherMap(const QString &fileName);
    virtual void loadBotOnTheMap(MapVisualiserThread::Map_full *parsedMap, const quint32 &botId, const quint8 &x, const quint8 &y, const QString &lookAt, const QString &skin);
    virtual QSet<QString> loadMap(MapVisualiserThread::Map_full *map, const bool &display);
    virtual void removeUnusedMap();
private slots:
    QSet<QString> loadTeleporter(MapVisualiserThread::Map_full *map);
    QSet<QString> loadNearMap(const QString &fileName, const bool &display, const qint32 &x=0, const qint32 &y=0, const qint32 &x_pixel=0, const qint32 &y_pixel=0,const QSet<QString> &previousLoadedNearMap=QSet<QString>());
    void paintEvent(QPaintEvent * event);
    void updateFPS();
protected slots:
    void render();
};

#endif
