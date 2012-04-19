/*
 * craft-clients.h
 * 
 */

#ifndef CRAFTCLIENTS_H
#define CRAFTCLIENTS_H

#include "player.h"
#include "MultiMap.h"
#include "graphicsviewkeyinput.h"
#include "../pokecraft-client/Pokecraft_client.h"

#include <QGraphicsView>
#include <QGraphicsScene>

namespace Tiled {
class MapRenderer;
class TileLayer;
class ObjectGroup;
}

class EmptyItem;
class craft_client;

class craftClients : public QObject
{
    Q_OBJECT

public:
    explicit craftClients(graphicsviewkeyinput *parent = 0);
    ~craftClients();

    void viewMap(QString fileName);
    void setPlayerZValue(int z);
    //int getTileHeight();
    QBool hasCellInLayerName(QString LayerName,int x,int y);
    QBool hasCellInLayer(Tiled::TileLayer *Layer,int x,int y);
    QBool hasCellInLayerName(QList<QString> LayerNameList,int x,int y);
    QBool hasCellInLayer(Tiled::ObjectGroup *Group,int x,int y);
    int getKey();
    void anim(int ukey);
    void centerOn(QGraphicsItem* item);
    QString getName(quint32 id);
    void checkItem(QString mapName);
    MultiMap* loadMap(QString map, int direction = 0,int px = 0, int py =0, int ajust = 0);
    void catchMap(QString map, int direction = 0,int px = 0, int py =0, int ajust = 0);

private:
    graphicsviewkeyinput *parent;
    QGraphicsScene *mScene;
    Tiled::MapRenderer *mRenderer;
    int getPlayerZValue();
    QKeyEvent * event;
    int key;
    void changeMap(MultiMap *map);
    bool noAction;
    QString lastMapName;
    QStringList fileSended;
    QStringList MapWaiting;
    gameData* data;
    
public slots:
    void update();
    void keyPressEvent(QKeyEvent * event);
    void keyReleaseEvent(QKeyEvent *event);
    void setCurrentPlayer(Player_private_and_public_informations info);
    void insertPlayer(quint32 id,QString mapName,quint16 x,quint16 y,quint8 direction,quint16 speed);
    void removePlayer(quint32 id);
    void movePlayer(const quint32 &id,const QList<QPair<quint8, quint8> >& move);
    void moveselfPlayer(quint8 first,Direction second);
    void check();
signals:
    void mapNeeded(QString name);

};

#endif // CRAFTCLIENTS_H
