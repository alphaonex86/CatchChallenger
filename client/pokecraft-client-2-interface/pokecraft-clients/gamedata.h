#ifndef GAMEDATA_H
#define GAMEDATA_H

#include <QtCore>
#include <QObject>

#include "../pokecraft-general/GeneralStructures.h"
#include "maprenderer.h"
#include "map.h"
#include "MultiMap.h"
#include "Singleton.h"

class player;
class craftClients;
class Pokecraft_client;
class MultiMap;

using namespace Tiled;

class gameData: public QObject, public Singleton<gameData>
{
    Q_OBJECT
public:
    gameData();
    ~gameData();
    static gameData* instance       ();
    static gameData* OneTimeInstance();
    static void forceDestroyInstance();
    void showDebug                  (QString message );
    void showError                  (QString message );
    void showWarning                (QString message );
    bool mapSupported               (Map* map);
    void moveMap                    (int x, int y);
    void moveMap                    (int direction);
    // -- getter
    craftClients* getSubClient       ();
    Pokecraft_client* getClient      ();
    QList<player*> getPlayer         ();
    QList<player*> getPlayerWatching ();
    player* getThisPlayer            ();
    player* getPlayerById            (const quint32 &id);
    player* getPlayerByName          (QString name);
    QList<MultiMap*> getMap          ();
    MultiMap* getMapByName           (QString name);
    Player_private_and_public_informations getPlayerInfo();
    QString getFileFromDatapack      (QString path);
    QDir&   workingDir               ();
    MapRenderer* getMapRenderer      ();
    QGraphicsItem* getGraphicsItem   ();
    int getTileHeight                ();
    // -- setter
    void setWorkingDir      (QDir dir);
    void addPlayerToWatch   (player *Player);
    void removePlayerToWatch(player *Player);
    void addPlayer          (player* Player);
    void removePlayer       (player* Player);
    void setThisPlayer      (player* Player);
    void setClient          (Pokecraft_client* client);
    void setSubClient       (craftClients* client);
    void setMapRenderer     (MapRenderer *renderer);
    void setMapRenderer     (Map *map);
    void setGraphicsItem    (QGraphicsItem* item);
    void addMap             (MultiMap* map);
    void removeMap          (MultiMap* map);
    void setTickSpeed       (quint16 speed);
public slots:
    void have_player_information(QList<Player_public_informations> information);
    void haveDatapack           ();
    void newMessage             (QString message);
    void newTick                ();
signals:
    void everyRefresh();
private:
    // -- general
    QDir                mDir;
    QTimer*             timer;
    // -- player
    QList<player*>      mPlayerList;
    QList<player*>      movingPlayer;
    player*             mPlayer;
    // -- map
    QList<MultiMap*>    mMap;
    int                 tileHeight;
    // -- graphics
    QGraphicsItem*      mItem;
    MapRenderer*        mRenderer;
    // -- clients
    craftClients*       mSubClient;
    Pokecraft_client*   mClient;
    // -- temp variable
    int index_loop;
    int loop_size;
};

#endif // GAMEDATA_H
