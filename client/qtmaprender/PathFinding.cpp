#include "PathFinding.hpp"
#include <QMutexLocker>
#include <QTime>
#include <iostream>

PathFinding::PathFinding() :
    tryCancel(false)
{
    setObjectName("PathFinding");
    #ifndef NOTHREADS
    start();
    moveToThread(this);
    #endif
    if(!connect(this,&PathFinding::internalCancel,this,&PathFinding::cancel,Qt::BlockingQueuedConnection))
        abort();
    if(!connect(this,&PathFinding::emitSearchPath,this,&PathFinding::internalSearchPath,Qt::QueuedConnection))
        abort();
}

PathFinding::~PathFinding()
{
    #ifndef NOTHREADS
    exit();
    quit();
    //emit internalCancel();
    wait();
    #endif
}

void PathFinding::searchPath(const std::unordered_map<std::string, Map_full *> &all_map,
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
        std::cerr << "searchPath(): all_map.find(current_map)==all_map.cend()" << std::endl;
        return;
    }
    tryCancel=false;
    std::unordered_map<std::string,SimplifiedMapForPathFinding> simplifiedMapList;
    //load the data
    for ( const auto &n : all_map ) {
        const Map_full * const map=n.second;
        if(map->displayed)
        {
            SimplifiedMapForPathFinding simplifiedMap;
            simplifiedMap.width=map->logicalMap.width;
            simplifiedMap.height=map->logicalMap.height;
            if(map->logicalMap.parsed_layer.simplifiedMap==NULL)
                simplifiedMap.simplifiedMap=NULL;
            else
            {
                simplifiedMap.simplifiedMap=new uint8_t[simplifiedMap.width*simplifiedMap.height];
                memcpy(simplifiedMap.simplifiedMap,map->logicalMap.parsed_layer.simplifiedMap,simplifiedMap.width*simplifiedMap.height);
            }
            simplifiedMapList[n.first]=simplifiedMap;
        }
    }
    //resolv the border
    for ( const auto &n : simplifiedMapList ) {
        const std::string &mapString=n.first;
        if(all_map.find(all_map.at(mapString)->logicalMap.border_semi.bottom.fileName)!=all_map.cend())
        {
            simplifiedMapList[mapString].border.bottom.map=all_map.at(mapString)->logicalMap.border_semi.bottom.fileName;
            simplifiedMapList[mapString].border.bottom.x_offset=all_map.at(mapString)->logicalMap.border_semi.bottom.x_offset;
        }
        else
        {
            //simplifiedMapList[mapString].border.bottom.map=NULL;
            simplifiedMapList[mapString].border.bottom.x_offset=0;
        }
        if(all_map.find(all_map.at(mapString)->logicalMap.border_semi.left.fileName)!=all_map.cend())
        {
            simplifiedMapList[mapString].border.left.map=all_map.at(mapString)->logicalMap.border_semi.left.fileName;
            simplifiedMapList[mapString].border.left.y_offset=all_map.at(mapString)->logicalMap.border_semi.left.y_offset;
        }
        else
        {
            //simplifiedMapList[mapString].border.left.map=NULL;
            simplifiedMapList[mapString].border.left.y_offset=0;
        }
        if(all_map.find(all_map.at(mapString)->logicalMap.border_semi.right.fileName)!=all_map.cend())
        {
            simplifiedMapList[mapString].border.right.map=all_map.at(mapString)->logicalMap.border_semi.right.fileName;
            simplifiedMapList[mapString].border.right.y_offset=all_map.at(mapString)->logicalMap.border_semi.right.y_offset;
        }
        else
        {
            //simplifiedMapList[mapString].border.right.map=NULL;
            simplifiedMapList[mapString].border.right.y_offset=0;
        }
        if(all_map.find(all_map.at(mapString)->logicalMap.border_semi.top.fileName)!=all_map.cend())
        {
            simplifiedMapList[mapString].border.top.map=all_map.at(mapString)->logicalMap.border_semi.top.fileName;
            simplifiedMapList[mapString].border.top.x_offset=all_map.at(mapString)->logicalMap.border_semi.top.x_offset;
        }
        else
        {
            //simplifiedMapList[mapString].border.top.map=NULL;
            simplifiedMapList[mapString].border.top.x_offset=0;
        }
    }
    //load for thread and unload if needed
    {
        QMutexLocker locker(&mutex);
        this->simplifiedMapList=simplifiedMapList;
    }
    emit emitSearchPath(destination_map,destination_x,destination_y,current_map,x,y,items);
}

bool PathFinding::canGoOn(const SimplifiedMapForPathFinding &simplifiedMapForPathFinding,const uint8_t &x, const uint8_t &y)
{
    if(x>=simplifiedMapForPathFinding.width)
    {
        std::cerr << "can go out of border x" << std::endl;
        abort();
    }
    if(y>=simplifiedMapForPathFinding.height)
    {
        std::cerr << "can go out of border y" << std::endl;
        abort();
    }
    const uint8_t &var=simplifiedMapForPathFinding.simplifiedMap[x+y*(simplifiedMapForPathFinding.width)];
    return var==255 || var<200;
}

bool PathFinding::canMove(const CatchChallenger::Orientation &orientation,std::string &current_map,uint8_t &x,uint8_t &y,const std::unordered_map<std::string,SimplifiedMapForPathFinding> &simplifiedMapList)
{
    const SimplifiedMapForPathFinding &mapObject=simplifiedMapList.at(current_map);
    switch(orientation)
    {
    case CatchChallenger::Orientation_left:
        if(x>0)
        {
            if(canGoOn(mapObject,x-1,y))
            {
                x--;
                return true;
            }
            else
                return false;
        }
        else
        {
            if(!mapObject.border.left.map.empty())
            {
                MapPointToParse newPoint;
                newPoint.map=mapObject.border.left.map;
                const SimplifiedMapForPathFinding &d=simplifiedMapList.at(newPoint.map);
                newPoint.x=d.width-1;
                newPoint.y=(int)y-(int)mapObject.border.left.y_offset+(int)d.border.right.y_offset;

                current_map=newPoint.map;
                x=newPoint.x;
                y=newPoint.y;
                return true;
            }
            else
                return false;
        }
        break;
    case CatchChallenger::Orientation_right:
        if(x<mapObject.width-1)
        {
            if(canGoOn(mapObject,x+1,y))
            {
                x++;
                return true;
            }
            else
                return false;
        }
        else
        {
            if(!mapObject.border.right.map.empty())
            {
                MapPointToParse newPoint;
                newPoint.map=mapObject.border.right.map;
                const SimplifiedMapForPathFinding &d=simplifiedMapList.at(newPoint.map);
                newPoint.x=0;
                newPoint.y=(int)y-(int)mapObject.border.right.y_offset+(int)d.border.left.y_offset;

                current_map=newPoint.map;
                x=newPoint.x;
                y=newPoint.y;
                return true;
            }
            else
                return false;
        }
        break;
    case CatchChallenger::Orientation_top:
        if(y>0)
        {
            if(canGoOn(mapObject,x,y-1))
            {
                y--;
                return true;
            }
            else
                return false;
        }
        else
        {
            if(!mapObject.border.top.map.empty())
            {
                MapPointToParse newPoint;
                newPoint.map=mapObject.border.top.map;
                const SimplifiedMapForPathFinding &d=simplifiedMapList.at(newPoint.map);
                newPoint.x=(int)x-(int)mapObject.border.top.x_offset+(int)d.border.bottom.x_offset;
                newPoint.y=d.height-1;

                current_map=newPoint.map;
                x=newPoint.x;
                y=newPoint.y;
                return true;
            }
            else
                return false;
        }
        break;
    case CatchChallenger::Orientation_bottom:
        if(y<mapObject.height-1)
        {
            if(canGoOn(mapObject,x,y+1))
            {
                y++;
                return true;
            }
            else
                return false;
        }
        else
        {
            if(!mapObject.border.bottom.map.empty())
            {
                MapPointToParse newPoint;
                newPoint.map=mapObject.border.bottom.map;
                const SimplifiedMapForPathFinding &d=simplifiedMapList.at(newPoint.map);
                newPoint.x=(int)x-(int)mapObject.border.bottom.x_offset+(int)d.border.top.x_offset;
                newPoint.y=0;

                current_map=newPoint.map;
                x=newPoint.x;
                y=newPoint.y;
                return true;
            }
            else
                return false;
        }
        break;
    default:
        abort();
        break;
    }
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
                std::cerr << "wrong path finding data, lower than 1 step" << std::endl;
                abort();
            }
            if(lastOrientation==CatchChallenger::Orientation_left)
            {
                if(controlVarCurrent.first!=CatchChallenger::Orientation_bottom && controlVarCurrent.first!=CatchChallenger::Orientation_top)
                {
                    std::cerr << "wrong path finding data (1)" << std::endl;
                    abort();
                }
            }
            if(lastOrientation==CatchChallenger::Orientation_right)
            {
                if(controlVarCurrent.first!=CatchChallenger::Orientation_bottom && controlVarCurrent.first!=CatchChallenger::Orientation_top)
                {
                    std::cerr << "wrong path finding data (2)" << std::endl;
                    abort();
                }
            }
            if(lastOrientation==CatchChallenger::Orientation_top)
            {
                if(controlVarCurrent.first!=CatchChallenger::Orientation_right && controlVarCurrent.first!=CatchChallenger::Orientation_left)
                {
                    std::cerr << "wrong path finding data (3)" << std::endl;
                    abort();
                }
            }
            if(lastOrientation==CatchChallenger::Orientation_bottom)
            {
                if(controlVarCurrent.first!=CatchChallenger::Orientation_right && controlVarCurrent.first!=CatchChallenger::Orientation_left)
                {
                    std::cerr << "wrong path finding data (4)" << std::endl;
                    abort();
                }
            }
            lastOrientation=controlVarCurrent.first;
            index++;
        }
        if(controlVar.back().first!=orientation)
        {
            std::cerr << "wrong path finding data, last data wrong" << std::endl;
            abort();
        }
    }
}
#endif

void PathFinding::internalSearchPath(const std::string &destination_map,const uint8_t &destination_x,const uint8_t &destination_y,
                                     const std::string &source_map,const uint8_t &source_x,const uint8_t &source_y,const std::unordered_map<uint16_t,uint32_t> &items)
{
    Q_UNUSED(items);
    std::cout << "start PathFinding::internalSearchPath(): " << source_map << " (" << std::to_string(source_x) << "," << std::to_string(source_y) << ") -> " << destination_map << " (" << std::to_string(destination_x) << "," << std::to_string(destination_y) << "), simplifiedMapList.size(): " << this->simplifiedMapList.size() << std::endl;

    QElapsedTimer time;
    time.restart();

    std::unordered_map<std::string,SimplifiedMapForPathFinding> simplifiedMapList;
    //transfer from object to local variable
    {
        QMutexLocker locker(&mutex);
        simplifiedMapList=this->simplifiedMapList;
        this->simplifiedMapList.clear();
    }
    if(simplifiedMapList.find(source_map)==simplifiedMapList.cend())
    {
        tryCancel=false;
        emit result(std::string(),0,0,std::vector<std::pair<CatchChallenger::Orientation,uint8_t> >());
        std::cerr << "bug: simplifiedMapList.find(current_map)==simplifiedMapList.cend()" << std::endl;
        return;
    }
    if(simplifiedMapList.at(source_map).simplifiedMap==nullptr)
        std::cout << "PathFinding::canGoOn() walkable is NULL for " << source_map << std::endl;
    //resolv the path
    if(!tryCancel)
    {
        std::vector<MapPointToParse> mapPointToParseList;

        //init the first case
        {
            MapPointToParse tempPoint;
            tempPoint.map=source_map;
            tempPoint.x=source_x;
            tempPoint.y=source_y;
            mapPointToParseList.push_back(tempPoint);

            std::pair<uint8_t,uint8_t> coord(tempPoint.x,tempPoint.y);
            SimplifiedMapForPathFinding &tempMap=simplifiedMapList[source_map];
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
            if(mapPointToParseList.size()>(64*64*4))
            {
                if(time.elapsed()>3000)
                {
                    emit result(std::string(),0,0,std::vector<std::pair<CatchChallenger::Orientation,uint8_t> >());
                    std::cerr << "Path timeout into " << time.elapsed() << "ms" << std::endl;
                    return;
                }
            }
            if(mapPointToParseList.size()>(64*64*32))//map size * map preloaded
                abort();
            const MapPointToParse tempPoint=mapPointToParseList.front();
            if(simplifiedMapList.at(tempPoint.map).width<=tempPoint.x)
                abort();
            if(simplifiedMapList.at(tempPoint.map).height<=tempPoint.y)
                abort();
            const std::string &local_current_map=tempPoint.map;
            mapPointToParseList.erase(mapPointToParseList.cbegin());
            if(destination_map==local_current_map && tempPoint.x==destination_x && tempPoint.y==destination_y)
                std::cerr << "final dest" << std::endl;
            //resolv the own point, this data set to empy, and initialised with real data based of near tile, see 1)
            SimplifiedMapForPathFinding::PathToGo pathToGo;
            int index=0;
            while(index<1)/*2*/
            {
                if(tryCancel)
                {
                    tryCancel=false;
                    std::cerr << "internalSearchPath(): tryCancel" << std::endl;
                    return;
                }
                // 1) initialise the base data of current coordinate with around coordinate validated, edge tile to current tile
                {
                    //if the right case have been parsed
                    {
                        std::string newMap=local_current_map;
                        uint8_t newX=tempPoint.x,newY=tempPoint.y;
                        if(canMove(CatchChallenger::Orientation_right,newMap,newX,newY,simplifiedMapList))
                        {
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(newX==tempPoint.x && newY==tempPoint.y)
                            {
                                std::cerr << "can't be same point after move (abort)" << std::endl;
                                abort();
                            }
                            #endif
                            const SimplifiedMapForPathFinding &newMapObject=simplifiedMapList.at(newMap);
                            std::pair<uint8_t,uint8_t> coord=std::pair<uint8_t,uint8_t>(newX,newY);
                            if(newMapObject.pathToGo.find(coord)!=newMapObject.pathToGo.cend())
                            {
                                const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=newMapObject.pathToGo.at(coord);
                                if(pathToGo.left.size()>nearPathToGo.left.size())
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
                        }
                    }
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(local_current_map!=source_map || tempPoint.x!=source_x || tempPoint.y!=source_y)
                    {
                        if(!pathToGo.bottom.empty() && pathToGo.bottom.back().second<=1)
                        {
                            std::cerr << "Bug due for last step bottom" << std::endl;
                            return;
                        }
                        if(!pathToGo.top.empty() && pathToGo.top.back().second<=1)
                        {
                            std::cerr << "Bug due for last step top" << std::endl;
                            return;
                        }
                        if(!pathToGo.right.empty() && pathToGo.right.back().second<=1)
                        {
                            std::cerr << "Bug due for last step right" << std::endl;
                            return;
                        }
                        if(!pathToGo.left.empty() && pathToGo.left.back().second<=1)
                        {
                            std::cerr << "Bug due for last step left" << std::endl;
                            return;
                        }
                    }
                    extraControlOnData(pathToGo.left,CatchChallenger::Orientation_left);
                    extraControlOnData(pathToGo.right,CatchChallenger::Orientation_right);
                    extraControlOnData(pathToGo.top,CatchChallenger::Orientation_top);
                    extraControlOnData(pathToGo.bottom,CatchChallenger::Orientation_bottom);
                    #endif
                    //if the left case have been parsed
                    {
                        std::string newMap=local_current_map;
                        uint8_t newX=tempPoint.x,newY=tempPoint.y;
                        if(canMove(CatchChallenger::Orientation_left,newMap,newX,newY,simplifiedMapList))
                        {
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(newX==tempPoint.x && newY==tempPoint.y)
                            {
                                std::cerr << "can't be same point after move (abort)" << std::endl;
                                abort();
                            }
                            #endif
                            const SimplifiedMapForPathFinding &newMapObject=simplifiedMapList.at(newMap);
                            std::pair<uint8_t,uint8_t> coord=std::pair<uint8_t,uint8_t>(newX,newY);
                            if(newMapObject.pathToGo.find(coord)!=newMapObject.pathToGo.cend())
                            {
                                const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=newMapObject.pathToGo.at(coord);
                                if(pathToGo.right.size()>nearPathToGo.right.size())
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
                        }
                    }
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(local_current_map!=source_map || tempPoint.x!=source_x || tempPoint.y!=source_y)
                    {
                        if(!pathToGo.bottom.empty() && pathToGo.bottom.back().second<=1)
                        {
                            std::cerr << "Bug due for last step bottom" << std::endl;
                            return;
                        }
                        if(!pathToGo.top.empty() && pathToGo.top.back().second<=1)
                        {
                            std::cerr << "Bug due for last step top" << std::endl;
                            return;
                        }
                        if(!pathToGo.right.empty() && pathToGo.right.back().second<=1)
                        {
                            std::cerr << "Bug due for last step right" << std::endl;
                            return;
                        }
                        if(!pathToGo.left.empty() && pathToGo.left.back().second<=1)
                        {
                            std::cerr << "Bug due for last step left" << std::endl;
                            return;
                        }
                    }
                    extraControlOnData(pathToGo.left,CatchChallenger::Orientation_left);
                    extraControlOnData(pathToGo.right,CatchChallenger::Orientation_right);
                    extraControlOnData(pathToGo.top,CatchChallenger::Orientation_top);
                    extraControlOnData(pathToGo.bottom,CatchChallenger::Orientation_bottom);
                    #endif
                    //if the top case have been parsed
                    {
                        std::string newMap=local_current_map;
                        uint8_t newX=tempPoint.x,newY=tempPoint.y;
                        if(canMove(CatchChallenger::Orientation_bottom,newMap,newX,newY,simplifiedMapList))
                        {
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(newX==tempPoint.x && newY==tempPoint.y)
                            {
                                std::cerr << "can't be same point after move (abort)" << std::endl;
                                abort();
                            }
                            #endif
                            const SimplifiedMapForPathFinding &newMapObject=simplifiedMapList.at(newMap);
                            std::pair<uint8_t,uint8_t> coord=std::pair<uint8_t,uint8_t>(newX,newY);
                            if(newMapObject.pathToGo.find(coord)!=newMapObject.pathToGo.cend())
                            {
                                const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=newMapObject.pathToGo.at(coord);
                                if(pathToGo.top.size()>nearPathToGo.top.size())
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
                        }
                    }
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(local_current_map!=source_map || tempPoint.x!=source_x || tempPoint.y!=source_y)
                    {
                        if(!pathToGo.bottom.empty() && pathToGo.bottom.back().second<=1)
                        {
                            std::cerr << "Bug due for last step bottom" << std::endl;
                            return;
                        }
                        if(!pathToGo.top.empty() && pathToGo.top.back().second<=1)
                        {
                            std::cerr << "Bug due for last step top" << std::endl;
                            return;
                        }
                        if(!pathToGo.right.empty() && pathToGo.right.back().second<=1)
                        {
                            std::cerr << "Bug due for last step right" << std::endl;
                            return;
                        }
                        if(!pathToGo.left.empty() && pathToGo.left.back().second<=1)
                        {
                            std::cerr << "Bug due for last step left" << std::endl;
                            return;
                        }
                    }
                    extraControlOnData(pathToGo.left,CatchChallenger::Orientation_left);
                    extraControlOnData(pathToGo.right,CatchChallenger::Orientation_right);
                    extraControlOnData(pathToGo.top,CatchChallenger::Orientation_top);
                    extraControlOnData(pathToGo.bottom,CatchChallenger::Orientation_bottom);
                    #endif
                    //if the bottom case have been parsed
                    {
                        std::string newMap=local_current_map;
                        uint8_t newX=tempPoint.x,newY=tempPoint.y;
                        if(canMove(CatchChallenger::Orientation_top,newMap,newX,newY,simplifiedMapList))
                        {
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(newX==tempPoint.x && newY==tempPoint.y)
                            {
                                std::cerr << "can't be same point after move (abort)" << std::endl;
                                abort();
                            }
                            #endif
                            const SimplifiedMapForPathFinding &newMapObject=simplifiedMapList.at(newMap);
                            std::pair<uint8_t,uint8_t> coord=std::pair<uint8_t,uint8_t>(newX,newY);
                            if(newMapObject.pathToGo.find(coord)!=newMapObject.pathToGo.cend())
                            {
                                const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=newMapObject.pathToGo.at(coord);
                                if(pathToGo.bottom.size()>nearPathToGo.bottom.size())
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
                    }
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(local_current_map!=source_map || tempPoint.x!=source_x || tempPoint.y!=source_y)
                    {
                        if(!pathToGo.bottom.empty() && pathToGo.bottom.back().second<=1)
                        {
                            std::cerr << "Bug due for last step bottom" << std::endl;
                            return;
                        }
                        if(!pathToGo.top.empty() && pathToGo.top.back().second<=1)
                        {
                            std::cerr << "Bug due for last step top" << std::endl;
                            return;
                        }
                        if(!pathToGo.right.empty() && pathToGo.right.back().second<=1)
                        {
                            std::cerr << "Bug due for last step right" << std::endl;
                            return;
                        }
                        if(!pathToGo.left.empty() && pathToGo.left.back().second<=1)
                        {
                            std::cerr << "Bug due for last step left" << std::endl;
                            return;
                        }
                    }
                    extraControlOnData(pathToGo.left,CatchChallenger::Orientation_left);
                    extraControlOnData(pathToGo.right,CatchChallenger::Orientation_right);
                    extraControlOnData(pathToGo.top,CatchChallenger::Orientation_top);
                    extraControlOnData(pathToGo.bottom,CatchChallenger::Orientation_bottom);
                    #endif
                }
                index++;
            }
            // 2) set path to go data
            coord=std::pair<uint8_t,uint8_t>(tempPoint.x,tempPoint.y);
            {
                const SimplifiedMapForPathFinding &currentMapObject=simplifiedMapList.at(local_current_map);
                if(currentMapObject.pathToGo.find(coord)==currentMapObject.pathToGo.cend())
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(local_current_map!=source_map || tempPoint.x!=source_x || tempPoint.y!=source_y)
                    {
                        if(!pathToGo.bottom.empty() && pathToGo.bottom.back().second<=1)
                        {
                            std::cerr << "Bug due for last step bottom" << std::endl;
                            return;
                        }
                        if(!pathToGo.top.empty() && pathToGo.top.back().second<=1)
                        {
                            std::cerr << "Bug due for last step top" << std::endl;
                            return;
                        }
                        if(!pathToGo.right.empty() && pathToGo.right.back().second<=1)
                        {
                            std::cerr << "Bug due for last step right" << std::endl;
                            return;
                        }
                        if(!pathToGo.left.empty() && pathToGo.left.back().second<=1)
                        {
                            std::cerr << "Bug due for last step left" << std::endl;
                            return;
                        }
                    }
                    extraControlOnData(pathToGo.left,CatchChallenger::Orientation_left);
                    extraControlOnData(pathToGo.right,CatchChallenger::Orientation_right);
                    extraControlOnData(pathToGo.top,CatchChallenger::Orientation_top);
                    extraControlOnData(pathToGo.bottom,CatchChallenger::Orientation_bottom);
                    #endif
                    simplifiedMapList[local_current_map].pathToGo[coord]=pathToGo;
                }
            }
            // 3) if final dest, quit
            if(destination_map==local_current_map && tempPoint.x==destination_x && tempPoint.y==destination_y)
            {
                if(pathToGo.bottom.empty() && pathToGo.top.empty() && pathToGo.right.empty() && pathToGo.left.empty())
                {
                    std::cerr << "Bug due to resolved path pre is empty into " << time.elapsed() << "ms" << std::endl;
                    return;
                }
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
                        std::cerr << "Bug due for last step" << std::endl;
                        return;
                    }
                    else
                    {
                        std::cout << "Path result into " << time.elapsed() << "ms" << std::endl;
                        returnedVar.back().second--;
                        emit result(source_map,source_x,source_y,returnedVar);
                        return;
                    }
                }
                else
                {
                    returnedVar.clear();
                    std::cerr << "Bug due to resolved path is empty into " << time.elapsed() << "ms" << std::endl;
                    return;
                }
            }
            // 4) add to point to parse, current tile to edge tile
            {
                //if the right case have to be parsed
                {
                    std::string newMap=local_current_map;
                    uint8_t newX=tempPoint.x,newY=tempPoint.y;
                    if(canMove(CatchChallenger::Orientation_right,newMap,newX,newY,simplifiedMapList))
                    {
                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        if(newX==tempPoint.x && newY==tempPoint.y)
                        {
                            std::cerr << "can't be same point after move (abort)" << std::endl;
                            abort();
                        }
                        #endif
                        const SimplifiedMapForPathFinding &newMapObject=simplifiedMapList.at(newMap);
                        std::pair<uint8_t,uint8_t> coord(newX,newY);
                        if(newMapObject.pathToGo.find(coord)==newMapObject.pathToGo.cend())//this tile was never parsed
                        {
                            if(PathFinding::canGoOn(newMapObject,newX,newY) || (destination_map==newMap && newX==destination_x && newY==destination_y))
                            {
                                std::pair<uint8_t,uint8_t> point(newX,newY);
                                if(newMapObject.pointQueued.find(point)==newMapObject.pointQueued.cend())
                                {
                                    simplifiedMapList[local_current_map].pointQueued.insert(point);
                                    MapPointToParse newPoint;
                                    newPoint.map=newMap;
                                    newPoint.x=newX;
                                    newPoint.y=newY;
                                    mapPointToParseList.push_back(newPoint);
                                }
                            }
                        }
                    }
                }
                //if the left case have to be parsed
                {
                    std::string newMap=local_current_map;
                    uint8_t newX=tempPoint.x,newY=tempPoint.y;
                    if(canMove(CatchChallenger::Orientation_left,newMap,newX,newY,simplifiedMapList))
                    {
                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        if(newX==tempPoint.x && newY==tempPoint.y)
                        {
                            std::cerr << "can't be same point after move (abort)" << std::endl;
                            abort();
                        }
                        #endif
                        const SimplifiedMapForPathFinding &newMapObject=simplifiedMapList.at(newMap);
                        std::pair<uint8_t,uint8_t> coord(newX,newY);
                        if(newMapObject.pathToGo.find(coord)==newMapObject.pathToGo.cend())//this tile was never parsed
                        {
                            if(PathFinding::canGoOn(newMapObject,newX,newY) || (destination_map==newMap && newX==destination_x && newY==destination_y))
                            {
                                std::pair<uint8_t,uint8_t> point(newX,newY);
                                if(newMapObject.pointQueued.find(point)==newMapObject.pointQueued.cend())
                                {
                                    simplifiedMapList[local_current_map].pointQueued.insert(point);
                                    MapPointToParse newPoint;
                                    newPoint.map=newMap;
                                    newPoint.x=newX;
                                    newPoint.y=newY;
                                    mapPointToParseList.push_back(newPoint);
                                }
                            }
                        }
                    }
                }
                //if the bottom case have to be parsed
                {
                    std::string newMap=local_current_map;
                    uint8_t newX=tempPoint.x,newY=tempPoint.y;
                    if(canMove(CatchChallenger::Orientation_bottom,newMap,newX,newY,simplifiedMapList))
                    {
                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        if(newX==tempPoint.x && newY==tempPoint.y)
                        {
                            std::cerr << "can't be same point after move (abort)" << std::endl;
                            abort();
                        }
                        #endif
                        const SimplifiedMapForPathFinding &newMapObject=simplifiedMapList.at(newMap);
                        std::pair<uint8_t,uint8_t> coord(newX,newY);
                        if(newMapObject.pathToGo.find(coord)==newMapObject.pathToGo.cend())//this tile was never parsed
                        {
                            if(PathFinding::canGoOn(newMapObject,newX,newY) || (destination_map==newMap && newX==destination_x && newY==destination_y))
                            {
                                std::pair<uint8_t,uint8_t> point(newX,newY);
                                if(newMapObject.pointQueued.find(point)==newMapObject.pointQueued.cend())
                                {
                                    simplifiedMapList[local_current_map].pointQueued.insert(point);
                                    MapPointToParse newPoint;
                                    newPoint.map=newMap;
                                    newPoint.x=newX;
                                    newPoint.y=newY;
                                    mapPointToParseList.push_back(newPoint);
                                }
                            }
                        }
                    }
                }
                //if the top case have to be parsed
                {
                    std::string newMap=local_current_map;
                    uint8_t newX=tempPoint.x,newY=tempPoint.y;
                    if(canMove(CatchChallenger::Orientation_top,newMap,newX,newY,simplifiedMapList))
                    {
                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        if(newX==tempPoint.x && newY==tempPoint.y)
                        {
                            std::cerr << "can't be same point after move (abort)" << std::endl;
                            abort();
                        }
                        #endif
                        const SimplifiedMapForPathFinding &newMapObject=simplifiedMapList.at(newMap);
                        std::pair<uint8_t,uint8_t> coord(newX,newY);
                        if(newMapObject.pathToGo.find(coord)==newMapObject.pathToGo.cend())//this tile was never parsed
                        {
                            if(PathFinding::canGoOn(newMapObject,newX,newY) || (destination_map==newMap && newX==destination_x && newY==destination_y))
                            {
                                std::pair<uint8_t,uint8_t> point(newX,newY);
                                if(newMapObject.pointQueued.find(point)==newMapObject.pointQueued.cend())
                                {
                                    simplifiedMapList[local_current_map].pointQueued.insert(point);
                                    MapPointToParse newPoint;
                                    newPoint.map=newMap;
                                    newPoint.x=newX;
                                    newPoint.y=newY;
                                    mapPointToParseList.push_back(newPoint);
                                }
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
            SimplifiedMapForPathFinding simplifiedMapForPathFinding=n.second;
            if(simplifiedMapForPathFinding.simplifiedMap!=NULL)
            {
                delete[] simplifiedMapForPathFinding.simplifiedMap;
                simplifiedMapForPathFinding.simplifiedMap=NULL;
            }
        }
    }
    tryCancel=false;
    emit result(std::string(),0,0,std::vector<std::pair<CatchChallenger::Orientation,uint8_t> >());
    std::cerr << "Path not found into " << time.elapsed() << "ms" << std::endl;
}

void PathFinding::cancel()
{
    tryCancel=true;
}
