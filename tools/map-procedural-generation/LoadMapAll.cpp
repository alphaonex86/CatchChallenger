#include "LoadMapAll.h"

#include "../../client/tiled/tiled_mapobject.h"
#include "../../client/tiled/tiled_objectgroup.h"
#include "../../general/base/cpp11addition.h"

#include "../map-procedural-generation-terrain/LoadMap.h"

#include <unordered_set>
#include <unordered_map>
#include <iostream>

std::vector<LoadMapAll::City> LoadMapAll::cities;

void LoadMapAll::addDebugCity(Tiled::Map &worldMap, unsigned int mapWidth, unsigned int mapHeight)
{
    Tiled::ObjectGroup *layerCity=new Tiled::ObjectGroup("City",0,0,worldMap.width(),worldMap.height());
    layerCity->setColor(QColor("#bb5555"));
    worldMap.addLayer(layerCity);

    unsigned int index=0;
    while(index<cities.size())
    {
        const City &city=cities.at(index);

        std::string citySize;
        switch(city.type) {
        case CityType_small:
            citySize="small";
            break;
        case CityType_medium:
            citySize="medium";
            break;
        default:
            citySize="big";
            break;
        }
        const uint32_t x=city.x;
        const uint32_t y=city.y;
        QPolygonF poly(QRectF(0,0,mapWidth,mapHeight));
        Tiled::MapObject *objectPolygon = new Tiled::MapObject(QString::fromStdString(city.name+" ("+citySize+")"),"",QPointF(mapWidth*x,mapHeight*y), QSizeF(0.0,0.0));
        objectPolygon->setPolygon(poly);
        objectPolygon->setShape(Tiled::MapObject::Polygon);
        layerCity->addObject(objectPolygon);

        index++;
    }
}

bool LoadMapAll::haveCityEntry(const std::unordered_map<uint32_t, std::unordered_map<uint32_t,CityInternal *> > &positionsAndIndex,
                              const unsigned int &x, const unsigned int &y)
{
    if(positionsAndIndex.find(x)==positionsAndIndex.cend())
        return false;
    const std::unordered_map<uint32_t,CityInternal *> &subEntry=positionsAndIndex.at(x);
    if(subEntry.find(y)==subEntry.cend())
        return false;
    return true;
}

void LoadMapAll::addCity(const Tiled::Map &worldMap,const Grid &grid, const std::vector<std::string> &citiesNames,const unsigned int &w, const unsigned int &h)
{
    if(grid.empty())
        return;
    std::vector<CityInternal *> citiesInternal;

    std::unordered_map<uint32_t,std::unordered_map<uint32_t,CityInternal *> > positionsAndIndex;
    unsigned int index=0;
    while(index<grid.size())
    {
        const Point &centroid=grid.at(index);

        const uint32_t x=centroid.x();
        const uint32_t y=centroid.y();
        if(!haveCityEntry(positionsAndIndex,x,y))
        {
            CityInternal *city=new CityInternal();
            //do the random value
            city->name=citiesNames.at(index);
            switch(rand()%3) {
            case 0:
                city->type=CityType_small;
                break;
            case 1:
                city->type=CityType_medium;
                break;
            default:
                city->type=CityType_big;
                break;
            }
            city->x=x;
            city->y=y;
            positionsAndIndex[x][y]=city;
            //count walkable tile
            unsigned int countWalkable=0;
            const unsigned int singleMapWitdh=worldMap.width()/w;
            const unsigned int singleMapHeight=worldMap.height()/h;
            {
                Tiled::TileLayer * tileLayer=LoadMap::searchTileLayerByName(worldMap,"Walkable");
                unsigned int yMap=y*singleMapHeight;
                while(yMap<(y*singleMapHeight+singleMapHeight))
                {
                    unsigned int xMap=x*singleMapWitdh;
                    while(xMap<(x*singleMapWitdh+singleMapWitdh))
                    {
                        if(tileLayer->cellAt(xMap,yMap).tile!=NULL)
                            countWalkable++;
                        xMap++;
                    }
                    yMap++;
                }
            }
            //add
            if(countWalkable*100/(singleMapWitdh*singleMapHeight)>75)
                citiesInternal.push_back(city);
        }

        index++;
    }

    //do top list of map with number of direct neighbor
    std::map<uint32_t,std::vector<CityInternal *> > citiesByNeighbor;
    index=0;
    while(index<citiesInternal.size())
    {
        CityInternal *city=citiesInternal.at(index);
        if(city->x>0)
        {
            if(city->y>0)
                if(haveCityEntry(positionsAndIndex,city->x-1,city->y-1))
                    city->citiesNeighbor.push_back(positionsAndIndex.at(city->x-1).at(city->y-1));
            if(haveCityEntry(positionsAndIndex,city->x-1,city->y))
                city->citiesNeighbor.push_back(positionsAndIndex.at(city->x-1).at(city->y));
            if(city->y<(h-1))
                if(haveCityEntry(positionsAndIndex,city->x-1,city->y+1))
                    city->citiesNeighbor.push_back(positionsAndIndex.at(city->x-1).at(city->y+1));
        }
        if(city->y>0)
            if(haveCityEntry(positionsAndIndex,city->x,city->y-1))
                city->citiesNeighbor.push_back(positionsAndIndex.at(city->x).at(city->y-1));
        if(city->y<(h-1))
            if(haveCityEntry(positionsAndIndex,city->x,city->y+1))
                city->citiesNeighbor.push_back(positionsAndIndex.at(city->x).at(city->y+1));
        if(city->x<(w-1))
        {
            if(city->y>0)
                if(haveCityEntry(positionsAndIndex,city->x+1,city->y-1))
                    city->citiesNeighbor.push_back(positionsAndIndex.at(city->x+1).at(city->y-1));
            if(haveCityEntry(positionsAndIndex,city->x+1,city->y))
                city->citiesNeighbor.push_back(positionsAndIndex.at(city->x+1).at(city->y));
            if(city->y<(h-1))
                if(haveCityEntry(positionsAndIndex,city->x+1,city->y+1))
                    city->citiesNeighbor.push_back(positionsAndIndex.at(city->x+1).at(city->y+1));
        }
        citiesByNeighbor[city->citiesNeighbor.size()].push_back(city);
        index++;
    }
    //drop the first and decremente their neighbor
    while(citiesByNeighbor.size()>1 || (citiesByNeighbor.find(0)==citiesByNeighbor.cend() && citiesByNeighbor.size()==1))
    {
        std::map<uint32_t,std::vector<CityInternal *> >::reverse_iterator rit;
        for(rit=citiesByNeighbor.rbegin(); rit!=citiesByNeighbor.rend(); ++rit) {
            uint32_t indexCity=rit->first;
            std::vector<CityInternal *> citiesList=rit->second;
            if(!citiesList.empty())
            {
                CityInternal * city=citiesList.front();
                if(city->citiesNeighbor.empty())
                    abort();
                unsigned int index=0;
                while(index<city->citiesNeighbor.size())
                {
                    CityInternal * cityNeighbor=city->citiesNeighbor.at(index);
                    const unsigned int oldCount=cityNeighbor->citiesNeighbor.size();
                    //remove from local
                    if(!vectorremoveOne(cityNeighbor->citiesNeighbor,city))
                        abort();
                    //remove into the global list
                    if(!vectorremoveOne(citiesByNeighbor[oldCount],cityNeighbor))
                        abort();
                    if(citiesByNeighbor[oldCount].empty())
                        citiesByNeighbor.erase(oldCount);
                    const unsigned int newCount=cityNeighbor->citiesNeighbor.size();
                    citiesByNeighbor[newCount].push_back(cityNeighbor);
                    index++;
                }
                if(!vectorremoveOne(citiesByNeighbor[indexCity],city))
                    abort();
                if(citiesByNeighbor[indexCity].empty())
                    citiesByNeighbor.erase(indexCity);
            }
            else
                citiesByNeighbor.erase(indexCity);
            break;
        }
    }
    //happen
    for(const auto& n:citiesByNeighbor) {
        const std::vector<CityInternal *> &citiesList=n.second;
        unsigned int index=0;
        while(index<citiesList.size())
        {
            const CityInternal *cityInternal=citiesList.at(index);
            City city;
            city.name=cityInternal->name;
            city.type=cityInternal->type;
            city.x=cityInternal->x;
            city.y=cityInternal->y;
            cities.push_back(city);
            delete cityInternal;
            index++;
        }
        break;
    }
    //calculate the zone
    {
        uint8_t cityRadius=4;
        unsigned int indexCities=0;
        while(indexCities<cities.size())
        {
            const City &city=cities.at(indexCities);
            int minX=(int)city.x-cityRadius;
            int maxX=(int)city.x+cityRadius;
            int minY=(int)city.y-cityRadius;
            int maxY=(int)city.y+cityRadius;
            if(minX<0)
                minX=0;
            if(maxX>w)
                maxX=w;
            if(minY<0)
                minY=0;
            if(maxY>h)
                maxY=h;

            QHash<QString,SimplifiedMapForPathFinding> simplifiedMapList;
            //resolv the path
            QList<MapPointToParse> mapPointToParseList;

            //init the first case
            {
                MapPointToParse tempPoint;
                tempPoint.x=x;
                tempPoint.y=y;
                mapPointToParseList <<  tempPoint;

                QPair<uint8_t,uint8_t> coord(tempPoint.x,tempPoint.y);
                SimplifiedMapForPathFinding &tempMap=simplifiedMapList[current_map];
                tempMap.pathToGo[coord].left <<
                        QPair<CatchChallenger::Orientation,uint8_t/*step number*/>(Orientation_left,1);
                tempMap.pathToGo[coord].right <<
                        QPair<CatchChallenger::Orientation,uint8_t/*step number*/>(Orientation_right,1);
                tempMap.pathToGo[coord].bottom <<
                        QPair<CatchChallenger::Orientation,uint8_t/*step number*/>(Orientation_bottom,1);
                tempMap.pathToGo[coord].top <<
                        QPair<CatchChallenger::Orientation,uint8_t/*step number*/>(Orientation_top,1);
            }

            QPair<uint8_t,uint8_t> coord;
            while(!mapPointToParseList.isEmpty())
            {
                const MapPointToParse &tempPoint=mapPointToParseList.takeFirst();
                SimplifiedMapForPathFinding::PathToGo pathToGo;
                if(destination_map==current_map && tempPoint.x==destination_x && tempPoint.y==destination_y)
                    std::cout << "final dest" << std::endl;
                //resolv the own point
                int index=0;
                while(index<1)/*2*/
                {
                    {
                        //if the right case have been parsed
                        coord=QPair<uint8_t,uint8_t>(tempPoint.x+1,tempPoint.y);
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
                                pathToGo.top << QPair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_top,1);
                            }
                            if(pathToGo.bottom.isEmpty() || pathToGo.bottom.size()>(nearPathToGo.left.size()+1))
                            {
                                pathToGo.bottom=nearPathToGo.left;
                                pathToGo.bottom << QPair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_bottom,1);
                            }
                        }
                        //if the left case have been parsed
                        coord=QPair<uint8_t,uint8_t>(tempPoint.x-1,tempPoint.y);
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
                                pathToGo.top << QPair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_top,1);
                            }
                            if(pathToGo.bottom.isEmpty() || pathToGo.bottom.size()>(nearPathToGo.right.size()+1))
                            {
                                pathToGo.bottom=nearPathToGo.right;
                                pathToGo.bottom << QPair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_bottom,1);
                            }
                        }
                        //if the top case have been parsed
                        coord=QPair<uint8_t,uint8_t>(tempPoint.x,tempPoint.y+1);
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
                                pathToGo.left << QPair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_left,1);
                            }
                            if(pathToGo.right.isEmpty() || pathToGo.right.size()>(nearPathToGo.top.size()+1))
                            {
                                pathToGo.right=nearPathToGo.top;
                                pathToGo.right << QPair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_right,1);
                            }
                        }
                        //if the bottom case have been parsed
                        coord=QPair<uint8_t,uint8_t>(tempPoint.x,tempPoint.y-1);
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
                                pathToGo.left << QPair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_left,1);
                            }
                            if(pathToGo.right.isEmpty() || pathToGo.right.size()>(nearPathToGo.bottom.size()+1))
                            {
                                pathToGo.right=nearPathToGo.bottom;
                                pathToGo.right << QPair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_right,1);
                            }
                        }
                    }
                    index++;
                }
                coord=QPair<uint8_t,uint8_t>(tempPoint.x,tempPoint.y);
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
                    QList<QPair<CatchChallenger::Orientation,uint8_t/*step number*/> > returnedVar;
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
                            std::cerr << "Bug due for last step" << std::endl;
                            abort();
                        }
                        else
                        {
                            returnedVar.last().second--;
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
                    coord=QPair<uint8_t,uint8_t>(tempPoint.x+1,tempPoint.y);
                    if(!simplifiedMapList.value(current_map).pathToGo.contains(coord))
                    {
                        MapPointToParse newPoint=tempPoint;
                        newPoint.x++;
                        if(newPoint.x<simplifiedMapList.value(current_map).width)
                            if(PathFinding::canGoOn(simplifiedMapList.value(current_map),newPoint.x,newPoint.y) || (destination_map==current_map && newPoint.x==destination_x && newPoint.y==destination_y))
                            {
                                QPair<uint8_t,uint8_t> point(newPoint.x,newPoint.y);
                                if(!simplifiedMapList.value(current_map).pointQueued.contains(point))
                                {
                                    simplifiedMapList[current_map].pointQueued << point;
                                    mapPointToParseList <<  newPoint;
                                }
                            }
                    }
                    //if the left case have been parsed
                    coord=QPair<uint8_t,uint8_t>(tempPoint.x-1,tempPoint.y);
                    if(!simplifiedMapList.value(current_map).pathToGo.contains(coord))
                    {
                        MapPointToParse newPoint=tempPoint;
                        if(newPoint.x>0)
                        {
                            newPoint.x--;
                            if(PathFinding::canGoOn(simplifiedMapList.value(current_map),newPoint.x,newPoint.y) || (destination_map==current_map && newPoint.x==destination_x && newPoint.y==destination_y))
                            {
                                QPair<uint8_t,uint8_t> point(newPoint.x,newPoint.y);
                                if(!simplifiedMapList.value(current_map).pointQueued.contains(point))
                                {
                                    simplifiedMapList[current_map].pointQueued << point;
                                    mapPointToParseList <<  newPoint;
                                }
                            }
                        }
                    }
                    //if the bottom case have been parsed
                    coord=QPair<uint8_t,uint8_t>(tempPoint.x,tempPoint.y+1);
                    if(!simplifiedMapList.value(current_map).pathToGo.contains(coord))
                    {
                        MapPointToParse newPoint=tempPoint;
                        newPoint.y++;
                        if(newPoint.y<simplifiedMapList.value(current_map).height)
                            if(PathFinding::canGoOn(simplifiedMapList.value(current_map),newPoint.x,newPoint.y) || (destination_map==current_map && newPoint.x==destination_x && newPoint.y==destination_y))
                            {
                                QPair<uint8_t,uint8_t> point(newPoint.x,newPoint.y);
                                if(!simplifiedMapList.value(current_map).pointQueued.contains(point))
                                {
                                    simplifiedMapList[current_map].pointQueued << point;
                                    mapPointToParseList <<  newPoint;
                                }
                            }
                    }
                    //if the top case have been parsed
                    coord=QPair<uint8_t,uint8_t>(tempPoint.x,tempPoint.y-1);
                    if(!simplifiedMapList.value(current_map).pathToGo.contains(coord))
                    {
                        MapPointToParse newPoint=tempPoint;
                        if(newPoint.y>0)
                        {
                            newPoint.y--;
                            if(PathFinding::canGoOn(simplifiedMapList.value(current_map),newPoint.x,newPoint.y) || (destination_map==current_map && newPoint.x==destination_x && newPoint.y==destination_y))
                            {
                                QPair<uint8_t,uint8_t> point(newPoint.x,newPoint.y);
                                if(!simplifiedMapList.value(current_map).pointQueued.contains(point))
                                {
                                    simplifiedMapList[current_map].pointQueued << point;
                                    mapPointToParseList <<  newPoint;
                                }
                            }
                        }
                    }
                }


            }

            indexCities++;
        }
    }
}
