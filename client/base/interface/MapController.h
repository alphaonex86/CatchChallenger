#ifndef POKECRAFT_MAPCONTROLLER_H
#define POKECRAFT_MAPCONTROLLER_H

#include "../../client/base/render/MapVisualiserPlayer.h"
#include "../../client/base/Api_protocol.h"

class MapController : public MapVisualiserPlayer
{
    Q_OBJECT
public:
    explicit MapController(Pokecraft::Api_protocol *client,const bool &centerOnPlayer=true,const bool &debugTags=false,const bool &useCache=true,const bool &OpenGL=false);
    ~MapController();

    void setScale(int scaleSize);
    void setBotNumber(quint16 botNumber);
public slots:
    //map move
    void insert_player(Pokecraft::Player_public_informations player,QString mapName,quint16 x,quint16 y,Pokecraft::Direction direction);
    void move_player(quint16 id,QList<QPair<quint8,Pokecraft::Direction> > movement);
    void remove_player(quint16 id);
    void reinsert_player(quint16 id,quint8 x,quint8 y,Pokecraft::Direction direction);
    void reinsert_player(quint16 id,QString mapName,quint8 x,quint8 y,Pokecraft::Direction direction);
    void dropAllPlayerOnTheMap();

    //player info
    void have_current_player_info(Pokecraft::Player_private_and_public_informations informations);

    //the datapack
    void setDatapackPath(const QString &path);
private:
    struct OtherPlayer
    {
        quint8 x,y;
        Tiled::MapObject * mapObject;
        QString map;

        int moveStep;
        Pokecraft::Direction direction;
        bool inMove;
    };
    QList<OtherPlayer> botList;
    struct BotSpawnPoint
    {
        MapVisualiser::Map_full * map;
        quint8 x,y;
    };
    QList<BotSpawnPoint> botSpawnPointList;
    quint16 botNumber;
    Tiled::Tileset * botTileset;

    Pokecraft::Player_private_and_public_informations player_informations;
    bool player_informations_is_set;

    QString datapackPath;

    QTimer timerBotMove;
    QTimer timerBotManagement;

    Pokecraft::Api_protocol *client;
private slots:
    void botMove();
    void botManagement();
    bool botMoveStepSlot(OtherPlayer *bot);

    //call after enter on new map
    void loadPlayerFromCurrentMap();
    //call before leave the old map (and before loadPlayerFromCurrentMap())
    void unloadPlayerFromCurrentMap();

    bool loadMap(const QString &fileName,const quint8 &x,const quint8 &y);
};

#endif // MAPCONTROLLER_H
