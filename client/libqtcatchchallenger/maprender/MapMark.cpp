#include "MapMark.hpp"
#include "ObjectGroupItem.hpp"
#include <tile.h>
#include <objectgroup.h>
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

void MapMark::detachObject()
{
    if(m_mapObject==NULL)
        return;
    //The click marker may outlive its map: walking through a door / border
    //unloads the source map, whose ObjectGroupItem erases itself from
    //objectGroupLink (ObjectGroupItem::~ObjectGroupItem). The Tiled object itself
    //survives in the cached old map, so it is still readable, but its group is
    //gone — at() would throw std::out_of_range and crash the next click / the
    //animation timer. find() and skip: no graphics item left to detach from.
    std::unordered_map<Tiled::ObjectGroup *,ObjectGroupItem *>::const_iterator it=ObjectGroupItem::objectGroupLink.find(m_mapObject->objectGroup());
    if(it!=ObjectGroupItem::objectGroupLink.cend())
        it->second->removeObject(m_mapObject);
    m_mapObject=NULL;
}

MapMark::~MapMark()
{
    detachObject();
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
        detachObject();
        timer.stop();
        return;
    }
    if(cell.tile()->id()>=cell.tile()->tileset()->tileCount())
    {
        detachObject();
        timer.stop();
        return;
    }
    else
    {
        cell.setTile(cell.tile()->tileset()->tileAt(cell.tile()->id()+1));
        if(cell.tile()==NULL)
        {
            detachObject();
            timer.stop();
            return;
        }
        m_mapObject->setCell(cell);
    }
}
