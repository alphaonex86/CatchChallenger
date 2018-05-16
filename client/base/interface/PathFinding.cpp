#include "PathFinding.h"
#include <QMutexLocker>
#include <QTime>
#include <QDebug>

PathFinding::PathFinding() :
    tryCancel(false)
{
    setObjectName("PathFinding");
    start();
    moveToThread(this);
    connect(this,&PathFinding::internalCancel,this,&PathFinding::cancel,Qt::BlockingQueuedConnection);
    connect(this,&PathFinding::emitSearchPath,this,&PathFinding::internalSearchPath,Qt::QueuedConnection);
}

PathFinding::~PathFinding()
{
    exit();
    quit();
    //emit internalCancel();
    wait();
}

void PathFinding::searchPath(const std::unordered_map<QString, MapVisualiserThread::Map_full *> &all_map,const std::string &destination_map,const uint8_t &destination_x,const uint8_t &destination_y,const std::string &current_map,const uint8_t &x,const uint8_t &y,const std::unordered_map<uint16_t,uint32_t> &items)
{
    //path finding buggy
    /*{
        QVector<std::pair<CatchChallenger::Orientation,uint8_t> > path;
        emit result(path);
        return;
    }*/
    if(!all_map.contains(current_map))
    {
        QVector<std::pair<CatchChallenger::Orientation,uint8_t> > path;
        emit result(QString(),0,0,path);
        return;
    }
    tryCancel=false;
    std::unordered_map<QString,SimplifiedMapForPathFinding> simplifiedMapList;
    std::unordered_map<QString,MapVisualiserThread::Map_full *>::const_iterator i = all_map.constBegin();
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
                simplifiedMap.ledges=new uint8_t[simplifiedMap.width*simplifiedMap.height];
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
                simplifiedMap.monstersCollisionMap=new uint8_t[simplifiedMap.width*simplifiedMap.height];
                memcpy(simplifiedMap.monstersCollisionMap,i.value()->logicalMap.parsed_layer.monstersCollisionMap,simplifiedMap.width*simplifiedMap.height);
            }
            simplifiedMapList[i.key()]=simplifiedMap;
        }
        ++i;
    }
    //resolv the border
    std::unordered_map<QString,SimplifiedMapForPathFinding>::const_iterator j = simplifiedMapList.constBegin();
    while (j != simplifiedMapList.constEnd()) {
        if(all_map.contains(QString::fromStdString(all_map.value(j.key())->logicalMap.border_semi.bottom.fileName)))
        {
            simplifiedMapList[j.key()].border.bottom.map=&simplifiedMapList[QString::fromStdString(all_map.value(j.key())->logicalMap.border_semi.bottom.fileName)];
            simplifiedMapList[j.key()].border.bottom.x_offset=all_map.value(j.key())->logicalMap.border_semi.bottom.x_offset;
        }
        else
        {
            simplifiedMapList[j.key()].border.bottom.map=NULL;
            simplifiedMapList[j.key()].border.bottom.x_offset=0;
        }
        if(all_map.contains(QString::fromStdString(all_map.value(j.key())->logicalMap.border_semi.left.fileName)))
        {
            simplifiedMapList[j.key()].border.left.map=&simplifiedMapList[QString::fromStdString(all_map.value(j.key())->logicalMap.border_semi.left.fileName)];
            simplifiedMapList[j.key()].border.left.y_offset=all_map.value(j.key())->logicalMap.border_semi.left.y_offset;
        }
        else
        {
            simplifiedMapList[j.key()].border.left.map=NULL;
            simplifiedMapList[j.key()].border.left.y_offset=0;
        }
        if(all_map.contains(QString::fromStdString(all_map.value(j.key())->logicalMap.border_semi.right.fileName)))
        {
            simplifiedMapList[j.key()].border.right.map=&simplifiedMapList[QString::fromStdString(all_map.value(j.key())->logicalMap.border_semi.right.fileName)];
            simplifiedMapList[j.key()].border.right.y_offset=all_map.value(j.key())->logicalMap.border_semi.right.y_offset;
        }
        else
        {
            simplifiedMapList[j.key()].border.right.map=NULL;
            simplifiedMapList[j.key()].border.right.y_offset=0;
        }
        if(all_map.contains(QString::fromStdString(all_map.value(j.key())->logicalMap.border_semi.top.fileName)))
        {
            simplifiedMapList[j.key()].border.top.map=&simplifiedMapList[QString::fromStdString(all_map.value(j.key())->logicalMap.border_semi.top.fileName)];
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
        std::unordered_map<QString,SimplifiedMapForPathFinding>::const_iterator k = this->simplifiedMapList.constBegin();
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

bool PathFinding::canGoOn(const SimplifiedMapForPathFinding &simplifiedMapForPathFinding,const uint8_t &x, const uint8_t &y)
{
    bool walkable;
    if(simplifiedMapForPathFinding.walkable==NULL)
        walkable=false;
    else
        walkable=simplifiedMapForPathFinding.walkable[x+y*(simplifiedMapForPathFinding.width)];
    bool dirt;
    if(simplifiedMapForPathFinding.dirt==NULL)
        dirt=false;
    else
        dirt=simplifiedMapForPathFinding.dirt[x+y*(simplifiedMapForPathFinding.width)];
    return dirt || walkable;
}

#ifdef CATCHCHALLENGER_EXTRA_CHECK
void PathFinding::extraControlOnData(const QVector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &controlVar,const CatchChallenger::Orientation &orientation)
{
    if(!controlVar.isEmpty())
    {
        int index=1;
        CatchChallenger::Orientation lastOrientation=controlVar.first().first;
        while(index<controlVar.size())
        {
            const std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> &controlVarCurrent=controlVar.at(index);
            if(controlVarCurrent.second<1)
            {
                qDebug() << "wrong path finding data, lower than 1 step";
                abort();
            }
            if(lastOrientation==CatchChallenger::Orientation_left)
            {
                if(controlVarCurrent.first!=CatchChallenger::Orientation_bottom && controlVarCurrent.first!=CatchChallenger::Orientation_top)
                {
                    qDebug() << "wrong path finding data (1)";
                    abort();
                }
            }
            if(lastOrientation==CatchChallenger::Orientation_right)
            {
                if(controlVarCurrent.first!=CatchChallenger::Orientation_bottom && controlVarCurrent.first!=CatchChallenger::Orientation_top)
                {
                    qDebug() << "wrong path finding data (2)";
                    abort();
                }
            }
            if(lastOrientation==CatchChallenger::Orientation_top)
            {
                if(controlVarCurrent.first!=CatchChallenger::Orientation_right && controlVarCurrent.first!=CatchChallenger::Orientation_left)
                {
                    qDebug() << "wrong path finding data (3)";
                    abort();
                }
            }
            if(lastOrientation==CatchChallenger::Orientation_bottom)
            {
                if(controlVarCurrent.first!=CatchChallenger::Orientation_right && controlVarCurrent.first!=CatchChallenger::Orientation_left)
                {
                    qDebug() << "wrong path finding data (4)";
                    abort();
                }
            }
            lastOrientation=controlVarCurrent.first;
            index++;
        }
        if(controlVar.last().first!=orientation)
        {
            qDebug() << "wrong path finding data, last data wrong";
            abort();
        }
    }
}
#endif

void PathFinding::internalSearchPath(const std::string &destination_map,const uint8_t &destination_x,const uint8_t &destination_y,const std::string &current_map,const uint8_t &x,const uint8_t &y,const std::unordered_map<uint16_t,uint32_t> &items)
{
    Q_UNUSED(items);

    QTime time;
    time.restart();

    std::unordered_map<QString,SimplifiedMapForPathFinding> simplifiedMapList;
    //transfer from object to local variable
    {
        QMutexLocker locker(&mutex);
        simplifiedMapList=this->simplifiedMapList;
        this->simplifiedMapList.clear();
    }
    //resolv the path
    if(!tryCancel)
    {
        QVector<MapPointToParse> mapPointToParseList;

        //init the first case
        {
            MapPointToParse tempPoint;
            tempPoint.map=current_map;
            tempPoint.x=x;
            tempPoint.y=y;
            mapPointToParseList <<  tempPoint;

            std::pair<uint8_t,uint8_t> coord(tempPoint.x,tempPoint.y);
            SimplifiedMapForPathFinding &tempMap=simplifiedMapList[current_map];
            tempMap.pathToGo[coord].left <<
                    std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_left,1);
            tempMap.pathToGo[coord].right <<
                    std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_right,1);
            tempMap.pathToGo[coord].bottom <<
                    std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_bottom,1);
            tempMap.pathToGo[coord].top <<
                    std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_top,1);
        }

        std::pair<uint8_t,uint8_t> coord;
        while(!mapPointToParseList.isEmpty())
        {
            const MapPointToParse &tempPoint=mapPointToParseList.takeFirst();
            SimplifiedMapForPathFinding::PathToGo pathToGo;
            if(destination_map==current_map && tempPoint.x==destination_x && tempPoint.y==destination_y)
                qDebug() << QStringLiteral("final dest");
            //resolv the own point
            int index=0;
            while(index<1)/*2*/
            {
                if(tryCancel)
                {
                    tryCancel=false;
                    return;
                }
                {
                    //if the right case have been parsed
                    coord=std::pair<uint8_t,uint8_t>(tempPoint.x+1,tempPoint.y);
                    if(simplifiedMapList.value(current_map).pathToGo.contains(coord))
                    {
                        const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=simplifiedMapList.value(current_map).pathToGo.value(coord);
                        if(pathToGo.left.isEmpty() || pathToGo.left.size()>nearPathToGo.left.size())
                        {
                            pathToGo.left=nearPathToGo.left;
                            pathToGo.left.last().second++;
                        }
                        if(pathToGo.top.isEmpty() || pathToGo.top.size()>(nearPathToGo.left.size()+1))
                        {
                            pathToGo.top=nearPathToGo.left;
                            pathToGo.top << std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_top,1);
                        }
                        if(pathToGo.bottom.isEmpty() || pathToGo.bottom.size()>(nearPathToGo.left.size()+1))
                        {
                            pathToGo.bottom=nearPathToGo.left;
                            pathToGo.bottom << std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_bottom,1);
                        }
                    }
                    //if the left case have been parsed
                    coord=std::pair<uint8_t,uint8_t>(tempPoint.x-1,tempPoint.y);
                    if(simplifiedMapList.value(current_map).pathToGo.contains(coord))
                    {
                        const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=simplifiedMapList.value(current_map).pathToGo.value(coord);
                        if(pathToGo.right.isEmpty() || pathToGo.right.size()>nearPathToGo.right.size())
                        {
                            pathToGo.right=nearPathToGo.right;
                            pathToGo.right.last().second++;
                        }
                        if(pathToGo.top.isEmpty() || pathToGo.top.size()>(nearPathToGo.right.size()+1))
                        {
                            pathToGo.top=nearPathToGo.right;
                            pathToGo.top << std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_top,1);
                        }
                        if(pathToGo.bottom.isEmpty() || pathToGo.bottom.size()>(nearPathToGo.right.size()+1))
                        {
                            pathToGo.bottom=nearPathToGo.right;
                            pathToGo.bottom << std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_bottom,1);
                        }
                    }
                    //if the top case have been parsed
                    coord=std::pair<uint8_t,uint8_t>(tempPoint.x,tempPoint.y+1);
                    if(simplifiedMapList.value(current_map).pathToGo.contains(coord))
                    {
                        const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=simplifiedMapList.value(current_map).pathToGo.value(coord);
                        if(pathToGo.top.isEmpty() || pathToGo.top.size()>nearPathToGo.top.size())
                        {
                            pathToGo.top=nearPathToGo.top;
                            pathToGo.top.last().second++;
                        }
                        if(pathToGo.left.isEmpty() || pathToGo.left.size()>(nearPathToGo.top.size()+1))
                        {
                            pathToGo.left=nearPathToGo.top;
                            pathToGo.left << std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_left,1);
                        }
                        if(pathToGo.right.isEmpty() || pathToGo.right.size()>(nearPathToGo.top.size()+1))
                        {
                            pathToGo.right=nearPathToGo.top;
                            pathToGo.right << std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_right,1);
                        }
                    }
                    //if the bottom case have been parsed
                    coord=std::pair<uint8_t,uint8_t>(tempPoint.x,tempPoint.y-1);
                    if(simplifiedMapList.value(current_map).pathToGo.contains(coord))
                    {
                        const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=simplifiedMapList.value(current_map).pathToGo.value(coord);
                        if(pathToGo.bottom.isEmpty() || pathToGo.bottom.size()>nearPathToGo.bottom.size())
                        {
                            pathToGo.bottom=nearPathToGo.bottom;
                            pathToGo.bottom.last().second++;
                        }
                        if(pathToGo.left.isEmpty() || pathToGo.left.size()>(nearPathToGo.bottom.size()+1))
                        {
                            pathToGo.left=nearPathToGo.bottom;
                            pathToGo.left << std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_left,1);
                        }
                        if(pathToGo.right.isEmpty() || pathToGo.right.size()>(nearPathToGo.bottom.size()+1))
                        {
                            pathToGo.right=nearPathToGo.bottom;
                            pathToGo.right << std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_right,1);
                        }
                    }
                }
                index++;
            }
            coord=std::pair<uint8_t,uint8_t>(tempPoint.x,tempPoint.y);
            if(!simplifiedMapList.value(current_map).pathToGo.contains(coord))
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                extraControlOnData(pathToGo.left,CatchChallenger::Orientation_left);
                extraControlOnData(pathToGo.right,CatchChallenger::Orientation_right);
                extraControlOnData(pathToGo.top,CatchChallenger::Orientation_top);
                extraControlOnData(pathToGo.bottom,CatchChallenger::Orientation_bottom);
                #endif
                simplifiedMapList[current_map].pathToGo[coord]=pathToGo;
            }
            if(destination_map==current_map && tempPoint.x==destination_x && tempPoint.y==destination_y)
            {
                tryCancel=false;
                QVector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > returnedVar;
                if(returnedVar.isEmpty() || pathToGo.bottom.size()<returnedVar.size())
                    if(!pathToGo.bottom.isEmpty())
                        returnedVar=pathToGo.bottom;
                if(returnedVar.isEmpty() || pathToGo.top.size()<returnedVar.size())
                    if(!pathToGo.top.isEmpty())
                        returnedVar=pathToGo.top;
                if(returnedVar.isEmpty() || pathToGo.right.size()<returnedVar.size())
                    if(!pathToGo.right.isEmpty())
                        returnedVar=pathToGo.right;
                if(returnedVar.isEmpty() || pathToGo.left.size()<returnedVar.size())
                    if(!pathToGo.left.isEmpty())
                        returnedVar=pathToGo.left;
                if(!returnedVar.isEmpty())
                {
                    if(returnedVar.last().second<=1)
                    {
                        qDebug() << "Bug due for last step";
                        return;
                    }
                    else
                    {
                        qDebug() << "Path result into " << time.elapsed() << "ms";
                        returnedVar.last().second--;
                        emit result(current_map,x,y,returnedVar);
                        return;
                    }
                }
                else
                {
                    returnedVar.clear();
                    qDebug() << "Bug due to resolved path is empty";
                    return;
                }
            }
            //revers resolv
            //add to point to parse
            {
                //if the right case have been parsed
                coord=std::pair<uint8_t,uint8_t>(tempPoint.x+1,tempPoint.y);
                if(!simplifiedMapList.value(current_map).pathToGo.contains(coord))
                {
                    MapPointToParse newPoint=tempPoint;
                    newPoint.x++;
                    if(newPoint.x<simplifiedMapList.value(current_map).width)
                        if(PathFinding::canGoOn(simplifiedMapList.value(current_map),newPoint.x,newPoint.y) || (destination_map==current_map && newPoint.x==destination_x && newPoint.y==destination_y))
                        {
                            std::pair<uint8_t,uint8_t> point(newPoint.x,newPoint.y);
                            if(!simplifiedMapList.value(current_map).pointQueued.contains(point))
                            {
                                simplifiedMapList[current_map].pointQueued << point;
                                mapPointToParseList <<  newPoint;
                            }
                        }
                }
                //if the left case have been parsed
                coord=std::pair<uint8_t,uint8_t>(tempPoint.x-1,tempPoint.y);
                if(!simplifiedMapList.value(current_map).pathToGo.contains(coord))
                {
                    MapPointToParse newPoint=tempPoint;
                    if(newPoint.x>0)
                    {
                        newPoint.x--;
                        if(PathFinding::canGoOn(simplifiedMapList.value(current_map),newPoint.x,newPoint.y) || (destination_map==current_map && newPoint.x==destination_x && newPoint.y==destination_y))
                        {
                            std::pair<uint8_t,uint8_t> point(newPoint.x,newPoint.y);
                            if(!simplifiedMapList.value(current_map).pointQueued.contains(point))
                            {
                                simplifiedMapList[current_map].pointQueued << point;
                                mapPointToParseList <<  newPoint;
                            }
                        }
                    }
                }
                //if the bottom case have been parsed
                coord=std::pair<uint8_t,uint8_t>(tempPoint.x,tempPoint.y+1);
                if(!simplifiedMapList.value(current_map).pathToGo.contains(coord))
                {
                    MapPointToParse newPoint=tempPoint;
                    newPoint.y++;
                    if(newPoint.y<simplifiedMapList.value(current_map).height)
                        if(PathFinding::canGoOn(simplifiedMapList.value(current_map),newPoint.x,newPoint.y) || (destination_map==current_map && newPoint.x==destination_x && newPoint.y==destination_y))
                        {
                            std::pair<uint8_t,uint8_t> point(newPoint.x,newPoint.y);
                            if(!simplifiedMapList.value(current_map).pointQueued.contains(point))
                            {
                                simplifiedMapList[current_map].pointQueued << point;
                                mapPointToParseList <<  newPoint;
                            }
                        }
                }
                //if the top case have been parsed
                coord=std::pair<uint8_t,uint8_t>(tempPoint.x,tempPoint.y-1);
                if(!simplifiedMapList.value(current_map).pathToGo.contains(coord))
                {
                    MapPointToParse newPoint=tempPoint;
                    if(newPoint.y>0)
                    {
                        newPoint.y--;
                        if(PathFinding::canGoOn(simplifiedMapList.value(current_map),newPoint.x,newPoint.y) || (destination_map==current_map && newPoint.x==destination_x && newPoint.y==destination_y))
                        {
                            std::pair<uint8_t,uint8_t> point(newPoint.x,newPoint.y);
                            if(!simplifiedMapList.value(current_map).pointQueued.contains(point))
                            {
                                simplifiedMapList[current_map].pointQueued << point;
                                mapPointToParseList <<  newPoint;
                            }
                        }
                    }
                }
            }
            /*uint8_t tempX=x,TempY=y;
            std::string tempMap=current_map;
            SimplifiedMapForPathFinding::PathToGo pathToGoTemp;
            simplifiedMapList[current_map].pathToGo[std::pair<uint8_t,uint8_t>(x,y)]=pathToGoTemp;*/
        }
    }
    //drop the local variable
    {
        std::unordered_map<QString,SimplifiedMapForPathFinding>::const_iterator k = simplifiedMapList.constBegin();
        while (k != simplifiedMapList.constEnd()) {
            delete k.value().dirt;
            delete k.value().ledges;
            delete k.value().walkable;
            delete k.value().monstersCollisionMap;
            ++k;
        }
    }
    tryCancel=false;
    emit result(QString(),0,0,QVector<std::pair<CatchChallenger::Orientation,uint8_t> >());
    qDebug() << "Path not found into " << time.elapsed() << "ms";
}

void PathFinding::cancel()
{
    tryCancel=true;
}
