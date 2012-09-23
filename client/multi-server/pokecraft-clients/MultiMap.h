#ifndef MULTIMAP_H
#define MULTIMAP_H

#include "map.h"
#include "mapreader.h"

#include "objectgroupitem.h"
#include "tilelayeritem.h"

#include <QDebug>
#include <QGraphicsItem>

class Tileset;
class MultiMap
{
public:
    MultiMap(Tiled::Map *map, QString fileName, int x = 0, int y = 0);
    MultiMap(QString fileName, int x = 0, int y = 0);
    ~MultiMap();
    void setX(int x);
    void setY(int y);
    void setZOffset(int z);
    void setFileName(QString fileName){mFileName = fileName;}
    QList<QGraphicsItem*> getChild();
    int x(){return mX;}
    int y(){return mY;}
    QString getFileName(){return mFileName;}
    QString getErrorString(){return mErrorString;}
    void view();
    bool contains(int x, int y);
    int checkExit(int x, int y);
    Tiled::Map* getMap(){return map;}
    bool canWalkAt(int x, int y);
    QList<QGraphicsItem*> getItems(){return itemList;}
protected:
    int mX;
    int mY;
    int mZ;
    Tiled::Map *map;
    QList<QPair<int,int> > colision;
    QString mFileName;
    QList<QGraphicsItem*> itemList;
    QString mErrorString;

    // border
    MultiMap* top;
    MultiMap* right;
    MultiMap* bottom;
    MultiMap* left;
};

#endif // MULTIMAP_H
