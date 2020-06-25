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
            simplifiedMapList[mapString].border.bottom.map=&simplifiedMapList[all_map.at(mapString)->logicalMap.border_semi.bottom.fileName];
            simplifiedMapList[mapString].border.bottom.x_offset=all_map.at(mapString)->logicalMap.border_semi.bottom.x_offset;
        }
        else
        {
            simplifiedMapList[mapString].border.bottom.map=NULL;
            simplifiedMapList[mapString].border.bottom.x_offset=0;
        }
        if(all_map.find(all_map.at(mapString)->logicalMap.border_semi.left.fileName)!=all_map.cend())
        {
            simplifiedMapList[mapString].border.left.map=&simplifiedMapList[all_map.at(mapString)->logicalMap.border_semi.left.fileName];
            simplifiedMapList[mapString].border.left.y_offset=all_map.at(mapString)->logicalMap.border_semi.left.y_offset;
        }
        else
        {
            simplifiedMapList[mapString].border.left.map=NULL;
            simplifiedMapList[mapString].border.left.y_offset=0;
        }
        if(all_map.find(all_map.at(mapString)->logicalMap.border_semi.right.fileName)!=all_map.cend())
        {
            simplifiedMapList[mapString].border.right.map=&simplifiedMapList[all_map.at(mapString)->logicalMap.border_semi.right.fileName];
            simplifiedMapList[mapString].border.right.y_offset=all_map.at(mapString)->logicalMap.border_semi.right.y_offset;
        }
        else
        {
            simplifiedMapList[mapString].border.right.map=NULL;
            simplifiedMapList[mapString].border.right.y_offset=0;
        }
        if(all_map.find(all_map.at(mapString)->logicalMap.border_semi.top.fileName)!=all_map.cend())
        {
            simplifiedMapList[mapString].border.top.map=&simplifiedMapList[all_map.at(mapString)->logicalMap.border_semi.top.fileName];
            simplifiedMapList[mapString].border.top.x_offset=all_map.at(mapString)->logicalMap.border_semi.top.x_offset;
        }
        else
        {
            simplifiedMapList[mapString].border.top.map=NULL;
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

bool PathFinding::tryMove(SimplifiedMapForPathFinding **current_map,
                          uint8_t &x, uint8_t &y,const CatchChallenger::Orientation &orientation)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    switch(orientation)
    {
        case CatchChallenger::Orientation::Orientation_bottom:
        case CatchChallenger::Orientation::Orientation_top:
        case CatchChallenger::Orientation::Orientation_right:
        case CatchChallenger::Orientation::Orientation_left:
        break;
        default:
            abort();//wrong direction send
    }
    #endif
    switch(orientation)
    {
        case CatchChallenger::Orientation::Orientation_left:
            if(x>0)
            {
                x-=1;
                return true;
            }
            else if((*current_map)->border.left.map!=nullptr)
            {
                x=(*current_map)->border.left.map->width-1;
                y+=(*current_map)->border.left.y_offset;
                *current_map=(*current_map)->border.left.map;
                return true;
            }
        break;
        case CatchChallenger::Orientation::Orientation_right:
            if(x<((*current_map)->width-1))
            {
                x+=1;
                return true;
            }
            else if((*current_map)->border.right.map!=nullptr)
            {
                x=0;
                y+=(*current_map)->border.right.y_offset;
                *current_map=(*current_map)->border.right.map;
                return true;
            }
        break;
        case CatchChallenger::Orientation::Orientation_top:
            if(y>0)
            {
                y-=1;
                return true;
            }
            else if((*current_map)->border.top.map!=nullptr)
            {
                y=(*current_map)->border.top.map->height-1;
                x+=(*current_map)->border.top.x_offset;
                *current_map=(*current_map)->border.top.map;
                return true;
            }
        break;
        case CatchChallenger::Orientation::Orientation_bottom:
            if(y<((*current_map)->height-1))
            {
                y+=1;
                return true;
            }
            else if((*current_map)->border.bottom.map!=nullptr)
            {
                y=0;
                x+=(*current_map)->border.bottom.x_offset;
                *current_map=(*current_map)->border.bottom.map;
                return true;
            }
        break;
        default:
            return false;
    }
    return false;
}

bool PathFinding::canGo(const SimplifiedMapForPathFinding &simplifiedMapForPathFinding,
                          const uint8_t &x, const uint8_t &y)
{
    if(x>=simplifiedMapForPathFinding.width)
        return false;
    if(y>=simplifiedMapForPathFinding.height)
        return false;
    const uint8_t &var=simplifiedMapForPathFinding.simplifiedMap[x+y*(simplifiedMapForPathFinding.width)];
    return var==255 || var<200;
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
    if(simplifiedMapList.find(current_map)==simplifiedMapList.cend())
    {
        tryCancel=false;
        emit result(std::string(),0,0,std::vector<std::pair<CatchChallenger::Orientation,uint8_t> >());
        std::cerr << "bug: simplifiedMapList.find(current_map)==simplifiedMapList.cend()" << std::endl;
        return;
    }
    if(simplifiedMapList.at(current_map).simplifiedMap==nullptr)
        std::cout << "PathFinding::canGoOn() walkable is NULL for " << current_map << std::endl;
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
                std::cerr << "final dest" << std::endl;
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
                    {
                        MapPointToParse newPoint=tempPoint;
                        SimplifiedMapForPathFinding *simplifiedMapForPathFinding=&simplifiedMapList.at(newPoint.map);
                        if(PathFinding::tryMove(&simplifiedMapForPathFinding,newPoint.x,newPoint.y,CatchChallenger::Orientation::Orientation_right))
                        {
                            if(simplifiedMapForPathFinding->pathToGo.find(coord)!=simplifiedMapForPathFinding->pathToGo.cend())
                            {
                                const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=simplifiedMapForPathFinding->pathToGo.at(coord);
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
                    //if the left case have been parsed
                    {
                        MapPointToParse newPoint=tempPoint;
                        SimplifiedMapForPathFinding *simplifiedMapForPathFinding=&simplifiedMapList.at(newPoint.map);
                        if(PathFinding::tryMove(&simplifiedMapForPathFinding,newPoint.x,newPoint.y,CatchChallenger::Orientation::Orientation_left))
                        {
                            if(simplifiedMapForPathFinding->pathToGo.find(coord)!=simplifiedMapForPathFinding->pathToGo.cend())
                            {
                                const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=simplifiedMapForPathFinding->pathToGo.at(coord);
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
                    //if the top case have been parsed
                    {
                        MapPointToParse newPoint=tempPoint;
                        SimplifiedMapForPathFinding *simplifiedMapForPathFinding=&simplifiedMapList.at(newPoint.map);
                        if(PathFinding::tryMove(&simplifiedMapForPathFinding,newPoint.x,newPoint.y,CatchChallenger::Orientation::Orientation_top))
                        {
                            if(simplifiedMapForPathFinding->pathToGo.find(coord)!=simplifiedMapForPathFinding->pathToGo.cend())
                            {
                                const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=simplifiedMapForPathFinding->pathToGo.at(coord);
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
                    //if the bottom case have been parsed
                    {
                        MapPointToParse newPoint=tempPoint;
                        SimplifiedMapForPathFinding *simplifiedMapForPathFinding=&simplifiedMapList.at(newPoint.map);
                        if(PathFinding::tryMove(&simplifiedMapForPathFinding,newPoint.x,newPoint.y,CatchChallenger::Orientation::Orientation_bottom))
                        {
                            if(simplifiedMapForPathFinding->pathToGo.find(coord)!=simplifiedMapForPathFinding->pathToGo.cend())
                            {
                                const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=simplifiedMapForPathFinding->pathToGo.at(coord);
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
                        std::cerr << "Bug due for last step" << std::endl;
                        return;
                    }
                    else
                    {
                        std::cout << "Path result into " << time.elapsed() << "ms" << std::endl;
                        returnedVar.back().second--;
                        emit result(current_map,x,y,returnedVar);
                        return;
                    }
                }
                else
                {
                    returnedVar.clear();
                    std::cerr << "Bug due to resolved path is empty" << std::endl;
                    return;
                }
            }
            //revers resolv
            //add to point to parse
            {
                //if the right case have been parsed
                {
                    MapPointToParse newPoint=tempPoint;
                    SimplifiedMapForPathFinding *simplifiedMapForPathFinding=&simplifiedMapList.at(newPoint.map);
                    if(PathFinding::tryMove(&simplifiedMapForPathFinding,newPoint.x,newPoint.y,CatchChallenger::Orientation::Orientation_right))
                    {
                        if(PathFinding::canGo(*simplifiedMapForPathFinding,newPoint.x,newPoint.y) || (destination_map==newPoint.map && newPoint.x==destination_x && newPoint.y==destination_y))
                        {
                            std::pair<uint8_t,uint8_t> p(newPoint.x,newPoint.y);
                            if(simplifiedMapForPathFinding->pointQueued.find(p)==simplifiedMapForPathFinding->pointQueued.cend())
                            {
                                simplifiedMapForPathFinding->pointQueued.insert(p);
                                mapPointToParseList.push_back(newPoint);
                            }
                        }
                    }
                }
                //if the left case have been parsed
                {
                    MapPointToParse newPoint=tempPoint;
                    SimplifiedMapForPathFinding *simplifiedMapForPathFinding=&simplifiedMapList.at(newPoint.map);
                    if(PathFinding::tryMove(&simplifiedMapForPathFinding,newPoint.x,newPoint.y,CatchChallenger::Orientation::Orientation_left))
                    {
                        SimplifiedMapForPathFinding &simplifiedMap=simplifiedMapList[current_map];
                        if(PathFinding::canGo(simplifiedMap,newPoint.x,newPoint.y) || (destination_map==newPoint.map && newPoint.x==destination_x && newPoint.y==destination_y))
                        {
                            std::pair<uint8_t,uint8_t> p(newPoint.x,newPoint.y);
                            if(simplifiedMapForPathFinding->pointQueued.find(p)==simplifiedMapForPathFinding->pointQueued.cend())
                            {
                                simplifiedMapForPathFinding->pointQueued.insert(p);
                                mapPointToParseList.push_back(newPoint);
                            }
                        }
                    }
                }
                //if the bottom case have been parsed
                {
                    MapPointToParse newPoint=tempPoint;
                    SimplifiedMapForPathFinding *simplifiedMapForPathFinding=&simplifiedMapList.at(newPoint.map);
                    if(PathFinding::tryMove(&simplifiedMapForPathFinding,newPoint.x,newPoint.y,CatchChallenger::Orientation::Orientation_bottom))
                    {
                        SimplifiedMapForPathFinding &simplifiedMap=simplifiedMapList[current_map];
                        if(PathFinding::canGo(simplifiedMap,newPoint.x,newPoint.y) || (destination_map==newPoint.map && newPoint.x==destination_x && newPoint.y==destination_y))
                        {
                            std::pair<uint8_t,uint8_t> p(newPoint.x,newPoint.y);
                            if(simplifiedMapForPathFinding->pointQueued.find(p)==simplifiedMapForPathFinding->pointQueued.cend())
                            {
                                simplifiedMapForPathFinding->pointQueued.insert(p);
                                mapPointToParseList.push_back(newPoint);
                            }
                        }
                    }
                }
                //if the top case have been parsed
                {
                    MapPointToParse newPoint=tempPoint;
                    SimplifiedMapForPathFinding *simplifiedMapForPathFinding=&simplifiedMapList.at(newPoint.map);
                    if(PathFinding::tryMove(&simplifiedMapForPathFinding,newPoint.x,newPoint.y,CatchChallenger::Orientation::Orientation_top))
                    {
                        SimplifiedMapForPathFinding &simplifiedMap=simplifiedMapList[current_map];
                        if(PathFinding::canGo(simplifiedMap,newPoint.x,newPoint.y) || (destination_map==newPoint.map && newPoint.x==destination_x && newPoint.y==destination_y))
                        {
                            std::pair<uint8_t,uint8_t> p(newPoint.x,newPoint.y);
                            if(simplifiedMapForPathFinding->pointQueued.find(p)==simplifiedMapForPathFinding->pointQueued.cend())
                            {
                                simplifiedMapForPathFinding->pointQueued.insert(p);
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
