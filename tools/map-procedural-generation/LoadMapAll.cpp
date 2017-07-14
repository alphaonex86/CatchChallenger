#include "LoadMapAll.h"

#include "../../client/tiled/tiled_mapobject.h"
#include "../../client/tiled/tiled_objectgroup.h"
#include "../../general/base/cpp11addition.h"

#include "../map-procedural-generation-terrain/LoadMap.h"

#include <unordered_set>
#include <unordered_map>
#include <iostream>

std::vector<LoadMapAll::City> LoadMapAll::cities;
std::unordered_map<uint16_t,std::unordered_map<uint16_t,unsigned int> > LoadMapAll::citiesCoordToIndex;
uint8_t * LoadMapAll::mapPathDirection=NULL;

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

    Tiled::ObjectGroup *layerRoad=new Tiled::ObjectGroup("Road",0,0,worldMap.width(),worldMap.height());
    layerRoad->setColor(QColor("#ffcc22"));
    worldMap.addLayer(layerRoad);
    const unsigned int w=worldMap.width()/mapWidth;
    const unsigned int h=worldMap.height()/mapHeight;
    {
        unsigned int y=0;
        while(y<h)
        {
            unsigned int x=0;
            while(x<w)
            {
                const uint8_t &zoneOrientation=mapPathDirection[x+y*w];
                if(zoneOrientation!=0)
                {
                    std::string str;
                    if(zoneOrientation&Orientation_bottom)
                        str+=" bottom";
                    if(zoneOrientation&Orientation_top)
                        str+=" top";
                    if(zoneOrientation&Orientation_left)
                        str+=" left";
                    if(zoneOrientation&Orientation_right)
                        str+=" right";
                    QPolygonF poly(QRectF(0,0,mapWidth,mapHeight));
                    Tiled::MapObject *objectPolygon = new Tiled::MapObject(QString::fromStdString(str),"",QPointF(mapWidth*x,mapHeight*y), QSizeF(0.0,0.0));
                    objectPolygon->setPolygon(poly);
                    objectPolygon->setShape(Tiled::MapObject::Polygon);
                    layerRoad->addObject(objectPolygon);
                }
                x++;
            }
            y++;
        }
    }
}

bool LoadMapAll::haveCityEntryInternal(const std::unordered_map<uint32_t,std::unordered_map<uint32_t,CityInternal *> > &positionsAndIndex,
                              const unsigned int &x, const unsigned int &y)
{
    if(positionsAndIndex.find(x)==positionsAndIndex.cend())
        return false;
    const std::unordered_map<uint32_t,CityInternal *> &subEntry=positionsAndIndex.at(x);
    if(subEntry.find(y)==subEntry.cend())
        return false;
    return true;
}

bool LoadMapAll::haveCityEntry(const std::unordered_map<uint16_t,std::unordered_map<uint16_t,unsigned int> > &positionsAndIndex,
                              const unsigned int &x, const unsigned int &y)
{
    if(positionsAndIndex.find(x)==positionsAndIndex.cend())
        return false;
    const std::unordered_map<uint16_t,unsigned int> &subEntry=positionsAndIndex.at(x);
    if(subEntry.find(y)==subEntry.cend())
        return false;
    return true;
}

bool LoadMapAll::haveCityPath(const std::unordered_map<uint32_t,std::unordered_map<uint32_t,std::unordered_map<uint32_t,std::unordered_set<uint32_t> > > > &resolvedPath,
                              const unsigned int &x1,const unsigned int &y1,
                              const unsigned int &x2,const unsigned int &y2)
{
    if(resolvedPath.find(x1)!=resolvedPath.cend())
        if(resolvedPath.at(x1).find(y1)!=resolvedPath.at(x1).cend())
            if(resolvedPath.at(x1).at(y1).find(x2)!=resolvedPath.at(x1).at(y1).cend())
                if(resolvedPath.at(x1).at(y1).at(x2).find(y2)!=resolvedPath.at(x1).at(y1).at(x2).cend())
                    return true;
    if(resolvedPath.find(x2)!=resolvedPath.cend())
        if(resolvedPath.at(x2).find(y2)!=resolvedPath.at(x2).cend())
            if(resolvedPath.at(x2).at(y2).find(x1)!=resolvedPath.at(x2).at(y2).cend())
                if(resolvedPath.at(x2).at(y2).at(x1).find(y1)!=resolvedPath.at(x2).at(y2).at(x1).cend())
                    return true;
    return false;
}

LoadMapAll::Orientation LoadMapAll::reverseOrientation(const Orientation &orientation)
{
    switch (orientation) {
    case Orientation_bottom:
            return Orientation_top;
    case Orientation_top:
            return Orientation_bottom;
    case Orientation_left:
            return Orientation_right;
    case Orientation_right:
            return Orientation_left;
    default:
        return orientation;
    }
}

void LoadMapAll::addCity(Tiled::Map &worldMap, const Grid &grid, const std::vector<std::string> &citiesNames, const unsigned int &mapXCount, const unsigned int &mapYCount)
{
    if(grid.empty())
        return;
    std::vector<CityInternal *> citiesInternal;
    std::unordered_map<uint32_t,std::unordered_map<uint32_t,std::unordered_map<uint32_t,std::unordered_set<uint32_t> > > > resolvedPath;
    const unsigned int singleMapWitdh=worldMap.width()/mapXCount;
    const unsigned int singleMapHeight=worldMap.height()/mapYCount;
    const unsigned int sWH=singleMapWitdh*singleMapHeight;

    uint16_t mapWalkable[mapXCount][mapYCount];
    for(unsigned int i = 0; i < mapYCount; i++)
        for(unsigned int j = 0; j < mapXCount; j++)
            mapWalkable[j][i]=0;
    if(LoadMapAll::mapPathDirection!=NULL)
        delete LoadMapAll::mapPathDirection;
    LoadMapAll::mapPathDirection=new uint8_t[mapXCount*mapYCount];
    for(unsigned int i = 0; i < mapYCount; i++)
        for(unsigned int j = 0; j < mapXCount; j++)
            mapPathDirection[j+i*mapXCount]=0;
    {
        unsigned int y=0;
        while(y<mapYCount)
        {
            unsigned int x=0;
            while(x<mapXCount)
            {
                unsigned int countWalkable=0;
                {
                    //Tiled::TileLayer * tileLayer=LoadMap::searchTileLayerByName(worldMap,"Walkable");
                    unsigned int yMap=y*singleMapHeight;
                    while(yMap<(y*singleMapHeight+singleMapHeight))
                    {
                        unsigned int xMap=x*singleMapWitdh;
                        while(xMap<(x*singleMapWitdh+singleMapWitdh))
                        {
                            const VoronioForTiledMapTmx::PolygonZone &polygonZone=VoronioForTiledMapTmx::voronoiMap.zones.at(
                                        VoronioForTiledMapTmx::voronoiMap.tileToPolygonZoneIndex[x+y*singleMapWitdh].index
                                    );
                            /*
                            if(tileLayer->cellAt(xMap,yMap).tile!=NULL)
                                countWalkable++;walkable not fill at this call*/
                            if(polygonZone.height>0)
                                countWalkable++;
                            xMap++;
                        }
                        yMap++;
                    }
                }
                mapWalkable[x][y]=countWalkable;
                x++;
            }
            y++;
        }
    }
    std::unordered_map<uint32_t,std::unordered_map<uint32_t,CityInternal *> > positionsAndIndex;
    unsigned int index=0;
    while(index<grid.size())
    {
        const Point &centroid=grid.at(index);

        const uint32_t x=centroid.x();
        const uint32_t y=centroid.y();
        if(!haveCityEntryInternal(positionsAndIndex,x,y))
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
            unsigned int countWalkable=mapWalkable[x][y];
            //add
            if(countWalkable*100/(sWH)>75)
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
                if(haveCityEntryInternal(positionsAndIndex,city->x-1,city->y-1))
                    city->citiesNeighbor.push_back(positionsAndIndex.at(city->x-1).at(city->y-1));
            if(haveCityEntryInternal(positionsAndIndex,city->x-1,city->y))
                city->citiesNeighbor.push_back(positionsAndIndex.at(city->x-1).at(city->y));
            if(city->y<(mapYCount-1))
                if(haveCityEntryInternal(positionsAndIndex,city->x-1,city->y+1))
                    city->citiesNeighbor.push_back(positionsAndIndex.at(city->x-1).at(city->y+1));
        }
        if(city->y>0)
            if(haveCityEntryInternal(positionsAndIndex,city->x,city->y-1))
                city->citiesNeighbor.push_back(positionsAndIndex.at(city->x).at(city->y-1));
        if(city->y<(mapYCount-1))
            if(haveCityEntryInternal(positionsAndIndex,city->x,city->y+1))
                city->citiesNeighbor.push_back(positionsAndIndex.at(city->x).at(city->y+1));
        if(city->x<(mapXCount-1))
        {
            if(city->y>0)
                if(haveCityEntryInternal(positionsAndIndex,city->x+1,city->y-1))
                    city->citiesNeighbor.push_back(positionsAndIndex.at(city->x+1).at(city->y-1));
            if(haveCityEntryInternal(positionsAndIndex,city->x+1,city->y))
                city->citiesNeighbor.push_back(positionsAndIndex.at(city->x+1).at(city->y));
            if(city->y<(mapYCount-1))
                if(haveCityEntryInternal(positionsAndIndex,city->x+1,city->y+1))
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
    citiesCoordToIndex.clear();
    cities.clear();
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
            citiesCoordToIndex[city.x][city.y]=cities.size();
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
            if(maxX>(int)mapXCount)
                maxX=mapXCount;
            if(minY<0)
                minY=0;
            if(maxY>(int)mapYCount)
                maxY=mapYCount;

            //resolv the path
            std::vector<MapPointToParse> mapPointToParseList;
            SimplifiedMapForPathFinding tempMap;

            //init the first case
            {
                MapPointToParse tempPoint;
                tempPoint.x=city.x;
                tempPoint.y=city.y;
                mapPointToParseList.push_back(tempPoint);

                std::pair<uint16_t,uint16_t> coord(tempPoint.x,tempPoint.y);
                tempMap.pathToGo[coord].left.push_back(
                        std::pair<Orientation,uint8_t/*step number*/>(Orientation_left,1)
                            );
                tempMap.pathToGo[coord].right.push_back(
                        std::pair<Orientation,uint8_t/*step number*/>(Orientation_right,1)
                            );
                tempMap.pathToGo[coord].bottom.push_back(
                        std::pair<Orientation,uint8_t/*step number*/>(Orientation_bottom,1)
                            );
                tempMap.pathToGo[coord].top.push_back(
                        std::pair<Orientation,uint8_t/*step number*/>(Orientation_top,1)
                            );
            }

            std::pair<uint16_t,uint16_t> coord;
            while(!mapPointToParseList.empty())
            {
                const MapPointToParse tempPoint=mapPointToParseList.at(0);
                mapPointToParseList.erase(mapPointToParseList.begin());
                SimplifiedMapForPathFinding::PathToGo pathToGo;
                //resolv the own point
                int index=0;
                while(index<1)/*2*/
                {
                    {
                        //if the right case have been parsed
                        if(tempPoint.x+1<maxX)
                        {
                            coord=std::pair<uint16_t,uint16_t>(tempPoint.x+1,tempPoint.y);
                            if(tempMap.pathToGo.find(coord)!=tempMap.pathToGo.cend())
                            {
                                const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=tempMap.pathToGo.at(coord);
                                if(pathToGo.left.empty() || pathToGo.left.size()>nearPathToGo.left.size())
                                {
                                    pathToGo.left=nearPathToGo.left;
                                    pathToGo.left.back().second++;
                                }
                                if(pathToGo.top.empty() || pathToGo.top.size()>(nearPathToGo.left.size()+1))
                                {
                                    pathToGo.top=nearPathToGo.left;
                                    pathToGo.top.push_back(std::pair<Orientation,uint8_t/*step number*/>(Orientation_top,1));
                                }
                                if(pathToGo.bottom.empty() || pathToGo.bottom.size()>(nearPathToGo.left.size()+1))
                                {
                                    pathToGo.bottom=nearPathToGo.left;
                                    pathToGo.bottom.push_back(std::pair<Orientation,uint8_t/*step number*/>(Orientation_bottom,1));
                                }
                            }
                        }
                        //if the left case have been parsed
                        if(tempPoint.x>0)
                        {
                            coord=std::pair<uint16_t,uint16_t>(tempPoint.x-1,tempPoint.y);
                            if(tempMap.pathToGo.find(coord)!=tempMap.pathToGo.cend())
                            {
                                const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=tempMap.pathToGo.at(coord);
                                if(pathToGo.right.empty() || pathToGo.right.size()>nearPathToGo.right.size())
                                {
                                    pathToGo.right=nearPathToGo.right;
                                    pathToGo.right.back().second++;
                                }
                                if(pathToGo.top.empty() || pathToGo.top.size()>(nearPathToGo.right.size()+1))
                                {
                                    pathToGo.top=nearPathToGo.right;
                                    pathToGo.top.push_back(std::pair<Orientation,uint8_t/*step number*/>(Orientation_top,1));
                                }
                                if(pathToGo.bottom.empty() || pathToGo.bottom.size()>(nearPathToGo.right.size()+1))
                                {
                                    pathToGo.bottom=nearPathToGo.right;
                                    pathToGo.bottom.push_back(std::pair<Orientation,uint8_t/*step number*/>(Orientation_bottom,1));
                                }
                            }
                        }
                        //if the top case have been parsed
                        if(tempPoint.y+1<maxY)
                        {
                            coord=std::pair<uint16_t,uint16_t>(tempPoint.x,tempPoint.y+1);
                            if(tempMap.pathToGo.find(coord)!=tempMap.pathToGo.cend())
                            {
                                const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=tempMap.pathToGo.at(coord);
                                if(pathToGo.top.empty() || pathToGo.top.size()>nearPathToGo.top.size())
                                {
                                    pathToGo.top=nearPathToGo.top;
                                    pathToGo.top.back().second++;
                                }
                                if(pathToGo.left.empty() || pathToGo.left.size()>(nearPathToGo.top.size()+1))
                                {
                                    pathToGo.left=nearPathToGo.top;
                                    pathToGo.left.push_back(std::pair<Orientation,uint8_t/*step number*/>(Orientation_left,1));
                                }
                                if(pathToGo.right.empty() || pathToGo.right.size()>(nearPathToGo.top.size()+1))
                                {
                                    pathToGo.right=nearPathToGo.top;
                                    pathToGo.right.push_back(std::pair<Orientation,uint8_t/*step number*/>(Orientation_right,1));
                                }
                            }
                        }
                        //if the bottom case have been parsed
                        if(tempPoint.y>0)
                        {
                            coord=std::pair<uint16_t,uint16_t>(tempPoint.x,tempPoint.y-1);
                            if(tempMap.pathToGo.find(coord)!=tempMap.pathToGo.cend())
                            {
                                const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=tempMap.pathToGo.at(coord);
                                if(pathToGo.bottom.empty() || pathToGo.bottom.size()>nearPathToGo.bottom.size())
                                {
                                    pathToGo.bottom=nearPathToGo.bottom;
                                    pathToGo.bottom.back().second++;
                                }
                                if(pathToGo.left.empty() || pathToGo.left.size()>(nearPathToGo.bottom.size()+1))
                                {
                                    pathToGo.left=nearPathToGo.bottom;
                                    pathToGo.left.push_back(std::pair<Orientation,uint8_t/*step number*/>(Orientation_left,1));
                                }
                                if(pathToGo.right.empty() || pathToGo.right.size()>(nearPathToGo.bottom.size()+1))
                                {
                                    pathToGo.right=nearPathToGo.bottom;
                                    pathToGo.right.push_back(std::pair<Orientation,uint8_t/*step number*/>(Orientation_right,1));
                                }
                            }
                        }
                    }
                    index++;
                }
                coord=std::pair<uint16_t,uint16_t>(tempPoint.x,tempPoint.y);
                if(tempMap.pathToGo.find(coord)==tempMap.pathToGo.cend())
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    extraControlOnData(pathToGo.left,Orientation_left);
                    extraControlOnData(pathToGo.right,Orientation_right);
                    extraControlOnData(pathToGo.top,Orientation_top);
                    extraControlOnData(pathToGo.bottom,Orientation_bottom);
                    #endif
                    tempMap.pathToGo[coord]=pathToGo;
                }
                if(haveCityEntry(citiesCoordToIndex,tempPoint.x,tempPoint.y) && (tempPoint.x!=city.x || tempPoint.y!=city.y))
                {
                    std::vector<std::pair<Orientation,uint8_t/*step number*/> > returnedVar;
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
                            abort();
                        }
                        else
                        {
                            returnedVar.back().second--;
                            if(!haveCityPath(resolvedPath,city.x,city.y,tempPoint.x,tempPoint.y))
                            {
                                resolvedPath[city.x][city.y][tempPoint.x].insert(tempPoint.y);
                                if(haveCityEntry(citiesCoordToIndex,tempPoint.x,tempPoint.y) && (tempPoint.x!=city.x || tempPoint.y!=city.y))
                                {
                                    std::cout << "city from " << city.x << "," << city.y << " to " << tempPoint.x << "," << tempPoint.y << std::endl;
                                    uint16_t x=city.x;
                                    uint16_t y=city.y;
                                    unsigned int index=0;
                                    while(index<returnedVar.size())
                                    {
                                        std::pair<Orientation,uint8_t/*step number*/> &returnedLine=returnedVar[index];
                                        while(returnedLine.second>0)
                                        {
                                            mapPathDirection[x+y*mapXCount]|=returnedLine.first;
                                            //change tile
                                            if(returnedLine.first!=Orientation_none)
                                            {
                                                switch(returnedLine.first)
                                                {
                                                    case Orientation_bottom:y++;break;
                                                    case Orientation_top:y--;break;
                                                    case Orientation_right:x++;break;
                                                    case Orientation_left:x--;break;
                                                    default:abort();
                                                }
                                                mapPathDirection[x+y*mapXCount]|=LoadMapAll::reverseOrientation(returnedLine.first);
                                                returnedLine.second--;
                                            }
                                        }
                                        index++;
                                    }
                                }
                            }
                            /// \todo the city path
                        }
                    }
                    else
                    {
                        returnedVar.clear();
                        std::cerr << "Bug due to resolved path is empty" << std::endl;
                        abort();
                    }
                }
                //revers resolv
                //add to point to parse
                {
                    //if the right case have been parsed
                    if(tempPoint.x+1<maxX)
                    {
                        coord=std::pair<uint16_t,uint16_t>(tempPoint.x+1,tempPoint.y);
                        if(tempMap.pathToGo.find(coord)==tempMap.pathToGo.cend())
                        {
                            MapPointToParse newPoint=tempPoint;
                            newPoint.x++;
                            if(newPoint.x<maxX)
                                if(mapWalkable[newPoint.x][newPoint.y]*100/(sWH)>75 || haveCityEntry(citiesCoordToIndex,newPoint.x,newPoint.y))
                                {
                                    std::pair<uint16_t,uint16_t> point(newPoint.x,newPoint.y);
                                    if(tempMap.pointQueued.find(point)==tempMap.pointQueued.cend())
                                    {
                                        tempMap.pointQueued.insert(point);
                                        mapPointToParseList.push_back(newPoint);
                                    }
                                }
                        }
                    }
                    //if the left case have been parsed
                    if(tempPoint.x>0)
                    {
                        coord=std::pair<uint16_t,uint16_t>(tempPoint.x-1,tempPoint.y);
                        if(tempMap.pathToGo.find(coord)==tempMap.pathToGo.cend())
                        {
                            MapPointToParse newPoint=tempPoint;
                            if(newPoint.x>0)
                            {
                                newPoint.x--;
                                if(mapWalkable[newPoint.x][newPoint.y]*100/(sWH)>75 || haveCityEntry(citiesCoordToIndex,newPoint.x,newPoint.y))
                                {
                                    std::pair<uint16_t,uint16_t> point(newPoint.x,newPoint.y);
                                    if(tempMap.pointQueued.find(point)==tempMap.pointQueued.cend())
                                    {
                                        tempMap.pointQueued.insert(point);
                                        mapPointToParseList.push_back(newPoint);
                                    }
                                }
                            }
                        }
                    }
                    //if the bottom case have been parsed
                    if(tempPoint.y+1<maxY)
                    {
                        coord=std::pair<uint16_t,uint16_t>(tempPoint.x,tempPoint.y+1);
                        if(tempMap.pathToGo.find(coord)==tempMap.pathToGo.cend())
                        {
                            MapPointToParse newPoint=tempPoint;
                            newPoint.y++;
                            if(newPoint.y<maxY)
                                if(mapWalkable[newPoint.x][newPoint.y]*100/(sWH)>75 || haveCityEntry(citiesCoordToIndex,newPoint.x,newPoint.y))
                                {
                                    std::pair<uint16_t,uint16_t> point(newPoint.x,newPoint.y);
                                    if(tempMap.pointQueued.find(point)==tempMap.pointQueued.cend())
                                    {
                                        tempMap.pointQueued.insert(point);
                                        mapPointToParseList.push_back(newPoint);
                                    }
                                }
                        }
                    }
                    //if the top case have been parsed
                    if(tempPoint.y>0)
                    {
                        coord=std::pair<uint16_t,uint16_t>(tempPoint.x,tempPoint.y-1);
                        if(tempMap.pathToGo.find(coord)==tempMap.pathToGo.cend())
                        {
                            MapPointToParse newPoint=tempPoint;
                            if(newPoint.y>0)
                            {
                                newPoint.y--;
                                if(mapWalkable[newPoint.x][newPoint.y]*100/(sWH)>75 || haveCityEntry(citiesCoordToIndex,newPoint.x,newPoint.y))
                                {
                                    std::pair<uint16_t,uint16_t> point(newPoint.x,newPoint.y);
                                    if(tempMap.pointQueued.find(point)==tempMap.pointQueued.cend())
                                    {
                                        tempMap.pointQueued.insert(point);
                                        mapPointToParseList.push_back(newPoint);
                                    }
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
