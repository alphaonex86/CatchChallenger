#include "gamedata.h"

#include "player.h"
#include "isometricrenderer.h"
#include "orthogonalrenderer.h"
#include "craft-clients.h"

gameData::gameData()
{
    mPlayer = 0;
    mClient = 0;
    mSubClient = 0;
    mRenderer = 0;
    mItem = 0;
    tileHeight = -1;
    timer = new QTimer();
    connect(timer,SIGNAL(timeout()),this,SLOT(newTick()));
    // 16 tick in 1s
    timer->start(16);
    mDir = QDir(QCoreApplication::applicationDirPath());
    mMap = QList<MultiMap*>();
    mPlayerList = QList<player*>();
}

gameData::~gameData()
{
    if(timer)
    {
        timer->stop();
        delete timer;
        timer = 0;
    }
    qDeleteAll(mPlayerList);
    qDeleteAll(mMap);
    if(mRenderer!=0)
        delete mRenderer;
    showDebug("Deleting gameData ...");
    number_load = 0;
}

gameData* gameData::OneTimeInstance()
{
    return _singleton;
}

gameData* gameData::instance()
{
    gameData* data = getInstance();
    if(number_load<1)
        number_load=1;
    return data;
}

void gameData::forceDestroyInstance()
{
    delete _singleton;
    _singleton  = NULL;
    qWarning()<< "number of gameData instance : "<<number_load;
    number_load = 0;
    //gameData::destroyInstanceAtTheLastCall();
}

void gameData::showDebug(QString message)
{
    qWarning()<<"[Debug] " << message;
}

void gameData::showError(QString message)
{
    qWarning()<<"[Error] " << message;
}

void gameData::showWarning(QString message)
{
    qWarning()<<"[Warning] " << message;
}

bool gameData::mapSupported(Tiled::Map* map)
{
    if(mRenderer == 0)
        setMapRenderer(map);
    if( map->orientation()!=Tiled::Map::Orthogonal)
        return false;
    return true;
}

void gameData::moveMap(int direction)
{
    switch(direction)
    {
    case 1:
    case 5:
        moveMap(0,1);
        break;
    case 2:
    case 6:
        moveMap(-1,0);
        break;
    case 3:
    case 7:
        moveMap(0,-1);
        break;
    case 4:
    case 8:
        moveMap(1,0);
        break;
    }
}

void gameData::moveMap(int x, int y)
{
    foreach(MultiMap *map,mMap)
    {
        map->setX(map->x()+x);
        map->setY(map->y()+y);
    }
    foreach(player *p, mPlayerList)
    {
        p->getGraphicsItem()->moveBy(x,y);
    }
}

// -- getter
QList<player*> gameData::getPlayer()
{
    return mPlayerList;
}

QList<player*> gameData::getPlayerWatching()
{
    return movingPlayer;
}

//heavy
player* gameData::getPlayerById(const quint32 &id)
{
    foreach(player* Player, mPlayerList)
        if(Player->getId() == id)
            return Player;
    return 0;
}

player* gameData::getPlayerByName(QString name)
{
    foreach(player* Player, mPlayerList)
        if(Player->getPlayerName() == name)
            return Player;
    return 0;
}

player* gameData::getThisPlayer()
{
    return mPlayer;
}

Pokecraft::Player_private_and_public_informations gameData::getPlayerInfo()
{
/*    if(mClient == 0)
        return mClient->get_player_informations();
    Pokecraft::Player_private_and_public_informations info;
    return info;*/
}

craftClients* gameData::getSubClient()
{
    return mSubClient;
}

QDir& gameData::workingDir()
{
    return mDir;
}

Pokecraft_client* gameData::getClient()
{
    return mClient;
}

Tiled::MapRenderer* gameData::getMapRenderer()
{
    return mRenderer;
}

QList<MultiMap*> gameData::getMap()
{
    return mMap;
}

QGraphicsItem* gameData::getGraphicsItem()
{
    return mItem;
}

MultiMap* gameData::getMapByName(QString name)
{
    if(name.isEmpty())
        return 0;
    foreach(MultiMap* map,mMap)
    {
        if(map->getFileName().contains(name))
            return map;
    }
    return 0;
}

int gameData::getTileHeight()
{
    return tileHeight;
}

// -- setter

void gameData::addPlayerToWatch(player *Player)
{
    if(Player==0)
        return;
    if(movingPlayer.indexOf(Player)!=-1)
        return;
    movingPlayer.append(Player);
    connect(this,SIGNAL(everyRefresh()),Player,SLOT(check()));
}

void gameData::removePlayerToWatch(player *Player)
{
    if(Player==0)
        return;
    movingPlayer.removeOne(Player);
    disconnect(this,SIGNAL(everyRefresh()),Player,SLOT(check()));
}

void gameData::addPlayer(player* Player)
{
    if(Player==0)
        return;
    /*if(Player->getId() == getClient()->get_player_informations().public_informations.id)
        setThisPlayer(Player);
    mPlayerList.append(Player);*/
}

void gameData::removePlayer(player* Player)
{
    if(Player==0)
        return;
    mPlayerList.removeOne(Player);
    removePlayerToWatch(Player);
    if(Player == mPlayer)
        mPlayer = 0;
}

void gameData::setThisPlayer(player *Player)
{
    if(Player==0)
        return;
    if(mPlayer!=0)
        disconnect(getSubClient(),SLOT(moveselfPlayer(quint8,Direction)));
    this->mPlayer = Player;
    showDebug("Setting your player....");
    getSubClient()->centerOn(Player->getGraphicsItem());
}

void gameData::setClient(Pokecraft::Api_client_real *client)
{
/*	mClient = client;
        mDir = QDir(QCoreApplication::applicationDirPath());
        mDir.mkpath( mDir.canonicalPath()
		     + "/"
		     + QString(client->getHost())
                     + QString("-")
                     + QString::number(client->getPort()));
        mDir.setPath( mDir.canonicalPath()
		      + "/"
		      + QString(client->getHost())
                      + QString("-")
                      + QString::number(client->getPort()));
	showDebug(QString("Working dir : ")+mDir.absolutePath());*/
}

void gameData::setSubClient(craftClients *client)
{
    mSubClient = client;
}

void gameData::setMapRenderer(MapRenderer *renderer)
{
    mRenderer=renderer;
    showDebug("New Map renderer");
}

void gameData::setMapRenderer(Tiled::Map *map)
{
    if(map->orientation()==Tiled::Map::Orthogonal&&mRenderer==0)
    {
        mRenderer=new OrthogonalRenderer(map);
        tileHeight = map->tileHeight();
    }
    else
        showWarning(QString("Unknow Map type"));
}

void gameData::addMap(MultiMap *map)
{
    if(mMap.indexOf(map) == -1)
        mMap.append(map);
}

void gameData::removeMap(MultiMap *map)
{
    mMap.removeOne(map);
}

void gameData::setGraphicsItem(QGraphicsItem *item)
{
    mItem = item;
    showDebug("New Graphics Item detected");
    foreach(player* play, getPlayer())
    {
        if(play != 0 && play->getGraphicsItem() != 0)
            play->getGraphicsItem()->setParentItem(item);
    }
    foreach(MultiMap* map, mMap)
    {
        foreach(QGraphicsItem *layer,map->getChild())
        {
            layer->setParentItem(item);
        }
    }
}

void gameData::setTickSpeed(quint16 speed)
{
    timer->setInterval(speed);
}

// -- slots

void gameData::have_player_information(QList<Pokecraft::Player_public_informations> information)
{
    foreach(Pokecraft::Player_public_informations info,information)
        foreach(player* play, getPlayer())
	    if(play->getId() == info.simplifiedId)
            {
                play->loadInfo(info);
                play->refresh();
            }
}

void gameData::haveDatapack()
{
    foreach(player* play,getPlayer())
    {
        play->changeSkin(play->skin());
    }
}

void gameData::newMessage(QString message)
{
    if(message.startsWith("/test"))
    {
        if(message == "/test center")
        {
            getSubClient()->centerOn(getThisPlayer()->getGraphicsItem());
        }
        else if(message.startsWith("/test move"))
        {
            getThisPlayer()->setX(message.split(" ").at(2).toInt());
            getThisPlayer()->setY(message.split(" ").at(3).toInt());
        }
    }
}

void gameData::newTick()
{
    emit everyRefresh();
    if(getThisPlayer()!=0)
        getSubClient()->centerOn(getThisPlayer()->getGraphicsItem());
}
