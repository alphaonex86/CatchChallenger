#include "LoadMapAll.h"

#include "../map-procedural-generation-terrain/LoadMap.h"
#include "../map-procedural-generation-terrain/MapBrush.h"

#include <libtiled/tileset.h>
#include <libtiled/tile.h>
#include <libtiled/objectgroup.h>
#include <libtiled/mapobject.h>
#include <libtiled/mapwriter.h>
#include "../../general/base/cpp11addition.hpp"

#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>

void LoadMapAll::addBuildingChain(const std::string &baseName, const std::string &description, const MapBrush::MapTemplate &mapTemplatebuilding, Tiled::Map &worldMap,
                                  const uint32_t &x, const uint32_t &y, const unsigned int mapWidth, const unsigned int mapHeight,
                                  const std::pair<uint8_t, uint8_t> pos, const City &city,const std::string &zone,
                                  const BotKind &botKind, const SettingsAll::SettingsExtra &setting,
                                  const std::vector<RoadMonster> &monsterPool, const uint8_t &level)
{
    bool ok=false;
    //tile inside the entrance floor where stepping through the door drops the
    //player; captured from the exterior door below. Defaults match the shipped
    //building templates (floor-0 spawn at 5,10).
    int spawnX=5;
    int spawnY=10;
    //search the brush door and retarget
    std::unordered_map<Tiled::MapObject*,Tiled::Properties> oldValue;
    std::vector<Tiled::MapObject*> doors=getDoorsListAndTp(mapTemplatebuilding.tiledMap);
    unsigned int index=0;
    while(index<(unsigned int)doors.size())
    {
        Tiled::MapObject* object=doors.at(index);
        Tiled::Properties properties=object->properties();
        oldValue[object]=object->properties();
        //the exterior door's destination x/y IS the player's spawn tile inside
        //the entrance floor — place the key-building bot(s) relative to it.
        if(index==0)
        {
            if(properties.contains("x"))
                spawnX=properties.value("x").toInt();
            if(properties.contains("y"))
                spawnY=properties.value("y").toInt();
        }
        if(mapTemplatebuilding.otherMap.size()>1)
            properties["map"]=QString::fromStdString(baseName)+"/"+properties.value("map").toString();
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
                if(properties.value("map").toString().toStdString()==mapTemplatebuilding.name)
                {
                    if(mapTemplatebuilding.otherMap.size()>1)
                        properties["map"]="../"+QString::fromStdString(LoadMapAll::lowerCase(city.name));
                    else
                        properties["map"]=QString::fromStdString(LoadMapAll::lowerCase(city.name));
                    properties["x"]=QString::number(properties.value("x").toUInt(&ok)+pos.first);
                    if(!ok)
                    {
                        std::cerr << "For one tmx map, x is not a number: " << properties.value("x").toString().toStdString() << std::endl;
                        abort();
                    }
                    properties["y"]=QString::number(properties.value("y").toUInt(&ok)+pos.second);
                    if(!ok)
                    {
                        std::cerr << "For one tmx map, y is not a number: " << properties.value("y").toString().toStdString() << std::endl;
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

        //Inject the key-building bot object(s) into the ENTRANCE floor (index 0)
        //and build the matching inline <bot> defs. Objects go into an "Object"
        //group (the only group the engine scans for bots; created here if the
        //template lacks one) with a marker cell from the interior's OWN
        //"invisible" tileset. They are removed again after the file is written
        //so the shared template stays pristine for the next city.
        QString botXml;
        std::vector<Tiled::MapObject*> injectedBots;
        Tiled::ObjectGroup *npcGroup=NULL;
        bool createdNpcGroup=false;
        if(index==0)
        {
            std::vector<std::pair<BotKind,std::string> > toPlace;//bot kind, skin name
            if(botKind==BotKind_fight)
            {
                //a gym: several trainers plus one leader
                unsigned int trainer=0;
                while(trainer<setting.gymTrainers)
                {
                    toPlace.push_back(std::pair<BotKind,std::string>(BotKind_fight,setting.gymTrainerSkin));
                    trainer++;
                }
                toPlace.push_back(std::pair<BotKind,std::string>(BotKind_leader,setting.gymLeaderSkin));
            }
            else if(botKind==BotKind_heal)
                toPlace.push_back(std::pair<BotKind,std::string>(BotKind_heal,setting.healSkin));
            else if(botKind==BotKind_shop)
                toPlace.push_back(std::pair<BotKind,std::string>(BotKind_shop,setting.shopSkin));

            if(!toPlace.empty())
            {
                npcGroup=LoadMap::searchObjectGroupByName(*nextHopMap,"Object");
                if(npcGroup==NULL)
                {
                    npcGroup=new Tiled::ObjectGroup("Object",0,0);
                    nextHopMap->addLayer(npcGroup);
                    createdNpcGroup=true;
                }
                Tiled::Tileset *invisInterior=LoadMap::searchTilesetByName(*nextHopMap,"invisible");
                const int tileW=nextHopMap->tileWidth();
                const int tileH=nextHopMap->tileHeight();
                unsigned int localBotId=1;
                unsigned int placeIndex=0;
                while(placeIndex<toPlace.size())
                {
                    //spread bots on walkable interior tiles a couple of rows
                    //above the entrance spawn; a lone key-building bot sits on
                    //the entrance column (blocking it like a service counter).
                    int tileX=2+(int)((placeIndex%4)*2);
                    int tileY=spawnY-2-(int)((placeIndex/4)*2);
                    if(toPlace.size()==1)
                        tileX=spawnX;
                    if(tileX>nextHopMap->width()-2)
                        tileX=nextHopMap->width()-2;
                    if(tileX<1)
                        tileX=1;
                    if(tileY<1)
                        tileY=1;
                    //object Y carries the engine's -1 tile correction, so add 1
                    Tiled::MapObject *npc=new Tiled::MapObject("","bot",
                        QPointF(tileX*tileW,(tileY+1)*tileH),QSizeF(tileW,tileH));
                    npc->setProperty("id",QString::number(localBotId));
                    npc->setProperty("lookAt","bottom");
                    npc->setProperty("skin",QString::fromStdString(toPlace.at(placeIndex).second));
                    if(invisInterior!=NULL)
                    {
                        Tiled::Cell markerCell;
                        markerCell.setTile(invisInterior->tileAt(3));
                        npc->setCell(markerCell);
                    }
                    npcGroup->addObject(npc);
                    injectedBots.push_back(npc);
                    botXml+=botStepXml(localBotId,toPlace.at(placeIndex).first,std::to_string(localBotId),
                                       "bottom",setting,monsterPool,level);
                    localBotId++;
                    placeIndex++;
                }
            }
        }

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

#ifdef TILED_CSV
        nextHopMap->setLayerDataFormat(Tiled::Map::CSV);  // DEBUG
#endif

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
                    content+=" type=\""+properties.value("type").toString()+"\"";
                if(!zone.empty())
                    content+=" zone=\""+QString::fromStdString(zone)+"\"";
                content+=">\n"
                         "  <name>"+QString::fromStdString(description)+"</name>\n";
                content+=botXml;
                content+="</map>";
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

        //remove the injected bot objects (and the group, if we created it) so
        //the shared interior template is unchanged for the next city.
        {
            unsigned int b=0;
            while(b<injectedBots.size())
            {
                npcGroup->removeObject(injectedBots.at(b));
                delete injectedBots.at(b);
                b++;
            }
        }
        if(createdNpcGroup)
        {
            int li=0;
            while(li<nextHopMap->layerCount())
            {
                if(nextHopMap->layerAt(li)==npcGroup)
                {
                    delete nextHopMap->takeLayerAt(li);
                    break;
                }
                li++;
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

void LoadMapAll::loadMapTemplate(const char * folderName, MapBrush::MapTemplate &mapTemplate, const QString &fileName, const unsigned int mapWidth, const unsigned int mapHeight, Tiled::Map &worldMap)
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
    mapTemplate.name=fileName.toStdString();
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
    mapToLoad.push_back(fileName.toStdString());
    while(!mapToLoad.empty())
    {
        const std::string mapFile=mapToLoad.front();
        mapToLoad.erase(mapToLoad.cbegin());
        Tiled::Map *mapPointer;
        if(mapFile==fileName.toStdString())
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
            const std::string &mapString=properties.value("map").toString().toStdString();
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

void LoadMapAll::deleteMapList(MapBrush::MapTemplate &mapTemplatebuilding)
{
    // delete mapTemplatebuilding.tiledMap; // FIX Libtiled 1.3.x - Tiled::Map is now smart pointer and is deleted automaticaly
    mapTemplatebuilding.tiledMap=NULL;
    unsigned int index=0;
    while(index<mapTemplatebuilding.otherMap.size())
    {
        // delete mapTemplatebuilding.otherMap.at(index); // FIX Libtiled 1.3.x - Tiled::Map is now smart pointer and is deleted automaticaly
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
    newCell.setFlippedAntiDiagonally(false);
    newCell.setFlippedHorizontally(false);
    newCell.setFlippedVertically(false);
    newCell.setTile(invisibleTileset->tileAt(3));

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
                int tileWidth = worldMap.tileWidth();
                int tileHeight = worldMap.tileHeight();

                QDir mapDir(QFileInfo(QString::fromStdString(LoadMapAll::getMapFile(x,y))).absoluteDir());
                if(zoneOrientation&Orientation_left)
                {
                    int tiles_x = x*mapWidth;
                    int tiles_y = y*mapHeight+mapHeight/2;

                    // Convert to pixel units when creating a new Tiled::MapObject
                    // FIX: API change in v0.10.x - MapObjects now use pixel units instead of tile units
                    int pixels_x = tiles_x * worldMap.tileWidth();
                    int pixels_y = tiles_y * worldMap.tileHeight();

                    Tiled::MapObject* newobject=new Tiled::MapObject("","border-left",QPointF(pixels_x, pixels_y),QSizeF(tileWidth,tileHeight));
                    const QString nextMap(mapDir.relativeFilePath(QString::fromStdString(LoadMapAll::getMapFile(x-1,y))));
                    newobject->setProperty("map",nextMap);
                    newobject->setCell(newCell);
                    movingLayer->addObject(newobject);
                }
                if(zoneOrientation&Orientation_right)
                {
                    int tiles_x = x*mapWidth+mapWidth-1;
                    int tiles_y = y*mapHeight+mapHeight/2;

                    // Convert to pixel units when creating a new Tiled::MapObject
                    // FIX: API change in v0.10.x - MapObjects now use pixel units instead of tile units
                    int pixels_x = tiles_x * worldMap.tileWidth();
                    int pixels_y = tiles_y * worldMap.tileHeight();

                    Tiled::MapObject* newobject=new Tiled::MapObject("","border-right",QPointF(pixels_x,pixels_y),QSizeF(tileWidth,tileHeight));
                    const QString nextMap(mapDir.relativeFilePath(QString::fromStdString(LoadMapAll::getMapFile(x+1,y))));
                    newobject->setProperty("map",nextMap);
                    newobject->setCell(newCell);
                    movingLayer->addObject(newobject);
                }
                if(zoneOrientation&Orientation_top)
                {
                    qreal tiles_x = x*mapWidth+mapWidth/2;
                    qreal tiles_y = y*mapHeight+1;

#ifdef DEBUG_DANIJEL
                    std::cout << "DEBUG border-top: Coord = " << x << ", " << y << std::endl;
                    std::cout << "DEBUG border-top: Tiles = " << tiles_x << ", " << tiles_y << std::endl;
#endif

                    // Convert to pixel units when creating a new Tiled::MapObject
                    // FIX: API change in v0.10.x - MapObjects now use pixel units instead of tile units
                    qreal pixels_x = tiles_x * worldMap.tileWidth();
                    qreal pixels_y = tiles_y * worldMap.tileHeight();

                    Tiled::MapObject* newobject=new Tiled::MapObject("","border-top",QPointF(pixels_x, pixels_y),QSizeF(tileWidth,tileHeight));
                    const QString nextMap(mapDir.relativeFilePath(QString::fromStdString(LoadMapAll::getMapFile(x,y-1))));
                    newobject->setProperty("map",nextMap);
                    newobject->setCell(newCell);
                    movingLayer->addObject(newobject);
                }
                if(zoneOrientation&Orientation_bottom)
                {
                    qreal tiles_x = x*mapWidth+mapWidth/2;
                    qreal tiles_y = y*mapHeight+mapHeight;

#ifdef DEBUG_DANIJEL
                    std::cout << "DEBUG border-bottom: Coord = " << x << ", " << y << std::endl;
                    std::cout << "DEBUG border-bottom: Tiles = " << tiles_x << ", " << tiles_y << std::endl;
#endif

                    // Convert to pixel units when creating a new Tiled::MapObject
                    // FIX: API change in v0.10.x - MapObjects now use pixel units instead of tile units
                    qreal pixels_x = tiles_x * worldMap.tileWidth();
                    qreal pixels_y = tiles_y * worldMap.tileHeight();

                    Tiled::MapObject* newobject=new Tiled::MapObject("","border-bottom",QPointF(pixels_x, pixels_y),QSizeF(tileWidth,tileHeight));
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
