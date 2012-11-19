#ifndef POKECRAFT_MAPCONTROLLER_H
#define POKECRAFT_MAPCONTROLLER_H

#include "../../client/base/render/MapVisualiserPlayer.h"
#include "../../client/base/Api_protocol.h"

#include <QString>
#include <QList>
#include <QStringList>

class MapController : public MapVisualiserPlayer
{
    Q_OBJECT
public:
    explicit MapController(Pokecraft::Api_protocol *client, const bool &centerOnPlayer=true, const bool &debugTags=false, const bool &useCache=true, const bool &OpenGL=false);
    ~MapController();

    void resetAll();
    void setScale(int scaleSize);
public slots:
    //map move
    void insert_player(const Pokecraft::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const Pokecraft::Direction &direction);
    void move_player(const quint16 &id,const QList<QPair<quint8,Pokecraft::Direction> > &movement);
    void remove_player(const quint16 &id);
    void reinsert_player(const quint16 &id,const quint8 &x,const quint8 &y,const Pokecraft::Direction &direction);
    void reinsert_player(const quint16 &id,const quint32 &mapId,const quint8 &x,const quint8 &y,const Pokecraft::Direction &direction);
    void dropAllPlayerOnTheMap();

    //player info
    void have_current_player_info(const Pokecraft::Player_private_and_public_informations &informations);

    //the datapack
    void setDatapackPath(const QString &path);
    void datapackParsed();
private:
    //the other player
    struct OtherPlayer
    {
        Tiled::MapObject * playerMapObject;
        Tiled::Tileset * playerTileset;
        int moveStep;
        Pokecraft::Direction direction;
        quint8 x,y;
        bool inMove;
        bool stepAlternance;
        Map_full *current_map;
    };
    QList<OtherPlayer> otherPlayerList;
    struct BotSpawnPoint
    {
        MapVisualiser::Map_full * map;
        quint8 x,y;
    };
    Pokecraft::Api_protocol *client;

    //current player
    Pokecraft::Player_private_and_public_informations player_informations;
    bool player_informations_is_set;

    //datapack
    QStringList skinFolderList;
    QString datapackPath;
    bool mHaveTheDatapack;

    //the delayed action
    struct DelayedInsert
    {
        Pokecraft::Player_public_informations player;
        quint32 mapId;
        quint16 x;
        quint16 y;
        Pokecraft::Direction direction;
    };
    QList<DelayedInsert> delayedInsert;
    struct DelayedMove
    {
        quint16 id;
        QList<QPair<quint8,Pokecraft::Direction> > movement;
    };
    QList<DelayedMove> delayedMove;
    QList<quint16> delayedRemove;
private slots:
    bool loadMap(const QString &fileName,const quint8 &x,const quint8 &y);
protected slots:
    //call after enter on new map
    virtual void loadOtherPlayerFromMap(OtherPlayer otherPlayer);
    //call before leave the old map (and before loadPlayerFromCurrentMap())
    virtual void unloadOtherPlayerFromMap(OtherPlayer otherPlayer);
};

#endif // MAPCONTROLLER_H
