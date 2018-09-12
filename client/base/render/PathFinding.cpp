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
    if(!connect(this,&PathFinding::internalCancel,this,&PathFinding::cancel,Qt::BlockingQueuedConnection))
        abort();
    if(!connect(this,&PathFinding::emitSearchPath,this,&PathFinding::internalSearchPath,Qt::QueuedConnection))
        abort();
}

PathFinding::~PathFinding()
{
    exit();
    quit();
    //emit internalCancel();
    wait();
}

void PathFinding::searchPath(const std::unordered_map<std::string, MapVisualiserThread::Map_full *> &all_map,
                             const std::string &destination_map,const uint8_t &destination_x,const uint8_t &destination_y,
                             const std::string &current_map,const uint8_t &x,const uint8_t &y,const std::unordered_map<uint16_t,uint32_t> &items)
{
    //path finding buggy
    /*{
        std::vector<std::pair<CatchChallenger::Orientation,uint8_t> > path;
        emit result(path);
        return;
    }*/
    if(all_map.find(current_map)==all_map.cend())
    {
        std::vector<std::pair<CatchChallenger::Orientation,uint8_t> > path;
        emit result(std::string(),0,0,path);
        return;
    }
    tryCancel=false;
    std::unordered_map<std::string,SimplifiedMapForPathFinding> simplifiedMapList;
    //load the data
    for ( const auto &n : all_map ) {
        if(n.second->displayed)
        {
            SimplifiedMapForPathFinding simplifiedMap;
            simplifiedMap.width=n.second->logicalMap.width;
            simplifiedMap.height=n.second->logicalMap.height;
            if(n.second->logicalMap.parsed_layer.dirt==NULL)
                simplifiedMap.dirt=NULL;
            else
            {
                simplifiedMap.dirt=new bool[simplifiedMap.width*simplifiedMap.height];
                memcpy(simplifiedMap.dirt,n.second->logicalMap.parsed_layer.dirt,simplifiedMap.width*simplifiedMap.height);
            }
            if(n.second->logicalMap.parsed_layer.ledges==NULL)
                simplifiedMap.ledges=NULL;
            else
            {
                simplifiedMap.ledges=new uint8_t[simplifiedMap.width*simplifiedMap.height];
                memcpy(simplifiedMap.ledges,n.second->logicalMap.parsed_layer.ledges,simplifiedMap.width*simplifiedMap.height);
            }
            if(n.second->logicalMap.parsed_layer.walkable==NULL)
                simplifiedMap.walkable=NULL;
            else
            {
                simplifiedMap.walkable=new bool[simplifiedMap.width*simplifiedMap.height];
                memcpy(simplifiedMap.walkable,n.second->logicalMap.parsed_layer.walkable,simplifiedMap.width*simplifiedMap.height);
            }
            if(n.second->logicalMap.parsed_layer.monstersCollisionMap==NULL)
                simplifiedMap.monstersCollisionMap=NULL;
            else
            {
                simplifiedMap.monstersCollisionMap=new uint8_t[simplifiedMap.width*simplifiedMap.height];
                memcpy(simplifiedMap.monstersCollisionMap,n.second->logicalMap.parsed_layer.monstersCollisionMap,simplifiedMap.width*simplifiedMap.height);
            }
            simplifiedMapList[n.first]=simplifiedMap;
        }
    }
    //resolv the border
    for ( const auto &n : simplifiedMapList ) {
        if(all_map.find(all_map.at(n.first)->logicalMap.border_semi.bottom.fileName)!=all_map.cend())
        {
            simplifiedMapList[n.first].border.bottom.map=&simplifiedMapList[all_map.at(n.first)->logicalMap.border_semi.bottom.fileName];
            simplifiedMapList[n.first].border.bottom.x_offset=all_map.at(n.first)->logicalMap.border_semi.bottom.x_offset;
        }
        else
        {
            simplifiedMapList[n.first].border.bottom.map=NULL;
            simplifiedMapList[n.first].border.bottom.x_offset=0;
        }
        if(all_map.find(all_map.at(n.first)->logicalMap.border_semi.left.fileName)!=all_map.cend())
        {
            simplifiedMapList[n.first].border.left.map=&simplifiedMapList[all_map.at(n.first)->logicalMap.border_semi.left.fileName];
            simplifiedMapList[n.first].border.left.y_offset=all_map.at(n.first)->logicalMap.border_semi.left.y_offset;
        }
        else
        {
            simplifiedMapList[n.first].border.left.map=NULL;
            simplifiedMapList[n.first].border.left.y_offset=0;
        }
        if(all_map.find(all_map.at(n.first)->logicalMap.border_semi.right.fileName)!=all_map.cend())
        {
            simplifiedMapList[n.first].border.right.map=&simplifiedMapList[all_map.at(n.first)->logicalMap.border_semi.right.fileName];
            simplifiedMapList[n.first].border.right.y_offset=all_map.at(n.first)->logicalMap.border_semi.right.y_offset;
        }
        else
        {
            simplifiedMapList[n.first].border.right.map=NULL;
            simplifiedMapList[n.first].border.right.y_offset=0;
        }
        if(all_map.find(all_map.at(n.first)->logicalMap.border_semi.top.fileName)!=all_map.cend())
        {
            simplifiedMapList[n.first].border.top.map=&simplifiedMapList[all_map.at(n.first)->logicalMap.border_semi.top.fileName];
            simplifiedMapList[n.first].border.top.x_offset=all_map.at(n.first)->logicalMap.border_semi.top.x_offset;
        }
        else
        {
            simplifiedMapList[n.first].border.top.map=NULL;
            simplifiedMapList[n.first].border.top.x_offset=0;
        }
    }
    //load for thread and unload if needed
    {
        QMutexLocker locker(&mutex);
        for ( auto &n : simplifiedMapList ) {
            if(n.second.dirt!=NULL)
            {
                delete n.second.dirt;
                n.second.dirt=NULL;
            }
            if(n.second.ledges!=NULL)
            {
                delete n.second.ledges;
                n.second.ledges=NULL;
            }
            if(n.second.walkable!=NULL)
            {
                delete n.second.walkable;
                n.second.walkable=NULL;
            }
            if(n.second.monstersCollisionMap!=NULL)
            {
                delete n.second.monstersCollisionMap;
                n.second.monstersCollisionMap=NULL;
            }
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
void PathFinding::extraControlOnData(const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &controlVar,const CatchChallenger::Orientation &orientation)
{
    if(!controlVar.empty())
    {
        unsigned int index=1;
        CatchChallenger::Orientation lastOrientation=controlVar.front().first;
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
        if(controlVar.back().first!=orientation)
        {
            qDebug() << "wrong path finding data, last data wrong";
            abort();
        }
    }
}
#endif

void PathFinding::internalSearchPath(const std::string &destination_map,const uint8_t &destination_x,const uint8_t &destination_y,
                                     const std::string &current_map,const uint8_t &x,const uint8_t &y,const std::unordered_map<uint16_t,uint32_t> &items)
{
    Q_UNUSED(items);

    QTime time;
    time.restart();

    std::unordered_map<std::string,SimplifiedMapForPathFinding> simplifiedMapList;
    //transfer from object to local variable
    {
        QMutexLocker locker(&mutex);
        simplifiedMapList=this->simplifiedMapList;
        this->simplifiedMapList.clear();
    }
    //resolv the path
    if(!tryCancel)
    {
        std::vector<MapPointToParse> mapPointToParseList;

        //init the first case
        {
            MapPointToParse tempPoint;
            tempPoint.map=current_map;
            tempPoint.x=x;
            tempPoint.y=y;
            mapPointToParseList.push_back(tempPoint);

            std::pair<uint8_t,uint8_t> coord(tempPoint.x,tempPoint.y);
            SimplifiedMapForPathFinding &tempMap=simplifiedMapList[current_map];
            tempMap.pathToGo[coord].left.push_back(
                    std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_left,1));
            tempMap.pathToGo[coord].right.push_back(
                    std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_right,1));
            tempMap.pathToGo[coord].bottom.push_back(
                    std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_bottom,1));
            tempMap.pathToGo[coord].top.push_back(
                    std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_top,1));
        }

        std::pair<uint8_t,uint8_t> coord;
        while(!mapPointToParseList.empty())
        {
            const SimplifiedMapForPathFinding &simplifiedMapForPathFinding=simplifiedMapList.at(current_map);
            const MapPointToParse tempPoint=mapPointToParseList.front();
            mapPointToParseList.erase(mapPointToParseList.cbegin());
            SimplifiedMapForPathFinding::PathToGo pathToGo;
            if(destination_map==current_map && tempPoint.x==destination_x && tempPoint.y==destination_y)
                qDebug() << "final dest";
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
                    if(simplifiedMapForPathFinding.pathToGo.find(coord)!=simplifiedMapForPathFinding.pathToGo.cend())
                    {
                        const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=simplifiedMapForPathFinding.pathToGo.at(coord);
                        if(pathToGo.left.empty() || pathToGo.left.size()>nearPathToGo.left.size())
                        {
                            pathToGo.left=nearPathToGo.left;
                            pathToGo.left.back().second++;
                        }
                        if(pathToGo.top.empty() || pathToGo.top.size()>(nearPathToGo.left.size()+1))
                        {
                            pathToGo.top=nearPathToGo.left;
                            pathToGo.top.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_top,1));
                        }
                        if(pathToGo.bottom.empty() || pathToGo.bottom.size()>(nearPathToGo.left.size()+1))
                        {
                            pathToGo.bottom=nearPathToGo.left;
                            pathToGo.bottom.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_bottom,1));
                        }
                    }
                    //if the left case have been parsed
                    coord=std::pair<uint8_t,uint8_t>(tempPoint.x-1,tempPoint.y);
                    if(simplifiedMapForPathFinding.pathToGo.find(coord)!=simplifiedMapForPathFinding.pathToGo.cend())
                    {
                        const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=simplifiedMapForPathFinding.pathToGo.at(coord);
                        if(pathToGo.right.empty() || pathToGo.right.size()>nearPathToGo.right.size())
                        {
                            pathToGo.right=nearPathToGo.right;
                            pathToGo.right.back().second++;
                        }
                        if(pathToGo.top.empty() || pathToGo.top.size()>(nearPathToGo.right.size()+1))
                        {
                            pathToGo.top=nearPathToGo.right;
                            pathToGo.top.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_top,1));
                        }
                        if(pathToGo.bottom.empty() || pathToGo.bottom.size()>(nearPathToGo.right.size()+1))
                        {
                            pathToGo.bottom=nearPathToGo.right;
                            pathToGo.bottom.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_bottom,1));
                        }
                    }
                    //if the top case have been parsed
                    coord=std::pair<uint8_t,uint8_t>(tempPoint.x,tempPoint.y+1);
                    if(simplifiedMapForPathFinding.pathToGo.find(coord)!=simplifiedMapForPathFinding.pathToGo.cend())
                    {
                        const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=simplifiedMapForPathFinding.pathToGo.at(coord);
                        if(pathToGo.top.empty() || pathToGo.top.size()>nearPathToGo.top.size())
                        {
                            pathToGo.top=nearPathToGo.top;
                            pathToGo.top.back().second++;
                        }
                        if(pathToGo.left.empty() || pathToGo.left.size()>(nearPathToGo.top.size()+1))
                        {
                            pathToGo.left=nearPathToGo.top;
                            pathToGo.left.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_left,1));
                        }
                        if(pathToGo.right.empty() || pathToGo.right.size()>(nearPathToGo.top.size()+1))
                        {
                            pathToGo.right=nearPathToGo.top;
                            pathToGo.right.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_right,1));
                        }
                    }
                    //if the bottom case have been parsed
                    coord=std::pair<uint8_t,uint8_t>(tempPoint.x,tempPoint.y-1);
                    if(simplifiedMapForPathFinding.pathToGo.find(coord)!=simplifiedMapForPathFinding.pathToGo.cend())
                    {
                        const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=simplifiedMapForPathFinding.pathToGo.at(coord);
                        if(pathToGo.bottom.empty() || pathToGo.bottom.size()>nearPathToGo.bottom.size())
                        {
                            pathToGo.bottom=nearPathToGo.bottom;
                            pathToGo.bottom.back().second++;
                        }
                        if(pathToGo.left.empty() || pathToGo.left.size()>(nearPathToGo.bottom.size()+1))
                        {
                            pathToGo.left=nearPathToGo.bottom;
                            pathToGo.left.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_left,1));
                        }
                        if(pathToGo.right.empty() || pathToGo.right.size()>(nearPathToGo.bottom.size()+1))
                        {
                            pathToGo.right=nearPathToGo.bottom;
                            pathToGo.right.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_right,1));
                        }
                    }
                }
                index++;
            }
            coord=std::pair<uint8_t,uint8_t>(tempPoint.x,tempPoint.y);
            if(simplifiedMapForPathFinding.pathToGo.find(coord)==simplifiedMapForPathFinding.pathToGo.cend())
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
                std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > returnedVar;
                if(returnedVar.empty() || pathToGo.bottom.size()<returnedVar.size())
                    if(!pathToGo.bottom.empty())
                        returnedVar=pathToGo.bottom;
                if(returnedVar.empty() || pathToGo.top.size()<returnedVar.size())
                    if(!pathToGo.top.empty())
                        returnedVar=pathToGo.top;
                if(returnedVar.empty() || pathToGo.right.size()<returnedVar.size())
                    if(!pathToGo.right.empty())
                        returnedVar=pathToGo.right;
                if(returnedVar.empty() || pathToGo.left.size()<returnedVar.size())
                    if(!pathToGo.left.empty())
                        returnedVar=pathToGo.left;
                if(!returnedVar.empty())
                {
                    if(returnedVar.back().second<=1)
                    {
                        qDebug() << "Bug due for last step";
                        return;
                    }
                    else
                    {
                        qDebug() << "Path result into " << time.elapsed() << "ms";
                        returnedVar.back().second--;
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
                if(simplifiedMapForPathFinding.pathToGo.find(coord)==simplifiedMapForPathFinding.pathToGo.cend())
                {
                    MapPointToParse newPoint=tempPoint;
                    newPoint.x++;
                    if(newPoint.x<simplifiedMapForPathFinding.width)
                        if(PathFinding::canGoOn(simplifiedMapForPathFinding,newPoint.x,newPoint.y) || (destination_map==current_map && newPoint.x==destination_x && newPoint.y==destination_y))
                        {
                            std::pair<uint8_t,uint8_t> point(newPoint.x,newPoint.y);
                            if(simplifiedMapForPathFinding.pointQueued.find(point)==simplifiedMapForPathFinding.pointQueued.cend())
                            {
                                simplifiedMapList[current_map].pointQueued.insert(point);
                                mapPointToParseList.push_back(newPoint);
                            }
                        }
                }
                //if the left case have been parsed
                coord=std::pair<uint8_t,uint8_t>(tempPoint.x-1,tempPoint.y);
                if(simplifiedMapForPathFinding.pathToGo.find(coord)==simplifiedMapForPathFinding.pathToGo.cend())
                {
                    MapPointToParse newPoint=tempPoint;
                    if(newPoint.x>0)
                    {
                        newPoint.x--;
                        if(PathFinding::canGoOn(simplifiedMapForPathFinding,newPoint.x,newPoint.y) || (destination_map==current_map && newPoint.x==destination_x && newPoint.y==destination_y))
                        {
                            std::pair<uint8_t,uint8_t> point(newPoint.x,newPoint.y);
                            if(simplifiedMapForPathFinding.pointQueued.find(point)==simplifiedMapForPathFinding.pointQueued.cend())
                            {
                                simplifiedMapList[current_map].pointQueued.insert(point);
                                mapPointToParseList.push_back(newPoint);
                            }
                        }
                    }
                }
                //if the bottom case have been parsed
                coord=std::pair<uint8_t,uint8_t>(tempPoint.x,tempPoint.y+1);
                if(simplifiedMapForPathFinding.pathToGo.find(coord)==simplifiedMapForPathFinding.pathToGo.cend())
                {
                    MapPointToParse newPoint=tempPoint;
                    newPoint.y++;
                    if(newPoint.y<simplifiedMapForPathFinding.height)
                        if(PathFinding::canGoOn(simplifiedMapForPathFinding,newPoint.x,newPoint.y) || (destination_map==current_map && newPoint.x==destination_x && newPoint.y==destination_y))
                        {
                            std::pair<uint8_t,uint8_t> point(newPoint.x,newPoint.y);
                            if(simplifiedMapForPathFinding.pointQueued.find(point)==simplifiedMapForPathFinding.pointQueued.cend())
                            {
                                simplifiedMapList[current_map].pointQueued.insert(point);
                                mapPointToParseList.push_back(newPoint);
                            }
                        }
                }
                //if the top case have been parsed
                coord=std::pair<uint8_t,uint8_t>(tempPoint.x,tempPoint.y-1);
                if(simplifiedMapForPathFinding.pathToGo.find(coord)==simplifiedMapForPathFinding.pathToGo.cend())
                {
                    MapPointToParse newPoint=tempPoint;
                    if(newPoint.y>0)
                    {
                        newPoint.y--;
                        if(PathFinding::canGoOn(simplifiedMapForPathFinding,newPoint.x,newPoint.y) || (destination_map==current_map && newPoint.x==destination_x && newPoint.y==destination_y))
                        {
                            std::pair<uint8_t,uint8_t> point(newPoint.x,newPoint.y);
                            if(simplifiedMapForPathFinding.pointQueued.find(point)==simplifiedMapForPathFinding.pointQueued.cend())
                            {
                                simplifiedMapList[current_map].pointQueued.insert(point);
                                mapPointToParseList.push_back(newPoint);
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
        for ( auto &n : simplifiedMapList ) {
            if(n.second.dirt!=NULL)
            {
                delete n.second.dirt;
                n.second.dirt=NULL;
            }
            if(n.second.ledges!=NULL)
            {
                delete n.second.ledges;
                n.second.ledges=NULL;
            }
            if(n.second.walkable!=NULL)
            {
                delete n.second.walkable;
                n.second.walkable=NULL;
            }
            if(n.second.monstersCollisionMap!=NULL)
            {
                delete n.second.monstersCollisionMap;
                n.second.monstersCollisionMap=NULL;
            }
        }
    }
    tryCancel=false;
    emit result(std::string(),0,0,std::vector<std::pair<CatchChallenger::Orientation,uint8_t> >());
    qDebug() << "Path not found into " << time.elapsed() << "ms";
}

void PathFinding::cancel()
{
    tryCancel=true;
}
