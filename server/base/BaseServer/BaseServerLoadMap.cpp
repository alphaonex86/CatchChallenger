#include "BaseServer.hpp"
#include "../GlobalServerData.hpp"
#include "../DictionaryServer.hpp"

#include <chrono>
#include <string.h>

#include "../MapManagement/Map_server_MapVisibility_Simple_StoreOnSender.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/CommonMap/CommonMap.hpp"
#include "../../general/base/Map_loader.hpp"

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
    #ifdef EPOLLCATCHCHALLENGERSERVER
    std::vector<tinyxml2::XMLDocument*> xmlDocsToKeep;
    #endif
    {
        std::vector<CommonMap> flat_map_list_temp;
        #ifdef EPOLLCATCHCHALLENGERSERVER
        Map_loader::loadAllMapsAndLink(flat_map_list_temp,GlobalServerData::serverPrivateVariables.datapack_mapPath,semi_loaded_map,mapPathToId,&xmlDocsToKeep);
        #else
        Map_loader::loadAllMapsAndLink(flat_map_list_temp,GlobalServerData::serverPrivateVariables.datapack_mapPath,semi_loaded_map,mapPathToId);
        #endif
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(flat_map_list_temp.size()!=semi_loaded_map.size())
        {
            std::cerr << "flat_map_list_temp.size()!=semi_loaded_map.size()" << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
        }
        #endif
        // create new MapServer vector same size as received CommonMap vector
        Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.resize(flat_map_list_temp.size());
        unsigned int i=0;
        while(i<flat_map_list_temp.size())
        {
            Map_server_MapVisibility_Simple_StoreOnSender &map_server=Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list[i];
            const Map_semi &map_semi=semi_loaded_map.at(i);
            // copy CommonMap data to MapServer (border, id, flat_simplified_map, teleporters, width, height, zones, industries, botFights, shops, botsFightTrigger, items, monsterDrops)
            // also moves the unknownMovingBuffer, unknownObjectBuffer, unknownBotStepBuffer
            static_cast<CommonMap &>(map_server)=std::move(flat_map_list_temp[i]);
            // initialize MapServer-specific attributes
            memset(map_server.localChatDrop,0x00,CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE);
            map_server.localChatDropTotalCache=0;
            map_server.localChatDropNewValue=0;
            map_server.id_db=0;
            map_server.zone=255;// no zone by default
            // complete data specific to MapServer: zone
            if(CommonDatapackServerSpec::commonDatapackServerSpec.get_zoneToId().find(map_semi.old_map.zoneName)!=CommonDatapackServerSpec::commonDatapackServerSpec.get_zoneToId().cend())
                map_server.zone=CommonDatapackServerSpec::commonDatapackServerSpec.get_zoneToId().at(map_semi.old_map.zoneName);
            i++;
        }
    }
    // build reverse map: id -> path for error messages
    std::unordered_map<CATCHCHALLENGER_TYPE_MAPID, std::string> mapIdToPath;
    for(const auto &pair : mapPathToId)
        mapIdToPath[pair.second]=pair.first;
    // process buffered unknown entries on each MapServer
    {
        unsigned int i=0;
        while(i<Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.size())
        {
            Map_server_MapVisibility_Simple_StoreOnSender &map_server=Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list[i];
            const std::string &mapFile=mapIdToPath.find(i)!=mapIdToPath.cend() ? mapIdToPath.at(i) : std::string("unknown");
            for(const UnknownMovingEntry &entry : map_server.unknownMovingBuffer)
            {
                if(!map_server.parseUnknownMoving(entry.type,entry.object_x,entry.object_y,entry.property_text))
                    std::cerr << "BaseServer::preload_9_sync_the_map() unknown moving type: " << entry.type
                              << ", object_x: " << entry.object_x
                              << ", object_y: " << entry.object_y
                              << ", map index: " << i << ", file: " << mapFile << std::endl;
            }
            for(const UnknownObjectEntry &entry : map_server.unknownObjectBuffer)
            {
                if(!map_server.parseUnknownObject(entry.type,entry.object_x,entry.object_y,entry.property_text))
                    std::cerr << "BaseServer::preload_9_sync_the_map() unknown object type: " << entry.type
                              << ", object_x: " << entry.object_x
                              << ", object_y: " << entry.object_y
                              << ", map index: " << i << ", file: " << mapFile << std::endl;
            }
            for(const UnknownBotStepEntry &entry : map_server.unknownBotStepBuffer)
            {
                if(!map_server.parseUnknownBotStep(entry.object_x,entry.object_y,entry.step))
                    std::cerr << "BaseServer::preload_9_sync_the_map() unknown bot step type: " << (entry.step->Attribute("type")!=nullptr?entry.step->Attribute("type"):"(null)")
                              << ", object_x: " << entry.object_x
                              << ", object_y: " << entry.object_y
                              << ", map index: " << i << ", file: " << mapFile << std::endl;
            }
            map_server.unknownMovingBuffer.clear();
            map_server.unknownObjectBuffer.clear();
            map_server.unknownBotStepBuffer.clear();
            i++;
        }
    }

    #ifdef EPOLLCATCHCHALLENGERSERVER
    {
        // delete XML docs kept alive for BotStep buffer processing
        unsigned int index=0;
        while(index<xmlDocsToKeep.size())
        {
            delete xmlDocsToKeep.at(index);
            index++;
        }
        xmlDocsToKeep.clear();
    }
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
