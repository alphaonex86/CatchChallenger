/*
 * craftClients.cpp
 * Copyright 2010, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 *
 * This file is part of the TMX Viewer example.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "craft-clients.h"

#include <QCoreApplication>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QStyleOptionGraphicsItem>
#include <QDebug>
#include <QKeyEvent>
#include <QDir>
#include <QScrollBar>
#include <QTime>

#include "gamedata.h"
#include "mainwindow.h"

using namespace Tiled;

class EmptyItem : public QGraphicsItem
{
public:
    EmptyItem(QGraphicsItem *parent = 0)
        : QGraphicsItem(parent)
    {
        setFlag(QGraphicsItem::ItemHasNoContents);
    }

    ~EmptyItem()
    {
        qWarning()<< "Deleting main item";
    }

    QRectF boundingRect() const { return QRectF(); }
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) {}
};

craftClients::craftClients(graphicsviewkeyinput *parent) :
    mScene(new QGraphicsScene())
{
        data = gameData::instance();
        gameData::destroyInstanceAtTheLastCall();
        noAction = false;
        data->showDebug("Sub-client created");
        Pokecraft_client* client = data->getClient();
        data->setSubClient(this);
        connect(parent,SIGNAL(keyPressed(QKeyEvent*)),this,SLOT(keyPressEvent(QKeyEvent*)));
        connect(parent,SIGNAL(keyRelease(QKeyEvent*)),this,SLOT(keyReleaseEvent(QKeyEvent*)));
        this->parent = parent;
        if(client!=0)
        {
            connect(client,SIGNAL(new_player_info(QList<Player_public_informations>)),data,SLOT(have_player_information(QList<Player_public_informations>)));
        }
        else
        {
            data->showWarning("No client detected");
        }
        data->setGraphicsItem(new EmptyItem());
        mScene = new QGraphicsScene(parent);
        parent->setScene(mScene);
        mScene->addItem(data->getGraphicsItem());
        if(mScene->items().length() == 0)
            data->showWarning("No item can be placed on Scene");
        parent->setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing
                                     | QGraphicsView::DontSavePainterState
                                     | QGraphicsView::IndirectPainting);
        parent->setBackgroundBrush(Qt::black);
        parent->setFrameStyle(QFrame::NoFrame);

        //parent->viewport()->setAttribute(Qt::WA_StaticContents);

        parent->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        parent->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        if(!parent)
        {
                qWarning() << "QGraphicsView not detected";
        }

}

craftClients::~craftClients()
{
}
void craftClients::check()
{
}

int craftClients::getPlayerZValue()
{
        int z = data->getThisPlayer()->getGraphicsItem()->zValue();
        return z;
}
void craftClients::setPlayerZValue(int z)
{
        data->getThisPlayer()->getGraphicsItem()->setZValue(z);
}

void craftClients::viewMap(QString fileName)
{
    data->getGraphicsItem()->setScale(2);
    catchMap(fileName);
    data->getThisPlayer()->setMap(data->getMap()[0]);;
    data->getThisPlayer()->setParentItem(data->getGraphicsItem());
    if(!mScene->items().contains(data->getThisPlayer()->getGraphicsItem()))
    {

            qWarning() << "Your player is NOT detected on the map";
            return;
    }
    parent->centerOn(data->getThisPlayer()->getGraphicsItem());
}

void craftClients::keyPressEvent(QKeyEvent * event)
{
        if(data->getThisPlayer() != NULL )
        {
                if(event->key()==Qt::Key_Up)
                {
                        anim(1);
                }
                else if(event->key()==Qt::Key_Right)
                {
                        anim(2);
                }
                else if(event->key()==Qt::Key_Down)
                {
                        anim(3);
                }
                else if(event->key()==Qt::Key_Left)
                {
                        anim(4);
                }
                else
                    data->showDebug("unknow key "+QString::number(event->key()));
        }
        else
            data->showWarning("Player not detected");
}
void craftClients::keyReleaseEvent(QKeyEvent *event)
{
        player* p = data->getThisPlayer();
        if(p!= NULL)
        {
            if(event->key()==Qt::Key_Up)
            {
                    p->stop(1);
            }
            else if(event->key()==Qt::Key_Right)
            {
                    p->stop(2);
            }
            else if(event->key()==Qt::Key_Down)
            {
                    p->stop(3);
            }
            else if(event->key()==Qt::Key_Left)
            {
                    p->stop(4);
            }
        }
}
void craftClients::update()
{
        if(data->getClient()==0)
        {
            return;
        }
        foreach(Player_public_informations info, data->getClient()->get_player_informations_list())
        {
                foreach(player*play,data->getPlayer())
                {
                        if(play->getId() == info.id)
                        {
                                play->changePlayerName(info.pseudo);
                                play->changeSkin(data->workingDir().canonicalPath()
                                                 +QString("/skin/mainCaracter/")
                                                 +info.skin
                                                 +QString("/trainer.png"));
                        }
                }
        }
}

void craftClients::anim(int ukey)
{
        if(data->getThisPlayer()!=0)
            data->getThisPlayer()->startMove(ukey);
        else
            data->showWarning("Your player isn't detected");
}
void craftClients::changeMap(MultiMap *map)
{
        player* selfplayer = data->getThisPlayer();
        qWarning() << "Loading map : " << map->getFileName();
        noAction = true;
        qDeleteAll(data->getMap());

        catchMap(map->getFileName());
        noAction = false;
        if(!mScene->items().contains(selfplayer->getGraphicsItem())&& data->getGraphicsItem()->childItems().contains(selfplayer->getGraphicsItem()))
        {

                qWarning()<< selfplayer->getId() << ": Your player is NOT detected on the map";
                return;
        }
}
MultiMap* craftClients::loadMap(QString map, int direction ,int px, int py, int ajust)
{
        foreach(MultiMap* dMap, data->getMap())
        {
                if(dMap
                        && (dMap->getFileName().contains(map)
                        || map.contains(dMap->getFileName())))
                {
                        return 0;
                }
                else if(!dMap
                                || dMap->getFileName().isEmpty())
                {
                        qWarning()<< "error on map " << map;
                }
        }

        MultiMap* x = new MultiMap(
                    QString(data->workingDir().canonicalPath() + QDir::fromNativeSeparators("/") + QString(map)),
                    0,
                    0);
        if(!x->getErrorString().isEmpty())
        {
                qWarning() << "Fail to load " << map << "cause of : "<< x->getErrorString();
                delete x;
                return 0;
        }
        if(direction != 0)
        {
                if(direction == 1)
                {
                        x->setX(px - (x->getMap()->width() * x->getMap()->tileWidth()));
                        x->setY(py + ajust);
                }
                else if(direction == 2)
                {
                        x->setX(px - (x->getMap()->width() * x->getMap()->tileWidth()));
                        x->setY(py + ajust);
                }
                else if(direction == 3)
                {
                        x->setX(px + ajust);
                        x->setY(py - (x->getMap()->height()*x->getMap()->tileWidth()));
                }
                else if(direction == 4)
                {
                        x->setX(px + ajust);
                        x->setY(py - (x->getMap()->height()*x->getMap()->tileWidth()));
                }
        }
        if(x->getMap()->indexOfLayer(QString("Tp"))!=-1)
        {
                int id = x->getMap()->indexOfLayer(QString("Tp"));
                Layer *tpLayer = x->getMap()->layers().at(id);
                if(dynamic_cast<ObjectGroup*>(tpLayer)!= 0)
                {
                        QString d;
                        if(direction ==1)
                        {
                            d = QString("border-right");
                        }
                        if(direction ==2)
                        {
                            d = QString("border-left");
                        }
                        if(direction ==3)
                        {
                            d = QString("border-bottom");
                        }
                        if(direction ==4)
                        {
                            d = QString("border-top");
                        }
                        ObjectGroup *mapObjectGroup = dynamic_cast<ObjectGroup*>(tpLayer);
                        int t;
                        for(t = 0 ; t < mapObjectGroup->objects().count() ; t++)
                        {
                                MapObject *object = mapObjectGroup->objects().at(t);
                                //mapObjectGroup->setVisible(false);
                                mapObjectGroup->setOpacity(0);
                                if(!object->property(QString("map")).isEmpty() && data->getMap().length()!= 0)
                                {
                                        foreach(MultiMap *dmap, data->getMap())
                                        {
                                                if( dmap->getFileName() == object->property(QString("map")))
                                                        continue;
                                        }
                                        if( d == object->type() || object->property(QString("map"))== map)
                                        {
                                            if(direction ==1 || direction == 2)
                                                x->setY(x->y() - object->y() * x->getMap()->tileWidth() + x->getMap()->tileWidth());
                                            if(direction ==4 || direction == 3)
                                                x->setX(x->x() - object->x() * x->getMap()->tileHeight());
                                            qWarning()<< x->getFileName();
                                            d = QString();
                                            continue;
                                        }
                                        if(object->type()==QString("border-left")  && direction != 1 && direction != 3)
                                        {
                                                /// left so   x = map.x - newmap.width   and y = object.y - object.proprety.y
                                                catchMap(QString("map/tmx/")+object->property(QString("map")),1,
                                                                             x->x(),
                                                                             x->y(),
                                                                             (object->y()-1-(object->property(QString("y")).toInt()))*x->getMap()->tileWidth());
                                        }
                                        else if(object->type()==QString("border-right") && direction != 2 && direction != 4)
                                        {
                                                /// right so  x = map.x - map.width      and y = object.y - object.proprety.y
                                                catchMap(QString("map/tmx/")+object->property(QString("map")),2,
                                                                             x->x(),
                                                                             x->y(),
                                                        ((object->y()-1-object->property(QString("y")).toInt()))*x->getMap()->tileWidth());
                                        }
                                        else if(object->type()==QString("border-top") && direction != 3 && direction != 1)
                                        {
                                                /// top so    y = map.y - newmap.height  and x =
                                                catchMap(QString("map/tmx/")+object->property(QString("map")),3,
                                                                             x->x(),
                                                                             x->y(),
                                                        (object->x()-(object->property(QString("x")).toInt()))*x->getMap()->tileHeight());
                                        }
                                        else if(object->type()==QString("border-bottom") && !object->property(QString("x")).isEmpty() && direction != 4 && direction != 2)
                                        {
                                                /// bottom so y = map.y - map.height     and x =
                                                catchMap(QString("map/tmx/")+object->property(QString("map")),4,
                                                                             x->x(),
                                                                             x->y(),
                                                        (object->x()-(object->property(QString("x")).toInt()))*x->getMap()->tileHeight());
                                        }

                                }
                                else
                                {
                                        if(object->type()!="spawn" && object->type()!="tp on push" && object->type()!="")
                                                qWarning() << object->type();
                                }
                        }
                }
        }
        return x;
}
void craftClients::setCurrentPlayer(Player_private_and_public_informations info)
{
        player* selfplayer = data->getThisPlayer();
        if(selfplayer != 0)
        {
                selfplayer->loadInfo(info.public_informations);
        }
        else if(selfplayer==0)
        {
                foreach(player *play, data->getPlayer())
                {
                        if(play->getId()==info.public_informations.id)
                        {
                                qWarning()<< "Map is loading ....";
                                data->setThisPlayer(play);
                                play->changePlayerName(info.public_informations.pseudo);
                                play->changeSkin(QString("skin/mainCaracter/")+info.public_informations.skin+QString("/trainer.png"));
                                connect(play, SIGNAL(new_player_movement(quint8,Direction)), this , SLOT(moveselfPlayer(quint8,quint8)));
                        }
                }
        }
}
void craftClients::insertPlayer(quint32 id,QString mapName,quint16 x,quint16 y,quint8 direction,quint16 speed)
{
        data->showDebug("Insert new player");
        foreach(player *dPlayer, data->getPlayer())
        {
                if(dPlayer->getId()==id)
                {
                        qWarning()<< "player already added : "<<id;
                        // if he has rejoin
                        dPlayer->cancelRemove();
                        return;
                }
        }
        bool checked = false;
        foreach(Player_public_informations info,data->getClient()->get_player_informations_list())
            if(info.id == id){
                checked = true;
                break;
            }
        if(!checked)
            data->getClient()->add_player_watching(id);
        MultiMap *tempMap = data->getMapByName(mapName);
        player *p = 0;
        if(tempMap==0)
        {
                if(id == data->getClient()->get_player_informations().public_informations.id )
                {
                        if(data->getMap().isEmpty())
                        {
                                catchMap(QString("map/tmx/")+mapName,0,0,0,0);
                        }
                        else
                        {
                                catchMap(QString("map/tmx/")+mapName,0,0,0,0);
                        }
                        tempMap = data->getMapByName(mapName);
                }
        }
        QString skin = QString("");
        foreach(Player_public_informations info, data->getClient()->get_player_informations_list())
        {
                if(info.id == id)
                {
                        skin = info.skin;
                        break;
                }
        }

        p = new player(QString(data->workingDir().canonicalPath()+"/skin/mainCaracter/"+skin+"/trainer.png"),
                              id,
                              mapName);
        p->setDirection(direction);
        int mapX = 0;
        int mapY = 0;
        if(tempMap!=0)
        {
            mapX = tempMap->x();
            mapY = tempMap->y();
        }
        if(data->getTileHeight()!=-1)
        {
            p->setX(x*data->getTileHeight()-mapX);
            p->setY(y*data->getTileHeight()-mapY);
        }
        else{
            // Temp fix
            p->setX(x*16-mapX);
            p->setY(y*16-mapY);
        }
        p->setSpeed(speed);
        p->refresh();
        p->setMap(tempMap);
        if(id == data->getPlayerInfo().public_informations.id && (data->getMap().length() == 0 || data->getMap().length() == 1) && data->getThisPlayer() == 0)
        {
                data->setThisPlayer(p);
        }
}
void craftClients::removePlayer(quint32 id)
{
        player *p = data->getPlayerById(id);
        if(p!=0)
        {
            p->startRemove();
        }
        else
        {
            qWarning()<<"player ["<<id<<"] not found";
        }
}
QString craftClients::getName(quint32 id)
{
        if(data->getClient()->get_player_informations_list().length()<=0)
        {
                qWarning()<< "no player informations";
        }
        foreach(Player_public_informations info,data->getClient()->get_player_informations_list())
        {
                if(info.id == id)
                {
                        return info.pseudo;
                }
        }
        data->getClient()->add_player_watching(id);
        return QString("Unknow player");
}

//heavy 14% + 18%
void craftClients::movePlayer(const quint32 &id,const QList<QPair<quint8, quint8> >& move)
{
        player* play = data->getPlayerById(id);
        if(data->getPlayerInfo().public_informations.id == id || play==0 || play==data->getThisPlayer())
        {
                return;
        }


        // -- fix
        if(play->getLastTilePos().x()==0 && play->getLastTilePos().y() == 0)
            play->posFix();
        play->animFix();
        play->stop(0);
        play->setTilePos(play->getLastTilePos().x(),play->getLastTilePos().y());

        QPair<quint8,quint8> mov;
        foreach(mov,move)
        {
                // each movement

                // -- start moving
                if(mov.second ==1 || mov.second ==5)
                {
                        play->setY(play->getGraphicsItem()->y() - mov.first*data->getTileHeight());
                }
                else if(mov.second ==2 || mov.second ==6)
                {
                        play->setX(play->getGraphicsItem()->x() + mov.first*data->getTileHeight());
                }
                else if(mov.second ==3 || mov.second ==7)
                {
                        play->setY(play->getGraphicsItem()->y() + mov.first*data->getTileHeight());
                }
                else if(mov.second ==4 || mov.second ==8)
                {
                        play->setX(play->getGraphicsItem()->x() - mov.first*data->getTileHeight());
                }
                // -- look good direction or move
                if(mov.second <5 && mov.second>0)
                {
                        play->stop(0);
                        play->setDirection(mov.second);
                }
                else
                {
                        play->startMove(mov.second-4);
                }
        }
}
void craftClients::checkItem(QString mapName)
{
        foreach(MultiMap* dMap,data->getMap())
        {
                if(dMap && dMap->getFileName() == mapName)
                {
                        foreach(Layer *layer,dMap->getMap()->layers())
                        {
                                if(ObjectGroup *group = layer->asObjectGroup())
                                {
                                        foreach(MapObject *object, group->objects())
                                        {
                                                if(object->type()==QString("border") && !object->property(QString("map")).isEmpty())
                                                {
                                                        if(object->x()==0 && !object->property(QString("y")).isEmpty())
                                                        {
                                                                /// left so   x = map.x - newmap.width   and y = object.y - object.proprety.y
                                                                catchMap(QString("map/tmx/")+object->property(QString("map")),1,
                                                                                             dMap->x(),
                                                                                             dMap->y(),
                                                                                             (object->y()-1-(object->property(QString("y")).toInt()))*dMap->getMap()->tileWidth());
                                                        }
                                                        else if(object->x()==dMap->getMap()->width()-1 && !object->property(QString("y")).isEmpty())
                                                        {
                                                                /// right so  x = map.x - map.width      and y = object.y - object.proprety.y
                                                                catchMap(QString("map/tmx/")+object->property(QString("map")),2,
                                                                                             dMap->x(),
                                                                                             dMap->y(),
                                                                        ((object->y()-1-object->property(QString("y")).toInt()))*dMap->getMap()->tileWidth());
                                                        }
                                                        else if(object->y()==1 && !object->property(QString("x")).isEmpty())
                                                        {
                                                                /// top so    y = map.y - newmap.height  and x =
                                                                catchMap(QString("map/tmx/")+object->property(QString("map")),3,
                                                                                             dMap->x(),
                                                                                             dMap->y(),
                                                                        (object->x()-(object->property(QString("x")).toInt()))*dMap->getMap()->tileHeight());
                                                        }
                                                        else if(object->y()==dMap->getMap()->height() && !object->property(QString("x")).isEmpty())
                                                        {
                                                                /// bottom so y = map.y - map.height     and x =
                                                                catchMap(QString("map/tmx/")+object->property(QString("map")),4,
                                                                                             dMap->x(),
                                                                                             dMap->y(),
                                                                        (object->x()-(object->property(QString("x")).toInt()))*dMap->getMap()->tileHeight());
                                                        }
                                                }
                                        }
                                }
                                return;
                        }
                }
        }
}
void craftClients::catchMap(QString map, int direction, int px, int py, int ajust)
{
        foreach(MultiMap* umap, data->getMap())
        {
            if(umap->getFileName() == map
                    || QString(umap->getFileName()).remove(data->workingDir().canonicalPath()) == map)
            {
                return;
            }
        }
        loadMap(map, direction, px, py, ajust);
}
void craftClients::moveselfPlayer(quint8 first, Direction second)
{
        data->getClient()->send_player_move(first, (quint8) second);
}

void craftClients::centerOn(QGraphicsItem *item)
{
    parent->centerOn(item);
}
