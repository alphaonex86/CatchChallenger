#ifndef CATCHCHALLENGER_MAP_LOADER_H
#define CATCHCHALLENGER_MAP_LOADER_H

#include <vector>
#include <string>

//for template
#include <iostream>
#include <sys/stat.h>
#include <chrono>
#include "FacilityLibGeneral.hpp"

#include "GeneralStructures.hpp"
#include "CommonMap.hpp"
#include "lib.h"
#include "../tinyXML2/tinyxml2.hpp"

#ifdef CATCHCHALLENGER_CLASS_GATEWAY
#error This kind don't need reply on that's, not need load the map content
#endif
#ifdef CATCHCHALLENGER_CLASS_LOGIN
#error This kind don't need reply on that's, not need load the map content
#endif
#ifdef CATCHCHALLENGER_CLASS_MASTER
#error This kind don't need reply on that's, not need load the map content
#endif

namespace CatchChallenger {
struct Map_semi_border_content_top_bottom
{
    std::string fileName;
    int16_t x_offset;//can be negative, it's an offset!
};
struct Map_semi_border_content_left_right
{
    std::string fileName;
    int16_t y_offset;//can be negative, it's an offset!
};
struct Map_semi_border
{
    Map_semi_border_content_top_bottom top;
    Map_semi_border_content_top_bottom bottom;
    Map_semi_border_content_left_right left;
    Map_semi_border_content_left_right right;
};
struct Map_semi_teleport
{
    COORD_TYPE source_x,source_y;
    COORD_TYPE destination_x,destination_y;
    std::string map;
    MapCondition condition;
};

class DLL_PUBLIC Map_to_send : public BaseMap
{
public:
    Map_semi_border border;
    //std::stringList other_map;//border and not

    //all the variables after have to resolv map/zone
    std::string zoneName;
    std::string file;
    //std::vector<uint8_t> simplifiedMap; into CommonMap::flat_simplified_map and monstersCollisionMap?

    std::unordered_map<std::string,std::string> property;

    std::vector<Map_semi_teleport> teleport;

    //load just after the xml extra file
    struct Bot_Semi
    {
        std::pair<uint8_t,uint8_t> point;
        uint8_t id;
        std::unordered_map<std::string,std::string> property_text;
        std::unordered_map<uint8_t,const tinyxml2::XMLElement *> steps;
    };
    std::vector<Bot_Semi> bots;//to pass pos to xml file, used into colision too
    struct ItemOnMap_Semi
    {
        bool infinite;
        bool visible;
        CATCHCHALLENGER_TYPE_ITEM item;
        std::pair<uint8_t,uint8_t> point;
    };
    std::vector<ItemOnMap_Semi> items;//used into colision

    /* see flat_simplified_map
     * after resolution the index is position (x+y*width)
     * not optimal, but memory safe, simply 2 cache miss max
     */
    /* 0 walkable: index = 0 for monster is used into cave
     * 254 not walkable
     * 253 ParsedLayerLedges_LedgesBottom
     * 252 ParsedLayerLedges_LedgesTop
     * 251 ParsedLayerLedges_LedgesRight
     * 250 ParsedLayerLedges_LedgesLeft
     * 249 dirt
     * 200 - 248 reserved
     * 0 cave def
     * 1-199 monster def and condition */
    std::vector<uint8_t> flat_simplified_map;//can't be pointer, server can have unique pointer, but client need another pointer or pointer mulitple in case of multi-bots

    const tinyxml2::XMLElement * xmlRoot;
};

struct Map_semi
{
    //conversion x,y to position: x+y*width
    CATCHCHALLENGER_TYPE_MAPID mapIndex;
    std::string file;
    Map_semi_border border;
    Map_to_send old_map;
};

class DLL_PUBLIC Map_loader
{
public:
    explicit Map_loader();
    ~Map_loader();

    Map_to_send map_to_send;
    std::string errorString();
    bool tryLoadMap(const std::string &file, CommonMap &mapFinal, const bool &botIsNotWalkable);
    bool loadExtraXml(CommonMap &mapFinal,const std::string &file, std::vector<Map_to_send::Bot_Semi> &botslist, std::vector<std::string> detectedMonsterCollisionMonsterType, std::vector<std::string> detectedMonsterCollisionLayer,std::string &zoneName);
    static std::string resolvRelativeMap(const std::string &file, const std::string &link, const std::string &datapackPath=std::string());
    static MapCondition xmlConditionToMapCondition(const std::string &conditionFile,const tinyxml2::XMLElement * const item);
    std::vector<MapMonster> loadSpecificMonster(const std::string &fileName,const std::string &monsterType);
    static std::unordered_map<std::string/*file*/, std::unordered_map<uint16_t/*id*/,tinyxml2::XMLElement *> > teleportConditionsUnparsed;

    template<class MapType>
    static void loadAllMapsAndLink(std::vector<MapType> &flat_map_list,const std::string &datapack_mapPath,std::vector<Map_semi> &semi_loaded_map,std::unordered_map<std::string, CATCHCHALLENGER_TYPE_MAPID> &mapPathToId)
    {
        Map_loader map_temp;
        std::vector<std::string> map_name;
        std::vector<std::string> map_name_to_do_id;
        struct stat buffer;
        if(::stat(datapack_mapPath.c_str(),&buffer)!=0)
            return;
        std::vector<std::string> returnList;
        switch(buffer.st_mode)
        {
            case S_IFREG:
                returnList.push_back(datapack_mapPath);
            break;
            case S_IFDIR:
                returnList=FacilityLibGeneral::listFolder(datapack_mapPath);
                std::sort(returnList.begin(), returnList.end());
            break;
        }
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

        if(returnList.size()==0)
        {
            std::cerr << "No file map to list" << std::endl;
            abort();
        }
        if(!semi_loaded_map.empty() || !flat_map_list.empty())
        {
            std::cerr << "preload_the_map() already call" << std::endl;
            abort();
        }
        //load the map
        unsigned int index=0;
        unsigned int sub_index;
        std::string tmxRemove(".tmx");
        std::regex mapFilter("\\.tmx$");
        std::regex mapExclude("[\"']");

        CATCHCHALLENGER_TYPE_MAPID mapIdToSyncServerAndClient=0;
        index=0;
        while(index<returnList.size())
        {
            std::string fileName=returnList.at(index);
            stringreplaceAll(fileName,"\\","/");
            if(regex_search(fileName,mapFilter) && !regex_search(fileName,mapExclude))
            {
                mapIdToSyncServerAndClient++;
                #ifdef DEBUG_MESSAGE_MAP_LOAD
                std::cout << "load the map: " << fileName << std::endl;
                #endif
                std::string fileNameWihtoutTmx=fileName;
                stringreplaceOne(fileNameWihtoutTmx,tmxRemove,"");
                map_name_to_do_id.push_back(fileNameWihtoutTmx);
                map_name.push_back(fileNameWihtoutTmx);
                //mapPathToId[sortFileName]=map_name_to_do_id.size(); need be sorted before
                flat_map_list.push_back(MapType());
                MapType &mapFinal=flat_map_list.back();
                if(map_temp.tryLoadMap(datapack_mapPath+fileName,mapFinal,true))
                {
                    //GlobalServerData::serverPrivateVariables.map_list[fileNameWihtoutTmx]=mapServer;

                    bool tryLoadNearMap=returnList.size()==1 && index==0;

                    mapFinal.border.top.mapIndex=65535;
                    mapFinal.border.top.x_offset=0;
                    mapFinal.border.bottom.mapIndex=65535;
                    mapFinal.border.bottom.x_offset=0;
                    mapFinal.border.right.mapIndex=65535;
                    mapFinal.border.right.y_offset=0;
                    mapFinal.border.left.mapIndex=65535;
                    mapFinal.border.left.y_offset=0;

                    mapFinal.width			= static_cast<uint8_t>(map_temp.map_to_send.width);
                    mapFinal.height			= static_cast<uint8_t>(map_temp.map_to_send.height);
                    mapFinal.id=mapIdToSyncServerAndClient;

                    Map_semi map_semi;
                    map_semi.old_map				= map_temp.map_to_send;
                    map_semi.file=fileName;

                    if(map_temp.map_to_send.border.top.fileName.size()>0)
                    {
                        map_semi.border.top.fileName		= Map_loader::resolvRelativeMap(datapack_mapPath+fileName,map_temp.map_to_send.border.top.fileName,datapack_mapPath);
                        stringreplaceOne(map_semi.border.top.fileName,".tmx","");
                        map_semi.border.top.x_offset		= map_temp.map_to_send.border.top.x_offset;
                        if(tryLoadNearMap)
                            returnList.push_back(map_semi.border.top.fileName);
                    }
                    if(map_temp.map_to_send.border.bottom.fileName.size()>0)
                    {
                        map_semi.border.bottom.fileName		= Map_loader::resolvRelativeMap(datapack_mapPath+fileName,map_temp.map_to_send.border.bottom.fileName,datapack_mapPath);
                        stringreplaceOne(map_semi.border.bottom.fileName,".tmx","");
                        map_semi.border.bottom.x_offset		= map_temp.map_to_send.border.bottom.x_offset;
                        if(tryLoadNearMap)
                            returnList.push_back(map_semi.border.bottom.fileName);
                    }
                    if(map_temp.map_to_send.border.left.fileName.size()>0)
                    {
                        map_semi.border.left.fileName		= Map_loader::resolvRelativeMap(datapack_mapPath+fileName,map_temp.map_to_send.border.left.fileName,datapack_mapPath);
                        stringreplaceOne(map_semi.border.left.fileName,".tmx","");
                        map_semi.border.left.y_offset		= map_temp.map_to_send.border.left.y_offset;
                        if(tryLoadNearMap)
                            returnList.push_back(map_semi.border.left.fileName);
                    }
                    if(map_temp.map_to_send.border.right.fileName.size()>0)
                    {
                        map_semi.border.right.fileName		= Map_loader::resolvRelativeMap(datapack_mapPath+fileName,map_temp.map_to_send.border.right.fileName,datapack_mapPath);
                        stringreplaceOne(map_semi.border.right.fileName,".tmx","");
                        map_semi.border.right.y_offset		= map_temp.map_to_send.border.right.y_offset;
                        if(tryLoadNearMap)
                            returnList.push_back(map_semi.border.right.fileName);
                    }

                    sub_index=0;
                    while(sub_index<map_temp.map_to_send.teleport.size())
                    {
                        map_temp.map_to_send.teleport[sub_index].map=Map_loader::resolvRelativeMap(datapack_mapPath+fileName,map_temp.map_to_send.teleport.at(sub_index).map,datapack_mapPath);
                        stringreplaceOne(map_temp.map_to_send.teleport[sub_index].map,".tmx","");
                        if(tryLoadNearMap)
                            returnList.push_back(map_temp.map_to_send.teleport[sub_index].map);
                        sub_index++;
                    }

                    map_semi.old_map=map_temp.map_to_send;
                    mapFinal.flat_simplified_map=map_temp.map_to_send.flat_simplified_map;

                    semi_loaded_map.push_back(map_semi);
                }
                else
                {
                    std::cout << "error at loading: " << datapack_mapPath << fileName << ", error: " << map_temp.errorString()
                              << "parsed due: " << "regex_search(" << fileName << ",\\.tmx$) && !regex_search("
                              << fileName << ",[\"'])"
                              << std::endl;

                    /* to always have id matching path id event if map not load
                     * ON CLIENT SIDE the map is loaded on demand, then unable to know what can't be loaded */
                    mapFinal.border.top.mapIndex=65535;
                    mapFinal.border.top.x_offset=0;
                    mapFinal.border.bottom.mapIndex=65535;
                    mapFinal.border.bottom.x_offset=0;
                    mapFinal.border.right.mapIndex=65535;
                    mapFinal.border.right.y_offset=0;
                    mapFinal.border.left.mapIndex=65535;
                    mapFinal.border.left.y_offset=0;

                    mapFinal.width			= 0;
                    mapFinal.height			= 0;
                    mapFinal.id=mapIdToSyncServerAndClient;
                }
            }
            index++;
        }

        if(flat_map_list.size()!=semi_loaded_map.size())
        {
            std::cerr << "flat_map_list_temp.size()!=semi_loaded_map.size() (abort)" << std::endl;
            abort();
        }

        std::sort(map_name_to_do_id.begin(),map_name_to_do_id.end());
        {
            unsigned int index=0;
            while(index<map_name_to_do_id.size())
            {
                mapPathToId[map_name_to_do_id.at(index)]=index;
                index++;
            }
        }

        index=0;
        while(index<semi_loaded_map.size())
        {
            Map_semi &map_semi=semi_loaded_map.at(index);
            MapType &mapFinal=flat_map_list.at(index);
            if(mapFinal.width==0 || mapFinal.height==0)
                continue;
            unsigned int sub_index=0;

            //resolv the border map name into their pointer + resolv the offset to change of map
            if(map_semi.border.bottom.fileName.size()>0 && mapPathToId.find(map_semi.border.bottom.fileName)!=mapPathToId.end() && flat_map_list.at(mapPathToId.at(map_semi.border.bottom.fileName)).width!=0)
            {
                mapFinal.border.bottom.mapIndex=mapPathToId.at(map_semi.border.bottom.fileName);
                mapFinal.border.bottom.x_offset=map_semi.border.bottom.x_offset;
            }
            else
            {
                mapFinal.border.bottom.mapIndex=65535;
                mapFinal.border.bottom.x_offset=0;
            }

            if(map_semi.border.top.fileName.size()>0 && mapPathToId.find(map_semi.border.top.fileName)!=mapPathToId.end() && flat_map_list.at(mapPathToId.at(map_semi.border.top.fileName)).width!=0)
            {
                mapFinal.border.top.mapIndex=mapPathToId.at(map_semi.border.top.fileName);
                mapFinal.border.top.x_offset=map_semi.border.top.x_offset;
            }
            else
            {
                mapFinal.border.top.mapIndex=65535;
                mapFinal.border.top.x_offset=0;
            }

            if(map_semi.border.left.fileName.size()>0 && mapPathToId.find(map_semi.border.left.fileName)!=mapPathToId.end() && flat_map_list.at(mapPathToId.at(map_semi.border.left.fileName)).width!=0)
            {
                mapFinal.border.left.mapIndex=mapPathToId.at(map_semi.border.left.fileName);
                mapFinal.border.left.y_offset=map_semi.border.left.y_offset;
            }
            else
            {
                mapFinal.border.left.mapIndex=65535;
                mapFinal.border.left.y_offset=0;
            }

            if(map_semi.border.right.fileName.size()>0 && mapPathToId.find(map_semi.border.right.fileName)!=mapPathToId.end() && flat_map_list.at(mapPathToId.at(map_semi.border.right.fileName)).width!=0)
            {
                mapFinal.border.right.mapIndex=mapPathToId.at(map_semi.border.right.fileName);
                mapFinal.border.right.y_offset=map_semi.border.right.y_offset;
            }
            else
            {
                mapFinal.border.right.mapIndex=65535;
                mapFinal.border.right.y_offset=0;
            }

            /*The datapack dev should fix it and then drop duplicate teleporter, if well done then the final size is map_semi.old_map.teleport.size()*/
            while(sub_index<map_semi.old_map.teleport.size() && sub_index<127)//code not ready for more than 127
            {
                const Map_semi_teleport &teleporter_semi=map_semi.old_map.teleport.at(sub_index);
                std::string teleportString=teleporter_semi.map;
                stringreplaceOne(teleportString,".tmx","");
                if(mapPathToId.find(teleportString)!=mapPathToId.end() && flat_map_list.at(mapPathToId.at(teleportString)).width>0)
                {
                    if(teleporter_semi.destination_x<map_semi.old_map.width
                            && teleporter_semi.destination_y<map_semi.old_map.height)
                    {
                        //remove duplicate
                        uint16_t index_search=0;
                        while(index_search<mapFinal.teleporters.size())
                        {
                            const Teleporter &teleporter=mapFinal.teleporters.at(index_search);
                            if(teleporter.source_x==teleporter_semi.source_x && teleporter.source_y==teleporter_semi.source_y)
                                break;
                            index_search++;
                        }
                        //no duplicate then add
                        if(index_search==mapFinal.teleporters.size())
                        {
                            #ifdef DEBUG_MESSAGE_MAP_LOAD
                            std::cout << "teleporter on the map: "
                                 << map_name.at(index)
                                 << "("
                                 << std::to_string(teleport.source_x)
                                 << ","
                                 << std::to_string(teleport.source_y)
                                 << "), to "
                                 << teleportString
                                 << "("
                                 << std::to_string(semi_loaded_map.at(index).old_map.teleport.at(sub_index).destination_x)
                                 << ","
                                 << std::to_string(teleport.destination_y)
                                 << ")"
                                 << std::endl;
                            #endif
                            Teleporter teleporter;
                            teleporter.mapIndex=mapPathToId.at(teleportString);
                            teleporter.source_x=teleporter_semi.source_x;
                            teleporter.source_y=teleporter_semi.source_y;
                            teleporter.destination_x=teleporter_semi.destination_x;
                            teleporter.destination_y=teleporter_semi.destination_y;
                            teleporter.condition=teleporter_semi.condition;
                            mapFinal.teleporters.push_back(teleporter);
                        }
                        else
                            std::cerr << "already found teleporter on the map: "
                                 << map_name.at(index)
                                 << "("
                                 << std::to_string(teleporter_semi.source_x)
                                 << ","
                                 << std::to_string(teleporter_semi.source_y)
                                 << "), to "
                                 << teleportString
                                 << "("
                                 << std::to_string(teleporter_semi.destination_x)
                                 << ","
                                 << std::to_string(teleporter_semi.destination_y)
                                 << ")"
                                 << std::endl;
                    }
                    else
                        std::cerr << "wrong teleporter on the map: "
                             << map_name.at(index)
                             << "("
                             << std::to_string(teleporter_semi.source_x)
                             << ","
                             << std::to_string(teleporter_semi.source_y)
                             << "), to "
                             << teleportString
                             << "("
                             << std::to_string(teleporter_semi.destination_x)
                             << ","
                             << std::to_string(teleporter_semi.destination_y)
                             << ") because the tp is out of range"
                             << std::endl;
                }
                else
                    std::cerr << "wrong teleporter on the map: "
                         << map_name.at(index)
                         << "("
                         << std::to_string(teleporter_semi.source_x)
                         << ","
                         << std::to_string(teleporter_semi.source_y)
                         << "), to "
                         << teleportString
                         << "("
                         << std::to_string(teleporter_semi.destination_x)
                         << ","
                         << std::to_string(teleporter_semi.destination_y)
                         << ") because the map is not found"
                         << std::endl;

                sub_index++;
            }
            index++;
        }

        //clean border balise without another oposite border
        index=0;
        while(index<semi_loaded_map.size())
        {
            const std::string &tempName=map_name.at(index);
            const uint16_t indexMap=mapPathToId.at(tempName);
            MapType &currentTempMap=flat_map_list[indexMap];
            if(currentTempMap.border.bottom.mapIndex!=65535)
            {
                MapType &bottomTempMap=flat_map_list[currentTempMap.border.bottom.mapIndex];
                if(bottomTempMap.border.top.mapIndex!=indexMap)
                {
                    if(bottomTempMap.border.top.mapIndex==65535)
                        std::cerr << "the map "
                                  << tempName
                                  << "have bottom map: "
                                  << map_name.at(currentTempMap.border.bottom.mapIndex)
                                  << ", but the bottom map have not a top map"
                                  << std::endl;
                    else
                        std::cerr << "the map "
                                  << tempName
                                  << "have bottom map: "
                                  << map_name.at(currentTempMap.border.bottom.mapIndex)
                                  << ", but the bottom map have different top map: "
                                  << map_name.at(bottomTempMap.border.top.mapIndex)
                                  << std::endl;
                    currentTempMap.border.bottom.mapIndex=65535;
                    bottomTempMap.border.top.mapIndex=65535;
                }
            }
            if(currentTempMap.border.top.mapIndex!=65535)
            {
                MapType &topTempMap=flat_map_list[currentTempMap.border.top.mapIndex];
                if(topTempMap.border.bottom.mapIndex!=indexMap)
                {
                    if(topTempMap.border.bottom.mapIndex==65535)
                        std::cerr << "the map "
                                  << tempName
                                  << "have top map: "
                                  << map_name.at(currentTempMap.border.top.mapIndex)
                                  << ", but the bottom map have not a bottom map"
                                  << std::endl;
                    else
                        std::cerr << "the map "
                                  << tempName
                                  << "have top map: "
                                  << map_name.at(currentTempMap.border.top.mapIndex)
                                  << ", but the bottom map have different bottom map: "
                                  << map_name.at(topTempMap.border.bottom.mapIndex)
                                  << std::endl;
                    currentTempMap.border.top.mapIndex=65535;
                    topTempMap.border.bottom.mapIndex=65535;
                }
            }
            if(currentTempMap.border.left.mapIndex!=65535)
            {
                MapType &leftTempMap=flat_map_list[currentTempMap.border.left.mapIndex];
                if(leftTempMap.border.right.mapIndex!=indexMap)
                {
                    if(leftTempMap.border.right.mapIndex==65535)
                        std::cerr << "the map "
                                  << tempName
                                  << "have left map: "
                                  << map_name.at(currentTempMap.border.left.mapIndex)
                                  << ", but the right map have not a right map"
                                  << std::endl;
                    else
                        std::cerr << "the map "
                                  << tempName
                                  << "have left map: "
                                  << map_name.at(currentTempMap.border.left.mapIndex)
                                  << ", but the right map have different right map: "
                                  << map_name.at(leftTempMap.border.right.mapIndex)
                                  << std::endl;
                    currentTempMap.border.left.mapIndex=65535;
                    leftTempMap.border.right.mapIndex=65535;
                }
            }
            if(currentTempMap.border.right.mapIndex!=65535)
            {
                MapType &rightTempMap=flat_map_list[currentTempMap.border.right.mapIndex];
                if(rightTempMap.border.left.mapIndex!=indexMap)
                {
                    if(rightTempMap.border.left.mapIndex==65535)
                        std::cerr << "the map "
                                  << tempName
                                  << "have right map: "
                                  << map_name.at(currentTempMap.border.right.mapIndex)
                                  << ", but the left map have not a left map"
                                  << std::endl;
                    else
                        std::cerr << "the map "
                                  << tempName
                                  << "have right map: "
                                  << map_name.at(currentTempMap.border.right.mapIndex)
                                  << ", but the left map have different left map: "
                                  << map_name.at(rightTempMap.border.top.mapIndex)
                                  << std::endl;
                    currentTempMap.border.right.mapIndex=65535;
                    rightTempMap.border.left.mapIndex=65535;
                }
            }
            index++;
        }

        //resolv the offset to change of map
        index=0;
        while(index<semi_loaded_map.size())
        {
            const std::string &tempName=map_name.at(index);
            const uint16_t indexMap=mapPathToId.at(tempName);
            MapType &currentTempMap=flat_map_list[indexMap];
            if(currentTempMap.border.bottom.mapIndex!=65535)
            {
                currentTempMap.border.bottom.x_offset=
                        semi_loaded_map.at(vectorindexOf(map_name,semi_loaded_map.at(index).border.bottom.fileName)).border.top.x_offset-
                        semi_loaded_map.at(index).border.bottom.x_offset;
            }
            else
                currentTempMap.border.bottom.x_offset=0;
            if(currentTempMap.border.top.mapIndex!=65535)
            {
                currentTempMap.border.top.x_offset=
                        semi_loaded_map.at(vectorindexOf(map_name,semi_loaded_map.at(index).border.top.fileName)).border.bottom.x_offset-
                        semi_loaded_map.at(index).border.top.x_offset;
            }
            else
                currentTempMap.border.top.x_offset=0;
            if(currentTempMap.border.left.mapIndex!=65535)
            {
                currentTempMap.border.left.y_offset=
                        semi_loaded_map.at(vectorindexOf(map_name,semi_loaded_map.at(index).border.left.fileName)).border.right.y_offset-
                        semi_loaded_map.at(index).border.left.y_offset;
            }
            else
                currentTempMap.border.left.y_offset=0;
            if(currentTempMap.border.right.mapIndex!=65535)
            {
                currentTempMap.border.right.y_offset=
                        semi_loaded_map.at(vectorindexOf(map_name,semi_loaded_map.at(index).border.right.fileName)).border.left.y_offset-
                        semi_loaded_map.at(index).border.right.y_offset;
            }
            else
                currentTempMap.border.right.y_offset=0;
            index++;
        }

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::cout << semi_loaded_map.size() << " map(s) loaded into " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
    }


    //for tiled
    #ifdef TILED_ZLIB
    static int32_t decompressZlib(const char * const input, const uint32_t &intputSize, char * const output, const uint32_t &maxOutputSize);
    #endif

private:
    std::string error;
    std::unordered_map<std::string,uint16_t> zoneNumber;//sub zone part into same map, NOT linked with zone/
};
}

#endif // MAP_LOADER_H
