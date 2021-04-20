#include "BaseServer.hpp"
#include "GlobalServerData.hpp"
#include "DictionaryLogin.hpp"
#include "ClientMapManagement/Map_server_MapVisibility_Simple_StoreOnSender.hpp"
#include "ClientMapManagement/Map_server_MapVisibility_WithBorder_StoreOnSender.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/FacilityLib.hpp"
#include "../../general/base/Version.hpp"
#include "../../general/hps/hps.h"
#include <fstream>

using namespace CatchChallenger;

void BaseServer::preload_the_data()
{
    std::cout << "Preload data for server version " << CatchChallenger::Version::str << std::endl;

    if(dataLoaded)
        return;
    dataLoaded=true;
    preload_market_monsters_prices_call=false;
    preload_industries_call=false;
    preload_market_items_call=false;
    GlobalServerData::serverPrivateVariables.stopIt=false;
    preload_the_randomData();

    std::cout << "Datapack, base: " << GlobalServerData::serverSettings.datapack_basePath
              << std::endl;
    preload_the_ddos();
    preload_randomBlock();

    //load from cache here
    #ifdef CATCHCHALLENGER_CACHE_HPS
    if(in_file!=nullptr)
    {
        size_t lastSize=serialBuffer->tellg();
        {
            const auto &now = msFrom1970();
            *serialBuffer >> CommonDatapack::commonDatapack;
            if(CommonDatapack::commonDatapack.monstersCollision.empty())
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
        {
            switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
            {
                case MapVisibilityAlgorithmSelection_Simple:
                    GlobalServerData::serverPrivateVariables.flat_map_list[i]=new Map_server_MapVisibility_Simple_StoreOnSender;
                break;
                case MapVisibilityAlgorithmSelection_WithBorder:
                    GlobalServerData::serverPrivateVariables.flat_map_list[i]=new Map_server_MapVisibility_WithBorder_StoreOnSender;
                break;
                case MapVisibilityAlgorithmSelection_None:
                default:
                    GlobalServerData::serverPrivateVariables.flat_map_list[i]=new MapServer;
                break;
            }
        }
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
            //std::cerr << "map string " << string << " at " << lastPos << std::endl;
            MapServer * map=static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.flat_map_list[i]);
            //lastPos=serialBuffer->tellg();
            *serialBuffer >> *map;
            //std::cerr << "map " << id << " at " << lastPos << std::endl;
            GlobalServerData::serverPrivateVariables.id_map_to_map[id]=string;
            GlobalServerData::serverPrivateVariables.map_list[string]=map;
            //std::cerr << "map end at " << serialBuffer->tellg() << std::endl;
        }
        std::cout << "map size: " << ((int32_t)serialBuffer->tellg()-(int32_t)lastSize) << "B" << std::endl;lastSize=serialBuffer->tellg();

        if(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm==CatchChallenger::MapVisibilityAlgorithmSelection_Simple)
        {
            Map_server_MapVisibility_Simple_StoreOnSender::map_to_update=
                    static_cast<Map_server_MapVisibility_Simple_StoreOnSender **>(malloc(sizeof(CommonMap *)*GlobalServerData::serverPrivateVariables.map_list.size()));
            memset(Map_server_MapVisibility_Simple_StoreOnSender::map_to_update,0x00,sizeof(CommonMap *)*GlobalServerData::serverPrivateVariables.map_list.size());
            Map_server_MapVisibility_Simple_StoreOnSender::map_to_update_size=0;
        }

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
        preload_the_events();
        preload_the_datapack();
        preload_the_skin();
        preload_monsters_drops();
        preload_the_map();
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
            uint32_t mapListSize=GlobalServerData::serverPrivateVariables.map_list.size();
            hps::to_stream(mapListSize, *out_file);

            std::unordered_map<const CommonMap *,std::string> map_list_reverse;
            for (const auto x : GlobalServerData::serverPrivateVariables.map_list)
                  map_list_reverse[x.second]=x.first;
            std::unordered_map<std::string,uint32_t> id_map_to_map_reverse;
            for (const auto x : GlobalServerData::serverPrivateVariables.id_map_to_map)
                  id_map_to_map_reverse[x.second]=x.first;

            uint32_t idSize=0;
            uint32_t pathSize=0;
            uint32_t mapSize=0;
            lastSize=out_file->tellp();
            for(unsigned int i=0; i<mapListSize; i++)
            {
                const MapServer * const map=static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.flat_map_list[i]);
                const std::string &string=map_list_reverse.at(static_cast<const CommonMap *>(map));
                const uint32_t &id=id_map_to_map_reverse.at(string);

                //std::cerr << "map id " << id << " at " << out_file->tellp() << std::endl;

                hps::to_stream(id, *out_file);
                idSize+=((uint32_t)out_file->tellp()-(uint32_t)lastSize);lastSize=out_file->tellp();

                //std::cerr << "map string " << string << " at " << out_file->tellp() << std::endl;

                hps::to_stream(string, *out_file);
                pathSize+=((uint32_t)out_file->tellp()-(uint32_t)lastSize);lastSize=out_file->tellp();

                //std::cerr << "map at " << out_file->tellp() << std::endl;

                hps::to_stream(*map, *out_file);
                mapSize+=((uint32_t)out_file->tellp()-(uint32_t)lastSize);lastSize=out_file->tellp();

                //std::cerr << "map end at " << out_file->tellp() << std::endl;
            }
            std::cout << "map id size: " << idSize << "B" << std::endl;
            std::cout << "map pathSize size: " << pathSize << "B" << std::endl;
            std::cout << "map size: " << mapSize << "B" << std::endl;
        }
        #endif
    }

    preload_the_gift();
    preload_the_players();
    preload_dictionary_map();

    /*
     * Load order:
    preload_pointOnMap_sql();
    preload_map_semi_after_db_id();
    preload_zone_sql();

    preload_market_monsters_sql();
    preload_market_items();

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
