#include "BaseServer.hpp"
#include "../GlobalServerData.hpp"
#include "../DictionaryServer.hpp"

#include <chrono>

#include "../ClientMapManagement/Map_server_MapVisibility_Simple_StoreOnSender.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/CommonMap.hpp"

using namespace CatchChallenger;

bool BaseServer::preload_9_sync_the_map()
{
    std::vector<Map_semi> semi_loaded_map;

    if(CommonDatapackServerSpec::commonDatapackServerSpec.get_zoneToId().empty())
    {
        std::cerr << "CommonDatapackServerSpec::commonDatapackServerSpec.get_zoneToId().empty()" << std::endl;
        abort();
    }

    GlobalServerData::serverPrivateVariables.datapack_mapPath=GlobalServerData::serverSettings.datapack_basePath+
            DATAPACK_BASE_PATH_MAPMAIN+
            CommonSettingsServer::commonSettingsServer.mainDatapackCode+
            "/";
    #ifdef DEBUG_MESSAGE_MAP_LOAD
    std::cout << "start preload the map, into: " << GlobalServerData::serverPrivateVariables.datapack_mapPath << std::endl;
    #endif
    CommonMap::loadAllMapsAndLink<Map_server_MapVisibility_Simple_StoreOnSender>(GlobalServerData::serverPrivateVariables.datapack_mapPath,semi_loaded_map,mapPathToId);

    //load the rescue, extra and zone
    CATCHCHALLENGER_TYPE_MAPID index=0;
    while(index<semi_loaded_map.size())
    {
        Map_semi &map_semi=semi_loaded_map.at(index);
        MapServer &map_server=*static_cast<MapServer *>(map_semi.map);
        if(CommonDatapackServerSpec::commonDatapackServerSpec.get_zoneToId().find(map_semi.old_map.zoneName)!=CommonDatapackServerSpec::commonDatapackServerSpec.get_zoneToId().cend())
            map_server.zone=CommonDatapackServerSpec::commonDatapackServerSpec.get_zoneToId().at(map_semi.old_map.zoneName);
        unsigned int sub_index=0;
        while(sub_index<semi_loaded_map.at(index).old_map.rescue_points.size())
        {
            const Map_semi &semi=semi_loaded_map.at(index);
            unsigned int bot_index=0;
            while(bot_index<semi.old_map.bots.size())
            {
                const Map_to_send::Bot_Semi &botsemi=semi.old_map.bots.at(bot_index);
                for (const std::pair<uint8_t,const tinyxml2::XMLElement *>& n : botsemi.steps)
                {
                    const tinyxml2::XMLElement * step=n.second;
                    if(strcmp(step->Attribute("type"),"heal")==0)
                        map_server.heal.insert(botsemi.point);
                }
                bot_index++;
            }
            const Map_to_send::Map_Point &point=semi.old_map.rescue_points.at(sub_index);
            std::pair<uint8_t,uint8_t> coord;
            coord.first=point.x;
            coord.second=point.y;
            map_server.rescue[coord]=Orientation_bottom;
            sub_index++;
        }
        const Map_to_send::Bot_Semi &botOnMap=semi_loaded_map.at(index).old_map.at(indexBot);
        //load heal

        index++;
    }

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
    botFiles.clear();
    CommonDatapackServerSpec::commonDatapackServerSpec.parseDatapackAfterZoneAndMap(GlobalServerData::serverSettings.datapack_basePath,CommonSettingsServer::commonSettingsServer.mainDatapackCode,CommonSettingsServer::commonSettingsServer.subDatapackCode,mapPathToId);
    return true;
}
