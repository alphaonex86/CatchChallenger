#include "player.h"

#include "isometricrenderer.h"
#include "map.h"
#include "mapobject.h"
#include "mapreader.h"
#include "objectgroup.h"
#include "orthogonalrenderer.h"
#include "tilelayer.h"
#include "tileset.h"
#include "craft-clients.h"
#include "gamedata.h"

player::player(QString skin, quint32 id,QString mapName)
{
        timer = new QTimer();
        sprite = new spriteImage(0);
        textitem = new QGraphicsTextItem(sprite);
        waitingWay = 0;
        mDirection = 1;
        animTrame = 10;
        animation = 0;
        mMap = 0;
	skinPath = skin;

	gameData* data = gameData::instance();
	this->data.simplifiedId = id;


        timer->setInterval(1000/32);
        connect(timer,SIGNAL(timeout()),this,SLOT(check()),Qt::QueuedConnection);
        spriteStatus = 1;
        sprite->setOpacity(0.0);
        timer->start();

        // sprite
        sprite->setZValue(0x2a);
        sprite->setOffset(0,-12);
        sprite->setParentItem(data->getGraphicsItem());

        if(sprite->pixmap().isNull() || skinPath.isEmpty())
        {
            data->showError(QString("Player skin can't be loaded: %1").arg(data->getPlayerInfo().public_informations.skin));
            changeSkin(":/images/player_default/trainer.png");
        }

        textitem->setY(-40);
        textitem->setScale(0.85);

        stopped = false;
        refresh();

        // map
        mMap = data->getMapByName(mapName);
        mapName = mapName;

        // other data
	/*foreach(Player_public_informations info,data->getClient()->get_player_informations_list())
        {
            if(info.id == id)
            {
                loadInfo(info);
                break;
            }
        }
        if(getPlayerName().isEmpty()
                || this->data.skin.isEmpty())
	    data->getClient()->add_player_watching(id);*/
        data->addPlayer(this);
        data->showDebug("New player ["
                        +QString::number(id)
                        +"] "
                        +getPlayerName());

        gameData::destroyInstanceAtTheLastCall();
}

player::~player()
{
    if(sprite!=0)
    {
        sprite->setParentItem(0);
        delete sprite;
        sprite = 0;
    }
    gameData* data = gameData::instance();
    data->removePlayer(this);
    gameData::destroyInstanceAtTheLastCall();
}

/// \brief order to stop in a direction
void player::stop(int direction)
{
        if(mDirection == direction && direction !=0)
        {
                // he already move and he has order to stop where he go
                this->stopped = false;
        }
        else if(direction == 0)
        {
                // force stopping
                this->stopped = false;
                animTrame = 10;
                this->refresh();
        }
        if(waitingWay==direction || direction == 0)
        {
            // no other move
            waitingWay = 0;
        }
}

/// \brief refresh the skin
void player::refresh()
{
    if(!sprite->isNull() && ( animTrame == 0 || animTrame == 10 || animTrame == 20 ) )
    {
            // look if the sprite need to be refresh
            sprite->setSprite((animTrame/10) + (mDirection-1)*3);
            animTrame++;
    }
	else
	{
        if(!skinPath.isEmpty() && sprite->isNull())
		{
			if(QFile::exists(skinPath))
				changeSkin(skinPath);
			else
				changeSkin(":/images/player_default/trainer.png");
        }
	}
}

/// \brief set position in tile
void player::setTilePos(int x, int y)
{
        int tile = gameData::OneTimeInstance()->getTileHeight();
        if(tile!=-1)
        {
            // set the new position (in px)
            sprite->setPos(x * tile,
                           y * tile);
        }
}

/// \brief Check and order to move to the next tile
void player::moveTo()
{
        // data of the game
        gameData* data = gameData::instance();
        // what can he do
        QPair<bool,bool> Tmove = movePossible();
        if(Tmove.first)
        {
                int tile = data->getTileHeight();
                if(tile==-1)
                {
                    // no map loaded
                    tile = 16;
                }
                // he can move
                send_New_Direction(mDirection,true);
                if(Tmove.second)
                {
                        // he "jump"
                        animation = tile*2;
                        send_New_Direction(mDirection,true);
                }
                else
                {
                        // he move normaly
                        animation = tile*1;
                }
                check();
        }
        else
        {
                // can't move
                stop(mDirection);
        }
        gameData::destroyInstanceAtTheLastCall();
}

/// \brief Every tick for movement
void player::check()
{
        if(spriteStatus!=0)
        {
            if(spriteStatus ==1)
            {
                if(sprite->opacity() > 1.0)
                {
                    spriteStatus =0;
                }
                else
                {
                    sprite->setOpacity(sprite->opacity()+0.03);
                }
            }
            else if(spriteStatus ==2)
            {
                if(sprite->opacity() < 0.0)
                {
                    delete this;
                    return;
                }
                sprite->setOpacity(sprite->opacity()-0.03);
            }
            else
                spriteStatus = 0;
        }
        if(animation>0)
        {
                // he move
                if(mDirection == 1)
                        sprite->moveBy(0 ,-1);
                else if(mDirection == 2)
                        sprite->moveBy(1 ,0 );
                else if(mDirection == 3)
                        sprite->moveBy(0 ,1 );
                else if(mDirection == 4)
                        sprite->moveBy(-1,0 );
                // animation and sprite delay of refresh
                animation--;
                animTrame++;
                if(animTrame==30)
                        animTrame = 0;
                this->refresh();
        }
        else if(animation < 0)
        {
                // time before movement
                animation++;
                animTrame = 10;
                refresh();
                if(animation==-1)
                {
                    int tile = gameData::instance()->getTileHeight();
                    gameData::destroyInstanceAtTheLastCall();
                    if(tile==-1)
                    {
                        // no map loaded
                        tile = 16;
                    }
                    // he can move
                    send_New_Direction(mDirection,true);
                    // he move normaly
                    animation = tile*1;
                }
        }
        else if(waitingWay!=0)
        {
                // new direction
                if(movePossible(waitingWay).first)
                {
                    stopped    = true;
                    mDirection = waitingWay;
                    animation  = -6;
                    refresh();
                }
                else
                {
                    stopped    = true;
                    mDirection = waitingWay;
                    refresh();
                }
                waitingWay = 0;
        }
        else if(stopped)
        {
                // try to move to the next step
                moveTo();
        }
        else if(animTrame != -1)
        {
                // end of move
                animTrame = 10;
                this->refresh();
                if(spriteStatus==0)
                    timer->stop();
                animTrame = -1;
                sprite->refresh();
                send_New_Direction(mDirection,false);
        }
}

/// \brief set a direction to the player
void player::setDirection(int direction)
{
/*        mDirection = direction;
        if(mDirection >4)
            mDirection -=4;
        if(moveClientEmiter ==0 && mainPlayer())
        {
            if(mDirection==1)
               moveClientEmiter = new MoveClientEmiter(Direction_look_at_top);
            else if(mDirection==2)
                moveClientEmiter = new MoveClientEmiter(Direction_look_at_right);
            else if(mDirection==3)
                moveClientEmiter = new MoveClientEmiter(Direction_look_at_bottom);
            else if(mDirection==4)
                moveClientEmiter = new MoveClientEmiter(Direction_look_at_left);
            else
            {
                qWarning()<< "Unknow direction "<<direction;
                return;
            }
            gameData* data = gameData::instance();
            connect(moveClientEmiter,
                    SIGNAL(new_player_movement(quint8,Direction)),
                    data->getSubClient(),
                    SLOT(moveselfPlayer(quint8,Direction)));
            gameData::destroyInstanceAtTheLastCall();
	}*/
}

/// \brief return player skin
QString player::getSkinName()
{
        if(!skinPath.isNull() && !skinPath.isEmpty())
        {
                return skinPath;
        }
        return QString();
}

/// \brief Change the player name
void player::changePlayerName(QString Name)
{
        textitem->setPlainText(Name);
        textitem->setX(textitem->toPlainText().length()*2);
        data.pseudo = Name;
}

/// \brief change player skin
bool player::changeSkin(QString path)
{
        this->skinPath = path;
        sprite->setPixmap(path);
        if(!sprite->isNull())
        {

                sprite->setSpriteHeight(sprite->pixmap().height()/4);
                sprite->setSpriteWidth(sprite->pixmap().width()/3);
                this->refresh();
		return true;
        }
        else
		return false;
}

/// \brief change player name color
void player::changeColor(QColor color)
{
        textitem->setDefaultTextColor(color);
}

/// \brief order one player to move in a direction
void player::startMove(int direction)
{
        if(!mapLoaded())
            return;
        if(mMap == 0)
        {
            if(!mapName.isEmpty())
            {
                mMap  = gameData::instance()->getMapByName(mapName);
                gameData::destroyInstanceAtTheLastCall();
                if(mMap==0)
                {
                    gameData::instance()->showWarning("No map detected");
                    gameData::destroyInstanceAtTheLastCall();
                    return;
                }
            }
            else
            {
                gameData::instance()->showWarning("No map detected");
                gameData::destroyInstanceAtTheLastCall();
                return;
            }
        }
        if(direction <= 0)
                return;
        if(animation != 0 && waitingWay== 0)
        {
                if(mDirection != direction)
                {
                        this->waitingWay = direction;
                }
                return;
        }
        if(!stopped && animation!=0)
                return;
        if(stopped == true)
        {
                return;
        }
        animFix();
        gameData* gData = gameData::instance();
        int tile = gData->getTileHeight();
        if(tile == -1)
        {
            tile = 16;
        }
        pos.setX(sprite->x()/tile);
        pos.setY(sprite->y()/tile);
        timer->start();
        int oldDirection = mDirection;
        mDirection = direction;
        gameData::destroyInstanceAtTheLastCall();
        if(!movePossible(direction).first)
        {
                send_New_Direction(mDirection,false);
                animTrame = 10;
                return;
        }
        stopped = true;
        if(animation == 0 && mainPlayer() && oldDirection != mDirection)
        {
                // time for looking at somewhere
                animation = -6;
        }
}

qreal player::zValue()
{
        return sprite->zValue();
}

void player::setZValue(int z)
{
        sprite->setZValue(z);
}

void player::setParentItem(QGraphicsItem *item)
{
        sprite->setParentItem(item);
}

QGraphicsItem* player::getGraphicsItem()
{
        return sprite;
}

/// \brief return pseudo of player
QString player::getPlayerName()
{
    return data.pseudo;
}

/// \brief ...
void player::get_player_move(quint8 movement, Pokecraft::Direction direction)
{
    gameData *data = gameData::instance();
    emit move( movement, quint8(direction));
    int tile = data->getTileHeight();
    if(tile ==-1)
        tile = 16;
    pos.setX(sprite->x()/tile);
    pos.setY(sprite->y()/tile);
    gameData::destroyInstanceAtTheLastCall();
}

/// \brief called when a new direction is detected or every move
void player::send_New_Direction(quint8 the_new_direction, bool move)
{
    /*if(!move)
    {
	moveClientEmiter->send_player_move(Direction(the_new_direction));
    }
    else
    {
	moveClientEmiter->send_player_move(Direction(the_new_direction+4));
        gameData* data = gameData::instance();
        data->moveMap(-sprite->x(),-sprite->y());
        gameData::destroyInstanceAtTheLastCall();
    }*/
}

/// \brief return number of tile walked
int player::getTileWalked()
{
    return -1;
}

QPointF player::getLastTilePos()
{
    return pos;
}

// *********** Colision Check ************
bool player::movePossible(QString LayerName,int x, int y)
{
        int id = mMap->getMap()->indexOfLayer(LayerName) ;
        // look if the layer exist
        if(id!=-1)
        {
                Layer *mLayer = mMap->getMap()->layers().at(id);
                if(TileLayer *TileLayerUsed = dynamic_cast<TileLayer*>(mLayer))
                {
                        return !hasCellInLayer(TileLayerUsed,x-mMap->x(),y-mMap->y());
                }
                else if(ObjectGroup *ObjectGroupUsed = dynamic_cast<ObjectGroup*>(mLayer))
                {
                        return !hasCellInLayer(ObjectGroupUsed,x-mMap->x(),y-mMap->y());
                }
        }
        return true;
}
bool player::hasCellInLayer(Tiled::TileLayer *Layer,int x,int y)
{
        // if position is in the Layer
        if(Layer->contains(x,y))
                // If cell in the layer at the position is undefined
                if( Layer->cellAt(x,y)!=Cell())
                        return true;
        return false;
}
bool player::hasCellInLayer(Tiled::ObjectGroup *Group,int x,int y)
{
        // if position is in the Layer
        if(Group->objectCount()>0)
                foreach(MapObject *Object,Group->objects())
                // If cell in the layer at the position is undefined
                        if( Object->position() == QPointF(x,y+1) && Object->type() !=QString("door"))
                                return true;
        return false;
}
bool player::hasCellInLayerName(QList<QString> LayerNameList,int x,int y)
{
        // look if the layer exist
        foreach(QString layerName,LayerNameList)
        {
                if(!movePossible(layerName,x,y))
                {
                        return true;
                }
        }
        return false;
}
QPair<bool,bool> player::movePossible()
{
        return movePossible(mDirection);
}

QPair<bool,bool> player::movePossible(int direction)
{
        int tile = gameData::instance()->getTileHeight();
        if(tile==-1)
            tile=16;
        int px = sprite->x()/tile;
        int py = sprite->y()/tile;
        px    -= mMap->x()/tile;
        py    -= mMap->y()/tile;
        gameData::destroyInstanceAtTheLastCall();
        if(direction==1){ py -= 1;}
        else if(direction==2){px+= 1;}
        else if(direction==3){py+= 1;}
        else if(direction==4){px-= 1;}
        return movePossible(px,py,direction);
}

QPair<bool,bool> player::movePossible(int x, int y)
{
    return movePossible(x,y,mDirection);
}

//heavy 18%
QPair<bool,bool> player::movePossible(int x, int y, int direction)
{
        if(mMap!=0)
        {
                gameData *data = gameData::instance();
                int tile = data->getTileHeight();
                int px = x*tile + mMap->x();
                int py = y*tile + mMap->y();
                if(!mMap->contains(px,py))
                {
                    foreach(MultiMap *map,data->getMap())
                    {
                        if(mMap != map && map->contains(px,py))
                        {
                            mMap = map;
                            gameData::destroyInstanceAtTheLastCall();
                            data->showDebug("Map : "+map->getFileName());
                            if(mainPlayer())
                            {
                                // load this map
                                ObjectGroup* objGr = 0;
                                QList<MultiMap*> mapList = data->getMap();
                                mapList.removeOne(mMap);
                                foreach(Layer * l,map->getMap()->layers())
                                {
                                    if(l->name()==QString("Tp")
                                            && objGr == 0
                                            && dynamic_cast<ObjectGroup*>(l)!=0)
                                    {
                                        objGr = dynamic_cast<ObjectGroup*>(l);
                                        foreach(MapObject *obj,objGr->objects())
                                        {
                                            if(obj->type().contains(QString("border")))
                                            {
                                                bool needLoad = true;
                                                foreach(MultiMap* map,mapList)
                                                {
                                                    if(map->getFileName().contains(obj->property(QString("map"))))
                                                    {
                                                        needLoad = false;
                                                        mapList.removeOne(map);
                                                        break;
                                                    }
                                                }
                                                if(needLoad)
                                                {
                                                    MultiMap *x = mMap;
                                                    if(obj->type()==QString("border-left"))
                                                    {
                                                            /// left so   x = map.x - newmap.width   and y = obj.y - obj.proprety.y
                                                            data->getSubClient()->catchMap(QString("map/tmx/")+obj->property(QString("map")),1,
                                                                     x->x(),
                                                                     x->y(),
                                                                     (obj->y()-1-(obj->property(QString("y")).toInt()))*x->getMap()->tileWidth());
                                                    }
                                                    else if(obj->type()==QString("border-right"))
                                                    {
                                                            /// right so  x = map.x - map.width      and y = obj.y - obj.proprety.y
                                                            data->getSubClient()->catchMap(QString("map/tmx/")+obj->property(QString("map")),2,
                                                                     x->x(),
                                                                     x->y(),
                                                                     ((obj->y()-1-obj->property(QString("y")).toInt()))*x->getMap()->tileWidth());
                                                    }
                                                    else if(obj->type()==QString("border-top"))
                                                    {
                                                            /// top so    y = map.y - newmap.height  and x =
                                                            data->getSubClient()->catchMap(QString("map/tmx/")+obj->property(QString("map")),3,
                                                                     x->x(),
                                                                     x->y(),
                                                                     (obj->x()-(obj->property(QString("x")).toInt()))*x->getMap()->tileHeight());
                                                    }
                                                    else if(obj->type()==QString("border-bottom") && !obj->property(QString("x")).isEmpty())
                                                    {
                                                            /// bottom so y = map.y - map.height     and x =
                                                            data->getSubClient()->catchMap(QString("map/tmx/")+obj->property(QString("map")),4,
                                                                     x->x(),
                                                                     x->y(),
                                                                     (obj->x()-(obj->property(QString("x")).toInt()))*x->getMap()->tileHeight());
                                                    }
                                                }
                                            }
                                        }
                                        foreach(MultiMap *map,mapList)
                                        {
                                            qDeleteAll(map->getItems());
                                            delete map;
                                        }

                                        break;
                                    }
                                }
                            }
                            return movePossible(px/tile,py/tile,direction);
                        }
                    }
                    gameData::destroyInstanceAtTheLastCall();
                    data->showDebug("No map for the new movement...");
                    return QPair<bool,bool>(false,false);
                }
                QList<QString> layerList;
                if(direction!=2)
                        layerList.append(QString("LedgesDown"));
                if(direction!=3)
                        layerList.append(QString("LedgesLeft"));
                if(direction!=4)
                        layerList.append(QString("LedgesRight"));
                if(mainPlayer())
                {
                    // Tp check
                    foreach(Layer *l,mMap->getMap()->layers())
                    {
                        QPointF pos = QPointF(x,y+1);
                        if(l->name()==QString("Object") && l->asObjectGroup()!=0)
                        {
                            foreach(MapObject* object,l->asObjectGroup()->objects())
                            {
                                if(object->position() == pos)
                                {
                                    qWarning()<<object->type() <<" "<<object->properties();
                                    /// \todo change map
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
                gameData::destroyInstanceAtTheLastCall();
                return QPair<bool,bool>(mMap->canWalkAt(x,y),
                                        hasCellInLayerName(layerList,x,y));
        }
        else
        {
            gameData::instance()->showDebug("No map detected for "+QString::number(getId()));
            gameData::destroyInstanceAtTheLastCall();
            return QPair<bool,bool>(false,false);
        }
        return QPair<bool,bool>(false,false);
}

// ****** other *****
/// \brief return the player name's item
QGraphicsItem* player::getTextItem()
{
    return this->textitem;
}

/// \brief return player id
quint32 player::getId()
{
    return this->data.simplifiedId;
}

/// \brief set the id of the player
void player::setId(quint32 id)
{
    this->data.simplifiedId = id;
}

/// \brief set the info of the player
void player::setInfo(Pokecraft::Player_public_informations info)
{
    this->data = info;
}

/// \brief load the info of the player
void player::loadInfo(Pokecraft::Player_public_informations info)
{
    this->setId( info.simplifiedId );
    this->changePlayerName(info.pseudo);
    QDir dir = QDir();
    dir.cd(gameData::instance()->workingDir().canonicalPath());
    gameData::destroyInstanceAtTheLastCall();
    dir.cd("skin");
    dir.cd("mainCaracter");
    dir.cd(info.skin);
    dir.filePath("trainer.png");
    if(!changeSkin(dir.filePath("trainer.png")))
	    changeSkin(":/images/player_default/trainer.png");
    refresh();
    switch(info.type)
    {
    case Pokecraft::Player_type_premium:
        this->changeColor(QColor(0,0,0));
    break;
    case Pokecraft::Player_type_gm:
        this->changeColor(QColor(155,155,155));
    break;
    case Pokecraft::Player_type_dev:
        this->changeColor(QColor(155,255,155));
    break;
    case Pokecraft::Player_type_normal:
    default:
        this->changeColor(QColor(0,0,0));
    break;
    }
}

/// \brief fix animation probleme
void player::animFix()
{
    animation = 0;
}

/// \brief fix position probleme
void player::posFix()
{
    pos = nextStepPos();
}

/// \brief set the x of the player
void player::setX(int x)
{
    gameData* data = gameData::instance();
    sprite->setX(x);
    if(data->getTileHeight()!=-1)
        pos.setX(x/data->getTileHeight());
    else
        pos.setX(x/16);
    if(mMap==0)
    {
        setMap(data->getMapByName(mapName));
    }
    gameData::destroyInstanceAtTheLastCall();
}

/// return the position of the next step
QPointF player::nextStepPos()
{
    QPointF stepPos = sprite->pos();
    if(mDirection == 1)
            stepPos.setY(stepPos.y()-1);
    else if(mDirection == 2)
            stepPos.setX(stepPos.x()+1);
    else if(mDirection == 3)
            stepPos.setY(stepPos.y()+1);
    else if(mDirection == 4)
            stepPos.setX(stepPos.x()-1);
    if(stepPos!=sprite->pos())
    {
        return stepPos;
    }
    return QPointF();
}

void player::setY(int y)
{
    gameData* data = gameData::instance();
    sprite->setY(y);
    if(data->getTileHeight()!=-1)
        pos.setY(y/data->getTileHeight());
    else
        pos.setY(y/16);
    gameData::destroyInstanceAtTheLastCall();
}

/// \returns true if the player is controled by this computer
bool player::mainPlayer()
{
    gameData* data = gameData::instance();
    if(this == data->getThisPlayer()
	    ||data->getPlayerInfo().public_informations.simplifiedId == getId())
    {
        gameData::destroyInstanceAtTheLastCall();
        return true;
    }
    gameData::destroyInstanceAtTheLastCall();
    return false;
}

/// \returns true if a map has been correctly loaded
bool player::mapLoaded()
{
    gameData* data = gameData::instance();
    if(data->getMapRenderer()==0)
    {
        gameData::destroyInstanceAtTheLastCall();
        return false;
    }
    gameData::destroyInstanceAtTheLastCall();
    return true;
}

/// \brief set the map where is the player
void player::setMap(MultiMap *map)
{
    if(map!=0)
    {
        this->mMap = map;
        setX(sprite->x()+map->x());
        setY(sprite->y()+map->y());
    }
}

/// \brief set the speed of movement of the player
void player::setSpeed(quint16 speed)
{
    int tile = gameData::instance()->getTileHeight();
    if(tile != -1)
        timer->setInterval(speed/tile/2+1);
    else
        timer->setInterval(speed/32+1);
    if(mainPlayer())
    {
        gameData::instance()->setTickSpeed(timer->interval());
        gameData::destroyInstanceAtTheLastCall();
    }
    gameData::destroyInstanceAtTheLastCall();
}

/// \brief order to start remove the player
void player::startRemove()
{
    spriteStatus = 2;
}

/// \brief cancel last remove order
void player::cancelRemove()
{
    spriteStatus = 1;
}
