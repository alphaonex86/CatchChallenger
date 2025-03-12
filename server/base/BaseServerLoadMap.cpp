#include "BaseServer.hpp"
#include "GlobalServerData.hpp"
#include "DictionaryServer.hpp"

#include <chrono>

#include "ClientMapManagement/Map_server_MapVisibility_Simple_StoreOnSender.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/CommonMap.hpp"

using namespace CatchChallenger;

bool BaseServer::preload_9_sync_the_map()
{
    std::vector<CommonMap::Map_semi> semi_loaded_map;

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
        parseJustLoadedMap(semi_loaded_map.at(index),map_name_to_do_id.at(index));
        if(CommonDatapackServerSpec::commonDatapackServerSpec.get_zoneToId().find(map_semi.old_map.zoneName)!=CommonDatapackServerSpec::commonDatapackServerSpec.get_zoneToId().cend())
            mapServer->zone=CommonDatapackServerSpec::commonDatapackServerSpec.get_zoneToId().at(map_semi.old_map.zoneName);
        sub_index=0;
        while(sub_index<semi_loaded_map.at(index).old_map.rescue_points.size())
        {
            const Map_to_send::Map_Point &point=semi_loaded_map.at(index).old_map.rescue_points.at(sub_index);
            std::pair<uint8_t,uint8_t> coord;
            coord.first=point.x;
            coord.second=point.y;
            static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)])->rescue[coord]=Orientation_bottom;
            sub_index++;
        }
        index++;
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << GlobalServerData::serverPrivateVariables.map_list.size() << " map(s) loaded into " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;

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
    return true;
}
