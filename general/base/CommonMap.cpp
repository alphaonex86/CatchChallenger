#include "CommonMap.hpp"
#include "Map_loader.hpp"
#include "FacilityLibGeneral.hpp"
#include <iostream>
#include <chrono>

using namespace CatchChallenger;

void * CommonMap::flat_map_list=nullptr;
CATCHCHALLENGER_TYPE_MAPID CommonMap::flat_map_list_size=0;
size_t CommonMap::flat_map_object_size=0;//store in full length to easy multiply by index (16Bits) and have full size pointer

CommonMap::Teleporter* CommonMap::flat_teleporter=nullptr;
CATCHCHALLENGER_TYPE_TELEPORTERID CommonMap::flat_teleporter_list_size=0;//temp, used as size when finish

uint8_t *CommonMap::flat_simplified_map=nullptr;
CATCHCHALLENGER_TYPE_MAPID CommonMap::flat_simplified_map_list_size=0;//temp, used as size when finish

const void * CommonMap::indexToMap(const CATCHCHALLENGER_TYPE_MAPID &index)
{
#ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(flat_map_list==nullptr)
    {
        std::cerr << "CommonMap::indexToMap() flat_map_list==nullptr" << std::endl;
        abort();
    }
    if(flat_map_object_size<=sizeof(CommonMap))
    {
        std::cerr << "CommonMap::indexToMap() map_object_size<=sizeof(CommonMap) " << sizeof(CommonMap) << std::endl;
        abort();
    }
    if(flat_map_list_size<=0)
    {
        std::cerr << "CommonMap::indexToMap() flat_map_list_size<=0" << std::endl;
        abort();
    }
    if(index>=flat_map_list_size)
    {
        std::cerr << "CommonMap::indexToMap() index>=flat_map_list_size the check have to be done at index creation" << std::endl;
        abort();
    }
#endif
    return ((char *)flat_map_list)+index*flat_map_object_size;
}


template<class MapType>
void CommonMap::loadAllMapsAndLink(const std::string &datapack_mapPath,std::vector<Map_semi> &semi_loaded_map,std::unordered_map<std::string, CATCHCHALLENGER_TYPE_MAPID> &mapPathToId)
{
    Map_loader map_temp;
    std::vector<std::string> map_name;
    std::vector<std::string> map_name_to_do_id;
    std::vector<std::string> returnList=FacilityLibGeneral::listFolder(datapack_mapPath);
    std::sort(returnList.begin(), returnList.end());
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    if(returnList.size()==0)
    {
        std::cerr << "No file map to list" << std::endl;
        abort();
    }
    if(!semi_loaded_map.empty() || CommonMap::flat_teleporter_list_size<=0)
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
    std::vector<CommonMap *> flat_map_list_temp;
    uint16_t teleport_count=0;
    while(index<returnList.size())
    {
        std::string fileName=returnList.at(index);
        stringreplaceAll(fileName,"\\","/");
        if(regex_search(fileName,mapFilter) && !regex_search(fileName,mapExclude))
            CommonMap::flat_simplified_map_list_size++;
        index++;
    }

    CommonMap::flat_simplified_map_list_size=0;
    index=0;
    while(index<returnList.size())
    {
        std::string fileName=returnList.at(index);
        stringreplaceAll(fileName,"\\","/");
        if(regex_search(fileName,mapFilter) && !regex_search(fileName,mapExclude))
        {
            #ifdef DEBUG_MESSAGE_MAP_LOAD
            std::cout << "load the map: " << fileName << std::endl;
            #endif
            std::string fileNameWihtoutTmx=fileName;
            stringreplaceOne(fileNameWihtoutTmx,tmxRemove,"");
            map_name_to_do_id.push_back(fileNameWihtoutTmx);
            map_name.push_back(fileNameWihtoutTmx);
            //mapPathToId[sortFileName]=map_name_to_do_id.size(); need be sorted before
            if(map_temp.tryLoadMap(datapack_mapPath+fileName))
            {
                flat_map_list_temp.push_back(new MapType);
                MapType *mapServer=static_cast<MapType *>(flat_map_list_temp.back());
                //GlobalServerData::serverPrivateVariables.map_list[fileNameWihtoutTmx]=mapServer;

                mapServer->width			= static_cast<uint8_t>(map_temp.map_to_send.width);
                mapServer->height			= static_cast<uint8_t>(map_temp.map_to_send.height);
                CommonMap::flat_simplified_map_list_size+=mapServer->width*mapServer->height;
                mapServer->parsed_layer	= map_temp.map_to_send.parsed_layer;
                //mapServer->map_file		= fileNameWihtoutTmx;
                mapServer->zone=255;

                Map_semi map_semi;
                map_semi.map				= mapServer;

                if(map_temp.map_to_send.border.top.fileName.size()>0)
                {
                    map_semi.border.top.fileName		= Map_loader::resolvRelativeMap(datapack_mapPath+fileName,map_temp.map_to_send.border.top.fileName,datapack_mapPath);
                    stringreplaceOne(map_semi.border.top.fileName,".tmx","");
                    map_semi.border.top.x_offset		= map_temp.map_to_send.border.top.x_offset;
                }
                if(map_temp.map_to_send.border.bottom.fileName.size()>0)
                {
                    map_semi.border.bottom.fileName		= Map_loader::resolvRelativeMap(datapack_mapPath+fileName,map_temp.map_to_send.border.bottom.fileName,datapack_mapPath);
                    stringreplaceOne(map_semi.border.bottom.fileName,".tmx","");
                    map_semi.border.bottom.x_offset		= map_temp.map_to_send.border.bottom.x_offset;
                }
                if(map_temp.map_to_send.border.left.fileName.size()>0)
                {
                    map_semi.border.left.fileName		= Map_loader::resolvRelativeMap(datapack_mapPath+fileName,map_temp.map_to_send.border.left.fileName,datapack_mapPath);
                    stringreplaceOne(map_semi.border.left.fileName,".tmx","");
                    map_semi.border.left.y_offset		= map_temp.map_to_send.border.left.y_offset;
                }
                if(map_temp.map_to_send.border.right.fileName.size()>0)
                {
                    map_semi.border.right.fileName		= Map_loader::resolvRelativeMap(datapack_mapPath+fileName,map_temp.map_to_send.border.right.fileName,datapack_mapPath);
                    stringreplaceOne(map_semi.border.right.fileName,".tmx","");
                    map_semi.border.right.y_offset		= map_temp.map_to_send.border.right.y_offset;
                }

                teleport_count+=map_temp.map_to_send.teleport.size();
                sub_index=0;
                while(sub_index<map_temp.map_to_send.teleport.size())
                {
                    map_temp.map_to_send.teleport[sub_index].map=Map_loader::resolvRelativeMap(datapack_mapPath+fileName,map_temp.map_to_send.teleport.at(sub_index).map,datapack_mapPath);
                    stringreplaceOne(map_temp.map_to_send.teleport[sub_index].map,".tmx","");
                    sub_index++;
                }

                map_semi.old_map=map_temp.map_to_send;

                semi_loaded_map.push_back(map_semi);
            }
            else
            {
                std::cout << "error at loading: " << datapack_mapPath << fileName << ", error: " << map_temp.errorString()
                          << "parsed due: " << "regex_search(" << fileName << ",\\.tmx$) && !regex_search("
                          << fileName << ",[\"'])"
                          << std::endl;

                flat_map_list_temp.push_back(new MapType);
                MapType *mapServer=static_cast<MapType *>(flat_map_list_temp.back());

                mapServer->width			= 0;
                mapServer->height			= 0;
                mapServer->parsed_layer.simplifiedMapIndex=65535;
                //mapServer->map_file		= fileNameWihtoutTmx;

                Map_semi map_semi;
                map_semi.map				= mapServer;

                map_semi.old_map=map_temp.map_to_send;

                semi_loaded_map.push_back(map_semi);

                //abort();
            }
        }
        index++;
    }

    //memory allocation
    {
        const size_t s=sizeof(MapType)*map_name.size();
        CommonMap::flat_map_list=static_cast<MapType *>(malloc(s));
        CommonMap::flat_map_object_size=sizeof(MapType);
        #ifdef CATCHCHALLENGER_HARDENED
        memset((void *)GlobalServerData::serverPrivateVariables.flat_map_list,0,tempMemSize);//security but performance problem
        for(CATCHCHALLENGER_TYPE_MAPID i=0; i<GlobalServerData::serverPrivateVariables.flat_map_size; i++)
            new(GlobalServerData::serverPrivateVariables.flat_map_list+i) MapType();
        #endif
    }
    {
        const size_t s=sizeof(CommonMap::Teleporter)*teleport_count;
        CommonMap::flat_teleporter=static_cast<CommonMap::Teleporter *>(malloc(s));
        #ifdef CATCHCHALLENGER_HARDENED
        memset((void *)CommonMap::flat_teleporter,0,s);//security but performance problem
        #endif
    }
    {
        CommonMap::flat_simplified_map=static_cast<uint8_t *>(malloc(CommonMap::flat_simplified_map_list_size));
        #ifdef CATCHCHALLENGER_HARDENED
        memset((void *)CommonMap::flat_simplified_map,0,CommonMap::flat_simplified_map_list_size);//security but performance problem
        #endif
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

    //resolv the border map name into their pointer
    index=0;
    while(index<semi_loaded_map.size())
    {
        Map_semi &map_semi=semi_loaded_map[index];
        if(map_semi.border.bottom.fileName.size()>0 && mapPathToId.find(map_semi.border.bottom.fileName)!=mapPathToId.end())
            map_semi.map->border.bottom.mapIndex=mapPathToId.at(map_semi.border.bottom.fileName);
        else
            map_semi.map->border.bottom.mapIndex=65535;

        if(map_semi.border.top.fileName.size()>0 && mapPathToId.find(map_semi.border.top.fileName)!=mapPathToId.end())
            map_semi.map->border.top.mapIndex=mapPathToId.at(map_semi.border.top.fileName);
        else
            map_semi.map->border.top.mapIndex=65535;

        if(map_semi.border.left.fileName.size()>0 && mapPathToId.find(map_semi.border.left.fileName)!=mapPathToId.end())
            map_semi.map->border.left.mapIndex=mapPathToId.at(map_semi.border.left.fileName);
        else
            map_semi.map->border.left.mapIndex=65535;

        if(map_semi.border.right.fileName.size()>0 && mapPathToId.find(map_semi.border.right.fileName)!=mapPathToId.end())
            map_semi.map->border.right.mapIndex=mapPathToId.at(map_semi.border.right.fileName);
        else
            map_semi.map->border.right.mapIndex=65535;

        index++;
    }

    //resolv the teleported into their pointer
    index=0;
    while(index<semi_loaded_map.size())
    {
        unsigned int sub_index=0;
        Map_semi &map_semi=semi_loaded_map.at(index);
        map_semi.map->teleporter_list_size=0;
        map_semi.map->teleporter_first_index=CommonMap::flat_teleporter_list_size;
        /*The datapack dev should fix it and then drop duplicate teleporter, if well done then the final size is map_semi.old_map.teleport.size()*/
        while(sub_index<map_semi.old_map.teleport.size() && sub_index<127)//code not ready for more than 127
        {
            const Map_semi_teleport &teleporter_semi=map_semi.old_map.teleport.at(sub_index);
            std::string teleportString=teleporter_semi.map;
            stringreplaceOne(teleportString,".tmx","");
            if(mapPathToId.find(teleportString)!=mapPathToId.end())
            {
                if(teleporter_semi.destination_x<map_semi.map->width
                        && teleporter_semi.destination_y<map_semi.map->height)
                {
                    uint16_t index_search=0;
                    while(index_search<map_semi.map->teleporter_list_size)
                    {
                        const CommonMap::Teleporter &teleporter=*(CommonMap::flat_teleporter+map_semi.map->teleporter_first_index+index_search);
                        if(teleporter.source_x==teleporter_semi.source_x && teleporter.source_y==teleporter_semi.source_y)
                            break;
                        index_search++;
                    }
                    if(index_search==map_semi.map->teleporter_list_size)
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
                        CommonMap::Teleporter &teleporter=*(CommonMap::flat_teleporter+map_semi.map->teleporter_first_index+index_search);
                        teleporter.mapIndex=mapPathToId.at(teleportString);
                        teleporter.source_x=teleporter_semi.source_x;
                        teleporter.source_y=teleporter_semi.source_y;
                        teleporter.destination_x=teleporter_semi.destination_x;
                        teleporter.destination_y=teleporter_semi.destination_y;
                        teleporter.condition=teleporter_semi.condition;
                        map_semi.map->teleporter_list_size++;
                        CommonMap::flat_teleporter_list_size++;
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
        MapType &currentTempMap=*((MapType *)CommonMap::flat_map_list+indexMap);
        if(currentTempMap.border.bottom.mapIndex!=65535)
        {
            MapType &bottomTempMap=*((MapType *)CommonMap::flat_map_list+currentTempMap.border.bottom.mapIndex);
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
            MapType &topTempMap=*((MapType *)CommonMap::flat_map_list+currentTempMap.border.top.mapIndex);
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
            MapType &leftTempMap=*((MapType *)CommonMap::flat_map_list+currentTempMap.border.left.mapIndex);
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
            MapType &rightTempMap=*((MapType *)CommonMap::flat_map_list+currentTempMap.border.right.mapIndex);
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
        MapType &currentTempMap=*((MapType *)CommonMap::flat_map_list+indexMap);
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
}
