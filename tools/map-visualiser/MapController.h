#ifndef MAPCONTROLLER_H
#define MAPCONTROLLER_H

#include <QObject>

#include "../../general/base/GeneralStructures.h"
#include "../../general/base/Map.h"
#include "../../general/base/MoveOnTheMap.h"
#include "../../client/base/Map_client.h"
#include "../../general/base/Map_loader.h"
#include "../../client/base/render/MapVisualiser.h"

class MapController : public QObject
{
    Q_OBJECT
public:
    explicit MapController(const bool &centerOnPlayer=true,const bool &debugTags=false,const bool &useCache=true,const bool &OpenGL=false);
    ~MapController();

    bool showFPS();
    void setShowFPS(const bool &showFPS);
    void setTargetFPS(int targetFPS);
    void setScale(int scale);

    bool viewMap(const QString &fileName);
private slots:
    void keyPressEvent(QKeyEvent * event);
    void keyReleaseEvent(QKeyEvent * event);
    void keyPressParse();

    void moveStepSlot();
    //have look into another direction, if the key remain pressed, apply like move
    void transformLookToMove();

    //call after enter on new map
    void loadPlayerFromCurrentMap();
    //call before leave the old map (and before loadPlayerFromCurrentMap())
    void unloadPlayerFromCurrentMap();

private:
    MapVisualiser mapVisualiser;

    Tiled::MapObject * playerMapObject;
    Tiled::Tileset * playerTileset;
    int moveStep;
    Pokecraft::Direction direction;
    quint8 xPerso,yPerso;
    bool inMove;

    QTimer timer;
    QTimer moveTimer;
    QTimer lookToMove;
    QSet<int> keyPressed;
};

#endif // MAPCONTROLLER_H
