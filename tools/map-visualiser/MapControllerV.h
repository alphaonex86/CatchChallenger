#ifndef MAPCONTROLLER_H
#define MAPCONTROLLER_H

#include "../../client/base/render/MapVisualiserPlayer.h"

class MapControllerV : public MapVisualiserPlayer
{
    Q_OBJECT
public:
    explicit MapControllerV(const bool &centerOnPlayer=true, const bool &debugTags=false, const bool &useCache=true);
    ~MapControllerV();

    void setScale(int scaleSize);
    void setBotNumber(quint16 botNumber);

    bool viewMap(const QString &fileName);
private:
    struct Bot
    {
        quint8 x,y;
        Tiled::MapObject * mapObject;
        QString map;

        int moveStep;
        CatchChallenger::Direction direction;
        bool inMove;
    };
    QList<Bot> botList;
    struct BotSpawnPoint
    {
        MapVisualiserThread::Map_full * map;
        quint8 x,y;
    };
    QList<BotSpawnPoint> botSpawnPointList;
    quint16 botNumber;
    Tiled::Tileset * botTileset;

    QTimer timerBotMove;
    QTimer timerBotManagement;
    bool nextPathStep();
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
