#include "BaseServer.hpp"
#include "../GlobalServerData.hpp"
#include "../DictionaryServer.hpp"

#include <chrono>
#include <string.h>
#ifdef CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK
#include <sys/stat.h>
#include <fstream>
#include "../../general/hps/hps.h"
#endif

#include "../MapManagement/MapVisibilityAlgorithm.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/CommonMap/CommonMap.hpp"
#include "../../general/base/Map_loader.hpp"

using namespace CatchChallenger;

bool BaseServer::preload_9_sync_the_map()
{
#ifndef CATCHCHALLENGER_NOXML
    std::vector<Map_semi> semi_loaded_map;

    if(CommonDatapackServerSpec::commonDatapackServerSpec.get_zoneToId_size() == 0)
    {
        std::cerr << "CommonDatapackServerSpec::commonDatapackServerSpec.get_zoneToId_size() == 0" << std::endl;
        abort();
    }

    GlobalServerData::serverPrivateVariables.datapack_mapPath=GlobalServerData::serverSettings.datapack_basePath+
            DATAPACK_BASE_PATH_MAPMAIN+
            CommonSettingsServer::commonSettingsServer.mainDatapackCode+
            "/";
    #ifdef DEBUG_MESSAGE_MAP_LOAD
    std::cout << "start preload the map, into: " << GlobalServerData::serverPrivateVariables.datapack_mapPath << std::endl;
    #endif
    MapVisibilityAlgorithm::flat_map_list.clear();
    mapPathToId.clear();
    #ifdef CATCHCHALLENGER_SERVER
    std::vector<tinyxml2::XMLDocument*> xmlDocsToKeep;
    #endif
    std::vector<MapLoadBuffers> mapLoadBuffers;
    {
        std::vector<CommonMap> flat_map_list_temp;
        #ifdef CATCHCHALLENGER_SERVER
        Map_loader::loadAllMapsAndLink(flat_map_list_temp,GlobalServerData::serverPrivateVariables.datapack_mapPath,semi_loaded_map,mapPathToId,&xmlDocsToKeep,&mapLoadBuffers);
        #else
        Map_loader::loadAllMapsAndLink(flat_map_list_temp,GlobalServerData::serverPrivateVariables.datapack_mapPath,semi_loaded_map,mapPathToId,nullptr,&mapLoadBuffers);
        #endif
        #ifdef CATCHCHALLENGER_HARDENED
        if(flat_map_list_temp.size()!=semi_loaded_map.size())
        {
            std::cerr << "flat_map_list_temp.size()!=semi_loaded_map.size()" << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
        }
        #endif
        // create new MapServer vector same size as received CommonMap vector
        MapVisibilityAlgorithm::flat_map_list.resize(flat_map_list_temp.size());
        unsigned int i=0;
        while(i<flat_map_list_temp.size())
        {
            MapVisibilityAlgorithm &map_server=MapVisibilityAlgorithm::flat_map_list[i];
            const Map_semi &map_semi=semi_loaded_map.at(i);
            // copy CommonMap data to MapServer (border, id, flat_simplified_map, teleporters, width, height, zones, industries, botFights, shops, botsFightTrigger, items, monsterDrops)
            static_cast<CommonMap &>(map_server)=std::move(flat_map_list_temp[i]);
            // initialize MapServer-specific attributes
            memset(map_server.localChatDrop,0x00,CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE);
            map_server.localChatDropTotalCache=0;
            map_server.localChatDropNewValue=0;
            map_server.id_db=0;
            map_server.zone=255;// no zone by default
            // complete data specific to MapServer: zone
            if(CommonDatapackServerSpec::commonDatapackServerSpec.has_zoneToId(map_semi.old_map.zoneName))
                map_server.zone=CommonDatapackServerSpec::commonDatapackServerSpec.get_zoneToId(map_semi.old_map.zoneName);
            i++;
        }
    }
    // build reverse map: id -> path for error messages
    std::unordered_map<CATCHCHALLENGER_TYPE_MAPID, std::string> mapIdToPath;
    for(const std::pair<const std::string, CATCHCHALLENGER_TYPE_MAPID> &pair : mapPathToId)
        mapIdToPath[pair.second]=pair.first;
    // process buffered unknown entries on each MapServer
    {
        unsigned int i=0;
        while(i<MapVisibilityAlgorithm::flat_map_list.size())
        {
            MapVisibilityAlgorithm &map_server=MapVisibilityAlgorithm::flat_map_list[i];
            const std::string &mapFile=mapIdToPath.find(i)!=mapIdToPath.cend() ? mapIdToPath.at(i) : std::string("unknown");
            if(i<mapLoadBuffers.size())
            {
                const MapLoadBuffers &buffers=mapLoadBuffers[i];
                for(const UnknownMovingEntry &entry : buffers.unknownMovingBuffer)
                {
                    if(!map_server.parseUnknownMoving(entry.type,entry.object_x,entry.object_y,std::unordered_map<std::string,std::string>(entry.property_text.begin(),entry.property_text.end())))
                        std::cerr << "BaseServer::preload_9_sync_the_map() unknown moving type: " << entry.type
                                  << ", object_x: " << entry.object_x
                                  << ", object_y: " << entry.object_y
                                  << ", map index: " << i << ", file: " << mapFile << std::endl;
                }
                for(const UnknownObjectEntry &entry : buffers.unknownObjectBuffer)
                {
                    if(!map_server.parseUnknownObject(entry.type,entry.object_x,entry.object_y,std::unordered_map<std::string,std::string>(entry.property_text.begin(),entry.property_text.end())))
                        std::cerr << "BaseServer::preload_9_sync_the_map() unknown object type: " << entry.type
                                  << ", object_x: " << entry.object_x
                                  << ", object_y: " << entry.object_y
                                  << ", map index: " << i << ", file: " << mapFile << std::endl;
                }
                for(const UnknownBotStepEntry &entry : buffers.unknownBotStepBuffer)
                {
                    if(!map_server.parseUnknownBotStep(entry.object_x,entry.object_y,entry.step))
                        std::cerr << "BaseServer::preload_9_sync_the_map() unknown bot step type: " << (entry.step->Attribute("type")!=nullptr?entry.step->Attribute("type"):"(null)")
                                  << ", object_x: " << entry.object_x
                                  << ", object_y: " << entry.object_y
                                  << ", map index: " << i << ", file: " << mapFile << std::endl;
                }
            }
            i++;
        }
    }
    mapLoadBuffers.clear();

    #ifdef CATCHCHALLENGER_SERVER
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

    #ifdef CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK
    {
        unsigned int i=0;
        while(i<MapVisibilityAlgorithm::flat_map_list.size())
        {
            const std::string &mapPath=mapIdToPath.find(i)!=mapIdToPath.cend() ? mapIdToPath.at(i) : std::string("unknown_")+std::to_string(i);
            const MapVisibilityAlgorithm &map_server=MapVisibilityAlgorithm::flat_map_list[i];
            const std::string dumpDir=std::string("dump/")+mapPath;
            //mkpath
            {
                std::string acc;
                size_t pos=0;
                while((pos=dumpDir.find('/',pos))!=std::string::npos)
                {
                    acc=dumpDir.substr(0,pos);
                    ::mkdir(acc.c_str(),0755);
                    pos++;
                }
                ::mkdir(dumpDir.c_str(),0755);
            }
            //full.dump via hps
            {
                std::ofstream f(dumpDir+"/full.dump",std::ios::binary);
                if(f.is_open())
                {
                    hps::to_stream(static_cast<const MapServer &>(map_server),f);
                    f.close();
                }
                else
                    std::cerr << "CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK: can't open " << dumpDir << "/full.dump" << std::endl;
            }
            //scalars.dump via hps (id_db, border, width, height)
            {
                std::ofstream f(dumpDir+"/scalars.dump",std::ios::binary);
                if(f.is_open())
                {
                    hps::StreamOutputBuffer buf(f);
                    buf << map_server.id_db;
                    buf << map_server.border.bottom.x_offset << map_server.border.bottom.mapIndex;
                    buf << map_server.border.left.y_offset << map_server.border.left.mapIndex;
                    buf << map_server.border.right.y_offset << map_server.border.right.mapIndex;
                    buf << map_server.border.top.x_offset << map_server.border.top.mapIndex;
                    buf << map_server.width;
                    buf << map_server.height;
                    buf.flush();
                    f.close();
                }
                else
                    std::cerr << "CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK: can't open " << dumpDir << "/scalars.dump" << std::endl;
            }
            //flat_simplified_map.dump via hps
            {
                std::ofstream f(dumpDir+"/flat_simplified_map.dump",std::ios::binary);
                if(f.is_open())
                {
                    hps::to_stream(map_server.flat_simplified_map,f);
                    f.close();
                }
                else
                    std::cerr << "CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK: can't open " << dumpDir << "/flat_simplified_map.dump" << std::endl;
            }
            //teleporters.dump via hps
            {
                std::ofstream f(dumpDir+"/teleporters.dump",std::ios::binary);
                if(f.is_open())
                {
                    hps::to_stream(map_server.teleporters,f);
                    f.close();
                }
                else
                    std::cerr << "CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK: can't open " << dumpDir << "/teleporters.dump" << std::endl;
            }
            //zones.dump via hps
            {
                std::ofstream f(dumpDir+"/zones.dump",std::ios::binary);
                if(f.is_open())
                {
                    hps::to_stream(map_server.zones,f);
                    f.close();
                }
                else
                    std::cerr << "CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK: can't open " << dumpDir << "/zones.dump" << std::endl;
            }
            //industries.dump via hps
            {
                std::ofstream f(dumpDir+"/industries.dump",std::ios::binary);
                if(f.is_open())
                {
                    hps::to_stream(map_server.industries,f);
                    f.close();
                }
                else
                    std::cerr << "CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK: can't open " << dumpDir << "/industries.dump" << std::endl;
            }
            //industries_pos.dump via hps
            {
                std::ofstream f(dumpDir+"/industries_pos.dump",std::ios::binary);
                if(f.is_open())
                {
                    hps::to_stream(map_server.industries_pos,f);
                    f.close();
                }
                else
                    std::cerr << "CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK: can't open " << dumpDir << "/industries_pos.dump" << std::endl;
            }
            //botFights.dump via hps
            {
                std::ofstream f(dumpDir+"/botFights.dump",std::ios::binary);
                if(f.is_open())
                {
                    hps::to_stream(map_server.botFights,f);
                    f.close();
                }
                else
                    std::cerr << "CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK: can't open " << dumpDir << "/botFights.dump" << std::endl;
            }
            //monsterDrops.dump via hps
            {
                std::ofstream f(dumpDir+"/monsterDrops.dump",std::ios::binary);
                if(f.is_open())
                {
                    hps::to_stream(map_server.monsterDrops,f);
                    f.close();
                }
                else
                    std::cerr << "CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK: can't open " << dumpDir << "/monsterDrops.dump" << std::endl;
            }
            //shops.dump via hps (pair-keyed, custom serialization)
            {
                std::ofstream f(dumpDir+"/shops.dump",std::ios::binary);
                if(f.is_open())
                {
                    hps::StreamOutputBuffer buf(f);
                    buf << (uint8_t)map_server.shops.size();
                    for(const std::pair<const std::pair<uint8_t,uint8_t>,Shop>& n : map_server.shops)
                    {
                        buf << n.first.first;
                        buf << n.first.second;
                        buf << n.second;
                    }
                    buf.flush();
                    f.close();
                }
                else
                    std::cerr << "CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK: can't open " << dumpDir << "/shops.dump" << std::endl;
            }
            //botsFightTrigger.dump via hps (pair-keyed, custom serialization)
            {
                std::ofstream f(dumpDir+"/botsFightTrigger.dump",std::ios::binary);
                if(f.is_open())
                {
                    hps::StreamOutputBuffer buf(f);
                    buf << (uint8_t)map_server.botsFightTrigger.size();
                    for(const std::pair<const std::pair<uint8_t,uint8_t>,uint8_t>& n : map_server.botsFightTrigger)
                    {
                        buf << n.first.first;
                        buf << n.first.second;
                        buf << n.second;
                    }
                    buf.flush();
                    f.close();
                }
                else
                    std::cerr << "CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK: can't open " << dumpDir << "/botsFightTrigger.dump" << std::endl;
            }
            //items.dump via hps (pair-keyed, custom serialization)
            {
                std::ofstream f(dumpDir+"/items.dump",std::ios::binary);
                if(f.is_open())
                {
                    hps::StreamOutputBuffer buf(f);
                    buf << (uint8_t)map_server.items.size();
                    for(const std::pair<const std::pair<uint8_t,uint8_t>,ItemOnMap>& n : map_server.items)
                    {
                        buf << n.first.first;
                        buf << n.first.second;
                        buf << n.second;
                    }
                    buf.flush();
                    f.close();
                }
                else
                    std::cerr << "CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK: can't open " << dumpDir << "/items.dump" << std::endl;
            }
            //rescue.dump via hps (pair-keyed, custom serialization)
            {
                std::ofstream f(dumpDir+"/rescue.dump",std::ios::binary);
                if(f.is_open())
                {
                    hps::StreamOutputBuffer buf(f);
                    buf << (uint8_t)map_server.rescue.size();
                    for(const std::pair<const std::pair<uint8_t,uint8_t>,Orientation>& n : map_server.rescue)
                    {
                        buf << n.first.first;
                        buf << n.first.second;
                        buf << (uint8_t)n.second;
                    }
                    buf.flush();
                    f.close();
                }
                else
                    std::cerr << "CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK: can't open " << dumpDir << "/rescue.dump" << std::endl;
            }
            //heal.dump via hps (pair-keyed, custom serialization)
            {
                std::ofstream f(dumpDir+"/heal.dump",std::ios::binary);
                if(f.is_open())
                {
                    hps::StreamOutputBuffer buf(f);
                    buf << (uint8_t)map_server.heal.size();
                    for(const std::pair<uint8_t,uint8_t>& n : map_server.heal)
                    {
                        buf << n.first;
                        buf << n.second;
                    }
                    buf.flush();
                    f.close();
                }
                else
                    std::cerr << "CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK: can't open " << dumpDir << "/heal.dump" << std::endl;
            }
            //zoneCapture.dump via hps (pair-keyed, custom serialization)
            {
                std::ofstream f(dumpDir+"/zoneCapture.dump",std::ios::binary);
                if(f.is_open())
                {
                    hps::StreamOutputBuffer buf(f);
                    buf << (uint8_t)map_server.zoneCapture.size();
                    for(const std::pair<uint8_t,uint8_t>& n : map_server.zoneCapture)
                    {
                        buf << n.first;
                        buf << n.second;
                    }
                    buf.flush();
                    f.close();
                }
                else
                    std::cerr << "CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK: can't open " << dumpDir << "/zoneCapture.dump" << std::endl;
            }
            i++;
        }
        std::cout << "CATCHCHALLENGER_DUMP_DATATREE_CACHE_DATAPACK: dumped " << MapVisibilityAlgorithm::flat_map_list.size() << " maps into dump/" << std::endl;
    }
    #endif

#else
    std::cerr << "preload_9_sync_the_map() called with CATCHCHALLENGER_NOXML defined (abort)" << std::endl;
    abort();
#endif
    return true;
}
