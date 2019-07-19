#include "TemporaryTile.h"

#include <QApplication>
#include <QDebug>

Tiled::Tile *TemporaryTile::empty=NULL;

TemporaryTile::TemporaryTile(Tiled::MapObject* object) :
    object(object),
    index(0)
{
    #ifndef NOTHREADS
    moveToThread(QApplication::instance()->thread());
    #endif
    Tiled::Cell cell=object->cell();
    cell.tile=TemporaryTile::empty;
    object->setCell(cell);
    if(!connect(&timer,&QTimer::timeout,this,&TemporaryTile::updateTheTile))
        abort();
}

TemporaryTile::~TemporaryTile()
{
}

void TemporaryTile::startAnimation(Tiled::Tile *tile,const uint32_t &ms,const uint8_t &count)
{
    Tiled::Cell cell=object->cell();
    cell.tile=tile;
    object->setCell(cell);
    timer.start(ms);
    this->count=count;
    this->index=0;
}

void TemporaryTile::updateTheTile()
{
    index++;
    if(index<count)
    {
        Tiled::Cell cell=object->cell();
        if((cell.tile->id()+1)<cell.tile->tileset()->tileCount())
        {
            cell.tile=cell.tile->tileset()->tileAt(cell.tile->id()+1);
            object->setCell(cell);
            return;
        }
        else
            qDebug() << "TemporaryTile::updateTheTile(): out of tile";
    }
    timer.stop();
    Tiled::Cell cell=object->cell();
    cell.tile=TemporaryTile::empty;
    object->setCell(cell);
}
