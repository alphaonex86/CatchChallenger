/*
 * player.h
 * 
 */

#ifndef PLAYER_H
#define PLAYER_H

#include <QCoreApplication>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QStyleOptionGraphicsItem>
#include <QDebug>
#include <QKeyEvent>
#include <QWidget>
#include <Qt>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QTimer>

#include "../pokecraft-general/GeneralStructures.h"
#include "MultiMap.h"
#include "spriteimage.h"
#include "MoveClientEmiter.h"

class craftClients;

class player : public QObject
{
    Q_OBJECT
public:
	player(QString path, quint32 id = 0, QString MapName = QString());
    ~player();  
    void setTilePos(int x, int y);
    void refresh();
    void moveTo();
    //void moveTo( QList<QPair<quint8, quint8 > > move );
    void setDirection(int direction);
    QString getSkinName();
    QString getPlayerName();
    QGraphicsItem* getTextItem();
    quint32 getId();
    void setMove(int x, int y, int lenght);
    QString skin(){return skinPath;}
    void startMove(int direction);
    int getDirection(){return mDirection;}
    int getTileWalked();
    void posFix();
    void animFix();
    QString getMapName(){return mapName;}
    //void resetTileWalked(){last_step = 0;}
    void setZValue(int z);
    qreal zValue();
    void setParentItem(QGraphicsItem *item);
    QGraphicsItem* getGraphicsItem();
    void setId(quint32 id);
    void setInfo(Player_public_informations info);
    void loadInfo(Player_public_informations info);
    void send_New_Direction(quint8 the_new_direction, bool move);
    QPointF getLastTilePos();
    void setX(int x);
    void setY(int y);
    void setSpeed(quint16 speed);
    bool mainPlayer();
public slots:
    void check();
    void changePlayerName(QString Name);
    bool changeSkin(QString skinPath);
    void changeColor(QColor color);
    void stop( int direction);
    void setMap(MultiMap *map);
    void get_player_move(quint8 move, Direction direc);
    void startRemove();
    void cancelRemove();
signals:
    void move(quint8 move,quint8 direction);
private:

    bool stopped;
    int animation;
    int animTrame;
    int ImageTrame;
    int mDirection;
    int waitingWay;
    int spriteStatus;
    MoveClientEmiter *moveClientEmiter;
    MultiMap *mMap;
    QGraphicsTextItem *textitem;
    QList<QPair<quint8,quint8> > moveList;
    QString mapName;
    QString skinPath;
    Player_public_informations data;
    spriteImage *sprite;
    QPointF pos;
    QTimer *timer;

    QPointF nextStepPos();
    bool mapLoaded();

    QPair<bool,bool> movePossible(int x, int y);
    QPair<bool,bool> movePossible(int x, int y, int direction);
    QPair<bool,bool> movePossible();
    QPair<bool,bool> movePossible(int direction);
    bool movePossible(QString LayerName, int x,int y);
    bool hasCellInLayer(Tiled::ObjectGroup *Group,int x,int y);
    bool hasCellInLayer(Tiled::TileLayer *Layer,int x,int y);
    bool hasCellInLayerName(QList<QString> layer,int x, int y);
};

#endif // PLAYER_H
