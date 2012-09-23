#include "MultiMap.h"

#include "tileset.h"
#include "gamedata.h"

MultiMap::MultiMap(Tiled::Map *map, QString fileName, int x, int y)
{
        gameData* data = gameData::instance();
        mFileName = fileName;
        this->map = map;
        if(!map)
        {
                data->showError(QString("Fail to load Map with the path : ")+ fileName);
                this->map = 0;
                gameData::destroyInstanceAtTheLastCall();
                return;
        }
        mX = x;
        mY = y;
        mZ = 0;
        view();
        gameData::destroyInstanceAtTheLastCall();
}
MultiMap::MultiMap(QString fileName, int x, int y)
{
        mFileName    = fileName;
        mX = x;
        mY = y;
        mZ = 0;
        MapReader reader = MapReader();
	Tiled::Map *map = reader.readMap(QString(fileName));
        if(map==0)
        {
                this->mErrorString = reader.errorString();
                this->map = 0;
                delete map;
                return;
        }
        this->map = map->clone();
        view();
        delete map;
}
void MultiMap::view()
{
        gameData* data = gameData::instance();
        if(map == 0 || map->layers().isEmpty() || !data->mapSupported(map))
        {
            data->showWarning("can't load correctly the map " + mFileName);
            gameData::destroyInstanceAtTheLastCall();
            return;
        }
        data->showDebug((QString("Loading map  [")+mFileName+QString("]")));
        data->addMap(this);
        int i = -1;
        foreach(Layer* layer,map->layers()) {
            if(layer == 0)
            {
                gameData::destroyInstanceAtTheLastCall();
                return;
            }
            i++;
            if(dynamic_cast<TileLayer*>(layer)!=0){
                    TileLayerItem *x = new TileLayerItem(dynamic_cast<TileLayer*>(layer), data->getGraphicsItem());
                    if(layer->name()=="Water"){x->setZValue(0x10+mZ);}
                    else if(layer->name()=="Collisions"){x->setZValue(0x20+mZ);}
                    else if(layer->name()=="Grass"){x->setZValue(0x08+mZ);}
                    else if(layer->name()=="WalkBehind"){x->setZValue(0x30+mZ);}
                    else{data->showDebug("unknow layer "+layer->name()); x->setZValue(i);}
                    x->setZValue(x->zValue()+mZ);
                    x->setX(mX);
                    x->setY(mY);
                    x->setVisible(layer->isVisible());
                    itemList.append(x);

            } else if(ObjectGroup *objectGroup = dynamic_cast<ObjectGroup*>(layer)){
                    if( objectGroup->name()!=QString("Tp"))
                    {
                        ObjectGroupItem *a = new ObjectGroupItem(objectGroup, data->getGraphicsItem());
                        a->setX(mX);
                        a->setY(mY);
                        a->setZValue(0x25);
                        a->setVisible(layer->isVisible());
                        a->setOpacity(objectGroup->opacity());
                        itemList.append(a);
                    }
            }
        }
        // generate colision check
        QStringList layerCheck;
        layerCheck.append(QString("Walkable"));
        layerCheck.append(QString("Collisions"));
        layerCheck.append(QString("Water"));
        layerCheck.append(QString("Object"));
        for(int x=0; x<map->width();x++)
        {
            for(int y=0; y<map->height();y++)
            {
                foreach(Layer* layer,map->layers()) {
                    if(layerCheck.contains(layer->name()))
                    {
                        if(dynamic_cast<TileLayer*>(layer)!=0){
                            TileLayer *z = dynamic_cast<TileLayer*>(layer);
                            if(layer->name()!="Walkable")
                            {
                                if(!z->cellAt(x,y).isEmpty())
                                {
                                    colision.append(QPair<int,int>(x,y));
                                    break; // foreach layer
                                }
                            }
                            else
                            {
                                if(z->cellAt(x,y).isEmpty())
                                {
                                    colision.append(QPair<int,int>(x,y));
                                    break; // foreach layer
                                }
                            }
                        }
                        else if(dynamic_cast<ObjectGroup*>(layer)!=0)
                        {
                            bool doBreak = false;
                            ObjectGroup *z = dynamic_cast<ObjectGroup*>(layer);
                            if(z->objectCount()>0)
                            {
                                foreach(MapObject *Object,z->objects())
                                {
                                    // If cell in the layer at the position is undefined
                                    if( Object->position() == QPointF(x,y+1) && Object->type() !=QString("door"))
                                    {
                                        colision.append(QPair<int,int>(x,y));
                                        doBreak = true;
                                        break; // foreach Object
                                    }
                                }
                            }
                            if(doBreak)
                                break; // foreach layer
                        }
                    }
                }
            }
        }
        gameData::destroyInstanceAtTheLastCall();
}

MultiMap::~MultiMap()
{
        gameData::instance()->removeMap(this);
        gameData::destroyInstanceAtTheLastCall();
        qDeleteAll(map->tilesets());
        delete map;
}
void MultiMap::setX(int x)
{
        foreach(QGraphicsItem *item,itemList){
                item->setX(x);
        }
        mX = x;
}
void MultiMap::setY(int y)
{
        foreach(QGraphicsItem *item,itemList){
                item->setY(y);
        }
        mY = y;
}
void MultiMap::setZOffset(int z)
{
        foreach(QGraphicsItem *item,itemList){
                item->setZValue(item->zValue()+z-mZ);
        }
        mZ = z;
}

int MultiMap::checkExit(int x, int y)
{
    if(x>=mX+1)
    {
        return 4;
    }
    else if( y >= mY+1 )
    {
        return 1;
    }
    else if( x+1 <= (map->width() *map->tileWidth() ) + mX )
    {
        return 2;
    }
    else if( y+1 <= (map->height()*map->tileHeight()) + mY )
    {
        return 3;
    }
    return 0;
}

bool MultiMap::contains(int x, int y)
{
    if(x>=mX-1
            &&  y >= mY-1
            &&  x-1 <= (map->width() *map->tileWidth() ) + mX
            &&  y-1 <= (map->height()*map->tileHeight()) + mY)
    {
        return true;
    }
    return false;
}

QList<QGraphicsItem*> MultiMap::getChild()
{
    return itemList;
}

bool MultiMap::canWalkAt(int x, int y)
{
    return !colision.contains(QPair<int,int>(x,y));
}
