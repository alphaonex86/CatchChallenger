#include "LoadMapAll.h"

#include "../map-procedural-generation-terrain/LoadMap.h"
#include "../map-procedural-generation-terrain/MapBrush.h"

#include "../../client/tiled/tiled_tileset.h"
#include "../../client/tiled/tiled_tile.h"
#include "../../client/tiled/tiled_objectgroup.h"
#include "../../client/tiled/tiled_mapobject.h"
#include "../../client/tiled/tiled_mapwriter.h"
#include "../../general/base/cpp11addition.h"

#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>

struct LedgeMarker
{
    unsigned int x;
    unsigned int y;
    unsigned int length;
    bool horizontal;
};

void LoadMapAll::addBuildingChain(const std::string &baseName, const std::string &description, const MapBrush::MapTemplate &mapTemplatebuilding, Tiled::Map &worldMap,
                                  const uint32_t &x, const uint32_t &y, const unsigned int mapWidth, const unsigned int mapHeight,
                                  const std::pair<uint8_t, uint8_t> pos, const City &city,const std::string &zone)
{
    bool ok=false;
    //search the brush door and retarget
    std::unordered_map<Tiled::MapObject*,Tiled::Properties> oldValue;
    std::vector<Tiled::MapObject*> doors=getDoorsListAndTp(mapTemplatebuilding.tiledMap);
    unsigned int index=0;
    while(index<(unsigned int)doors.size())
    {
        Tiled::MapObject* object=doors.at(index);
        Tiled::Properties properties=object->properties();
        oldValue[object]=object->properties();
        if(mapTemplatebuilding.otherMap.size()>1)
            properties["map"]=QString::fromStdString(baseName)+"/"+properties.value("map");
        else
            properties["map"]=QString::fromStdString(baseName);
        object->setProperties(properties);
        index++;
    }
    MapBrush::brushTheMap(worldMap,mapTemplatebuilding,x*mapWidth+pos.first,y*mapHeight+pos.second,MapBrush::mapMask,true);
    index=0;
    while(index<(unsigned int)doors.size())//reset to the old value
    {
        Tiled::MapObject* object=doors.at(index);
        object->setProperties(oldValue.at(object));
        index++;
    }
    doors.clear();
    //search next hop door and retarget
    {
        unsigned int indexMap=0;
        while(indexMap<mapTemplatebuilding.otherMap.size())
        {
            std::vector<Tiled::MapObject*> doorsLocale=getDoorsListAndTp(mapTemplatebuilding.otherMap.at(indexMap));
            doors.insert(doors.end(),doorsLocale.begin(),doorsLocale.end());
            unsigned int index=0;
            while(index<(unsigned int)doorsLocale.size())
            {
                Tiled::MapObject* object=doorsLocale.at(index);
                Tiled::Properties properties=object->properties();
                oldValue[object]=properties;
                if(properties.value("map").toStdString()==mapTemplatebuilding.name)
                {
                    if(mapTemplatebuilding.otherMap.size()>1)
                        properties["map"]="../"+QString::fromStdString(LoadMapAll::lowerCase(city.name));
                    else
                        properties["map"]=QString::fromStdString(LoadMapAll::lowerCase(city.name));
                    properties["x"]=QString::number(properties.value("x").toUInt(&ok)+pos.first);
                    if(!ok)
                    {
                        std::cerr << "For one tmx map, x is not a number: " << properties.value("x").toStdString() << std::endl;
                        abort();
                    }
                    properties["y"]=QString::number(properties.value("y").toUInt(&ok)+pos.second);
                    if(!ok)
                    {
                        std::cerr << "For one tmx map, y is not a number: " << properties.value("y").toStdString() << std::endl;
                        abort();
                    }
                    object->setProperties(properties);
                }
                index++;
            }
            indexMap++;
        }
    }
    //write all next hop
    index=0;
    while(index<(unsigned int)mapTemplatebuilding.otherMap.size())
    {
        Tiled::Map *nextHopMap=mapTemplatebuilding.otherMap.at(index);
        Tiled::Properties properties=nextHopMap->properties();
        std::string filePath="/dest/map/main/official/"+LoadMapAll::lowerCase(city.name)+"/"+baseName+".tmx";
        if(mapTemplatebuilding.otherMap.size()>1)
            filePath="/dest/map/main/official/"+LoadMapAll::lowerCase(city.name)+"/"+baseName+"/"+mapTemplatebuilding.otherMapName.at(index)+".tmx";

        QFileInfo fileInfo(QCoreApplication::applicationDirPath()+QString::fromStdString(filePath));
        QDir mapDir(fileInfo.absolutePath());
        if(!mapDir.mkpath(fileInfo.absolutePath()))
        {
            std::cerr << "Unable to create path: " << fileInfo.absolutePath().toStdString() << std::endl;
            abort();
        }
        Tiled::MapWriter maprwriter;
        nextHopMap->setProperties(Tiled::Properties());
        if(!maprwriter.writeMap(nextHopMap,fileInfo.absoluteFilePath()))
        {
            std::cerr << "Unable to write " << fileInfo.absoluteFilePath().toStdString() << std::endl;
            abort();
        }
        nextHopMap->setProperties(properties);

        {
            QString xmlPath(fileInfo.absoluteFilePath());
            xmlPath.remove(xmlPath.size()-4,4);
            xmlPath+=".xml";
            QFile xmlinfo(xmlPath);
            if(xmlinfo.open(QFile::WriteOnly))
            {
                QString content("<map");
                if(properties.contains("type"))
                    content+=" type=\""+properties.value("type")+"\"";
                if(!zone.empty())
                    content+=" zone=\""+QString::fromStdString(zone)+"\"";
                content+=">\n"
                         "  <name>"+QString::fromStdString(description)+"</name>\n"
                                                                        "</map>";
                QByteArray contentData(content.toUtf8());
                xmlinfo.write(contentData.constData(),contentData.size());
                xmlinfo.close();
            }
            else
            {
                std::cerr << "Unable to write " << xmlPath.toStdString() << std::endl;
                abort();
            }
        }
        index++;
    }
    //reset next hop
    {
        unsigned int index=0;
        while(index<(unsigned int)doors.size())//reset to the old value
        {
            Tiled::MapObject* object=doors.at(index);
            object->setProperties(oldValue.at(object));
            index++;
        }
    }
    doors.clear();
}

void LoadMapAll::loadMapTemplate(const char * folderName,MapBrush::MapTemplate &mapTemplate, const char * fileName, const unsigned int mapWidth, const unsigned int mapHeight, Tiled::Map &worldMap)
{
    Tiled::Map *map=LoadMap::readMap(QString("template/")+QString(folderName)+fileName+".tmx");
    if((unsigned int)map->width()>mapWidth)
    {
        std::cout << "map->width()>mapWitdh for city" << std::endl;
        abort();
    }
    if((unsigned int)map->height()>mapHeight)
    {
        std::cout << "map->height()>mapHeight for city" << std::endl;
        abort();
    }
    mapTemplate=MapBrush::tiledMapToMapTemplate(map,worldMap);
    //reset the auto detection to grab ALL
    mapTemplate.x=0;
    mapTemplate.y=0;
    mapTemplate.name=fileName;
    mapTemplate.width=map->width();
    mapTemplate.height=map->height();
    //force collision layer
    if(LoadMap::haveTileLayer(*map,"Walkable"))
        mapTemplate.baseLayerIndex=LoadMap::searchTileIndexByName(*map,"Walkable");
    else if(LoadMap::haveTileLayer(*map,"OnGrass"))
        mapTemplate.baseLayerIndex=LoadMap::searchTileIndexByName(*map,"OnGrass");
    //else do nothing

    if(mapTemplate.templateTilesetToMapTileset.empty())
    {
        std::cerr << "LoadMapAll::addCityContent(): mapTemplate.templateTilesetToMapTileset.empty()" << std::endl;
        abort();
    }

    //now search the next map
    std::vector<Tiled::Map *> mapList;
    std::vector<std::string> mapToLoad;
    std::unordered_map<std::string,unsigned int> fileToIndex;
    mapToLoad.push_back(fileName);
    while(!mapToLoad.empty())
    {
        const std::string mapFile=mapToLoad.front();
        mapToLoad.erase(mapToLoad.cbegin());
        Tiled::Map *mapPointer;
        if(mapFile==fileName)
            mapPointer=map;
        else
        {
            mapPointer=LoadMap::readMap(QString("template/")+QString(folderName)+QString::fromStdString(mapFile)+".tmx");
            mapTemplate.otherMapName.push_back(mapFile);
        }
        fileToIndex[mapFile]=mapList.size();
        mapList.push_back(mapPointer);
        std::vector<Tiled::MapObject*> doors=getDoorsListAndTp(mapPointer);
        unsigned int index=0;
        while(index<(unsigned int)doors.size())
        {
            Tiled::MapObject* object=doors.at(index);
            Tiled::Properties properties=object->properties();
            const std::string &mapString=properties.value("map").toStdString();
            if(fileToIndex.find(mapString)!=fileToIndex.cend())
            {}//properties["map"]=QString::fromStdString(mapString);
            else
            {
                unsigned int newIndex=fileToIndex.size();
                fileToIndex[mapString]=newIndex;
                mapToLoad.push_back(mapString);
            }
            object->setProperties(properties);
            index++;
        }
    }
    mapList.erase(mapList.cbegin());
    mapTemplate.otherMap=mapList;
    if(mapTemplate.otherMap.size()!=mapTemplate.otherMapName.size())
    {
        std::cerr << "mapTemplate.otherMap.size()!=mapTemplate.otherMapName.size()" << std::endl;
        abort();
    }
}

std::vector<Tiled::MapObject*> LoadMapAll::getDoorsListAndTp(Tiled::Map * map)
{
    std::vector<Tiled::MapObject*> doors;
    const Tiled::ObjectGroup * const objectGroup=LoadMap::searchObjectGroupByName(*map,"Moving");
    if(objectGroup!=NULL)
    {
        const QList<Tiled::MapObject*> &objects=objectGroup->objects();
        unsigned int index=0;
        while(index<(unsigned int)objects.size())
        {
            Tiled::MapObject* object=objects.at(index);
            if(object->type()=="door" || object->type()=="teleport on it" || object->type()=="teleport on push")
            {
                Tiled::Properties properties=object->properties();
                if(properties.contains("map"))
                    doors.push_back(object);
            }
            index++;
        }
    }
    return doors;
}

void LoadMapAll::addCityContent(Tiled::Map &worldMap, const unsigned int &mapXCount, const unsigned int &mapYCount,bool full)
{
    if(MapBrush::mapMask==NULL)
    {
        std::cerr << "MapBrush::mapMask==NULL (abort) into LoadMapAll::addCityContent" << std::endl;
        abort();
    }
    const unsigned int mapWidth=worldMap.width()/mapXCount;
    const unsigned int mapHeight=worldMap.height()/mapYCount;
    MapBrush::MapTemplate mapTemplateBig;
    MapBrush::MapTemplate mapTemplateMedium;
    MapBrush::MapTemplate mapTemplateSmall;
    loadMapTemplate("",mapTemplateBig,"city-big",mapWidth,mapHeight,worldMap);
    loadMapTemplate("",mapTemplateMedium,"city-medium",mapWidth,mapHeight,worldMap);
    loadMapTemplate("",mapTemplateSmall,"city-small",mapWidth,mapHeight,worldMap);

    MapBrush::MapTemplate mapTemplatebuildingshop;
    MapBrush::MapTemplate mapTemplatebuildingheal;
    MapBrush::MapTemplate mapTemplatebuilding1;
    MapBrush::MapTemplate mapTemplatebuilding2;
    MapBrush::MapTemplate mapTemplatebuildingbig1;
    if(full)
    {
        loadMapTemplate("building-shop/",mapTemplatebuildingshop,"building-shop",mapWidth,mapHeight,worldMap);
        loadMapTemplate("building-heal/",mapTemplatebuildingheal,"building-heal",mapWidth,mapHeight,worldMap);
        loadMapTemplate("building-1/",mapTemplatebuilding1,"building-1",mapWidth,mapHeight,worldMap);
        loadMapTemplate("building-2/",mapTemplatebuilding2,"building-2",mapWidth,mapHeight,worldMap);
        loadMapTemplate("building-big-1/",mapTemplatebuildingbig1,"building-big-1",mapWidth,mapHeight,worldMap);
    }

    unsigned int index=0;
    while(index<cities.size())
    {
        const City &city=cities.at(index);
        const std::string &cityLowerCaseName=LoadMapAll::lowerCase(city.name);
        const uint32_t x=city.x;
        const uint32_t y=city.y;
        Tiled::Map *map=NULL;
        MapBrush::MapTemplate mapTemplate;
        std::vector<std::pair<uint8_t,uint8_t> > positionBuilding;
        switch(city.type) {
        case CityType_small:
            map=mapTemplateSmall.tiledMap;
            mapTemplate=mapTemplateSmall;
            if(full)
            {
                positionBuilding.push_back(std::pair<uint8_t,uint8_t>(15,15));
                positionBuilding.push_back(std::pair<uint8_t,uint8_t>(24,15));
                positionBuilding.push_back(std::pair<uint8_t,uint8_t>(24,22));
                positionBuilding.push_back(std::pair<uint8_t,uint8_t>(15,22));
            }
            break;
        case CityType_medium:
            map=mapTemplateMedium.tiledMap;
            mapTemplate=mapTemplateMedium;
            if(full)
            {
                positionBuilding.push_back(std::pair<uint8_t,uint8_t>(15,15));
                positionBuilding.push_back(std::pair<uint8_t,uint8_t>(24,15));
                positionBuilding.push_back(std::pair<uint8_t,uint8_t>(24,22));
                positionBuilding.push_back(std::pair<uint8_t,uint8_t>(15,22));
            }
            break;
        default:
            map=mapTemplateBig.tiledMap;
            mapTemplate=mapTemplateBig;
            if(full)
            {
                positionBuilding.push_back(std::pair<uint8_t,uint8_t>(11,11));
                positionBuilding.push_back(std::pair<uint8_t,uint8_t>(11,18));
                positionBuilding.push_back(std::pair<uint8_t,uint8_t>(11,25));
                positionBuilding.push_back(std::pair<uint8_t,uint8_t>(20,11));
                positionBuilding.push_back(std::pair<uint8_t,uint8_t>(20,18));
                positionBuilding.push_back(std::pair<uint8_t,uint8_t>(20,25));
                positionBuilding.push_back(std::pair<uint8_t,uint8_t>(29,11));
                positionBuilding.push_back(std::pair<uint8_t,uint8_t>(29,18));
                positionBuilding.push_back(std::pair<uint8_t,uint8_t>(29,25));
            }
            break;
        }
        const uint8_t xoffset=(mapWidth-map->width())/2;
        const uint8_t yoffset=(mapHeight-map->height())/2;
        MapBrush::brushTheMap(worldMap,mapTemplate,x*mapWidth+xoffset,y*mapHeight+yoffset,MapBrush::mapMask,true);
        bool haveHeal=false;
        bool haveShop=false;
        unsigned int housecount=0;
        while(!positionBuilding.empty())
        {
            unsigned int randomIndex=rand()%positionBuilding.size();
            std::pair<uint8_t,uint8_t> pos=positionBuilding.at(randomIndex);
            positionBuilding.erase(positionBuilding.cbegin()+randomIndex);
            if(!haveHeal)
            {
                haveHeal=true;
                addBuildingChain("heal","Heal",mapTemplatebuildingheal,worldMap,x,y,mapWidth,mapHeight,pos,city,cityLowerCaseName);
            }
            else if(!haveShop)
            {
                haveShop=true;
                addBuildingChain("shop","Shop",mapTemplatebuildingshop,worldMap,x,y,mapWidth,mapHeight,pos,city,cityLowerCaseName);
            }
            else
            {
                unsigned int randmax=2;
                if(city.type==CityType_big)
                    randmax=3;
                switch(rand()%randmax)
                {
                case 0:
                    housecount++;
                    addBuildingChain("house-"+std::to_string(housecount),"House "+std::to_string(housecount),mapTemplatebuilding1,worldMap,x,y,mapWidth,mapHeight,pos,city,cityLowerCaseName);
                    break;
                case 1:
                default:
                    housecount++;
                    addBuildingChain("house-"+std::to_string(housecount),"House "+std::to_string(housecount),mapTemplatebuilding2,worldMap,x,y,mapWidth,mapHeight,pos,city,cityLowerCaseName);
                    break;
                case 2:
                    housecount++;
                    addBuildingChain("house-"+std::to_string(housecount),"House "+std::to_string(housecount),mapTemplatebuildingbig1,worldMap,x,y,mapWidth,mapHeight,pos,city,cityLowerCaseName);
                    break;
                }
            }
        }
        index++;
    }

    LoadMapAll::deleteMapList(mapTemplateBig);
    LoadMapAll::deleteMapList(mapTemplateMedium);
    LoadMapAll::deleteMapList(mapTemplateSmall);
    if(full)
    {
        LoadMapAll::deleteMapList(mapTemplatebuildingshop);
        LoadMapAll::deleteMapList(mapTemplatebuildingheal);
        LoadMapAll::deleteMapList(mapTemplatebuilding1);
        LoadMapAll::deleteMapList(mapTemplatebuilding2);
        LoadMapAll::deleteMapList(mapTemplatebuildingbig1);
    }
}

void LoadMapAll::addRoadContent(Tiled::Map &worldMap, const unsigned int &mapXCount, const unsigned int &mapYCount)
{
    const unsigned int mapWidth=worldMap.width()/mapXCount;
    const unsigned int mapHeight=worldMap.height()/mapYCount;
    const unsigned int w=worldMap.width()/mapWidth;
    const unsigned int h=worldMap.height()/mapHeight;

    unsigned int botCount = 0;

    MapBrush::MapTemplate mapTemplategrass;
    Tiled::TileLayer *waterLayer=LoadMap::searchTileLayerByName(worldMap,"Water");
    Tiled::TileLayer *ldownLayer=LoadMap::searchTileLayerByName(worldMap,"LedgesDown");
    Tiled::TileLayer *lleftLayer=LoadMap::searchTileLayerByName(worldMap,"LedgesLeft");
    Tiled::TileLayer *lrighLayer=LoadMap::searchTileLayerByName(worldMap,"LedgesRight");
    Tiled::TileLayer *grassLayer=LoadMap::searchTileLayerByName(worldMap,"Grass");
    const Tiled::Tileset * const invisibleTileset=LoadMap::searchTilesetByName(worldMap,"invisible");
    const Tiled::Tileset * const mainTileset=LoadMap::searchTilesetByName(worldMap,"t1.tsx");
    Tiled::Cell newCell;

    newCell.flippedAntiDiagonally=false;
    newCell.flippedHorizontally=false;
    newCell.flippedVertically=false;
    newCell.tile=invisibleTileset->tileAt(0);

    loadMapTemplate("",mapTemplategrass,"grass",mapWidth,mapHeight,worldMap);

    unsigned int y=0;

    while(y<h)
    {
        unsigned int x=0;
        while(x<w)
        {
            // For each chunk
            // Do the random road gen
            const uint8_t &zoneOrientation=mapPathDirection[x+y*w];
            const bool isCity=haveCityEntry(citiesCoordToIndex,x,y);

            if((zoneOrientation&(Orientation_bottom|Orientation_left|Orientation_right|Orientation_top)) != 0 && !isCity){

                unsigned int* map = NULL;

                // Generate road path [normal, grass, normal]
                for(unsigned int i =0; i<3; i++){
                    std::vector<RoadPart> roadParts;
                    unsigned int type = 1;

                    // Random center point
                    const unsigned int cx = mapWidth/2 + rand() % (mapWidth/4);
                    const unsigned int cy = mapHeight/2 + rand() % (mapHeight/4);

                    if(i==1) type = 3;
                    else type = 1;

                    if(zoneOrientation&Orientation_bottom){
                        auto parts = constructRandomRoad(Orientation_bottom, mapWidth /2, mapHeight+1, cx, cy, mapWidth, mapHeight, type);
                        roadParts.insert(roadParts.end(), parts.begin(), parts.end());
                    }

                    if(zoneOrientation&Orientation_top){
                        auto parts = constructRandomRoad(Orientation_top, mapWidth /2, 0, cx, cy, mapWidth, mapHeight, type);
                        roadParts.insert(roadParts.end(), parts.begin(), parts.end());
                    }

                    if(zoneOrientation&Orientation_left){
                        auto parts = constructRandomRoad(Orientation_left, 0, mapHeight/2, cx, cy, mapWidth, mapHeight, type);
                        roadParts.insert(roadParts.end(), parts.begin(), parts.end());
                    }

                    if(zoneOrientation&Orientation_right){
                        auto parts = constructRandomRoad(Orientation_right, mapWidth+1, mapHeight/2, cx, cy, mapWidth, mapHeight, type);
                        roadParts.insert(roadParts.end(), parts.begin(), parts.end());
                    }

                    if(roadParts.size() != 0){
                        auto cleanMap = cleanRoadPath(roadParts, mapWidth, mapHeight);

                        if(map == NULL){
                            map = cleanMap;
                        }else{
                            unsigned int length = mapWidth / 4 * mapHeight;
                            for(unsigned int i = 0; i<length; i++){
                                if(cleanMap[i] != 0)
                                    map[i] = cleanMap[i];
                            }

                            delete [] cleanMap;
                        }
                    }
                }

                // Paint the road
                for(unsigned int dx=0; dx<mapWidth; dx++){
                    for(unsigned int dy=0; dy<mapHeight; dy++){
                        const unsigned int tx = dx + mapWidth * x;
                        const unsigned int ty = dy + mapHeight * y;
                        const unsigned int type = map[(dx/2) + (dy/2)*(mapWidth/2)];

                        if((type & 0x1) == 0x1) {
                            const unsigned int &bitMask=tx+ty*worldMap.width();
                            const unsigned int maxMapSize=(worldMap.width()*worldMap.height()/8+1);
                            if(bitMask/8>=maxMapSize)
                                abort();
                            MapBrush::mapMask[bitMask/8]|=(1<<(7-bitMask%8));
                        }

                        if((type & 0x3) == 0x3 && waterLayer->cellAt(tx,ty).isEmpty()){
                            MapBrush::brushTheMap(worldMap,mapTemplategrass,tx,ty,MapBrush::mapMask);
                        }
                    }
                }

                // Ledges
                //*
                {
                    unsigned int size = mapHeight * mapWidth / 4;
                    std::vector<QPoint> entry;
                    std::vector<LedgeMarker> possibleLedge;

                    // LedgeMarker
                    int maxX = mapWidth/2-1;
                    int maxY = mapHeight/2-1;

                    for(int ty=1; ty < maxY; ty++){
                        for(int tx=1; tx < maxX; tx++){
                            unsigned int coord = tx + ty*(mapWidth/2);

                            if((map[coord] & 0x1) == 0x1){
                                int orientation = 0;
                                int directions = 0;

                                if((map[coord-1] & 0x1) == 0x1){
                                    orientation |= Orientation_left;
                                    directions ++;
                                }
                                if((map[coord+1] & 0x1) == 0x1){
                                    orientation |= Orientation_right;
                                    directions ++;
                                }
                                if((map[coord-mapWidth/2] & 0x1) == 0x1){
                                    orientation |= Orientation_top;
                                    directions ++;
                                }
                                if((map[coord+mapWidth/2] & 0x1) == 0x1){
                                    orientation |= Orientation_bottom;
                                    directions ++;
                                }

                                if(directions == 2){
                                    if(orientation == (Orientation_left | Orientation_right)){
                                        LedgeMarker m;
                                        m.horizontal = true;
                                        m.x = tx;
                                        m.y = ty;
                                        m.length = 1;
                                        possibleLedge.push_back(m);
                                    }else if(orientation == (Orientation_top | Orientation_bottom)){
                                        LedgeMarker m;
                                        m.horizontal = false;
                                        m.x = tx;
                                        m.y = ty;
                                        m.length = 1;
                                        possibleLedge.push_back(m);
                                    }
                                }else if(directions == 3){
                                    if(orientation == (Orientation_top | Orientation_bottom | Orientation_right)){
                                        LedgeMarker m;
                                        m.horizontal = true;
                                        m.x = tx;
                                        m.y = ty;
                                        m.length = 1;

                                        int dx = tx;

                                        while( dx < maxX){
                                            if((map[coord+m.length-1-mapWidth/2] & 0x1) == 0x1
                                                    || (map[coord+m.length-1+mapWidth/2] & 0x1) == 0x1){
                                                break;
                                            }

                                            if((map[coord+m.length] & 0xF1) == 0x1){
                                                possibleLedge.push_back(m);
                                                break;
                                            }
                                            dx++;
                                            m.length++;
                                        }
                                    }else if(orientation == (Orientation_left | Orientation_bottom | Orientation_right)){
                                        LedgeMarker m;
                                        m.horizontal = false;
                                        m.x = tx;
                                        m.y = ty;
                                        m.length = 1;

                                        int dy = ty;

                                        while( dy < maxY){
                                            if((map[coord+(m.length-1)*(mapWidth/2)-1] & 0x1) == 0x1
                                                    || (map[coord+(m.length-1)*(mapWidth/2)+1] & 0x1) == 0x1){
                                                break;
                                            }

                                            if((map[coord+m.length*(mapWidth/2)] & 0xF1) == 0x1){
                                                possibleLedge.push_back(m);
                                                break;
                                            }
                                            dy++;
                                            m.length++;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    while(!possibleLedge.empty()){
                        int s = rand() % possibleLedge.size();

                        LedgeMarker m = possibleLedge[s];

                        possibleLedge[s] = possibleLedge[possibleLedge.size()-1];
                        possibleLedge.pop_back();

                        if(rand() / (float)RAND_MAX > 0.7){
                            bool cancel = false;

                            for(unsigned int i=0; i<m.length; i++){
                                if(m.horizontal){
                                    bool inverted = rand()%2;
                                    if(inverted){
                                        map[m.x+(i+m.y)*mapWidth/2] |= 0x10;
                                        if((map[m.x+(i+m.y)*mapWidth/2-1] & 0x20) == 0x20){
                                            cancel = true;
                                            break;
                                        }
                                        if((map[m.x+(i+m.y)*mapWidth/2+1] & 0x20) == 0x20){
                                            cancel = true;
                                            break;
                                        }
                                    }else{
                                        map[i+m.x+(i+m.y)*mapWidth/2] |= 0x20;
                                        if((map[m.x+(i+m.y)*mapWidth/2-1] & 0x10) == 0x10){
                                            cancel = true;
                                            break;
                                        }
                                        if((map[m.x+(i+m.y)*mapWidth/2+1] & 0x10) == 0x10){
                                            cancel = true;
                                            break;
                                        }
                                    }
                                }else{
                                    map[i+m.x+i+m.y*mapWidth/2] |= 0x80;
                                }
                            }

                            if(cancel){
                                if(m.horizontal){
                                    for(unsigned int i=0; i<m.length; i++){
                                        map[m.x+(i+m.y)*mapWidth/2] &= ~0xF0;
                                    }
                                }else{
                                    for(unsigned int i=0; i<m.length; i++){
                                        map[i+m.x+m.y*mapWidth/2] &= ~0xF0;
                                    }
                                }
                            }else if(m.horizontal){
                                if(!checkPathing(map, mapWidth/2, mapHeight/2, m.x-1, m.y, m.x+1, m.y)
                                        || !checkPathing(map, mapWidth/2, mapHeight/2, m.x+1, m.y, m.x-1, m.y)){
                                    for(unsigned int i=0; i<m.length; i++){
                                        map[m.x+(i+m.y)*mapWidth/2] &= ~0xF0;
                                    }
                                }
                            }else{
                                if(!checkPathing(map, mapWidth/2, mapHeight/2, m.x, m.y-1, m.x, m.y+1)
                                        || !checkPathing(map, mapWidth/2, mapHeight/2, m.x, m.y+1, m.x, m.y-1)){
                                    for(unsigned int i=0; i<m.length; i++){
                                        map[i+m.x+m.y*mapWidth/2] &= ~0xF0;
                                    }
                                }
                            }

                        }
                    }

                    Tiled::Cell bottomLedge;
                    bottomLedge.tile=mainTileset->tileAt(740);
                    bottomLedge.flippedHorizontally=false;
                    bottomLedge.flippedVertically=false;
                    bottomLedge.flippedAntiDiagonally=false;

                    Tiled::Cell leftLedge;
                    leftLedge.tile=mainTileset->tileAt(808);
                    leftLedge.flippedHorizontally=false;
                    leftLedge.flippedVertically=false;
                    leftLedge.flippedAntiDiagonally=false;

                    Tiled::Cell rightLedge;
                    rightLedge.tile=mainTileset->tileAt(810);
                    rightLedge.flippedHorizontally=false;
                    rightLedge.flippedVertically=false;
                    rightLedge.flippedAntiDiagonally=false;

                    Tiled::Cell topLedge;
                    topLedge.tile=mainTileset->tileAt(740);
                    topLedge.flippedHorizontally=false;
                    topLedge.flippedVertically=true;
                    topLedge.flippedAntiDiagonally=false;

                    Tiled::Cell empty;
                    empty.tile=NULL;
                    empty.flippedHorizontally=false;
                    empty.flippedVertically=false;
                    empty.flippedAntiDiagonally=false;

                    for(unsigned int i = 0; i<size; i++){
                        unsigned int c = map[i] & 0xF0;
                        switch (c) {
                        case 0x10:
                            lleftLayer->setCell((i*2)%mapWidth+x*mapWidth, y*mapHeight+(i*2)/mapWidth*2+1, leftLedge);
                            grassLayer->setCell((i*2)%mapWidth+x*mapWidth, y*mapHeight+(i*2)/mapWidth*2+1, empty);
                            lleftLayer->setCell((i*2)%mapWidth+x*mapWidth, y*mapHeight+(i*2)/mapWidth*2, leftLedge);
                            grassLayer->setCell((i*2)%mapWidth+x*mapWidth, y*mapHeight+(i*2)/mapWidth*2, empty);
                            break;
                        case 0x20:
                            lrighLayer->setCell((i*2)%mapWidth+x*mapWidth+1, y*mapHeight+(i*2)/mapWidth*2+1, rightLedge);
                            grassLayer->setCell((i*2)%mapWidth+x*mapWidth+1, y*mapHeight+(i*2)/mapWidth*2+1, empty);
                            lrighLayer->setCell((i*2)%mapWidth+x*mapWidth+1, y*mapHeight+(i*2)/mapWidth*2, rightLedge);
                            grassLayer->setCell((i*2)%mapWidth+x*mapWidth+1, y*mapHeight+(i*2)/mapWidth*2, empty);
                            break;
                        case 0x40: // Unused
                            ldownLayer->setCell((i*2)%mapWidth+x*mapWidth, y*mapHeight+(i*2)/mapWidth*2, topLedge);
                            grassLayer->setCell((i*2)%mapWidth+x*mapWidth, y*mapHeight+(i*2)/mapWidth*2, empty);
                            ldownLayer->setCell((i*2)%mapWidth+x*mapWidth+1, y*mapHeight+(i*2)/mapWidth*2, topLedge);
                            grassLayer->setCell((i*2)%mapWidth+x*mapWidth+1, y*mapHeight+(i*2)/mapWidth*2, empty);
                            break;
                        case 0x80:
                            ldownLayer->setCell((i*2)%mapWidth+x*mapWidth, y*mapHeight+(i*2)/mapWidth*2+1, bottomLedge);
                            grassLayer->setCell((i*2)%mapWidth+x*mapWidth, y*mapHeight+(i*2)/mapWidth*2+1, empty);
                            ldownLayer->setCell((i*2)%mapWidth+x*mapWidth+1, y*mapHeight+(i*2)/mapWidth*2+1, bottomLedge);
                            grassLayer->setCell((i*2)%mapWidth+x*mapWidth+1, y*mapHeight+(i*2)/mapWidth*2+1, empty);
                            break;
                        default:
                            break;
                        }
                    }
                }//*/

                // Add bots
                {
                    Tiled::ObjectGroup *objectLayer=LoadMap::searchObjectGroupByName(worldMap,"Object");
                    LoadMapAll::RoadIndex &roadIndex=LoadMapAll::roadCoordToIndex.at(x).at(y);
                    const char* directions[] = {"left", "right", "up", "bottom"};
                    std::string file = getMapFile(x, y);
                    std::string filename = file.substr(file.find_last_of('/')+1);

                    for(int i = 0; i<15; i++){
                        unsigned int ox = rand()%mapWidth;
                        unsigned int oy = rand()%mapHeight;
                        unsigned int j = (ox/2) + (oy/2)*(mapWidth/2);

                        if((map[j] & 0x5) == 0x1){
                            map[j] |= 0x4;

                            RoadBot roadBot;
                            roadBot.x = ox + x * mapWidth;
                            roadBot.y = oy + y * mapHeight;
                            roadBot.id = botCount;
                            roadBot.look_at = rand()%4;
                            roadBot.skin = rand()%80;
                            roadIndex.roadBot.push_back(roadBot);

                            Tiled::MapObject *bot = new Tiled::MapObject("", "bot", QPointF(roadBot.x, roadBot.y), QSizeF(1, 1));
                            bot->setProperty("file", QString::fromStdString(filename+"-bots"));
                            bot->setProperty("id", QString::number(roadBot.id));
                            bot->setProperty("lookAt", directions[roadBot.look_at]);
                            bot->setProperty("skin", QString::number(roadBot.skin));
                            bot->setCell(newCell);
                            objectLayer->addObject(bot);

                            botCount++;
                        }
                    }
                    if(map != NULL) delete []map;
                }
            }
            x++;
        }
        y++;
    }
    LoadMapAll::deleteMapList(mapTemplategrass);
}

std::vector<LoadMapAll::RoadPart> LoadMapAll::constructRandomRoad(unsigned int orientation, unsigned int sx, unsigned int sy, unsigned int dx, unsigned int dy, unsigned int width, unsigned int height, unsigned int type){
    std::vector<LoadMapAll::RoadPart> parts;
    unsigned int main = 7;
    unsigned int side = 5;
    unsigned int step = 2;
    unsigned int size = 2;

    size --;

    dx = (dx/step)*step;
    dy = (dy/step)*step;
    sx = (sx/step)*step;
    sy = (sy/step)*step;

    if(orientation == Orientation_bottom){ // Bottom
        while(sy > dy){
            unsigned int tx;
            unsigned int ty = sy - (rand() % main)*step-step;

            do{
                tx = sx + (rand() % side  - side / 2 )* step;
            }while(tx > width);

            RoadPart p(sx, sy, sx+size, ty+size, type);
            RoadPart p2(sx, ty, tx+size, ty+size, type);

            parts.push_back(p);
            parts.push_back(p2);

            sx = tx;
            sy = ty;
        }
    }else if(orientation == Orientation_top){ // Top
        while(sy < dy){
            unsigned int tx;
            unsigned int ty = sy + (rand() % main)*step+step;

            do{
                tx = sx + (rand() % side - side / 2 ) * step;
            }while(tx > width);

            RoadPart p(sx, sy, sx+size, ty+size, type);
            RoadPart p2(sx, ty, tx+size, ty+size, type);

            parts.push_back(p);
            parts.push_back(p2);

            sx = tx;
            sy = ty;
        }
    }else if(orientation == Orientation_left){ // Left
        while(sx < dx){
            unsigned int tx = sx + (rand() % main)*step+step;
            unsigned int ty;

            do{
                ty = sy + (rand() % side - side / 2 )*step;
            }while(ty > height);

            RoadPart p(sx, sy, tx+size, sy+size, type);
            RoadPart p2(tx, sy, tx+size, ty+size, type);

            parts.push_back(p);
            parts.push_back(p2);

            sx = tx;
            sy = ty;
        }
    }else if(orientation == Orientation_right){ // Right
        while(sx > dx){
            unsigned int tx = sx - (rand() % main)*step-step;
            unsigned int ty;

            do{
                ty = sy + (rand() % side - side / 2 )*step;
            }while(ty > height);

            RoadPart p(sx, sy, tx+size, sy+size, type);
            RoadPart p2(tx, sy, tx+size, ty+size, type);

            parts.push_back(p);
            parts.push_back(p2);

            sx = tx;
            sy = ty;
        }
    }

    if(sx - dx > size){
        parts.push_back(RoadPart(sx, sy, dx+size, sy+size, type));
    }

    if(sy - dy > size){
        parts.push_back(RoadPart(sx, sy, sx+size, dy+size, type));
    }

    return parts;
}

unsigned int* LoadMapAll::cleanRoadPath(std::vector<RoadPart> &path, unsigned int width, unsigned int height){
    unsigned int size = 2;
    width /= size;
    height /= size;
    unsigned int* map = new unsigned int[width*height];

    for(unsigned int i=0; i<width*height; i++){
        map[i] = 0;
    }

    for(RoadPart p: path){
        unsigned int sx = p.sx;
        unsigned int sy = p.sy;
        unsigned int dx = p.dx;
        unsigned int dy = p.dy;

        if(dx < sx){
            unsigned int tmp = sx;
            sx = dx;
            dx = tmp;
        }
        if(dy < sy){
            unsigned int tmp = sy;
            sy = dy;
            dy = tmp;
        }

        while(sy <= dy){
            unsigned int tx = sx;
            while(tx <= dx){
                map[(tx/size) + ((unsigned int)(sy/size)*width)] = p.type;
                tx+=size;
            }
            sy+=size;
        }
    }

    bool done = false;

    //*
    while(!done){
        done = true;

        for(unsigned int x=1; x<width-1; x++){
            for(unsigned int y=1; y<height-1; y++){
                if(map[x+(y*width)] != 0){
                    int neighbour = 0;

                    if(map[x-1+y*width] != 0) neighbour++;
                    if(map[x+(y-1)*width] != 0) neighbour++;
                    if(map[x+1+y*width] != 0) neighbour++;
                    if(map[x+(y+1)*width] != 0) neighbour++;

                    if(neighbour < 2){
                        done = false;
                        map[x+(y*width)] = 0;
                        //qDebug() << "neighbour : "<<neighbour<<x<<y;
                    }
                }
            }
        }
    }//*/

    return map;
}

bool LoadMapAll::checkPathing(unsigned int *map, unsigned int width, unsigned int height, unsigned int sx, unsigned int sy, unsigned int dx, unsigned int dy)
{
    bool *valid = new bool[width*height];
    std::vector<unsigned int> pathLeft;
    unsigned int target = dx+dy*width;

    for(unsigned int i =0; i<width*height;i++){
        valid[i] = ((map[i] & 0x1) == 0x1);
    }

    if(!valid[target]){
        delete[] valid;
        return false;
    }

    int s = 0;
    pathLeft.push_back(sx+sy*width);
    valid[pathLeft.at(0)] = false;

    while(!pathLeft.empty()){
        unsigned int point = pathLeft.at(pathLeft.size()-1);
        pathLeft.pop_back();

        if(!valid[target]){
            delete[] valid;
            return true;
        }

        if((point%width) != 0 && valid[point-1] && (map[point-1]&0xE0) == 0){
            valid[point-1] = false;
            pathLeft.push_back(point-1);
        }
        if((point%width) != width-1 && valid[point+1] && (map[point+1]&0xD0) == 0){
            valid[point+1] = false;
            pathLeft.push_back(point+1);
        }
        if(point>=width && valid[point-width] && (map[point-width]&0xB0) == 0){
            valid[point-width] = false;
            pathLeft.push_back(point-width);
        }
        if(point+width<width*height && valid[point+width] && (map[point+width]&0x70) == 0){
            valid[point+width] = false;
            pathLeft.push_back(point+width);
        }
        s++;
    }

    delete[] valid;

    return false;
}

void LoadMapAll::writeRoadContent(Tiled::Map &worldMap, const unsigned int &mapXCount, const unsigned int &mapYCount)
{
    const unsigned int mapWidth=worldMap.width()/mapXCount;
    const unsigned int mapHeight=worldMap.height()/mapYCount;
    const unsigned int w=worldMap.width()/mapWidth;
    const unsigned int h=worldMap.height()/mapHeight;
    unsigned int y=0;
    unsigned fightId = 0;

    QString fightDir = QCoreApplication::applicationDirPath()+"/dest/map/main/official/fight/";

    if(!QDir(fightDir).exists()){
        QDir().mkdir(fightDir);
    }

    while(y<h)
    {
        unsigned int x=0;
        while(x<w)
        {
            const uint8_t &zoneOrientation=mapPathDirection[x+y*w];
            const bool isCity=haveCityEntry(citiesCoordToIndex,x,y);

            if(zoneOrientation != 0 && !isCity){
                std::string file = getMapFile(x, y);
                std::string filename = file.substr(file.find_last_of('/')+1); // parent folder + name
                std::string fightname = file.substr(file.find_last_of('/', file.find_last_of('/')-1)+1); // parent folder + name
                std::replace( fightname.begin(), fightname.end(), '/', '-');

                QString botxml = "<bots>\n";
                QString fightxml = "<fights>\n";
                const LoadMapAll::RoadIndex &roadIndex=LoadMapAll::roadCoordToIndex.at(x).at(y);

                for(RoadBot bot: roadIndex.roadBot){
                    if(bot.x >= mapWidth*x && bot.x < (x+1)*mapWidth
                            && bot.y >= mapHeight*y && bot.y < (y+1)*mapHeight
                            && !roadIndex.roadMonsters.empty()){
                        int monster = rand()%2 + rand()%3 +1; // Max: 4
                        int reward = roadIndex.level * 30 + 100;
                        fightId++;

                        botxml += "<bot id=\"" + QString::number(bot.id) + "\">\n";
                        botxml += " <name>" + QString::number(bot.id) + "</name>\n";
                        botxml += " <step fightid=\"" + QString::number(fightId) + "\" id=\"1\" type=\"fight\"/>\n";
                        botxml += "</bot>\n";

                        fightxml += "<fight id=\""+QString::number(fightId)+"\">\n";
                        fightxml += " <start><![CDATA[I lost to trainers before you, so I don't mind.]]></start>\n";

                        for(int i = 0; i<monster; i++){
                            int selected = rand() % roadIndex.roadMonsters.size();
                            int level = roadIndex.level * (95. + rand()%10) / 100.;
                            fightxml += " <monster id=\""+QString::number(roadIndex.roadMonsters.at(selected).monsterId)+"\" level=\""+QString::number(level)+"\"/>\n";
                            reward += level * level;
                        }
                        fightxml += " <gain cash=\""+QString::number(reward)+"\"/>\n";

                        fightxml += "</fight>\n";
                    }
                }

                botxml += "</bots>";
                fightxml += "</fights>";
                QFile botinfo(QString::fromStdString(file+"-bots.xml"));
                QFile fightinfo(fightDir+QString::fromStdString(fightname)+".xml");

                if(botinfo.open(QFile::WriteOnly))
                {
                    QByteArray contentData(botxml.toUtf8());
                    botinfo.write(contentData.constData(),contentData.size());
                    botinfo.close();
                }
                else
                {
                    std::cerr << "Unable to write bot file " << filename << std::endl;
                    abort();
                }

                if(fightinfo.open(QFile::WriteOnly))
                {
                    QByteArray contentData(fightxml.toUtf8());
                    fightinfo.write(contentData.constData(),contentData.size());
                    fightinfo.close();
                }
                else
                {
                    std::cerr << "Unable to write fight file " << fightname << std::endl;
                    abort();
                }
            }
            x++;
        }
        y++;
    }
}

void LoadMapAll::deleteMapList(MapBrush::MapTemplate &mapTemplatebuilding)
{
    delete mapTemplatebuilding.tiledMap;
    mapTemplatebuilding.tiledMap=NULL;
    unsigned int index=0;
    while(index<mapTemplatebuilding.otherMap.size())
    {
        delete mapTemplatebuilding.otherMap.at(index);
        index++;
    }
    mapTemplatebuilding.otherMap.clear();
    mapTemplatebuilding.otherMapName.clear();
}

void LoadMapAll::addMapChange(Tiled::Map &worldMap, const unsigned int &mapXCount, const unsigned int &mapYCount)
{
    Tiled::ObjectGroup * const movingLayer=LoadMap::searchObjectGroupByName(worldMap,"Moving");
    const Tiled::Tileset * const invisibleTileset=LoadMap::searchTilesetByName(worldMap,"invisible");

    Tiled::Cell newCell;
    newCell.flippedAntiDiagonally=false;
    newCell.flippedHorizontally=false;
    newCell.flippedVertically=false;
    newCell.tile=invisibleTileset->tileAt(3);

    const unsigned int mapWidth=worldMap.width()/mapXCount;
    const unsigned int mapHeight=worldMap.height()/mapYCount;
    unsigned int y=0;
    while(y<mapYCount)
    {
        unsigned int x=0;
        while(x<mapXCount)
        {
            const uint8_t &zoneOrientation=LoadMapAll::mapPathDirection[x+y*mapXCount];
            if(zoneOrientation!=0)
            {
                QDir mapDir(QFileInfo(QString::fromStdString(LoadMapAll::getMapFile(x,y))).absoluteDir());
                if(zoneOrientation&Orientation_left)
                {
                    Tiled::MapObject* newobject=new Tiled::MapObject("","border-left",QPointF(x*mapWidth,y*mapHeight+mapHeight/2),QSizeF(1,1));
                    const QString nextMap(mapDir.relativeFilePath(QString::fromStdString(LoadMapAll::getMapFile(x-1,y))));
                    newobject->setProperty("map",nextMap);
                    newobject->setCell(newCell);
                    movingLayer->addObject(newobject);
                }
                if(zoneOrientation&Orientation_right)
                {
                    Tiled::MapObject* newobject=new Tiled::MapObject("","border-right",QPointF(x*mapWidth+mapWidth-1,y*mapHeight+mapHeight/2),QSizeF(1,1));
                    const QString nextMap(mapDir.relativeFilePath(QString::fromStdString(LoadMapAll::getMapFile(x+1,y))));
                    newobject->setProperty("map",nextMap);
                    newobject->setCell(newCell);
                    movingLayer->addObject(newobject);
                }
                if(zoneOrientation&Orientation_top)
                {
                    Tiled::MapObject* newobject=new Tiled::MapObject("","border-top",QPointF(x*mapWidth+mapWidth/2,-0.001/*not exact float representation correction*/+y*mapHeight+1),QSizeF(1,1));
                    const QString nextMap(mapDir.relativeFilePath(QString::fromStdString(LoadMapAll::getMapFile(x,y-1))));
                    newobject->setProperty("map",nextMap);
                    newobject->setCell(newCell);
                    movingLayer->addObject(newobject);
                }
                if(zoneOrientation&Orientation_bottom)
                {
                    Tiled::MapObject* newobject=new Tiled::MapObject("","border-bottom",QPointF(x*mapWidth+mapWidth/2,-0.001/*not exact float representation correction*/+y*mapHeight+mapHeight),QSizeF(1,1));
                    const QString nextMap(mapDir.relativeFilePath(QString::fromStdString(LoadMapAll::getMapFile(x,y+1))));
                    newobject->setProperty("map",nextMap);
                    newobject->setCell(newCell);
                    movingLayer->addObject(newobject);
                }
            }
            x++;
        }
        y++;
    }
}

std::string LoadMapAll::getMapFile(const unsigned int &x, const unsigned int &y)
{
    if(haveCityEntry(citiesCoordToIndex,x,y))
    {
        const LoadMapAll::City &city=LoadMapAll::cities.at(LoadMapAll::citiesCoordToIndex.at(x).at(y));
        return QCoreApplication::applicationDirPath().toStdString()+"/dest/map/main/official/"+LoadMapAll::lowerCase(city.name)+"/"+LoadMapAll::lowerCase(city.name);
    }

    const RoadIndex &roadIndex=roadCoordToIndex.at(x).at(y);
    const LoadMapAll::Road &road=LoadMapAll::roads.at(roadIndex.roadIndex);
    if(road.haveOnlySegmentNearCity)
    {
        if(roadIndex.cityIndex.empty())
        {
            std::cerr << "road.haveOnlySegmentNearCity and roadIndex.cityIndex.empty()" << std::endl;
            abort();
        }
        const LoadMapAll::RoadToCity &cityIndex=roadIndex.cityIndex.front();
        return QCoreApplication::applicationDirPath().toStdString()+"/dest/map/main/official/"+
                LoadMapAll::lowerCase(LoadMapAll::cities.at(cityIndex.cityIndex).name)+"/road-"+std::to_string(roadIndex.roadIndex+1)+
                "-"+LoadMapAll::orientationToString(LoadMapAll::reverseOrientation(cityIndex.orientation));
    }
    else
    {
        const unsigned int &indexCoord=vectorindexOf(road.coords,std::pair<uint16_t,uint16_t>(x,y));
        return QCoreApplication::applicationDirPath().toStdString()+"/dest/map/main/official/road-"+
                std::to_string(roadIndex.roadIndex+1)+"/"+std::to_string(indexCoord+1);
    }
}

std::string LoadMapAll::lowerCase(std::string str)
{
    std::transform(str.begin(),str.end(),str.begin(),::tolower);
    return str;
}
