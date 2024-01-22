#include "MapMark.hpp"
#include "ObjectGroupItem.hpp"
#include <libtiled/tile.h>
#include <libtiled/objectgroup.h>
#include <QDebug>

MapMark::MapMark(Tiled::MapObject *mapObject) :
    m_mapObject(mapObject)
{
    Tiled::Cell cell=mapObject->cell();
    if(cell.tile()==NULL)
    {
        qDebug() << "Tile NULL at map mark contructor";
        timer.stop();
        return;
    }
    timer.start(100);
    if(!connect(&timer,&QTimer::timeout,this,&MapMark::updateTheFrame))
        abort();
}

MapMark::~MapMark()
{
    if(m_mapObject!=NULL)
    {
        ObjectGroupItem::objectGroupLink.at(m_mapObject->objectGroup())->removeObject(m_mapObject);
        m_mapObject=NULL;
    }
}

Tiled::MapObject *MapMark::mapObject()
{
    return m_mapObject;
}

void MapMark::updateTheFrame()
{
    if(m_mapObject==NULL)
    {
        timer.stop();
        return;
    }
    Tiled::Cell cell=m_mapObject->cell();
    if(cell.tile()==NULL)
    {
        qDebug() << "Tile NULL at mark";
        timer.stop();
        return;
    }
    if(cell.tile()->tileset()==NULL)
    {
        qDebug() << "Tileset NULL at mark";
        ObjectGroupItem::objectGroupLink.at(m_mapObject->objectGroup())->removeObject(m_mapObject);
        m_mapObject=NULL;
        timer.stop();
        return;
    }
    if(cell.tile()->id()>=cell.tile()->tileset()->tileCount())
    {
        ObjectGroupItem::objectGroupLink.at(m_mapObject->objectGroup())->removeObject(m_mapObject);
        m_mapObject=NULL;
        timer.stop();
        return;
    }
    else
    {
        cell.setTile(cell.tile()->tileset()->tileAt(cell.tile()->id()+1));
        if(cell.tile()==NULL)
        {
            ObjectGroupItem::objectGroupLink.at(m_mapObject->objectGroup())->removeObject(m_mapObject);
            m_mapObject=NULL;
            timer.stop();
            return;
        }
        m_mapObject->setCell(cell);
    }
}
