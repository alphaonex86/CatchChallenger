#include "BaseServer.hpp"
#include "../GlobalServerData.hpp"
#include "../DictionaryServer.hpp"

#include <chrono>

#include "../MapManagement/Map_server_MapVisibility_Simple_StoreOnSender.hpp"
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
    Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.clear();
    mapPathToId.clear();
    Map_loader::loadAllMapsAndLink<Map_server_MapVisibility_Simple_StoreOnSender>(Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list,GlobalServerData::serverPrivateVariables.datapack_mapPath,semi_loaded_map,mapPathToId);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.size()!=semi_loaded_map.size())
    {
        std::cerr << "Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.size()!=semi_loaded_map.size()" << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
    }
    #endif

    //load the rescue, extra and zone
    CATCHCHALLENGER_TYPE_MAPID index=0;
    while(index<semi_loaded_map.size())
    {
        Map_semi &map_semi=semi_loaded_map.at(index);
        Map_server_MapVisibility_Simple_StoreOnSender &map_server=Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list[index];
        if(CommonDatapackServerSpec::commonDatapackServerSpec.get_zoneToId().find(map_semi.old_map.zoneName)!=CommonDatapackServerSpec::commonDatapackServerSpec.get_zoneToId().cend())
            map_server.zone=CommonDatapackServerSpec::commonDatapackServerSpec.get_zoneToId().at(map_semi.old_map.zoneName);
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
    CommonDatapackServerSpec::commonDatapackServerSpec.parseDatapackAfterZoneAndMap(GlobalServerData::serverSettings.datapack_basePath,CommonSettingsServer::commonSettingsServer.mainDatapackCode,CommonSettingsServer::commonSettingsServer.subDatapackCode,mapPathToId);
    return true;
}
