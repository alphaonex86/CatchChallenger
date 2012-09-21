#ifndef MAPCONTROLLER_H
#define MAPCONTROLLER_H

#include "../../client/base/render/MapVisualiserPlayer.h"

class MapController : public MapVisualiserPlayer
{
    Q_OBJECT
public:
    explicit MapController(QWidget *parent = 0,const bool &centerOnPlayer=true,const bool &debugTags=false,const bool &useCache=true,const bool &OpenGL=false);
    ~MapController();

    void setScale(int scaleSize);
    void setBotNumber(quint16 botNumber);
private:
    struct Bot
    {
        quint8 x,y;
        Tiled::MapObject * mapObject;
        QString map;

        int moveStep;
        Pokecraft::Direction direction;
        bool inMove;
    };
    QList<Bot> botList;
    struct BotSpawnPoint
    {
        MapVisualiser::Map_full * map;
        quint8 x,y;
    };
    QList<BotSpawnPoint> botSpawnPointList;
    quint16 botNumber;
    Tiled::Tileset * botTileset;

    QTimer timerBotMove;
    QTimer timerBotManagement;
private slots:
    void botMove();
    void botManagement();
    bool botMoveStepSlot(Bot *bot);

    //call after enter on new map
    void loadPlayerFromCurrentMap();
    //call before leave the old map (and before loadPlayerFromCurrentMap())
    void unloadPlayerFromCurrentMap();
};

#endif // MAPCONTROLLER_H
