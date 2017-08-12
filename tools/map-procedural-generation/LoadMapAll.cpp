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
std::vector<LoadMapAll::Road> LoadMapAll::roads;
std::unordered_map<uint16_t,std::unordered_map<uint16_t,LoadMapAll::RoadIndex> > LoadMapAll::roadCoordToIndex;
std::unordered_map<std::string,LoadMapAll::Zone> LoadMapAll::zones;

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
        QPolygonF poly(QRectF(0,0,mapWidth-4,mapHeight-4));
        Tiled::MapObject *objectPolygon = new Tiled::MapObject(QString::fromStdString(city.name+" ("+citySize+", level: "+std::to_string(city.level)+")"),"",QPointF(mapWidth*x+2,mapHeight*y+2), QSizeF(0.0,0.0));
        objectPolygon->setPolygon(poly);
        objectPolygon->setShape(Tiled::MapObject::Polygon);
        layerCity->addObject(objectPolygon);

        index++;
    }

    Tiled::ObjectGroup *layerRoad=new Tiled::ObjectGroup("Road",0,0,worldMap.width(),worldMap.height());
    layerRoad->setColor(QColor("#ffcc22"));
    worldMap.addLayer(layerRoad);
    const unsigned int w=worldMap.width()/mapWidth;
    //const unsigned int h=worldMap.height()/mapHeight;
    {
        unsigned int indexIntRoad=0;
        while(indexIntRoad<roads.size())
        {
            const Road &road=roads.at(indexIntRoad);
            unsigned int indexCoord=0;
            while(indexCoord<road.coords.size())
            {
                const std::pair<uint16_t,uint16_t> &coord=road.coords.at(indexCoord);
                const unsigned int &x=coord.first;
                const unsigned int &y=coord.second;

                const uint8_t &zoneOrientation=mapPathDirection[x+y*w];
                if(zoneOrientation!=0)
                {
                    //orientation
                    std::vector<std::string> orientationList;
                    if(zoneOrientation&Orientation_bottom)
                        orientationList.push_back("bottom");
                    if(zoneOrientation&Orientation_top)
                        orientationList.push_back("top");
                    if(zoneOrientation&Orientation_left)
                        orientationList.push_back("left");
                    if(zoneOrientation&Orientation_right)
                        orientationList.push_back("right");

                    const RoadIndex &indexRoad=roadCoordToIndex[x][y];

                    //compose string
                    std::string str="Road "+std::to_string(indexRoad.roadIndex+1)+" (";
                    if(road.haveOnlySegmentNearCity)
                    {
                        if(indexRoad.cityIndex.empty())
                        {
                            std::cerr << "road.haveOnlySegmentNearCity and indexRoad.cityIndex.empty()" << std::endl;
                            abort();
                        }
                        const RoadToCity &cityIndex=indexRoad.cityIndex.front();
                        str+=stringimplode(orientationList,",")+","+cities.at(cityIndex.cityIndex).name;
                    }
                    else
                        str+=stringimplode(orientationList,",");
                    str+=", level: "+std::to_string(indexRoad.level)+")";

                    //paint it
                    QPolygonF poly(QRectF(0,0,mapWidth,mapHeight));
                    Tiled::MapObject *objectPolygon = new Tiled::MapObject(QString::fromStdString(str),"",QPointF(mapWidth*x,mapHeight*y), QSizeF(0.0,0.0));
                    objectPolygon->setPolygon(poly);
                    objectPolygon->setShape(Tiled::MapObject::Polygon);
                    layerRoad->addObject(objectPolygon);
                }

                indexCoord++;
            }
            indexIntRoad++;
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

std::string LoadMapAll::orientationToString(const Orientation &orientation)
{
    switch (orientation) {
    case Orientation_bottom:
            return "bottom";
    case Orientation_top:
            return "top";
    case Orientation_left:
            return "left";
    case Orientation_right:
            return "right";
    default:
        return "unknown";
    }
}

void LoadMapAll::addCity(Tiled::Map &worldMap, const Grid &grid, const std::vector<std::string> &citiesNames,
                         const unsigned int &mapXCount, const unsigned int &mapYCount,
                         const unsigned int &maxCityLinks, const unsigned int &cityRadius, const Simplex &levelmap,
                         const float &levelmapscale, const unsigned int &levelmapmin, const unsigned int &levelmapmax,
                         const Simplex &heightmap, const Simplex &moisuremap, const float &noiseMapScaleMoisure, const float &noiseMapScaleMap)
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
    //detect the min and max level
    unsigned int maxLevel=0;
    unsigned int minLevel=999999;
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
            city.level=(levelmap.Get({(float)city.x,(float)city.y},levelmapscale)+1.0)/2.0*(levelmapmax-levelmapmin)+levelmapmin;
            if(city.level<minLevel)
                minLevel=city.level;
            if(city.level>maxLevel)
                maxLevel=city.level;
            citiesCoordToIndex[city.x][city.y]=cities.size();
            cities.push_back(city);
            delete cityInternal;
            index++;
        }
        break;
    }
    //calculate the zone
    {
        unsigned int indexCities=0;
        while(indexCities<cities.size())
        {
            City &city=cities[indexCities];
            //Calibration
            city.level=city.level-(minLevel-levelmapmin);
            if(city.level<levelmapmin)
                city.level=levelmapmin;

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

            if(maxCityLinks<2)
            {
                std::cerr << "maxCityLinks<2 abort" << std::endl;
                abort();
            }
            uint8_t citycount=0;
            std::pair<uint16_t,uint16_t> coord;
            while(!mapPointToParseList.empty() && citycount<maxCityLinks)
            {
                const MapPointToParse tempPoint=mapPointToParseList.at(0);
                mapPointToParseList.erase(mapPointToParseList.begin());
                SimplifiedMapForPathFinding::PathToGo pathToGo;
                //resolv the own point
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
                    if(tempPoint.x>minX)
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
                    if(tempPoint.y>minY)
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
                    citycount++;
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
                    //just display
                    const bool displayPath=false;
                    if(displayPath)
                    {
                        std::cout << "city from " << city.x << "," << city.y << " to " << tempPoint.x << "," << tempPoint.y << ": ";
                        unsigned int index=0;
                        while(index<returnedVar.size())
                        {
                            std::pair<Orientation,uint8_t/*step number*/> &returnedLine=returnedVar[index];
                            unsigned int indexSecond=0;
                            while(indexSecond<returnedLine.second)
                            {
                                //change tile
                                if(returnedLine.first!=Orientation_none)
                                {
                                    switch(returnedLine.first)
                                    {
                                        case Orientation_bottom:std::cout << "b";break;
                                        case Orientation_top:std::cout << "t";break;
                                        case Orientation_right:std::cout << "r";break;
                                        case Orientation_left:std::cout << "l";break;
                                        default:abort();
                                    }
                                }
                                indexSecond++;
                            }
                            index++;
                        }
                        std::cout << std::endl;
                    }
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
                    if(tempPoint.x>minX)
                    {
                        coord=std::pair<uint16_t,uint16_t>(tempPoint.x-1,tempPoint.y);
                        if(tempMap.pathToGo.find(coord)==tempMap.pathToGo.cend())
                        {
                            MapPointToParse newPoint=tempPoint;
                            if(newPoint.x>minX)
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
                    if(tempPoint.y>minY)
                    {
                        coord=std::pair<uint16_t,uint16_t>(tempPoint.x,tempPoint.y-1);
                        if(tempMap.pathToGo.find(coord)==tempMap.pathToGo.cend())
                        {
                            MapPointToParse newPoint=tempPoint;
                            if(newPoint.y>minY)
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
    //Do the road group
    {
        unsigned int mapY=0;
        while(mapY<mapYCount)
        {
            unsigned int mapX=0;
            while(mapX<mapXCount)
            {
                if(mapPathDirection[mapX+mapY*mapXCount]!=0 &&
                        !haveCityEntry(citiesCoordToIndex,mapX,mapY) &&
                        (roadCoordToIndex.find(mapX)==roadCoordToIndex.cend() || roadCoordToIndex.at(mapX).find(mapY)==roadCoordToIndex.at(mapX).cend())
                        )
                {
                    unsigned int roadIntIndex=roads.size();;
                    {
                        Road road;
                        road.haveOnlySegmentNearCity=true;
                        roads.push_back(road);
                    }
                    Road &road=roads.back();
                    std::vector<uint32_t> pointToScan;
                    std::vector<uint32_t> pointDone;
                    pointToScan.push_back(mapX+mapY*mapXCount);
                    while(!pointToScan.empty())
                    {
                        const uint16_t x=pointToScan.front()%mapXCount;
                        const uint16_t y=(pointToScan.front()-x)/mapXCount;
                        pointDone.push_back(pointToScan.front());
                        pointToScan.erase(pointToScan.cbegin());

                        RoadIndex roadIndex;
                        roadIndex.roadIndex=roadIntIndex;

                        //left tile
                        if(x>0)
                        {
                            const uint16_t newX=x-1;
                            const uint16_t newY=y;
                            if(!haveCityEntry(citiesCoordToIndex,newX,newY))
                            {
                                if(mapPathDirection[newX+newY*mapXCount]!=0)
                                    if(!vectorcontainsAtLeastOne(pointToScan,newX+newY*mapXCount) && !vectorcontainsAtLeastOne(pointDone,newX+newY*mapXCount))
                                        pointToScan.push_back(newX+newY*mapXCount);
                            }
                            else
                            {
                                const unsigned int cityIndex=citiesCoordToIndex.at(newX).at(newY);
                                RoadToCity roadToCity;
                                roadToCity.cityIndex=cityIndex;
                                roadToCity.orientation=Orientation_left;
                                roadIndex.cityIndex.push_back(roadToCity);
                                cities[cityIndex].nearRoad[roadIntIndex].push_back(Orientation_right);
                            }
                        }
                        //right tile
                        if(x<(mapXCount-1))
                        {
                            const uint16_t newX=x+1;
                            const uint16_t newY=y;
                            if(!haveCityEntry(citiesCoordToIndex,newX,newY))
                            {
                                if(mapPathDirection[newX+newY*mapXCount]!=0)
                                    if(!vectorcontainsAtLeastOne(pointToScan,newX+newY*mapXCount) && !vectorcontainsAtLeastOne(pointDone,newX+newY*mapXCount))
                                        pointToScan.push_back(newX+newY*mapXCount);
                            }
                            else
                            {
                                const unsigned int cityIndex=citiesCoordToIndex.at(newX).at(newY);
                                RoadToCity roadToCity;
                                roadToCity.cityIndex=cityIndex;
                                roadToCity.orientation=Orientation_right;
                                roadIndex.cityIndex.push_back(roadToCity);
                                cities[cityIndex].nearRoad[roadIntIndex].push_back(Orientation_left);
                            }
                        }
                        //top tile
                        if(y>0)
                        {
                            const uint16_t newX=x;
                            const uint16_t newY=y-1;
                            if(!haveCityEntry(citiesCoordToIndex,newX,newY))
                            {
                                if(mapPathDirection[newX+newY*mapXCount]!=0)
                                    if(!vectorcontainsAtLeastOne(pointToScan,newX+newY*mapXCount) && !vectorcontainsAtLeastOne(pointDone,newX+newY*mapXCount))
                                        pointToScan.push_back(newX+newY*mapXCount);
                            }
                            else
                            {
                                const unsigned int cityIndex=citiesCoordToIndex.at(newX).at(newY);
                                RoadToCity roadToCity;
                                roadToCity.cityIndex=cityIndex;
                                roadToCity.orientation=Orientation_top;
                                roadIndex.cityIndex.push_back(roadToCity);
                                cities[cityIndex].nearRoad[roadIntIndex].push_back(Orientation_bottom);
                            }
                        }
                        //bottom tile
                        if(y<(mapYCount-1))
                        {
                            const uint16_t newX=x;
                            const uint16_t newY=y+1;
                            if(!haveCityEntry(citiesCoordToIndex,newX,newY))
                            {
                                if(mapPathDirection[newX+newY*mapXCount]!=0)
                                    if(!vectorcontainsAtLeastOne(pointToScan,newX+newY*mapXCount) && !vectorcontainsAtLeastOne(pointDone,newX+newY*mapXCount))
                                        pointToScan.push_back(newX+newY*mapXCount);
                            }
                            else
                            {
                                const unsigned int cityIndex=citiesCoordToIndex.at(newX).at(newY);
                                RoadToCity roadToCity;
                                roadToCity.cityIndex=cityIndex;
                                roadToCity.orientation=Orientation_bottom;
                                roadIndex.cityIndex.push_back(roadToCity);
                                cities[cityIndex].nearRoad[roadIntIndex].push_back(Orientation_top);
                            }
                        }

                        road.coords.push_back(std::pair<uint16_t,uint16_t>(x,y));
                        roadCoordToIndex[x][y]=roadIndex;
                        if(roadIndex.cityIndex.empty())
                            road.haveOnlySegmentNearCity=false;
                    }
                }
                mapX++;
            }
            mapY++;
        }
    }

    //set the road level
    for(auto& p:LoadMapAll::roadCoordToIndex)
    {
        const unsigned int &x=p.first;
        for(auto& q:p.second)
        {
            const unsigned int &y=q.first;
            LoadMapAll::RoadIndex &roadIndex=q.second;
            unsigned int tempLevel=(levelmap.Get({(float)x,(float)y},levelmapscale)+1.0)/2.0*(levelmapmax-levelmapmin)+levelmapmin;
            roadIndex.level=tempLevel-(minLevel-levelmapmin);
            if(roadIndex.level<levelmapmin)
                roadIndex.level=levelmapmin;
            uint8_t levelDiff=((int)roadIndex.level)*100/10;
            if(levelDiff<2)
                levelDiff=2;
            std::vector<uint8_t> levelRange;
            uint8_t inferiorLevel=roadIndex.level-levelDiff;
            if(inferiorLevel<2)
                inferiorLevel=2;
            while(inferiorLevel<=(roadIndex.level+levelDiff))
            {
                levelRange.push_back(inferiorLevel);
                inferiorLevel++;
            }
            if(levelRange.size()<2)
            {
                std::cerr << "levelRange.size()<2" << std::endl;
                abort();
            }
            std::vector<std::pair<uint8_t,uint8_t> > minMaxLevel;
            uint8_t minMaxLevelIndex=0;
            while(minMaxLevelIndex<8)
            {
                //fixed level
                if(rand()%2==0)
                {
                    uint8_t randomIndex=rand()%levelRange.size();
                    minMaxLevel.push_back(std::pair<uint8_t,uint8_t>(levelRange.at(randomIndex),levelRange.at(randomIndex)));
                }
                else
                {
                    uint8_t randomIndexL=rand()%levelRange.size();
                    uint8_t randomIndexT=0;
                    do {
                        randomIndexT=rand()%levelRange.size();
                    } while(randomIndexT==randomIndexL);
                    if(randomIndexL<randomIndexT)
                        minMaxLevel.push_back(std::pair<uint8_t,uint8_t>(levelRange.at(randomIndexL),levelRange.at(randomIndexT)));
                    else
                        minMaxLevel.push_back(std::pair<uint8_t,uint8_t>(levelRange.at(randomIndexT),levelRange.at(randomIndexL)));
                }
                minMaxLevelIndex++;
            }

            //for now fixed number of monster
            const unsigned int numberOfMonster=5;

            float luckSum=0;
            unsigned int numberOfMonsterIndex=0;
            while(numberOfMonsterIndex<numberOfMonster)
            {
                //to fine grass use: VoronioForTiledMapTmx::voronoiMap;
                //but to do simpler, do height,moisure by map, not 4x4
                const unsigned int &height=LoadMap::floatToHigh(heightmap.Get({(float)x/100,(float)y/100},noiseMapScaleMap));
                const unsigned int &moisure=LoadMap::floatToMoisure(moisuremap.Get({(float)x,(float)y/100},noiseMapScaleMoisure));
                //take the monster list and clean it
                /// \todo clean it
                std::map<unsigned int,std::vector<LoadMap::TerrainMonster> > terrainMonsterMap=LoadMap::terrainList[height][moisure].terrainMonsters;
                //take proportional  random index into terrainMonsters
                std::vector<unsigned int> indexesProportional;
                for(auto const &it : terrainMonsterMap)
                    indexesProportional.insert(indexesProportional.cend(),it.first,it.first);
                unsigned int index=indexesProportional.at(rand()%indexesProportional.size());
                //take random monster
                const uint8_t randomLevelIndex=rand()%minMaxLevel.size();
                const std::vector<LoadMap::TerrainMonster> &localLuckMonster=terrainMonsterMap.at(index);
                const LoadMap::TerrainMonster &terrainMonster=localLuckMonster.at(rand()%localLuckMonster.size());
                LoadMapAll::RoadMonster roadMonster;
                roadMonster.luck=index;
                luckSum+=roadMonster.luck;
                roadMonster.minLevel=minMaxLevel.at(randomLevelIndex).first;
                roadMonster.maxLevel=minMaxLevel.at(randomLevelIndex).second;
                roadMonster.monsterId=terrainMonster.monster;
                roadIndex.roadMonsters.push_back(roadMonster);

                numberOfMonsterIndex++;
            }
            //normalise the luck
            {
                const float &ratioLuck=(float)100.0/luckSum;
                numberOfMonsterIndex=0;
                unsigned int newLuckSum=0;
                while(numberOfMonsterIndex<roadIndex.roadMonsters.size())
                {
                    LoadMapAll::RoadMonster &roadMonster=roadIndex.roadMonsters[numberOfMonsterIndex];
                    roadMonster.luck*=ratioLuck;
                    if(roadMonster.luck<1)
                        roadMonster.luck=1;
                    newLuckSum+=roadMonster.luck;
                    numberOfMonsterIndex++;
                }
                while(newLuckSum<100)
                {
                    LoadMapAll::RoadMonster &roadMonster=roadIndex.roadMonsters[rand()%roadIndex.roadMonsters.size()];
                    roadMonster.luck++;
                    newLuckSum++;
                }
                while(newLuckSum>100)
                {
                    LoadMapAll::RoadMonster &roadMonster=roadIndex.roadMonsters[rand()%roadIndex.roadMonsters.size()];
                    if(roadMonster.luck>1)
                    {
                        roadMonster.luck--;
                        newLuckSum--;
                    }
                }
            }
            //to drop random list and improve the compression ratio
            std::sort(roadIndex.roadMonsters.begin(),roadIndex.roadMonsters.end(),[](LoadMapAll::RoadMonster a, LoadMapAll::RoadMonster b) {
                return b.monsterId < a.monsterId;
            });
        }
    }
}
