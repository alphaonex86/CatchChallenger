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

        {
            *serialBuffer >> CommonMap::flat_map_list_size;
            *serialBuffer >> CommonMap::flat_map_object_size;//store in full length to easy multiply by index (16Bits) and have full size pointer
            const size_t s=sizeof(Map_server_MapVisibility_Simple_StoreOnSender)*CommonMap::flat_map_list_size;
            CommonMap::flat_map_list=static_cast<Map_server_MapVisibility_Simple_StoreOnSender *>(malloc(s));
            CommonMap::flat_map_object_size=sizeof(Map_server_MapVisibility_Simple_StoreOnSender);
            #ifdef CATCHCHALLENGER_HARDENED
            memset((void *)GlobalServerData::serverPrivateVariables.flat_map_list,0,tempMemSize);//security but performance problem
            for(CATCHCHALLENGER_TYPE_MAPID i=0; i<GlobalServerData::serverPrivateVariables.flat_map_size; i++)
                new(GlobalServerData::serverPrivateVariables.flat_map_list+i) Map_server_MapVisibility_Simple_StoreOnSender();
            #endif
            serialBuffer->read((char *)CommonMap::flat_teleporter,s);
        }

        {
            *serialBuffer >> CommonMap::flat_teleporter_list_size;//temp, used as size when finish
            const size_t s=sizeof(CommonMap::Teleporter)*CommonMap::flat_teleporter_list_size;
            CommonMap::flat_teleporter=static_cast<CommonMap::Teleporter *>(malloc(s));
            #ifdef CATCHCHALLENGER_HARDENED
            memset((void *)CommonMap::flat_teleporter,0,s);//security but performance problem
            #endif
            serialBuffer->read((char *)CommonMap::flat_teleporter,s);
        }

        {
            *serialBuffer >> CommonMap::flat_simplified_map_list_size;//temp, used as size when finish
            CommonMap::flat_simplified_map=static_cast<uint8_t *>(malloc(CommonMap::flat_simplified_map_list_size));
            #ifdef CATCHCHALLENGER_HARDENED
            memset((void *)CommonMap::flat_simplified_map,0,CommonMap::flat_simplified_map_list_size);//security but performance problem
            #endif
            serialBuffer->read((char *)CommonMap::flat_simplified_map,CommonMap::flat_simplified_map_list_size);
        }

        std::cout << "map size: " << ((int32_t)serialBuffer->tellg()-(int32_t)lastSize) << "B" << std::endl;lastSize=serialBuffer->tellg();

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
        mapPathToId.clear();
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

            {
                hps::to_stream(CommonMap::flat_map_list_size, *out_file);
                hps::to_stream(CommonMap::flat_map_object_size, *out_file);
                const size_t s=sizeof(Map_server_MapVisibility_Simple_StoreOnSender)*CommonMap::flat_map_list_size;
                out_file->write((char *)CommonMap::flat_map_list,s);
            }

            {
                hps::to_stream(CommonMap::flat_teleporter_list_size, *out_file);
                const size_t s=sizeof(CommonMap::Teleporter)*CommonMap::flat_teleporter_list_size;
                out_file->write((char *)CommonMap::flat_teleporter,s);
            }

            {
                hps::to_stream(CommonMap::flat_simplified_map_list_size, *out_file);
                out_file->write((char *)CommonMap::flat_simplified_map,CommonMap::flat_simplified_map_list_size);
            }
            std::cout << "map size: " << ((uint32_t)out_file->tellp()-(uint32_t)lastSize) << "B" << std::endl;lastSize=out_file->tellp();
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
