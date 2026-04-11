#include "PathFinding.hpp"
#include <QMutexLocker>
#include <QTime>
#include <QElapsedTimer>
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
    qRegisterMetaType<PathFinding::PathFinding_status >("PathFinding::PathFinding_status");
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

void PathFinding::searchPath(const std::vector<CatchChallenger::CommonMap> &mapList,
                             const CATCHCHALLENGER_TYPE_MAPID &destination_map_index,const COORD_TYPE &destination_x,const COORD_TYPE &destination_y,
                             const CATCHCHALLENGER_TYPE_MAPID &current_map_index,const COORD_TYPE &x,const COORD_TYPE &y,const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,CATCHCHALLENGER_TYPE_ITEM_QUANTITY> &items)
{
    if(current_map_index>=mapList.size())
    {
        std::vector<std::pair<CatchChallenger::Orientation,uint8_t> > path;
        emit result(0,0,0,path,PathFinding_status_InternalError);
        std::cerr << "searchPath(): current_map_index out of range" << std::endl;
        return;
    }
    tryCancel=false;
    std::unordered_map<CATCHCHALLENGER_TYPE_MAPID,SimplifiedMapForPathFinding> simplifiedMapList;
    //load the data
    // Key simplifiedMapList by the vector index in mapList (same space as
    // current_map_index / source_map_index). CommonMap::id must NOT be used as
    // a key: in Map_loader it is 1-based (incremented before assignment), which
    // caused a crash when later doing mapList.at(map.id) / simplifiedMapList.at(source_map_index).
    for (CATCHCHALLENGER_TYPE_MAPID mapIndex=0; mapIndex<(CATCHCHALLENGER_TYPE_MAPID)mapList.size(); ++mapIndex) {
        const CatchChallenger::CommonMap &map=mapList.at(mapIndex);
        if(map.flat_simplified_map.empty())
        {
            std::cerr << "PathFinding::searchPath(): flat_simplified_map.empty() for map index "
                      << std::to_string(mapIndex) << ", this should not happen" << std::endl;
            continue;
        }
        SimplifiedMapForPathFinding simplifiedMap;
        simplifiedMap.width=map.width;
        simplifiedMap.height=map.height;
        simplifiedMap.simplifiedMap=new uint8_t[simplifiedMap.width*simplifiedMap.height];
        memcpy(simplifiedMap.simplifiedMap,map.flat_simplified_map.data(),simplifiedMap.width*simplifiedMap.height);
        simplifiedMapList[mapIndex]=simplifiedMap;
    }
    //resolv the border
    for (CATCHCHALLENGER_TYPE_MAPID mapIndex=0; mapIndex<(CATCHCHALLENGER_TYPE_MAPID)mapList.size(); ++mapIndex) {
        const CatchChallenger::CommonMap &map=mapList.at(mapIndex);
        if(map.flat_simplified_map.empty())
            continue;
        const CATCHCHALLENGER_TYPE_MAPID &mapId=mapIndex;
        if(simplifiedMapList.find(mapId)==simplifiedMapList.cend())
            continue;
        if(map.border.bottom.mapIndex!=65535 && simplifiedMapList.find(map.border.bottom.mapIndex)!=simplifiedMapList.cend())
        {
            simplifiedMapList[mapId].border.bottom.map=map.border.bottom.mapIndex;
            simplifiedMapList[mapId].border.bottom.x_offset=map.border.bottom.x_offset;
        }
        else
        {
            simplifiedMapList[mapId].border.bottom.map=65535;
            simplifiedMapList[mapId].border.bottom.x_offset=0;
        }
        if(map.border.left.mapIndex!=65535 && simplifiedMapList.find(map.border.left.mapIndex)!=simplifiedMapList.cend())
        {
            simplifiedMapList[mapId].border.left.map=map.border.left.mapIndex;
            simplifiedMapList[mapId].border.left.y_offset=map.border.left.y_offset;
        }
        else
        {
            simplifiedMapList[mapId].border.left.map=65535;
            simplifiedMapList[mapId].border.left.y_offset=0;
        }
        if(map.border.right.mapIndex!=65535 && simplifiedMapList.find(map.border.right.mapIndex)!=simplifiedMapList.cend())
        {
            simplifiedMapList[mapId].border.right.map=map.border.right.mapIndex;
            simplifiedMapList[mapId].border.right.y_offset=map.border.right.y_offset;
        }
        else
        {
            simplifiedMapList[mapId].border.right.map=65535;
            simplifiedMapList[mapId].border.right.y_offset=0;
        }
        if(map.border.top.mapIndex!=65535 && simplifiedMapList.find(map.border.top.mapIndex)!=simplifiedMapList.cend())
        {
            simplifiedMapList[mapId].border.top.map=map.border.top.mapIndex;
            simplifiedMapList[mapId].border.top.x_offset=map.border.top.x_offset;
        }
        else
        {
            simplifiedMapList[mapId].border.top.map=65535;
            simplifiedMapList[mapId].border.top.x_offset=0;
        }
    }
    //load for thread and unload if needed
    {
        QMutexLocker locker(&mutex);
        this->simplifiedMapList=simplifiedMapList;
    }
    emit emitSearchPath(destination_map_index,destination_x,destination_y,current_map_index,x,y,items);
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

bool PathFinding::canMove(const CatchChallenger::Orientation &orientation,CATCHCHALLENGER_TYPE_MAPID &current_map_index,COORD_TYPE &x,COORD_TYPE &y,const std::unordered_map<CATCHCHALLENGER_TYPE_MAPID,SimplifiedMapForPathFinding> &simplifiedMapList)
{
    const SimplifiedMapForPathFinding &mapObject=simplifiedMapList.at(current_map_index);
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
            if(mapObject.border.left.map!=65535 && /*to prevent crash when click on colision point*/ simplifiedMapList.find(mapObject.border.left.map)!=simplifiedMapList.cend())
            {
                MapPointToParse newPoint;
                newPoint.map=mapObject.border.left.map;
                const SimplifiedMapForPathFinding &d=simplifiedMapList.at(newPoint.map);
                newPoint.x=d.width-1;
                newPoint.y=(int)y-(int)mapObject.border.left.y_offset+(int)d.border.right.y_offset;

                current_map_index=newPoint.map;
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
            if(mapObject.border.right.map!=65535 && /*to prevent crash when click on colision point*/ simplifiedMapList.find(mapObject.border.right.map)!=simplifiedMapList.cend())
            {
                MapPointToParse newPoint;
                newPoint.map=mapObject.border.right.map;
                const SimplifiedMapForPathFinding &d=simplifiedMapList.at(newPoint.map);
                newPoint.x=0;
                newPoint.y=(int)y-(int)mapObject.border.right.y_offset+(int)d.border.left.y_offset;

                current_map_index=newPoint.map;
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
            if(mapObject.border.top.map!=65535 && /*to prevent crash when click on colision point*/ simplifiedMapList.find(mapObject.border.top.map)!=simplifiedMapList.cend())
            {
                MapPointToParse newPoint;
                newPoint.map=mapObject.border.top.map;
                const SimplifiedMapForPathFinding &d=simplifiedMapList.at(newPoint.map);
                newPoint.x=(int)x-(int)mapObject.border.top.x_offset+(int)d.border.bottom.x_offset;
                newPoint.y=d.height-1;

                current_map_index=newPoint.map;
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
            if(mapObject.border.bottom.map!=65535 && /*to prevent crash when click on colision point*/ simplifiedMapList.find(mapObject.border.bottom.map)!=simplifiedMapList.cend())
            {
                MapPointToParse newPoint;
                newPoint.map=mapObject.border.bottom.map;
                const SimplifiedMapForPathFinding &d=simplifiedMapList.at(newPoint.map);
                newPoint.x=(int)x-(int)mapObject.border.bottom.x_offset+(int)d.border.top.x_offset;
                newPoint.y=0;

                current_map_index=newPoint.map;
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

void PathFinding::internalSearchPath(const CATCHCHALLENGER_TYPE_MAPID &destination_map_index,const COORD_TYPE &destination_x,const COORD_TYPE &destination_y,
                                     const CATCHCHALLENGER_TYPE_MAPID &source_map_index,const COORD_TYPE &source_x,const COORD_TYPE &source_y,const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,CATCHCHALLENGER_TYPE_ITEM_QUANTITY> &items)
{
    Q_UNUSED(items);
    std::cout << "start PathFinding::internalSearchPath(): " << std::to_string(source_map_index) << " (" << std::to_string(source_x) << "," << std::to_string(source_y) << ") -> " << std::to_string(destination_map_index) << " (" << std::to_string(destination_x) << "," << std::to_string(destination_y) << "), simplifiedMapList.size(): " << this->simplifiedMapList.size() << std::endl;

    QElapsedTimer time;
    time.restart();

    std::unordered_map<CATCHCHALLENGER_TYPE_MAPID,SimplifiedMapForPathFinding> simplifiedMapList;
    //transfer from object to local variable
    {
        QMutexLocker locker(&mutex);
        simplifiedMapList=this->simplifiedMapList;
        this->simplifiedMapList.clear();
    }
    if(simplifiedMapList.find(source_map_index)==simplifiedMapList.cend())
    {
        tryCancel=false;
        emit result(0,0,0,std::vector<std::pair<CatchChallenger::Orientation,uint8_t> >(),PathFinding_status_InternalError);
        std::cerr << "bug: simplifiedMapList.find(source_map_index)==simplifiedMapList.cend()" << std::endl;
        return;
    }
    if(simplifiedMapList.at(source_map_index).simplifiedMap==nullptr)
        std::cout << "PathFinding::canGoOn() walkable is NULL for " << std::to_string(source_map_index) << std::endl;
    //resolv the path
    if(!tryCancel)
    {
        std::vector<MapPointToParse> mapPointToParseList;

        //init the first case
        {
            MapPointToParse tempPoint;
            tempPoint.map=source_map_index;
            tempPoint.x=source_x;
            tempPoint.y=source_y;
            mapPointToParseList.push_back(tempPoint);

            std::pair<uint8_t,uint8_t> coord(tempPoint.x,tempPoint.y);
            SimplifiedMapForPathFinding &tempMap=simplifiedMapList[source_map_index];
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
                    emit result(0,0,0,std::vector<std::pair<CatchChallenger::Orientation,uint8_t> >(),PathFinding_status_PathNotFound);
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
            const CATCHCHALLENGER_TYPE_MAPID local_current_map=tempPoint.map;
            mapPointToParseList.erase(mapPointToParseList.cbegin());
            if(destination_map_index==local_current_map && tempPoint.x==destination_x && tempPoint.y==destination_y)
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
                        CATCHCHALLENGER_TYPE_MAPID newMap=local_current_map;
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
                        }
                    }
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(local_current_map!=source_map_index || tempPoint.x!=source_x || tempPoint.y!=source_y)
                    {
                        if(!pathToGo.bottom.empty() && pathToGo.bottom.back().second<1)
                        {
                            std::cerr << "Bug due for last step bottom " << __FILE__ << ":" << __LINE__ << std::endl;
                            return;
                        }
                        if(!pathToGo.top.empty() && pathToGo.top.back().second<1)
                        {
                            std::cerr << "Bug due for last step top " << __FILE__ << ":" << __LINE__ << std::endl;
                            return;
                        }
                        if(!pathToGo.right.empty() && pathToGo.right.back().second<1)
                        {
                            std::cerr << "Bug due for last step right " << __FILE__ << ":" << __LINE__ << std::endl;
                            return;
                        }
                        if(!pathToGo.left.empty() && pathToGo.left.back().second<1)
                        {
                            std::cerr << "Bug due for last step left " << __FILE__ << ":" << __LINE__ << std::endl;
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
                        CATCHCHALLENGER_TYPE_MAPID newMap=local_current_map;
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
                        }
                    }
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(local_current_map!=source_map_index || tempPoint.x!=source_x || tempPoint.y!=source_y)
                    {
                        if(!pathToGo.bottom.empty() && pathToGo.bottom.back().second<1)
                        {
                            std::cerr << "Bug due for last step bottom " << __FILE__ << ":" << __LINE__ << std::endl;
                            return;
                        }
                        if(!pathToGo.top.empty() && pathToGo.top.back().second<1)
                        {
                            std::cerr << "Bug due for last step top " << __FILE__ << ":" << __LINE__ << std::endl;
                            return;
                        }
                        if(!pathToGo.right.empty() && pathToGo.right.back().second<1)
                        {
                            std::cerr << "Bug due for last step right " << __FILE__ << ":" << __LINE__ << std::endl;
                            return;
                        }
                        if(!pathToGo.left.empty() && pathToGo.left.back().second<1)
                        {
                            std::cerr << "Bug due for last step left " << __FILE__ << ":" << __LINE__ << std::endl;
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
                        CATCHCHALLENGER_TYPE_MAPID newMap=local_current_map;
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
                        }
                    }
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(local_current_map!=source_map_index || tempPoint.x!=source_x || tempPoint.y!=source_y)
                    {
                        if(!pathToGo.bottom.empty() && pathToGo.bottom.back().second<1)
                        {
                            std::cerr << "Bug due for last step bottom " << __FILE__ << ":" << __LINE__ << std::endl;
                            return;
                        }
                        if(!pathToGo.top.empty() && pathToGo.top.back().second<1)
                        {
                            std::cerr << "Bug due for last step top " << __FILE__ << ":" << __LINE__ << std::endl;
                            return;
                        }
                        if(!pathToGo.right.empty() && pathToGo.right.back().second<1)
                        {
                            std::cerr << "Bug due for last step right " << __FILE__ << ":" << __LINE__ << std::endl;
                            return;
                        }
                        if(!pathToGo.left.empty() && pathToGo.left.back().second<1)
                        {
                            std::cerr << "Bug due for last step left " << __FILE__ << ":" << __LINE__ << std::endl;
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
                        CATCHCHALLENGER_TYPE_MAPID newMap=local_current_map;
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
                    }
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(local_current_map!=source_map_index || tempPoint.x!=source_x || tempPoint.y!=source_y)
                    {
                        if(!pathToGo.bottom.empty() && pathToGo.bottom.back().second<1)
                        {
                            std::cerr << "Bug due for last step bottom " << __FILE__ << ":" << __LINE__ << std::endl;
                            return;
                        }
                        if(!pathToGo.top.empty() && pathToGo.top.back().second<1)
                        {
                            std::cerr << "Bug due for last step top " << __FILE__ << ":" << __LINE__ << std::endl;
                            return;
                        }
                        if(!pathToGo.right.empty() && pathToGo.right.back().second<1)
                        {
                            std::cerr << "Bug due for last step right " << __FILE__ << ":" << __LINE__ << std::endl;
                            return;
                        }
                        if(!pathToGo.left.empty() && pathToGo.left.back().second<1)
                        {
                            std::cerr << "Bug due for last step left " << __FILE__ << ":" << __LINE__ << std::endl;
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
                    if(local_current_map!=source_map_index || tempPoint.x!=source_x || tempPoint.y!=source_y)
                    {
                        if(!pathToGo.bottom.empty() && pathToGo.bottom.back().second<1)
                        {
                            std::cerr << "Bug due for last step bottom " << __FILE__ << ":" << __LINE__ << std::endl;
                            return;
                        }
                        if(!pathToGo.top.empty() && pathToGo.top.back().second<1)
                        {
                            std::cerr << "Bug due for last step top " << __FILE__ << ":" << __LINE__ << std::endl;
                            return;
                        }
                        if(!pathToGo.right.empty() && pathToGo.right.back().second<1)
                        {
                            std::cerr << "Bug due for last step right " << __FILE__ << ":" << __LINE__ << std::endl;
                            return;
                        }
                        if(!pathToGo.left.empty() && pathToGo.left.back().second<1)
                        {
                            std::cerr << "Bug due for last step left " << __FILE__ << ":" << __LINE__ << std::endl;
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
            if(destination_map_index==local_current_map && tempPoint.x==destination_x && tempPoint.y==destination_y)
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
                    if(returnedVar.back().second<1)
                    {
                        std::cerr << "Bug due for last step" << std::endl;
                        return;
                    }
                    else
                    {
                        std::cout << "Path result into " << time.elapsed() << "ms" << std::endl;
                        returnedVar.back().second--;
                        emit result(source_map_index,source_x,source_y,returnedVar,PathFinding_status_OK);
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
                    CATCHCHALLENGER_TYPE_MAPID newMap=local_current_map;
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
                            if(PathFinding::canGoOn(newMapObject,newX,newY) || (destination_map_index==newMap && newX==destination_x && newY==destination_y))
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
                    CATCHCHALLENGER_TYPE_MAPID newMap=local_current_map;
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
                            if(PathFinding::canGoOn(newMapObject,newX,newY) || (destination_map_index==newMap && newX==destination_x && newY==destination_y))
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
                    CATCHCHALLENGER_TYPE_MAPID newMap=local_current_map;
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
                            if(PathFinding::canGoOn(newMapObject,newX,newY) || (destination_map_index==newMap && newX==destination_x && newY==destination_y))
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
                    CATCHCHALLENGER_TYPE_MAPID newMap=local_current_map;
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
                            if(PathFinding::canGoOn(newMapObject,newX,newY) || (destination_map_index==newMap && newX==destination_x && newY==destination_y))
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
    emit result(0,0,0,std::vector<std::pair<CatchChallenger::Orientation,uint8_t> >(),PathFinding_status_PathNotFound);
    std::cerr << "Path not found into " << time.elapsed() << "ms" << std::endl;
}

void PathFinding::cancel()
{
    tryCancel=true;
}
