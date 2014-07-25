#include "MapMark.h"
#include "ObjectGroupItem.h"
#include <QDebug>

MapMark::MapMark(Tiled::MapObject *mapObject) :
    mapObject(mapObject)
{
    Tiled::Cell cell=mapObject->cell();
    if(cell.tile==NULL)
    {
        qDebug() << "Tile NULL at map mark contructor";
        timer.stop();
        return;
    }
    timer.start(100);
    connect(&timer,&QTimer::timeout,this,&MapMark::updateTheFrame);
}

MapMark::~MapMark()
{
    if(mapObject!=NULL)
    {
        ObjectGroupItem::objectGroupLink.value(mapObject->objectGroup())->removeObject(mapObject);
        delete mapObject;
        mapObject=NULL;
    }
}

void MapMark::updateTheFrame()
{
    if(mapObject==NULL)
    {
        timer.stop();
        return;
    }
    Tiled::Cell cell=mapObject->cell();
    if(cell.tile==NULL)
    {
        qDebug() << "Tile NULL at market";
        timer.stop();
        return;
    }
    if(cell.tile->id()>=cell.tile->tileset()->tileCount())
    {
        ObjectGroupItem::objectGroupLink.value(mapObject->objectGroup())->removeObject(mapObject);
        delete mapObject;
        mapObject=NULL;
        timer.stop();
    }
    else
    {
        cell.tile=cell.tile->tileset()->tileAt(cell.tile->id()+1);
        if(cell.tile==NULL)
        {
            ObjectGroupItem::objectGroupLink.value(mapObject->objectGroup())->removeObject(mapObject);
            delete mapObject;
            mapObject=NULL;
            timer.stop();
            return;
        }
        mapObject->setCell(cell);
    }
}
