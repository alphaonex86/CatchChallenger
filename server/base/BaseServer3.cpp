#include "BaseServer.hpp"
#include "GlobalServerData.hpp"
#include "DictionaryServer.hpp"
#include "Client.hpp"
#include "ClientMapManagement/Map_server_MapVisibility_Simple_StoreOnSender.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/Version.hpp"
#include "../../general/hps/hps.h"
#include <fstream>

using namespace CatchChallenger;

void BaseServer::preload_1_the_data()
{
    std::cout << "Preload data for server version " << CatchChallenger::Version::str << std::endl;

    if(dataLoaded)
        return;
    dataLoaded=true;
    preload_industries_call=false;
    GlobalServerData::serverPrivateVariables.stopIt=false;
    preload_2_sync_the_randomData();

    std::cout << "Datapack, base: " << GlobalServerData::serverSettings.datapack_basePath
              << std::endl;
    preload_3_sync_the_ddos();
    preload_4_sync_randomBlock();

    //load from cache here
    #ifdef CATCHCHALLENGER_CACHE_HPS
    if(in_file!=nullptr)
    {
        size_t lastSize=serialBuffer->tellg();
        {
            const auto &now = msFrom1970();
            *serialBuffer >> CommonDatapack::commonDatapack;
            if(CommonDatapack::commonDatapack.get_monstersCollision().empty())
            {
                std::cerr << "CommonDatapack::commonDatapack.monstersCollision.empty() (abort)" << std::endl;
                abort();
            }
            std::cout << "commonDatapack size: " << ((int32_t)serialBuffer->tellg()-(int32_t)lastSize) << "B" << std::endl;lastSize=serialBuffer->tellg();
            *serialBuffer >> CommonDatapackServerSpec::commonDatapackServerSpec;
            std::cout << "commonDatapackServerSpec size: " << ((int32_t)serialBuffer->tellg()-(int32_t)lastSize) << "B" << std::endl;lastSize=serialBuffer->tellg();
            const auto &after = msFrom1970();
            std::cout << "Loaded the common datapack into " << (after-now) << "ms" << std::endl;
        }
        timeDatapack = msFrom1970();
        const auto &now = msFrom1970();

        *serialBuffer >> GlobalServerData::serverPrivateVariables.randomData;
        std::cout << "randomData size: " << ((int32_t)serialBuffer->tellg()-(int32_t)lastSize) << "B" << std::endl;lastSize=serialBuffer->tellg();
        *serialBuffer >> GlobalServerData::serverPrivateVariables.events;
        std::cout << "events size: " << ((int32_t)serialBuffer->tellg()-(int32_t)lastSize) << "B" << std::endl;lastSize=serialBuffer->tellg();
        *serialBuffer >> CommonSettingsCommon::commonSettingsCommon.datapackHashBase;
        *serialBuffer >> CommonSettingsServer::commonSettingsServer.datapackHashServerMain;
        *serialBuffer >> CommonSettingsServer::commonSettingsServer.datapackHashServerSub;
        std::cout << "hash size: " << ((int32_t)serialBuffer->tellg()-(int32_t)lastSize) << "B" << std::endl;lastSize=serialBuffer->tellg();
        #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
        *serialBuffer >> BaseServerMasterSendDatapack::datapack_file_hash_cache_base;
        std::cout << "datapack_file_hash_cache_base size: " << ((int32_t)serialBuffer->tellg()-(int32_t)lastSize) << "B" << std::endl;lastSize=serialBuffer->tellg();
        *serialBuffer >> Client::datapack_file_hash_cache_main;
        std::cout << "datapack_file_hash_cache_main size: " << ((int32_t)serialBuffer->tellg()-(int32_t)lastSize) << "B" << std::endl;lastSize=serialBuffer->tellg();
        *serialBuffer >> Client::datapack_file_hash_cache_sub;
        std::cout << "datapack_file_hash_cache_sub size: " << ((int32_t)serialBuffer->tellg()-(int32_t)lastSize) << "B" << std::endl;lastSize=serialBuffer->tellg();
        #endif
        *serialBuffer >> GlobalServerData::serverPrivateVariables.skinList;
        std::cout << "skinList size: " << ((int32_t)serialBuffer->tellg()-(int32_t)lastSize) << "B" << std::endl;lastSize=serialBuffer->tellg();
        *serialBuffer >> GlobalServerData::serverPrivateVariables.monsterDrops;
        std::cout << "monsterDrops size: " << ((int32_t)serialBuffer->tellg()-(int32_t)lastSize) << "B" << std::endl;lastSize=serialBuffer->tellg();
        *serialBuffer >> MapServer::mapListSize;
        GlobalServerData::serverPrivateVariables.flat_map_list=static_cast<CommonMap **>(malloc(sizeof(CommonMap *)*MapServer::mapListSize));
        for(unsigned int i=0; i<MapServer::mapListSize; i++)
            GlobalServerData::serverPrivateVariables.flat_map_list[i]=new Map_server_MapVisibility_Simple_StoreOnSender;
        std::unordered_set<uint32_t> detectDuplicateMapId;
        for(unsigned int i=0; i<MapServer::mapListSize; i++)
        {
            std::string string;
            uint32_t id=0;
            //ssize_t lastPos=0;
            //lastPos=serialBuffer->tellg();
            *serialBuffer >> id;
            //std::cerr << "map id " << id << " at " << lastPos << std::endl;
            if(detectDuplicateMapId.find(id)!=detectDuplicateMapId.cend())
            {
                std::cerr << "duplicate id for map: " << id << std::endl;
                abort();
            }
            if(id>100000)
            {
                std::cerr << "map id " << id << " too big, abort to prevent problem at " << serialBuffer->tellg() << std::endl;
                abort();
            }
            //lastPos=serialBuffer->tellg();
            *serialBuffer >> string;

            MapServer * map=static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.flat_map_list[i]);
            //lastPos=serialBuffer->tellg();
            *serialBuffer >> *map;
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            //std::cerr << "map string " << string << " map " << id << " map->pointOnMap_Item " << std::to_string(map->pointOnMap_Item.size()) << std::endl;
            for (const auto& kv : map->pointOnMap_Item)
            {
                const MapServer::ItemOnMap &item=kv.second;
                //std::cerr << "Loaded map item: " << std::to_string(item.item) << " item.pointOnMapDbCode: " << std::to_string(item.pointOnMapDbCode) << " item.infinite: " << std::to_string(item.infinite) << std::endl;
                /*if(!item.infinite)
                    std::cerr << "Loaded map item: " << std::to_string(item.item) << " item.pointOnMapDbCode: " << std::to_string(item.pointOnMapDbCode) << std::endl;*/
                if(CommonDatapack::commonDatapack.get_items().item.find(item.item)==CommonDatapack::commonDatapack.get_items().item.cend())
                {
                    std::cerr << "Object " << std::to_string(item.item) << " is not found into the item list" << std::endl;
                    abort();
                }
            }
            #endif
            GlobalServerData::serverPrivateVariables.id_map_to_map[id]=string;
            GlobalServerData::serverPrivateVariables.map_list[string]=map;
            //std::cerr << "map end at " << serialBuffer->tellg() << std::endl;
        }
        std::cout << "map size: " << ((int32_t)serialBuffer->tellg()-(int32_t)lastSize) << "B" << std::endl;lastSize=serialBuffer->tellg();

        DictionaryServer::dictionary_pointOnMap_item_database_to_internal.clear();
        uint32_t uint32size=0;
        *serialBuffer >> uint32size;
        for(uint32_t i=0; i<uint32size; i++)
        {
            DictionaryServer::MapAndPointItem v;
            *serialBuffer >> v.datapack_index_item;
            int32_t pos=0;
            *serialBuffer >> pos;
            v.map=static_cast<MapServer *>(MapServer::posToPointer(pos));
            *serialBuffer >> v.x;
            *serialBuffer >> v.y;
            DictionaryServer::dictionary_pointOnMap_item_database_to_internal.push_back(v);
        }
        DictionaryServer::dictionary_pointOnMap_plant_database_to_internal.clear();
        *serialBuffer >> uint32size;
        for(uint32_t i=0; i<uint32size; i++)
        {
            DictionaryServer::MapAndPointPlant v;
            *serialBuffer >> v.datapack_index_plant;
            int32_t pos=0;
            *serialBuffer >> pos;
            v.map=static_cast<MapServer *>(MapServer::posToPointer(pos));
            *serialBuffer >> v.x;
            *serialBuffer >> v.y;
            DictionaryServer::dictionary_pointOnMap_plant_database_to_internal.push_back(v);
        }

        /*std::cout << __FILE__ << ":" << __LINE__ << " DictionaryServer::dictionary_pointOnMap_item_database_to_internal: " << DictionaryServer::dictionary_pointOnMap_item_database_to_internal.size() << std::endl;
        for(unsigned int i=0; i<DictionaryServer::dictionary_pointOnMap_item_database_to_internal.size(); i++)
        {
            const DictionaryServer::MapAndPointItem &t=DictionaryServer::dictionary_pointOnMap_item_database_to_internal.at(i);
            std::cerr << "DictionaryServer::MapAndPointItem &t " << &t << std::endl;
            if(t.map!=nullptr)
                std::cerr << t.datapack_index_item << " " << t.map->id << " " << std::to_string(t.x) << " " << std::to_string(t.y) << " " << std::endl;
        }*/


        Map_server_MapVisibility_Simple_StoreOnSender::map_to_update=
                static_cast<Map_server_MapVisibility_Simple_StoreOnSender **>(malloc(sizeof(CommonMap *)*GlobalServerData::serverPrivateVariables.map_list.size()));
        memset(Map_server_MapVisibility_Simple_StoreOnSender::map_to_update,0x00,sizeof(CommonMap *)*GlobalServerData::serverPrivateVariables.map_list.size());
        Map_server_MapVisibility_Simple_StoreOnSender::map_to_update_size=0;

        const auto &after = msFrom1970();
        std::cout << "Loaded map and other " << (after-now) << "ms" << std::endl;

        //baseServerMasterSendDatapack.load(GlobalServerData::serverSettings.datapack_basePath);
        in_file=nullptr;
    }
    else
    #endif
    {
        {
            const auto &now = msFrom1970();
            CommonDatapack::commonDatapack.parseDatapack(GlobalServerData::serverSettings.datapack_basePath);
            CommonDatapackServerSpec::commonDatapackServerSpec.parseDatapack(GlobalServerData::serverSettings.datapack_basePath,CommonSettingsServer::commonSettingsServer.mainDatapackCode,CommonSettingsServer::commonSettingsServer.subDatapackCode);
            const auto &after = msFrom1970();
            std::cout << "Loaded the common datapack into " << (after-now) << "ms" << std::endl;
        }
        timeDatapack = msFrom1970();
        const auto &now = msFrom1970();
        preload_5_sync_the_events();
        preload_6_sync_the_datapack();
        preload_7_sync_the_skin();
        preload_8_sync_monsters_drops();
        preload_9_sync_the_map();
        const auto &after = msFrom1970();
        std::cout << "Loaded map and other " << (after-now) << "ms" << std::endl;
        baseServerMasterSendDatapack.load(GlobalServerData::serverSettings.datapack_basePath);//skinList
        #ifdef CATCHCHALLENGER_CACHE_HPS
        if(out_file!=nullptr)
        {
            size_t lastSize=0;
            hps::to_stream(CommonDatapack::commonDatapack, *out_file);
            std::cout << "commonDatapack size: " << ((uint32_t)out_file->tellp()-(uint32_t)lastSize) << "B" << std::endl;lastSize=out_file->tellp();
            hps::to_stream(CommonDatapackServerSpec::commonDatapackServerSpec, *out_file);
            std::cout << "commonDatapackServerSpec size: " << ((uint32_t)out_file->tellp()-(uint32_t)lastSize) << "B" << std::endl;lastSize=out_file->tellp();
            hps::to_stream(GlobalServerData::serverPrivateVariables.randomData, *out_file);
            std::cout << "randomData size: " << ((uint32_t)out_file->tellp()-(uint32_t)lastSize) << "B" << std::endl;lastSize=out_file->tellp();
            hps::to_stream(GlobalServerData::serverPrivateVariables.events, *out_file);
            std::cout << "events size: " << ((uint32_t)out_file->tellp()-(uint32_t)lastSize) << "B" << std::endl;lastSize=out_file->tellp();
            hps::to_stream(CommonSettingsCommon::commonSettingsCommon.datapackHashBase, *out_file);
            hps::to_stream(CommonSettingsServer::commonSettingsServer.datapackHashServerMain, *out_file);
            hps::to_stream(CommonSettingsServer::commonSettingsServer.datapackHashServerSub, *out_file);
            std::cout << "hash size: " << ((uint32_t)out_file->tellp()-(uint32_t)lastSize) << "B" << std::endl;lastSize=out_file->tellp();
            #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
            hps::to_stream(BaseServerMasterSendDatapack::datapack_file_hash_cache_base, *out_file);
            std::cout << "datapack_file_hash_cache_base size: " << ((uint32_t)out_file->tellp()-(uint32_t)lastSize) << "B" << std::endl;lastSize=out_file->tellp();
            hps::to_stream(Client::datapack_file_hash_cache_main, *out_file);
            std::cout << "datapack_file_hash_cache_main size: " << ((uint32_t)out_file->tellp()-(uint32_t)lastSize) << "B" << std::endl;lastSize=out_file->tellp();
            hps::to_stream(Client::datapack_file_hash_cache_sub, *out_file);
            std::cout << "datapack_file_hash_cache_sub size: " << ((uint32_t)out_file->tellp()-(uint32_t)lastSize) << "B" << std::endl;lastSize=out_file->tellp();
            #endif
            hps::to_stream(GlobalServerData::serverPrivateVariables.skinList, *out_file);
            std::cout << "other data size: " << ((uint32_t)out_file->tellp()-(uint32_t)lastSize) << "B" << std::endl;lastSize=out_file->tellp();
            hps::to_stream(GlobalServerData::serverPrivateVariables.monsterDrops, *out_file);
            std::cout << "monsterDrops size: " << ((uint32_t)out_file->tellp()-(uint32_t)lastSize) << "B" << std::endl;lastSize=out_file->tellp();
        }
        #endif
    }

    preload_10_sync_the_gift();
    preload_11_sync_the_players();
    preload_12_async_dictionary_map();

    /*
     * Load order:
    preload_pointOnMap_sql();
    preload_map_semi_after_db_id();
    preload_zone_sql();

    if(GlobalServerData::serverSettings.automatic_account_creation)
        load_account_max_id();
    else if(CommonSettingsCommon::commonSettingsCommon.max_character)
        load_character_max_id();
    else
        BaseServerMasterLoadDictionary::load(&GlobalServerData::serverPrivateVariables.db);

    load_sql_monsters_max_id();
    load_clan_max_id();
    preload_industries();
    */
}
