#include "PathFinding.h"
#include <QMutexLocker>

PathFinding::PathFinding() :
    tryCancel(false)
{
    start();
    moveToThread(this);
    connect(this,&PathFinding::internalCancel,this,&PathFinding::cancel,Qt::BlockingQueuedConnection);
    connect(this,&PathFinding::emitSearchPath,this,&PathFinding::internalSearchPath,Qt::QueuedConnection);
}

PathFinding::~PathFinding()
{
    emit internalCancel();
}

void PathFinding::searchPath(const QHash<QString, MapVisualiserThread::Map_full *> &all_map,const QString &destination_map,const quint8 &destination_x,const quint8 &destination_y,const QString &current_map,const quint8 &x,const quint8 &y,const QHash<quint16,quint32> &items)
{
    if(!all_map.contains(current_map))
    {
        QList<QPair<CatchChallenger::Direction,quint8> > path;
        emit result(path);
    }
    tryCancel=false;
    QHash<QString,SimplifiedMapForPathFinding> simplifiedMapList;
    QHash<QString,MapVisualiserThread::Map_full *>::const_iterator i = all_map.constBegin();
    //load the data
    while (i != all_map.constEnd()) {
        if(i.value()->displayed)
        {
            SimplifiedMapForPathFinding simplifiedMap;
            simplifiedMap.width=i.value()->logicalMap.width;
            simplifiedMap.height=i.value()->logicalMap.height;
            if(i.value()->logicalMap.parsed_layer.dirt==NULL)
                simplifiedMap.dirt=NULL;
            else
            {
                simplifiedMap.dirt=new bool[simplifiedMap.width*simplifiedMap.height];
                memcpy(simplifiedMap.dirt,i.value()->logicalMap.parsed_layer.dirt,simplifiedMap.width*simplifiedMap.height);
            }
            if(i.value()->logicalMap.parsed_layer.ledges==NULL)
                simplifiedMap.ledges=NULL;
            else
            {
                simplifiedMap.ledges=new quint8[simplifiedMap.width*simplifiedMap.height];
                memcpy(simplifiedMap.ledges,i.value()->logicalMap.parsed_layer.ledges,simplifiedMap.width*simplifiedMap.height);
            }
            if(i.value()->logicalMap.parsed_layer.walkable==NULL)
                simplifiedMap.walkable=NULL;
            else
            {
                simplifiedMap.walkable=new bool[simplifiedMap.width*simplifiedMap.height];
                memcpy(simplifiedMap.walkable,i.value()->logicalMap.parsed_layer.walkable,simplifiedMap.width*simplifiedMap.height);
            }
            if(i.value()->logicalMap.parsed_layer.monstersCollisionMap==NULL)
                simplifiedMap.monstersCollisionMap=NULL;
            else
            {
                simplifiedMap.monstersCollisionMap=new quint8[simplifiedMap.width*simplifiedMap.height];
                memcpy(simplifiedMap.monstersCollisionMap,i.value()->logicalMap.parsed_layer.monstersCollisionMap,simplifiedMap.width*simplifiedMap.height);
            }
            simplifiedMapList[i.key()]=simplifiedMap;
        }
        ++i;
    }
    //resolv the border
    QHash<QString,SimplifiedMapForPathFinding>::const_iterator j = simplifiedMapList.constBegin();
    while (j != simplifiedMapList.constEnd()) {
        if(all_map.contains(all_map.value(j.key())->logicalMap.border_semi.bottom.fileName))
        {
            simplifiedMapList[j.key()].border.bottom.map=&simplifiedMapList[all_map.value(j.key())->logicalMap.border_semi.bottom.fileName];
            simplifiedMapList[j.key()].border.bottom.x_offset=all_map.value(j.key())->logicalMap.border_semi.bottom.x_offset;
        }
        else
        {
            simplifiedMapList[j.key()].border.bottom.map=NULL;
            simplifiedMapList[j.key()].border.bottom.x_offset=0;
        }
        if(all_map.contains(all_map.value(j.key())->logicalMap.border_semi.left.fileName))
        {
            simplifiedMapList[j.key()].border.left.map=&simplifiedMapList[all_map.value(j.key())->logicalMap.border_semi.left.fileName];
            simplifiedMapList[j.key()].border.left.y_offset=all_map.value(j.key())->logicalMap.border_semi.left.y_offset;
        }
        else
        {
            simplifiedMapList[j.key()].border.left.map=NULL;
            simplifiedMapList[j.key()].border.left.y_offset=0;
        }
        if(all_map.contains(all_map.value(j.key())->logicalMap.border_semi.right.fileName))
        {
            simplifiedMapList[j.key()].border.right.map=&simplifiedMapList[all_map.value(j.key())->logicalMap.border_semi.right.fileName];
            simplifiedMapList[j.key()].border.right.y_offset=all_map.value(j.key())->logicalMap.border_semi.right.y_offset;
        }
        else
        {
            simplifiedMapList[j.key()].border.right.map=NULL;
            simplifiedMapList[j.key()].border.right.y_offset=0;
        }
        if(all_map.contains(all_map.value(j.key())->logicalMap.border_semi.top.fileName))
        {
            simplifiedMapList[j.key()].border.top.map=&simplifiedMapList[all_map.value(j.key())->logicalMap.border_semi.top.fileName];
            simplifiedMapList[j.key()].border.top.x_offset=all_map.value(j.key())->logicalMap.border_semi.top.x_offset;
        }
        else
        {
            simplifiedMapList[j.key()].border.top.map=NULL;
            simplifiedMapList[j.key()].border.top.x_offset=0;
        }
        ++j;
    }
    //load for thread and unload if needed
    {
        QMutexLocker locker(&mutex);
        QHash<QString,SimplifiedMapForPathFinding>::const_iterator k = this->simplifiedMapList.constBegin();
        while (k != this->simplifiedMapList.constEnd()) {
            delete k.value().dirt;
            delete k.value().ledges;
            delete k.value().walkable;
            delete k.value().monstersCollisionMap;
            ++k;
        }
        this->simplifiedMapList=simplifiedMapList;
    }
    emit emitSearchPath(destination_map,destination_x,destination_y,current_map,x,y,items);
}

void PathFinding::internalSearchPath(const QString &destination_map,const quint8 &destination_x,const quint8 &destination_y,const QString &current_map,const quint8 &x,const quint8 &y,const QHash<quint16,quint32> &items)
{
    QHash<QString,SimplifiedMapForPathFinding> simplifiedMapList;
    //transfer from object to local variable
    {
        QMutexLocker locker(&mutex);
        simplifiedMapList=this->simplifiedMapList;
        this->simplifiedMapList.clear();
    }
    //resolv the path
    if(!tryCancel)
    {
        QList<QPair<CatchChallenger::Direction,quint8> > path;

        QList<MapPointToParse> mapPointToParseList;

        //init the first case
        {
            MapPointToParse tempPoint;
            tempPoint.map=current_map;
            tempPoint.x=x;
            tempPoint.y=y;
            mapPointToParseList <<  tempPoint;
        }

        while(!mapPointToParseList.isEmpty())
        {
            MapPointToParse tempPoint=mapPointToParseList.takeFirst();
            SimplifiedMapForPathFinding::PathToGo pathToGo;
            //resolv the own point
            {
                //if the right case have been parsed
                if(simplifiedMapList.value(current_map).pathToGo.contains(QPair<quint8,quint8>(tempPoint.x+1,tempPoint.y)))
                {
                    if(pathToGo.left.isEmpty() || pathToGo.left.size()>simplifiedMapList.value(current_map).pathToGo.value(QPair<quint8,quint8>(tempPoint.x+1,tempPoint.y)).left.size())
                    {
                        if(!simplifiedMapList.value(current_map).pathToGo.value(QPair<quint8,quint8>(tempPoint.x+1,tempPoint.y)).left.isEmpty())
                        {
                            pathToGo.left=simplifiedMapList.value(current_map).pathToGo.value(QPair<quint8,quint8>(tempPoint.x+1,tempPoint.y)).left;
                            pathToGo.left.last().second++;
                        }
                        else
                        {

                        }
                    }
                }
            }
            //revers resolv
            //add to point to parse
        /*quint8 tempX=x,TempY=y;
        QString tempMap=current_map;
        SimplifiedMapForPathFinding::PathToGo pathToGoTemp;
        simplifiedMapList[current_map].pathToGo[QPair<quint8,quint8>(x,y)]=pathToGoTemp;*/
        }

        emit result(path);
    }
    //drop the local variable
    {
        QHash<QString,SimplifiedMapForPathFinding>::const_iterator k = simplifiedMapList.constBegin();
        while (k != simplifiedMapList.constEnd()) {
            delete k.value().dirt;
            delete k.value().ledges;
            delete k.value().walkable;
            delete k.value().monstersCollisionMap;
            ++k;
        }
    }
    tryCancel=false;
}

void PathFinding::cancel()
{
    tryCancel=true;
}
