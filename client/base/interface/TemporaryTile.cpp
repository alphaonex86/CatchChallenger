#include "TemporaryTile.h"

#include <QApplication>
#include <QDebug>

Tiled::Tile *TemporaryTile::empty=NULL;

TemporaryTile::TemporaryTile(Tiled::MapObject* object) :
    object(object),
    index(0)
{
    moveToThread(QApplication::instance()->thread());
    Tiled::Cell cell=object->cell();
    cell.tile=TemporaryTile::empty;
    object->setCell(cell);
    connect(&timer,&QTimer::timeout,this,&TemporaryTile::updateTheTile);
}

TemporaryTile::~TemporaryTile()
{
}

void TemporaryTile::startAnimation(Tiled::Tile *tile,const quint32 &ms,const quint8 &count)
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
        }
        else
        {
            qDebug() << "TemporaryTile::updateTheTile(): out of tile";
            timer.stop();
        }
    }
    else
        timer.stop();
}
