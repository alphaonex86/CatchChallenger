#include "LoadMapAll.h"

#include "../map-procedural-generation-terrain/LoadMap.h"
#include "../map-procedural-generation-terrain/MapBrush.h"

#include "../../client/tiled/tiled_tileset.h"
#include "../../client/tiled/tiled_tile.h"
#include "../../client/tiled/tiled_objectgroup.h"
#include "../../client/tiled/tiled_mapobject.h"
#include "../../client/tiled/tiled_mapwriter.h"

#include <iostream>
#include <unordered_map>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>

void LoadMapAll::addBuildingChain(const std::string &baseName, const std::string &description, const MapBrush::MapTemplate &mapTemplatebuilding, Tiled::Map &worldMap,
                                  std::vector<Tiled::Map *> mapbuilding, const uint32_t &x, const uint32_t &y, const unsigned int mapWidth, const unsigned int mapHeight,
                                  const std::pair<uint8_t, uint8_t> pos, const City &city)
{
    bool ok=false;
    //search the brush door and retarget
    std::unordered_map<Tiled::MapObject*,Tiled::Properties> oldValue;
    std::vector<Tiled::MapObject*> doors=getDoorsList(mapTemplatebuilding.tiledMap);
    unsigned int index=0;
    while(index<(unsigned int)doors.size())
    {
        Tiled::MapObject* object=doors.at(index);
        Tiled::Properties properties=object->properties();
        oldValue[object]=object->properties();
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
    doors=getDoorsList(mapbuilding.at(1));
    index=0;
    while(index<(unsigned int)doors.size())
    {
        Tiled::MapObject* object=doors.at(index);
        Tiled::Properties properties=object->properties();
        oldValue[object]=properties;
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
        index++;
    }
    //write all next hop
    index=1;
    while(index<(unsigned int)mapbuilding.size())
    {
        Tiled::Map *nextHopMap=mapbuilding.at(index);
        Tiled::Properties properties=nextHopMap->properties();
        const std::string &filePath="/dest/main/official/"+LoadMapAll::lowerCase(city.name)+"/"+baseName+".tmx";

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
                /*if(!zone.empty())
                    content+=" zone=\""+QString::fromStdString(zone)+"\"";*/
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
    index=0;
    while(index<(unsigned int)doors.size())//reset to the old value
    {
        Tiled::MapObject* object=doors.at(index);
        object->setProperties(oldValue.at(object));
        index++;
    }
    doors.clear();
}

std::vector<Tiled::Map *> LoadMapAll::loadMapTemplate(const char * folderName,MapBrush::MapTemplate &mapTemplate, const char * fileName, const unsigned int mapWidth, const unsigned int mapHeight, Tiled::Map &worldMap)
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
            mapPointer=LoadMap::readMap(QString("template/")+QString(folderName)+QString::fromStdString(mapFile)+".tmx");
        fileToIndex[mapFile]=mapList.size();
        mapList.push_back(mapPointer);
        std::vector<Tiled::MapObject*> doors=getDoorsList(mapPointer);
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
    return mapList;
}

std::vector<Tiled::MapObject*> LoadMapAll::getDoorsList(Tiled::Map * map)
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
            if(object->type()=="door")
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
    std::vector<Tiled::Map *> mapBig=loadMapTemplate("",mapTemplateBig,"city-big",mapWidth,mapHeight,worldMap);
    std::vector<Tiled::Map *> mapMedium=loadMapTemplate("",mapTemplateMedium,"city-medium",mapWidth,mapHeight,worldMap);
    std::vector<Tiled::Map *> mapSmall=loadMapTemplate("",mapTemplateSmall,"city-small",mapWidth,mapHeight,worldMap);

    MapBrush::MapTemplate mapTemplatebuildingshop;
    MapBrush::MapTemplate mapTemplatebuildingheal;
    MapBrush::MapTemplate mapTemplatebuilding1;
    MapBrush::MapTemplate mapTemplatebuilding2;
    std::vector<Tiled::Map *> mapbuildingshop;
    std::vector<Tiled::Map *> mapbuildingheal;
    std::vector<Tiled::Map *> mapbuilding1;
    std::vector<Tiled::Map *> mapbuilding2;
    if(full)
    {
        mapbuildingshop=loadMapTemplate("building-shop/",mapTemplatebuildingshop,"building-shop",mapWidth,mapHeight,worldMap);
        mapbuildingheal=loadMapTemplate("building-heal/",mapTemplatebuildingheal,"building-heal",mapWidth,mapHeight,worldMap);
        mapbuilding1=loadMapTemplate("building-1/",mapTemplatebuilding1,"building-1",mapWidth,mapHeight,worldMap);
        mapbuilding2=loadMapTemplate("building-2/",mapTemplatebuilding2,"building-2",mapWidth,mapHeight,worldMap);
    }

    unsigned int index=0;
    while(index<cities.size())
    {
        const City &city=cities.at(index);

        const uint32_t x=city.x;
        const uint32_t y=city.y;
        Tiled::Map *map=NULL;
        MapBrush::MapTemplate mapTemplate;
        std::vector<std::pair<uint8_t,uint8_t> > positionBuilding;
        switch(city.type) {
        case CityType_small:
            map=mapSmall.front();
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
            map=mapMedium.front();
            mapTemplate=mapTemplateMedium;
            break;
        default:
            map=mapBig.front();
            mapTemplate=mapTemplateBig;
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
                addBuildingChain("heal","Heal",mapTemplatebuildingheal,worldMap,mapbuildingheal,x,y,mapWidth,mapHeight,pos,city);
            }
            else if(!haveShop)
            {
                haveShop=true;
                addBuildingChain("shop","Shop",mapTemplatebuildingshop,worldMap,mapbuildingshop,x,y,mapWidth,mapHeight,pos,city);
            }
            else
            {
                switch(rand()%2)
                {
                case 0:
                    housecount++;
                    addBuildingChain("house-"+std::to_string(housecount),"House "+std::to_string(housecount),mapTemplatebuilding1,worldMap,mapbuilding1,x,y,mapWidth,mapHeight,pos,city);
                    break;
                default:
                    housecount++;
                    addBuildingChain("house-"+std::to_string(housecount),"House "+std::to_string(housecount),mapTemplatebuilding2,worldMap,mapbuilding2,x,y,mapWidth,mapHeight,pos,city);
                    break;
                }
            }
        }
        index++;
    }

    //do the path
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
                    if(zoneOrientation&Orientation_bottom)
                    {
                        const unsigned int minX=x*mapWidth+mapWidth/2-2;
                        const unsigned int maxX=x*mapWidth+mapWidth/2+2;
                        unsigned int yTile=y*mapHeight+mapHeight/2;
                        while(yTile<(y+1)*mapHeight)
                        {
                            unsigned int xTile=minX;
                            while(xTile<maxX)
                            {
                                const unsigned int &bitMask=xTile+yTile*worldMap.width();
                                const unsigned int maxMapSize=(worldMap.width()*worldMap.height()/8+1);
                                if(bitMask/8>=maxMapSize)
                                    abort();
                                MapBrush::mapMask[bitMask/8]|=(1<<(7-bitMask%8));
                                xTile++;
                            }
                            yTile++;
                        }
                    }
                    if(zoneOrientation&Orientation_top)
                    {
                        const unsigned int minX=x*mapWidth+mapWidth/2-2;
                        const unsigned int maxX=x*mapWidth+mapWidth/2+2;
                        unsigned int yTile=y*mapHeight;
                        while(yTile<y*mapHeight+mapHeight/2)
                        {
                            unsigned int xTile=minX;
                            while(xTile<maxX)
                            {
                                const unsigned int &bitMask=xTile+yTile*worldMap.width();
                                const unsigned int maxMapSize=(worldMap.width()*worldMap.height()/8+1);
                                if(bitMask/8>=maxMapSize)
                                    abort();
                                MapBrush::mapMask[bitMask/8]|=(1<<(7-bitMask%8));
                                xTile++;
                            }
                            yTile++;
                        }
                    }
                    if(zoneOrientation&Orientation_left)
                    {
                        const unsigned int minX=x*mapWidth;
                        const unsigned int maxX=x*mapWidth+mapWidth/2;
                        unsigned int yTile=y*mapHeight+mapHeight/2-2;
                        while(yTile<y*mapHeight+mapHeight/2+2)
                        {
                            unsigned int xTile=minX;
                            while(xTile<maxX)
                            {
                                const unsigned int &bitMask=xTile+yTile*worldMap.width();
                                const unsigned int maxMapSize=(worldMap.width()*worldMap.height()/8+1);
                                if(bitMask/8>=maxMapSize)
                                    abort();
                                MapBrush::mapMask[bitMask/8]|=(1<<(7-bitMask%8));
                                xTile++;
                            }
                            yTile++;
                        }
                    }
                    if(zoneOrientation&Orientation_right)
                    {
                        const unsigned int minX=x*mapWidth+mapWidth/2;
                        const unsigned int maxX=(x+1)*mapWidth;
                        unsigned int yTile=y*mapHeight+mapHeight/2-2;
                        while(yTile<y*mapHeight+mapHeight/2+2)
                        {
                            unsigned int xTile=minX;
                            while(xTile<maxX)
                            {
                                const unsigned int &bitMask=xTile+yTile*worldMap.width();
                                const unsigned int maxMapSize=(worldMap.width()*worldMap.height()/8+1);
                                if(bitMask/8>=maxMapSize)
                                    abort();
                                MapBrush::mapMask[bitMask/8]|=(1<<(7-bitMask%8));
                                xTile++;
                            }
                            yTile++;
                        }
                    }
                }
                x++;
            }
            y++;
        }
    }
    LoadMapAll::deleteMapList(mapBig);
    LoadMapAll::deleteMapList(mapMedium);
    LoadMapAll::deleteMapList(mapSmall);
    if(full)
    {
        LoadMapAll::deleteMapList(mapbuildingshop);
        LoadMapAll::deleteMapList(mapbuildingheal);
        LoadMapAll::deleteMapList(mapbuilding1);
        LoadMapAll::deleteMapList(mapbuilding2);
    }
}

void LoadMapAll::deleteMapList(const std::vector<Tiled::Map *> &mapList)
{
    unsigned int index=0;
    while(index<mapList.size())
    {
        delete mapList.at(index);
        index++;
    }
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
        return QCoreApplication::applicationDirPath().toStdString()+"/dest/main/official/"+LoadMapAll::lowerCase(city.name)+"/"+LoadMapAll::lowerCase(city.name);
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
        return QCoreApplication::applicationDirPath().toStdString()+"/dest/main/official/"+
                LoadMapAll::lowerCase(LoadMapAll::cities.at(cityIndex.cityIndex).name)+"/road-"+std::to_string(roadIndex.roadIndex+1)+
                "-"+LoadMapAll::orientationToString(LoadMapAll::reverseOrientation(cityIndex.orientation));
    }
    else
    {
        const unsigned int &indexCoord=vectorindexOf(road.coords,std::pair<uint16_t,uint16_t>(x,y));
        return QCoreApplication::applicationDirPath().toStdString()+"/dest/main/official/road-"+
                std::to_string(roadIndex.roadIndex+1)+"/"+std::to_string(indexCoord+1);
    }
}

std::string LoadMapAll::lowerCase(std::string str)
{
    std::transform(str.begin(),str.end(),str.begin(),::tolower);
    return str;
}