#include "ActionsAction.h"

#include "../../general/base/CommonSettingsServer.h"
#include "../../general/base/Map_loader.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../client/base/interface/DatapackClientLoader.h"

#include <regex>
#include <string>
#include <vector>
#include <iostream>

bool ActionsAction::preload_the_map()
{
    const std::string &datapack_mapPath=QCoreApplication::applicationDirPath().toStdString()+"/datapack/"+
            DATAPACK_BASE_PATH_MAPMAIN+
            CommonSettingsServer::commonSettingsServer.mainDatapackCode+
            "/";
    std::cout << "start preload the map, into: " << datapack_mapPath << std::endl;
    CatchChallenger::Map_loader map_temp;
    std::vector<std::string> map_name;
    std::vector<std::string> map_name_to_do_id;
    std::vector<std::string> returnList=CatchChallenger::FacilityLibGeneral::listFolder(datapack_mapPath);
    std::sort(returnList.begin(), returnList.end());

    if(returnList.size()==0)
    {
        std::cerr << "No file map to list" << std::endl;
        abort();
    }
    if(!semi_loaded_map.empty() || flat_map_list!=NULL)
    {
        std::cerr << "preload_the_map() already call" << std::endl;
        abort();
    }
    //load the map
    unsigned int size=returnList.size();
    unsigned int index=0;
    unsigned int sub_index;
    std::string tmxRemove(".tmx");
    std::regex mapFilter("\\.tmx$");
    std::regex mapExclude("[\"']");
    std::vector<CatchChallenger::CommonMap *> flat_map_list_temp;
    while(index<size)
    {
        std::string fileName=returnList.at(index);
        stringreplaceAll(fileName,"\\","/");
        if(regex_search(fileName,mapFilter) && !regex_search(fileName,mapExclude))
        {
            #ifdef DEBUG_MESSAGE_MAP_LOAD
            std::cout << "load the map: " << fileName << std::endl;
            #endif
            std::string sortFileName=fileName;
            stringreplaceOne(sortFileName,tmxRemove,"");
            map_name_to_do_id.push_back(sortFileName);
            if(map_temp.tryLoadMap(datapack_mapPath+fileName,true))
            {
                flat_map_list_temp.push_back(new MapServerMini);
                MapServerMini *mapServer=static_cast<MapServerMini *>(flat_map_list_temp.back());
                map_list[sortFileName]=mapServer;

                mapServer->width			= map_temp.map_to_send.width;
                mapServer->height			= map_temp.map_to_send.height;
                mapServer->parsed_layer	= map_temp.map_to_send.parsed_layer;
                mapServer->map_file		= sortFileName;
                {
                    unsigned int index=0;
                    while(index<map_temp.map_to_send.bots.size())
                    {
                        const CatchChallenger::Map_to_send::Bot_Semi &bot=map_temp.map_to_send.bots.at(index);
                        //it's moving bot consider it into all the current zone, not on the tile
                        if(bot.property_text.find("skin")!=bot.property_text.cend() && (bot.property_text.find("lookAt")==bot.property_text.cend() || bot.property_text.at("lookAt")!="move"))
                        {
                            std::pair<uint8_t,uint8_t> p(bot.point.x,bot.point.y);
                            mapServer->botOnMap[p].push_back(bot.id);
                        }
                        index++;
                    }
                }

                map_name.push_back(sortFileName);

                Map_semi map_semi;
                map_semi.map				= map_list.at(sortFileName);

                if(map_temp.map_to_send.border.top.fileName.size()>0)
                {
                    map_semi.border.top.fileName		= CatchChallenger::Map_loader::resolvRelativeMap(datapack_mapPath+fileName,map_temp.map_to_send.border.top.fileName,datapack_mapPath);
                    stringreplaceOne(map_semi.border.top.fileName,".tmx","");
                    map_semi.border.top.x_offset		= map_temp.map_to_send.border.top.x_offset;
                }
                if(map_temp.map_to_send.border.bottom.fileName.size()>0)
                {
                    map_semi.border.bottom.fileName		= CatchChallenger::Map_loader::resolvRelativeMap(datapack_mapPath+fileName,map_temp.map_to_send.border.bottom.fileName,datapack_mapPath);
                    stringreplaceOne(map_semi.border.bottom.fileName,".tmx","");
                    map_semi.border.bottom.x_offset		= map_temp.map_to_send.border.bottom.x_offset;
                }
                if(map_temp.map_to_send.border.left.fileName.size()>0)
                {
                    map_semi.border.left.fileName		= CatchChallenger::Map_loader::resolvRelativeMap(datapack_mapPath+fileName,map_temp.map_to_send.border.left.fileName,datapack_mapPath);
                    stringreplaceOne(map_semi.border.left.fileName,".tmx","");
                    map_semi.border.left.y_offset		= map_temp.map_to_send.border.left.y_offset;
                }
                if(map_temp.map_to_send.border.right.fileName.size()>0)
                {
                    map_semi.border.right.fileName		= CatchChallenger::Map_loader::resolvRelativeMap(datapack_mapPath+fileName,map_temp.map_to_send.border.right.fileName,datapack_mapPath);
                    stringreplaceOne(map_semi.border.right.fileName,".tmx","");
                    map_semi.border.right.y_offset		= map_temp.map_to_send.border.right.y_offset;
                }

                sub_index=0;
                const unsigned int &listsize=map_temp.map_to_send.teleport.size();
                while(sub_index<listsize)
                {
                    map_temp.map_to_send.teleport[sub_index].map=CatchChallenger::Map_loader::resolvRelativeMap(datapack_mapPath+fileName,map_temp.map_to_send.teleport.at(sub_index).map,datapack_mapPath);
                    stringreplaceOne(map_temp.map_to_send.teleport[sub_index].map,".tmx","");
                    sub_index++;
                }

                map_semi.old_map=map_temp.map_to_send;

                semi_loaded_map.push_back(map_semi);
                loaded++;
            }
            else
            {
                std::cout << "error at loading: " << datapack_mapPath << fileName << ", error: " << map_temp.errorString()
                          << "parsed due: " << "regex_search(" << fileName << ",\\.tmx$) && !regex_search("
                          << fileName << ",[\"'])"
                          << std::endl;
                abort();
            }
        }
        index++;
    }
    {
        flat_map_list=static_cast<CatchChallenger::CommonMap **>(malloc(sizeof(CatchChallenger::CommonMap *)*flat_map_list_temp.size()));
        memset(flat_map_list,0x00,sizeof(CatchChallenger::CommonMap *)*flat_map_list_temp.size());
        unsigned int index=0;
        while(index<flat_map_list_temp.size())
        {
            flat_map_list[index]=flat_map_list_temp.at(index);
            index++;
        }
    }

    std::sort(map_name_to_do_id.begin(),map_name_to_do_id.end());

    //resolv the border map name into their pointer
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        if(semi_loaded_map.at(index).border.bottom.fileName.size()>0 && map_list.find(semi_loaded_map.at(index).border.bottom.fileName)!=map_list.end())
            semi_loaded_map[index].map->border.bottom.map=map_list.at(semi_loaded_map.at(index).border.bottom.fileName);
        else
            semi_loaded_map[index].map->border.bottom.map=NULL;

        if(semi_loaded_map.at(index).border.top.fileName.size()>0 && map_list.find(semi_loaded_map.at(index).border.top.fileName)!=map_list.end())
            semi_loaded_map[index].map->border.top.map=map_list.at(semi_loaded_map.at(index).border.top.fileName);
        else
            semi_loaded_map[index].map->border.top.map=NULL;

        if(semi_loaded_map.at(index).border.left.fileName.size()>0 && map_list.find(semi_loaded_map.at(index).border.left.fileName)!=map_list.end())
            semi_loaded_map[index].map->border.left.map=map_list.at(semi_loaded_map.at(index).border.left.fileName);
        else
            semi_loaded_map[index].map->border.left.map=NULL;

        if(semi_loaded_map.at(index).border.right.fileName.size()>0 && map_list.find(semi_loaded_map.at(index).border.right.fileName)!=map_list.end())
            semi_loaded_map[index].map->border.right.map=map_list.at(semi_loaded_map.at(index).border.right.fileName);
        else
            semi_loaded_map[index].map->border.right.map=NULL;

        index++;
    }

    //resolv the teleported into their pointer
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        unsigned int sub_index=0;
        Map_semi &map_semi=semi_loaded_map.at(index);
        map_semi.map->teleporter_list_size=0;
        /*The datapack dev should fix it and then drop duplicate teleporter, if well done then the final size is map_semi.old_map.teleport.size()*/
        map_semi.map->teleporter=(CatchChallenger::CommonMap::Teleporter *)malloc(sizeof(CatchChallenger::CommonMap::Teleporter)*map_semi.old_map.teleport.size());
        memset(map_semi.map->teleporter,0x00,sizeof(CatchChallenger::CommonMap::Teleporter)*map_semi.old_map.teleport.size());
        while(sub_index<map_semi.old_map.teleport.size() && sub_index<127)//code not ready for more than 127
        {
            const auto &teleport=map_semi.old_map.teleport.at(sub_index);
            std::string teleportString=teleport.map;
            stringreplaceOne(teleportString,".tmx","");
            if(map_list.find(teleportString)!=map_list.end())
            {
                if(teleport.destination_x<map_list.at(teleportString)->width
                        && teleport.destination_y<map_list.at(teleportString)->height)
                {
                    uint16_t index_search=0;
                    while(index_search<map_semi.map->teleporter_list_size)
                    {
                        if(map_semi.map->teleporter[index_search].source_x==teleport.source_x && map_semi.map->teleporter[index_search].source_y==teleport.source_y)
                            break;
                        index_search++;
                    }
                    if(index_search==map_semi.map->teleporter_list_size)
                    {
                        #ifdef DEBUG_MESSAGE_MAP_LOAD
                        std::cout << "teleporter on the map: "
                             << map_semi.map->map_file
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
                        CatchChallenger::CommonMap::Teleporter teleporter;
                        teleporter.map=map_list.at(teleportString);
                        teleporter.source_x=teleport.source_x;
                        teleporter.source_y=teleport.source_y;
                        teleporter.destination_x=teleport.destination_x;
                        teleporter.destination_y=teleport.destination_y;
                        teleporter.condition=teleport.condition;
                        semi_loaded_map[index].map->teleporter[map_semi.map->teleporter_list_size]=teleporter;
                        map_semi.map->teleporter_list_size++;
                    }
                    else
                        std::cerr << "already found teleporter on the map: "
                             << map_semi.map->map_file
                             << "("
                             << std::to_string(teleport.source_x)
                             << ","
                             << std::to_string(teleport.source_y)
                             << "), to "
                             << teleportString
                             << "("
                             << std::to_string(teleport.destination_x)
                             << ","
                             << std::to_string(teleport.destination_y)
                             << ")"
                             << std::endl;
                }
                else
                    std::cerr << "wrong teleporter on the map: "
                         << map_semi.map->map_file
                         << "("
                         << std::to_string(teleport.source_x)
                         << ","
                         << std::to_string(teleport.source_y)
                         << "), to "
                         << teleportString
                         << "("
                         << std::to_string(teleport.destination_x)
                         << ","
                         << std::to_string(teleport.destination_y)
                         << ") because the tp is out of range"
                         << std::endl;
            }
            else
                std::cerr << "wrong teleporter on the map: "
                     << map_semi.map->map_file
                     << "("
                     << std::to_string(teleport.source_x)
                     << ","
                     << std::to_string(teleport.source_y)
                     << "), to "
                     << teleportString
                     << "("
                     << std::to_string(teleport.destination_x)
                     << ","
                     << std::to_string(teleport.destination_y)
                     << ") because the map is not found"
                     << std::endl;

            sub_index++;
        }
        index++;
    }

    //clean border balise without another oposite border
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        const auto &currentTempMap=map_list.at(map_name.at(index));
        if(currentTempMap->border.bottom.map!=NULL && currentTempMap->border.bottom.map->border.top.map!=currentTempMap)
        {
            if(currentTempMap->border.bottom.map->border.top.map==NULL)
                std::cerr << "the map "
                          << currentTempMap->map_file
                          << "have bottom map: "
                          << currentTempMap->border.bottom.map->map_file
                          << ", but the bottom map have not a top map"
                          << std::endl;
            else
                std::cerr << "the map "
                          << currentTempMap->map_file
                          << "have bottom map: "
                          << currentTempMap->border.bottom.map->map_file
                          << ", but the bottom map have different top map: "
                          << currentTempMap->border.bottom.map->border.top.map->map_file
                          << std::endl;
            map_list[map_name.at(index)]->border.bottom.map=NULL;
        }
        if(currentTempMap->border.top.map!=NULL && currentTempMap->border.top.map->border.bottom.map!=currentTempMap)
        {
            if(currentTempMap->border.top.map->border.bottom.map==NULL)
                std::cerr << "the map "
                          << currentTempMap->map_file
                          << "have top map: "
                          << currentTempMap->border.top.map->map_file
                          << ", but the bottom map have not a bottom map"
                          << std::endl;
            else
                std::cerr << "the map "
                          << currentTempMap->map_file
                          << "have top map: "
                          << currentTempMap->border.top.map->map_file
                          << ", but the bottom map have different bottom map: "
                          << currentTempMap->border.top.map->border.bottom.map->map_file
                          << std::endl;
            map_list[map_name.at(index)]->border.top.map=NULL;
        }
        if(currentTempMap->border.left.map!=NULL && currentTempMap->border.left.map->border.right.map!=currentTempMap)
        {
            if(currentTempMap->border.left.map->border.right.map==NULL)
                std::cerr << "the map "
                          << currentTempMap->map_file
                          << "have left map: "
                          << currentTempMap->border.left.map->map_file
                          << ", but the right map have not a right map"
                          << std::endl;
            else
                std::cerr << "the map "
                          << currentTempMap->map_file
                          << "have left map: "
                          << currentTempMap->border.left.map->map_file
                          << ", but the right map have different right map: "
                          << currentTempMap->border.left.map->border.right.map->map_file
                          << std::endl;
            map_list[map_name.at(index)]->border.left.map=NULL;
        }
        if(currentTempMap->border.right.map!=NULL && currentTempMap->border.right.map->border.left.map!=currentTempMap)
        {
            if(currentTempMap->border.right.map->border.left.map==NULL)
                std::cerr << "the map "
                          << currentTempMap->map_file
                          << "have right map: "
                          << currentTempMap->border.right.map->map_file
                          << ", but the left map have not a left map"
                          << std::endl;
            else
                std::cerr << "the map "
                          << currentTempMap->map_file
                          << "have right map: "
                          << currentTempMap->border.right.map->map_file
                          << ", but the left map have different left map: "
                          << currentTempMap->border.right.map->border.left.map->map_file
                          << std::endl;
            map_list[map_name.at(index)]->border.right.map=NULL;
        }
        index++;
    }

    //resolv the near map
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        CatchChallenger::CommonMap * const currentTempMap=map_list.at(map_name.at(index));
        if(currentTempMap->border.bottom.map!=NULL &&
                std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),currentTempMap->border.bottom.map)
                ==
                currentTempMap->near_map.end())
        {
            map_list[map_name.at(index)]->near_map.push_back(currentTempMap->border.bottom.map);
            if(currentTempMap->border.left.map!=NULL && std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),currentTempMap->border.left.map)
                    ==
                    currentTempMap->near_map.end())
                map_list[map_name.at(index)]->near_map.push_back(currentTempMap->border.left.map);
            if(currentTempMap->border.right.map!=NULL && std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),currentTempMap->border.right.map)
                    ==
                    currentTempMap->near_map.end())
                map_list[map_name.at(index)]->near_map.push_back(currentTempMap->border.right.map);
        }

        if(currentTempMap->border.top.map!=NULL &&
                std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),currentTempMap->border.top.map)
                ==
                currentTempMap->near_map.end()
                )
        {
            map_list[map_name.at(index)]->near_map.push_back(currentTempMap->border.top.map);
            if(currentTempMap->border.left.map!=NULL &&  std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),currentTempMap->border.left.map)
                    ==
                    currentTempMap->near_map.end())
                map_list[map_name.at(index)]->near_map.push_back(currentTempMap->border.left.map);
            if(currentTempMap->border.right.map!=NULL &&  std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),currentTempMap->border.right.map)
                    ==
                    currentTempMap->near_map.end())
                map_list[map_name.at(index)]->near_map.push_back(currentTempMap->border.right.map);
        }

        if(currentTempMap->border.right.map!=NULL &&
                std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),currentTempMap->border.right.map)
                ==
                currentTempMap->near_map.end()
                )
        {
            map_list[map_name.at(index)]->near_map.push_back(currentTempMap->border.right.map);
            if(currentTempMap->border.top.map!=NULL &&  std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),currentTempMap->border.top.map)
                    ==
                    currentTempMap->near_map.end())
                map_list[map_name.at(index)]->near_map.push_back(currentTempMap->border.top.map);
            if(currentTempMap->border.bottom.map!=NULL &&  std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),currentTempMap->border.bottom.map)
                    ==
                    currentTempMap->near_map.end())
                map_list[map_name.at(index)]->near_map.push_back(currentTempMap->border.bottom.map);
        }

        if(currentTempMap->border.left.map!=NULL &&
                std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),currentTempMap->border.left.map)
                ==
                currentTempMap->near_map.end()
                )
        {
            map_list[map_name.at(index)]->near_map.push_back(currentTempMap->border.left.map);
            if(currentTempMap->border.top.map!=NULL &&  std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),currentTempMap->border.top.map)
                    ==
                    currentTempMap->near_map.end())
                map_list[map_name.at(index)]->near_map.push_back(currentTempMap->border.top.map);
            if(currentTempMap->border.bottom.map!=NULL &&  std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),currentTempMap->border.bottom.map)
                    ==
                    currentTempMap->near_map.end())
                map_list[map_name.at(index)]->near_map.push_back(currentTempMap->border.bottom.map);
        }

        map_list[map_name.at(index)]->linked_map=map_list[map_name.at(index)]->near_map;
        //the teleporter
        {
            unsigned int index=0;
            while(index<currentTempMap->teleporter_list_size)
            {
                const CatchChallenger::CommonMap::Teleporter &teleporter=currentTempMap->teleporter[index];
                if(!vectorcontainsAtLeastOne(currentTempMap->linked_map,teleporter.map))
                    currentTempMap->linked_map.push_back(teleporter.map);
                index++;
            }
        }

        map_list[map_name.at(index)]->near_map.push_back(currentTempMap);
        index++;
    }

    //resolv the offset to change of map
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        const auto &currentTempMap=map_list.at(map_name.at(index));
        if(currentTempMap->border.bottom.map!=NULL)
        {
            map_list[map_name.at(index)]->border.bottom.x_offset=
                    semi_loaded_map.at(vectorindexOf(map_name,semi_loaded_map.at(index).border.bottom.fileName)).border.top.x_offset-
                    semi_loaded_map.at(index).border.bottom.x_offset;
        }
        else
            map_list[map_name.at(index)]->border.bottom.x_offset=0;
        if(currentTempMap->border.top.map!=NULL)
        {
            map_list[map_name.at(index)]->border.top.x_offset=
                    semi_loaded_map.at(vectorindexOf(map_name,semi_loaded_map.at(index).border.top.fileName)).border.bottom.x_offset-
                    semi_loaded_map.at(index).border.top.x_offset;
        }
        else
            map_list[map_name.at(index)]->border.top.x_offset=0;
        if(currentTempMap->border.left.map!=NULL)
        {
            map_list[map_name.at(index)]->border.left.y_offset=
                    semi_loaded_map.at(vectorindexOf(map_name,semi_loaded_map.at(index).border.left.fileName)).border.right.y_offset-
                    semi_loaded_map.at(index).border.left.y_offset;
        }
        else
            map_list[map_name.at(index)]->border.left.y_offset=0;
        if(currentTempMap->border.right.map!=NULL)
        {
            map_list[map_name.at(index)]->border.right.y_offset=
                    semi_loaded_map.at(vectorindexOf(map_name,semi_loaded_map.at(index).border.right.fileName)).border.left.y_offset-
                    semi_loaded_map.at(index).border.right.y_offset;
        }
        else
            map_list[map_name.at(index)]->border.right.y_offset=0;
        index++;
    }

    //preload_map_semi_after_db_id load the item id

    //nead be after the offet
    preload_the_bots(semi_loaded_map);

    //load the rescue
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        sub_index=0;
        while(sub_index<semi_loaded_map.at(index).old_map.rescue_points.size())
        {
            const CatchChallenger::Map_to_send::Map_Point &point=semi_loaded_map.at(index).old_map.rescue_points.at(sub_index);
            std::pair<uint8_t,uint8_t> coord;
            coord.first=point.x;
            coord.second=point.y;
            static_cast<MapServerMini *>(map_list[map_name.at(index)])->rescue[coord]=CatchChallenger::Orientation_bottom;
            sub_index++;
        }
        index++;
    }

    size=map_name_to_do_id.size();
    index=0;
    while(index<size)
    {
        if(map_list.find(map_name_to_do_id.at(index))!=map_list.end())
        {
            map_list[map_name_to_do_id.at(index)]->id=index;
            id_map_to_map[map_list[map_name_to_do_id.at(index)]->id]=map_name_to_do_id.at(index);
        }
        else
            abort();
        index++;
    }

    std::cout << map_list.size() << " map(s) loaded" << std::endl;

    /*DictionaryServer::dictionary_pointOnMap_item_internal_to_database.clear();
    DictionaryServer::dictionary_pointOnMap_plant_internal_to_database.clear();
    #ifdef EPOLLCATCHCHALLENGERSERVER
    {
        unsigned int index=0;
        while(index<toDeleteAfterBotLoad.size())
        {
            delete toDeleteAfterBotLoad.at(index);
            index++;
        }
        toDeleteAfterBotLoad.clear();
    }
    #endif
    botFiles.clear();*/

    //item on map
    {
        unsigned int index=0;
        while(index<semi_loaded_map.size())
        {
            const ActionsAction::Map_semi &map_semi=semi_loaded_map.at(index);
            unsigned int sub_index=0;
            while(sub_index<map_semi.old_map.items.size())
            {
                const CatchChallenger::Map_to_send::ItemOnMap_Semi &item=map_semi.old_map.items.at(sub_index);
                MapServerMini * const mapServer=static_cast<MapServerMini *>(map_semi.map);

                const std::pair<uint8_t/*x*/,uint8_t/*y*/> pair(item.point.x,item.point.y);
                MapServerMini::ItemOnMap itemOnMap;
                itemOnMap.infinite=item.infinite;
                itemOnMap.item=item.item;
                itemOnMap.visible=item.visible;
                itemOnMap.indexOfItemOnMap=0;
                mapServer->pointOnMap_Item[pair]=itemOnMap;

                sub_index++;
            }

            index++;
        }
    }

    if(!actionsAction->preload_other_pre())
        return false;
    if(!actionsAction->preload_the_map_step1())
        return false;
    emit preload_the_map_finished();
    return true;
}
