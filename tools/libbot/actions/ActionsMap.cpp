#include "ActionsAction.h"

#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/Map_loader.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp"

#include <regex>
#include <string>
#include <vector>
#include <iostream>
#include <QCoreApplication>

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

            flat_map_list_temp.push_back(new MapServerMini);
            MapServerMini *mapServer=static_cast<MapServerMini *>(flat_map_list_temp.back());

            if(map_temp.tryLoadMap(datapack_mapPath+fileName,*mapServer,true))
            {
                map_list[sortFileName]=mapServer;

                // width/height/flat_simplified_map already filled by tryLoadMap into *mapServer
                // copy from map_to_send as fallback
                if(mapServer->flat_simplified_map.empty())
                    mapServer->flat_simplified_map=map_temp.map_to_send.flat_simplified_map;
                if(mapServer->width==0)
                    mapServer->width=map_temp.map_to_send.width;
                if(mapServer->height==0)
                    mapServer->height=map_temp.map_to_send.height;
                mapServer->map_file		= sortFileName;
                {
                    unsigned int index=0;
                    while(index<map_temp.map_to_send.bots.size())
                    {
                        const CatchChallenger::Map_to_send::Bot_Semi &bot=map_temp.map_to_send.bots.at(index);
                        //it's moving bot consider it into all the current zone, not on the tile
                        if(bot.property_text.find("skin")!=bot.property_text.cend() && (bot.property_text.find("lookAt")==bot.property_text.cend() || bot.property_text.at("lookAt")!="move"))
                        {
                            std::pair<uint8_t,uint8_t> p(bot.point.first,bot.point.second);
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
                delete flat_map_list_temp.back();
                flat_map_list_temp.pop_back();
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

    // Build name-to-index map for resolving border/teleporter references
    std::unordered_map<std::string, CATCHCHALLENGER_TYPE_MAPID> name_to_index;
    for(CATCHCHALLENGER_TYPE_MAPID i=0;i<map_name.size();i++)
        name_to_index[map_name[i]]=i;

    //resolv the border map name into their index
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        if(semi_loaded_map.at(index).border.bottom.fileName.size()>0 && name_to_index.find(semi_loaded_map.at(index).border.bottom.fileName)!=name_to_index.end())
            semi_loaded_map[index].map->border.bottom.mapIndex=name_to_index.at(semi_loaded_map.at(index).border.bottom.fileName);
        else
            semi_loaded_map[index].map->border.bottom.mapIndex=65535;

        if(semi_loaded_map.at(index).border.top.fileName.size()>0 && name_to_index.find(semi_loaded_map.at(index).border.top.fileName)!=name_to_index.end())
            semi_loaded_map[index].map->border.top.mapIndex=name_to_index.at(semi_loaded_map.at(index).border.top.fileName);
        else
            semi_loaded_map[index].map->border.top.mapIndex=65535;

        if(semi_loaded_map.at(index).border.left.fileName.size()>0 && name_to_index.find(semi_loaded_map.at(index).border.left.fileName)!=name_to_index.end())
            semi_loaded_map[index].map->border.left.mapIndex=name_to_index.at(semi_loaded_map.at(index).border.left.fileName);
        else
            semi_loaded_map[index].map->border.left.mapIndex=65535;

        if(semi_loaded_map.at(index).border.right.fileName.size()>0 && name_to_index.find(semi_loaded_map.at(index).border.right.fileName)!=name_to_index.end())
            semi_loaded_map[index].map->border.right.mapIndex=name_to_index.at(semi_loaded_map.at(index).border.right.fileName);
        else
            semi_loaded_map[index].map->border.right.mapIndex=65535;

        index++;
    }

    //resolv the teleporter into their mapIndex
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        unsigned int sub_index=0;
        Map_semi &map_semi=semi_loaded_map.at(index);
        MapServerMini *mapServer=static_cast<MapServerMini *>(map_semi.map);
        map_semi.map->teleporters.clear();
        while(sub_index<map_semi.old_map.teleport.size() && sub_index<127)//code not ready for more than 127
        {
            const CatchChallenger::Map_semi_teleport &teleport=map_semi.old_map.teleport.at(sub_index);
            std::string teleportString=teleport.map;
            stringreplaceOne(teleportString,".tmx","");
            if(name_to_index.find(teleportString)!=name_to_index.end())
            {
                CATCHCHALLENGER_TYPE_MAPID destMapIndex=name_to_index.at(teleportString);
                CatchChallenger::CommonMap *destMap=flat_map_list[destMapIndex];
                if(teleport.destination_x<destMap->width
                        && teleport.destination_y<destMap->height)
                {
                    //check for duplicate
                    bool duplicate=false;
                    for(unsigned int idx=0;idx<map_semi.map->teleporters.size();idx++)
                    {
                        if(map_semi.map->teleporters[idx].source_x==teleport.source_x && map_semi.map->teleporters[idx].source_y==teleport.source_y)
                        {
                            duplicate=true;
                            break;
                        }
                    }
                    if(!duplicate)
                    {
                        #ifdef DEBUG_MESSAGE_MAP_LOAD
                        std::cout << "teleporter on the map: "
                             << mapServer->map_file
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
                        #endif
                        CatchChallenger::Teleporter tp;
                        tp.mapIndex=destMapIndex;
                        tp.source_x=teleport.source_x;
                        tp.source_y=teleport.source_y;
                        tp.destination_x=teleport.destination_x;
                        tp.destination_y=teleport.destination_y;
                        tp.condition=teleport.condition;
                        map_semi.map->teleporters.push_back(tp);
                    }
                    else
                        std::cerr << "already found teleporter on the map: "
                             << mapServer->map_file
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
                         << mapServer->map_file
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
                     << mapServer->map_file
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
        CatchChallenger::CommonMap *currentTempMap=map_list.at(map_name.at(index));
        MapServerMini *currentMapServer=static_cast<MapServerMini *>(currentTempMap);
        if(currentTempMap->border.bottom.mapIndex!=65535 && flat_map_list[currentTempMap->border.bottom.mapIndex]->border.top.mapIndex!=name_to_index[map_name.at(index)])
        {
            if(flat_map_list[currentTempMap->border.bottom.mapIndex]->border.top.mapIndex==65535)
                std::cerr << "the map "
                          << currentMapServer->map_file
                          << " have bottom map: "
                          << static_cast<MapServerMini *>(flat_map_list[currentTempMap->border.bottom.mapIndex])->map_file
                          << ", but the bottom map have not a top map"
                          << std::endl;
            else
                std::cerr << "the map "
                          << currentMapServer->map_file
                          << " have bottom map: "
                          << static_cast<MapServerMini *>(flat_map_list[currentTempMap->border.bottom.mapIndex])->map_file
                          << ", but the bottom map have different top map: "
                          << static_cast<MapServerMini *>(flat_map_list[flat_map_list[currentTempMap->border.bottom.mapIndex]->border.top.mapIndex])->map_file
                          << std::endl;
            map_list[map_name.at(index)]->border.bottom.mapIndex=65535;
        }
        if(currentTempMap->border.top.mapIndex!=65535 && flat_map_list[currentTempMap->border.top.mapIndex]->border.bottom.mapIndex!=name_to_index[map_name.at(index)])
        {
            if(flat_map_list[currentTempMap->border.top.mapIndex]->border.bottom.mapIndex==65535)
                std::cerr << "the map "
                          << currentMapServer->map_file
                          << " have top map: "
                          << static_cast<MapServerMini *>(flat_map_list[currentTempMap->border.top.mapIndex])->map_file
                          << ", but the bottom map have not a bottom map"
                          << std::endl;
            else
                std::cerr << "the map "
                          << currentMapServer->map_file
                          << " have top map: "
                          << static_cast<MapServerMini *>(flat_map_list[currentTempMap->border.top.mapIndex])->map_file
                          << ", but the bottom map have different bottom map: "
                          << static_cast<MapServerMini *>(flat_map_list[flat_map_list[currentTempMap->border.top.mapIndex]->border.bottom.mapIndex])->map_file
                          << std::endl;
            map_list[map_name.at(index)]->border.top.mapIndex=65535;
        }
        if(currentTempMap->border.left.mapIndex!=65535 && flat_map_list[currentTempMap->border.left.mapIndex]->border.right.mapIndex!=name_to_index[map_name.at(index)])
        {
            if(flat_map_list[currentTempMap->border.left.mapIndex]->border.right.mapIndex==65535)
                std::cerr << "the map "
                          << currentMapServer->map_file
                          << " have left map: "
                          << static_cast<MapServerMini *>(flat_map_list[currentTempMap->border.left.mapIndex])->map_file
                          << ", but the right map have not a right map"
                          << std::endl;
            else
                std::cerr << "the map "
                          << currentMapServer->map_file
                          << " have left map: "
                          << static_cast<MapServerMini *>(flat_map_list[currentTempMap->border.left.mapIndex])->map_file
                          << ", but the right map have different right map: "
                          << static_cast<MapServerMini *>(flat_map_list[flat_map_list[currentTempMap->border.left.mapIndex]->border.right.mapIndex])->map_file
                          << std::endl;
            map_list[map_name.at(index)]->border.left.mapIndex=65535;
        }
        if(currentTempMap->border.right.mapIndex!=65535 && flat_map_list[currentTempMap->border.right.mapIndex]->border.left.mapIndex!=name_to_index[map_name.at(index)])
        {
            if(flat_map_list[currentTempMap->border.right.mapIndex]->border.left.mapIndex==65535)
                std::cerr << "the map "
                          << currentMapServer->map_file
                          << " have right map: "
                          << static_cast<MapServerMini *>(flat_map_list[currentTempMap->border.right.mapIndex])->map_file
                          << ", but the left map have not a left map"
                          << std::endl;
            else
                std::cerr << "the map "
                          << currentMapServer->map_file
                          << " have right map: "
                          << static_cast<MapServerMini *>(flat_map_list[currentTempMap->border.right.mapIndex])->map_file
                          << ", but the left map have different left map: "
                          << static_cast<MapServerMini *>(flat_map_list[flat_map_list[currentTempMap->border.right.mapIndex]->border.left.mapIndex])->map_file
                          << std::endl;
            map_list[map_name.at(index)]->border.right.mapIndex=65535;
        }
        index++;
    }

    //resolv the near map
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        MapServerMini * const currentTempMap=static_cast<MapServerMini *>(map_list.at(map_name.at(index)));
        if(currentTempMap->border.bottom.mapIndex!=65535 &&
                std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),flat_map_list[currentTempMap->border.bottom.mapIndex])
                ==
                currentTempMap->near_map.end())
        {
            static_cast<MapServerMini *>(map_list[map_name.at(index)])->near_map.push_back(flat_map_list[currentTempMap->border.bottom.mapIndex]);
            if(currentTempMap->border.left.mapIndex!=65535 && std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),flat_map_list[currentTempMap->border.left.mapIndex])
                    ==
                    currentTempMap->near_map.end())
                static_cast<MapServerMini *>(map_list[map_name.at(index)])->near_map.push_back(flat_map_list[currentTempMap->border.left.mapIndex]);
            if(currentTempMap->border.right.mapIndex!=65535 && std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),flat_map_list[currentTempMap->border.right.mapIndex])
                    ==
                    currentTempMap->near_map.end())
                static_cast<MapServerMini *>(map_list[map_name.at(index)])->near_map.push_back(flat_map_list[currentTempMap->border.right.mapIndex]);
        }

        if(currentTempMap->border.top.mapIndex!=65535 &&
                std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),flat_map_list[currentTempMap->border.top.mapIndex])
                ==
                currentTempMap->near_map.end()
                )
        {
            static_cast<MapServerMini *>(map_list[map_name.at(index)])->near_map.push_back(flat_map_list[currentTempMap->border.top.mapIndex]);
            if(currentTempMap->border.left.mapIndex!=65535 &&  std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),flat_map_list[currentTempMap->border.left.mapIndex])
                    ==
                    currentTempMap->near_map.end())
                static_cast<MapServerMini *>(map_list[map_name.at(index)])->near_map.push_back(flat_map_list[currentTempMap->border.left.mapIndex]);
            if(currentTempMap->border.right.mapIndex!=65535 &&  std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),flat_map_list[currentTempMap->border.right.mapIndex])
                    ==
                    currentTempMap->near_map.end())
                static_cast<MapServerMini *>(map_list[map_name.at(index)])->near_map.push_back(flat_map_list[currentTempMap->border.right.mapIndex]);
        }

        if(currentTempMap->border.right.mapIndex!=65535 &&
                std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),flat_map_list[currentTempMap->border.right.mapIndex])
                ==
                currentTempMap->near_map.end()
                )
        {
            static_cast<MapServerMini *>(map_list[map_name.at(index)])->near_map.push_back(flat_map_list[currentTempMap->border.right.mapIndex]);
            if(currentTempMap->border.top.mapIndex!=65535 &&  std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),flat_map_list[currentTempMap->border.top.mapIndex])
                    ==
                    currentTempMap->near_map.end())
                static_cast<MapServerMini *>(map_list[map_name.at(index)])->near_map.push_back(flat_map_list[currentTempMap->border.top.mapIndex]);
            if(currentTempMap->border.bottom.mapIndex!=65535 &&  std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),flat_map_list[currentTempMap->border.bottom.mapIndex])
                    ==
                    currentTempMap->near_map.end())
                static_cast<MapServerMini *>(map_list[map_name.at(index)])->near_map.push_back(flat_map_list[currentTempMap->border.bottom.mapIndex]);
        }

        if(currentTempMap->border.left.mapIndex!=65535 &&
                std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),flat_map_list[currentTempMap->border.left.mapIndex])
                ==
                currentTempMap->near_map.end()
                )
        {
            static_cast<MapServerMini *>(map_list[map_name.at(index)])->near_map.push_back(flat_map_list[currentTempMap->border.left.mapIndex]);
            if(currentTempMap->border.top.mapIndex!=65535 &&  std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),flat_map_list[currentTempMap->border.top.mapIndex])
                    ==
                    currentTempMap->near_map.end())
                static_cast<MapServerMini *>(map_list[map_name.at(index)])->near_map.push_back(flat_map_list[currentTempMap->border.top.mapIndex]);
            if(currentTempMap->border.bottom.mapIndex!=65535 &&  std::find(currentTempMap->near_map.begin(),currentTempMap->near_map.end(),flat_map_list[currentTempMap->border.bottom.mapIndex])
                    ==
                    currentTempMap->near_map.end())
                static_cast<MapServerMini *>(map_list[map_name.at(index)])->near_map.push_back(flat_map_list[currentTempMap->border.bottom.mapIndex]);
        }

        static_cast<MapServerMini *>(map_list[map_name.at(index)])->linked_map=static_cast<MapServerMini *>(map_list[map_name.at(index)])->near_map;
        //the teleporter
        {
            unsigned int index=0;
            while(index<currentTempMap->teleporters.size())
            {
                const CatchChallenger::Teleporter &tp=currentTempMap->teleporters[index];
                CatchChallenger::CommonMap *tpMap=flat_map_list[tp.mapIndex];
                if(!vectorcontainsAtLeastOne(currentTempMap->linked_map,tpMap))
                    currentTempMap->linked_map.push_back(tpMap);
                index++;
            }
        }

        static_cast<MapServerMini *>(map_list[map_name.at(index)])->near_map.push_back(currentTempMap);
        index++;
    }

    //resolv the offset to change of map
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        CatchChallenger::CommonMap *currentTempMap=map_list.at(map_name.at(index));
        if(currentTempMap->border.bottom.mapIndex!=65535)
        {
            map_list[map_name.at(index)]->border.bottom.x_offset=
                    semi_loaded_map.at(vectorindexOf(map_name,semi_loaded_map.at(index).border.bottom.fileName)).border.top.x_offset-
                    semi_loaded_map.at(index).border.bottom.x_offset;
        }
        else
            map_list[map_name.at(index)]->border.bottom.x_offset=0;
        if(currentTempMap->border.top.mapIndex!=65535)
        {
            map_list[map_name.at(index)]->border.top.x_offset=
                    semi_loaded_map.at(vectorindexOf(map_name,semi_loaded_map.at(index).border.top.fileName)).border.bottom.x_offset-
                    semi_loaded_map.at(index).border.top.x_offset;
        }
        else
            map_list[map_name.at(index)]->border.top.x_offset=0;
        if(currentTempMap->border.left.mapIndex!=65535)
        {
            map_list[map_name.at(index)]->border.left.y_offset=
                    semi_loaded_map.at(vectorindexOf(map_name,semi_loaded_map.at(index).border.left.fileName)).border.right.y_offset-
                    semi_loaded_map.at(index).border.left.y_offset;
        }
        else
            map_list[map_name.at(index)]->border.left.y_offset=0;
        if(currentTempMap->border.right.mapIndex!=65535)
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

    //load the rescue (rescue_points removed from BaseMap, loaded via bot XML now)
    // MapServerMini::rescue is populated by bot steps if needed

    size=map_name_to_do_id.size();
    index=0;
    while(index<size)
    {
        if(map_list.find(map_name_to_do_id.at(index))!=map_list.end())
        {
            static_cast<MapServerMini *>(map_list[map_name_to_do_id.at(index)])->id=index;
            id_map_to_map[static_cast<MapServerMini *>(map_list[map_name_to_do_id.at(index)])->id]=map_name_to_do_id.at(index);
        }
        else
            abort();
        index++;
    }

    std::cout << map_list.size() << " map(s) loaded" << std::endl;

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

                const std::pair<uint8_t/*x*/,uint8_t/*y*/> pair(item.point.first,item.point.second);
                MapServerMini::ItemOnMap itemOnMap;
                itemOnMap.infinite=item.infinite;
                itemOnMap.item=item.item;
                itemOnMap.visible=item.visible;
                itemOnMap.indexOfItemOnMap=sub_index;
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
