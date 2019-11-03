#include "BaseServer.h"
#include "GlobalServerData.h"
#include "DictionaryLogin.h"
#include "ClientMapManagement/Map_server_MapVisibility_Simple_StoreOnSender.h"
#include "ClientMapManagement/Map_server_MapVisibility_WithBorder_StoreOnSender.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/Version.h"
#include "../../general/hps/hps.h"
#include <fstream>

using namespace CatchChallenger;

BaseServer::BaseServer() :
    stat(Down),
    dataLoaded(false)
{
    ProtocolParsing::initialiseTheVariable();

    dictionary_pointOnMap_maxId_item=0;
    dictionary_pointOnMap_maxId_plant=0;
    ProtocolParsing::compressionTypeServer                                = ProtocolParsing::CompressionType::Zstandard;

    GlobalServerData::serverPrivateVariables.connected_players      = 0;
    GlobalServerData::serverPrivateVariables.number_of_bots_logged  = 0;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    GlobalServerData::serverPrivateVariables.timer_city_capture     = NULL;
    #endif

    GlobalServerData::serverPrivateVariables.botSpawnIndex          = 0;
    GlobalServerData::serverPrivateVariables.datapack_rightFileName	= std::regex(DATAPACK_FILE_REGEX,std::regex_constants::optimize);
    GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove=NULL;

    GlobalServerData::serverSettings.automatic_account_creation             = false;
    GlobalServerData::serverSettings.max_players                            = 1;
    GlobalServerData::serverSettings.sendPlayerNumber                       = true;
    GlobalServerData::serverSettings.pvp                                    = true;

    GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm       = CatchChallenger::MapVisibilityAlgorithmSelection_None;
    GlobalServerData::serverSettings.datapackCache                              = -1;
    if(FacilityLibGeneral::applicationDirPath.empty())
    {
        std::cerr << "FacilityLibGeneral::applicationDirPath is empty" << std::endl;
        abort();
    }
    GlobalServerData::serverSettings.datapack_basePath                          = FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/datapack/";
    GlobalServerData::serverSettings.compressionType                            = CompressionType_Zstandard;
    GlobalServerData::serverSettings.dontSendPlayerType                         = false;
    CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange = true;
    CommonSettingsServer::commonSettingsServer.forcedSpeed            = CATCHCHALLENGER_SERVER_NORMAL_SPEED;
    CommonSettingsServer::commonSettingsServer.useSP                  = true;
    CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters            = 8;
    CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters   = 30;
    CommonSettingsCommon::commonSettingsCommon.maxPlayerItems               = 30;
    CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems      = 150;
    GlobalServerData::serverPrivateVariables.server_blobversion_datapack=0;
    GlobalServerData::serverSettings.server_blobversion_datapack=0;
    GlobalServerData::serverPrivateVariables.common_blobversion_datapack=0;
    GlobalServerData::serverSettings.common_blobversion_datapack=0;
    GlobalServerData::serverSettings.anonymous            = false;
    GlobalServerData::serverSettings.everyBodyIsRoot=0;
    GlobalServerData::serverSettings.teleportIfMapNotFoundOrOutOfMap=0;
    CommonSettingsServer::commonSettingsServer.autoLearn              = false;//need useSP to false
    CommonSettingsServer::commonSettingsServer.dontSendPseudo         = false;
    CommonSettingsServer::commonSettingsServer.chat_allow_clan        = true;
    CommonSettingsServer::commonSettingsServer.chat_allow_local       = true;
    CommonSettingsServer::commonSettingsServer.chat_allow_all         = true;
    CommonSettingsServer::commonSettingsServer.chat_allow_private     = true;
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    CommonSettingsCommon::commonSettingsCommon.max_character          = 3;
    CommonSettingsCommon::commonSettingsCommon.min_character          = 0;
    CommonSettingsCommon::commonSettingsCommon.max_pseudo_size        = 20;
    CommonSettingsCommon::commonSettingsCommon.character_delete_time  = 604800; // 7 day
    #endif
    CommonSettingsServer::commonSettingsServer.rates_gold             = 1000;//this is value *1000, then 1.0
    CommonSettingsServer::commonSettingsServer.rates_drop             = 1000;//this is value *1000, then 1.0
    CommonSettingsServer::commonSettingsServer.rates_xp               = 1000;//this is value *1000, then 1.0
    CommonSettingsServer::commonSettingsServer.rates_xp_pow           = 1000;//this is value *1000, then 1.0
    CommonSettingsServer::commonSettingsServer.factoryPriceChange     = 20;
    CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick=30;
    CommonSettingsServer::commonSettingsServer.everyBodyIsRoot           = false;//this is value *1000, then 1.0
    GlobalServerData::serverSettings.fightSync                         = GameServerSettings::FightSync_AtTheEndOfBattle;
    GlobalServerData::serverSettings.positionTeleportSync              = true;
    GlobalServerData::serverSettings.secondToPositionSync              = 0;
    GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm       = MapVisibilityAlgorithmSelection_Simple;
    GlobalServerData::serverSettings.mapVisibility.simple.max                   = 30;
    GlobalServerData::serverSettings.mapVisibility.simple.reshow                = 20;
    GlobalServerData::serverSettings.mapVisibility.simple.reemit                = true;
    GlobalServerData::serverSettings.mapVisibility.withBorder.maxWithBorder     = 20;
    GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder  = 10;
    GlobalServerData::serverSettings.mapVisibility.withBorder.max               = 30;
    GlobalServerData::serverSettings.mapVisibility.withBorder.reshow            = 20;
    #ifdef CATCHCHALLENGER_DDOS_FILTER
    GlobalServerData::serverSettings.ddos.kickLimitMove                         = 60;
    GlobalServerData::serverSettings.ddos.kickLimitChat                         = 5;
    GlobalServerData::serverSettings.ddos.kickLimitOther                        = 30;
    #endif
    GlobalServerData::serverSettings.ddos.computeAverageValueTimeInterval       = 5;
    GlobalServerData::serverSettings.ddos.dropGlobalChatMessageGeneral          = 20;
    GlobalServerData::serverSettings.ddos.dropGlobalChatMessageLocalClan        = 20;
    GlobalServerData::serverSettings.ddos.dropGlobalChatMessagePrivate          = 20;
    GlobalServerData::serverSettings.city.capture.frenquency                    = City::Capture::Frequency_week;
    GlobalServerData::serverSettings.city.capture.day                           = City::Capture::Monday;
    GlobalServerData::serverSettings.city.capture.hour                          = 0;
    GlobalServerData::serverSettings.city.capture.minute                        = 0;
    GlobalServerData::serverPrivateVariables.flat_map_list                      = NULL;
    //start to 0 due to pre incrementation before use
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    GlobalServerData::serverPrivateVariables.maxClanId=0;
    GlobalServerData::serverPrivateVariables.maxAccountId=0;
    GlobalServerData::serverPrivateVariables.maxCharacterId=0;
    GlobalServerData::serverPrivateVariables.maxMonsterId=1;
    GlobalServerData::serverSettings.database_login.tryInterval=5;
    GlobalServerData::serverSettings.database_login.considerDownAfterNumberOfTry=3;
    GlobalServerData::serverSettings.database_base.tryInterval=5;
    GlobalServerData::serverSettings.database_base.considerDownAfterNumberOfTry=3;
    #endif
    GlobalServerData::serverSettings.database_common.tryInterval=5;
    GlobalServerData::serverSettings.database_common.considerDownAfterNumberOfTry=3;
    GlobalServerData::serverSettings.database_server.tryInterval=5;
    GlobalServerData::serverSettings.database_server.considerDownAfterNumberOfTry=3;

    #ifdef CATCHCHALLENGER_CACHE_HPS
    in_file=nullptr;
    serialBuffer=nullptr;
    out_file=nullptr;
    #endif

    initAll();

    srand(static_cast<int>(time(NULL)));
}

/** call only when the server is down
 * \warning this function is thread safe because it quit all thread before remove */
BaseServer::~BaseServer()
{
    GlobalServerData::serverPrivateVariables.stopIt=true;
    closeDB();
    if(GlobalServerData::serverPrivateVariables.flat_map_list!=NULL)
    {
        delete GlobalServerData::serverPrivateVariables.flat_map_list;
        GlobalServerData::serverPrivateVariables.flat_map_list=NULL;
    }
    if(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm==CatchChallenger::MapVisibilityAlgorithmSelection_Simple)
        if(Map_server_MapVisibility_Simple_StoreOnSender::map_to_update!=NULL)
        {
            delete Map_server_MapVisibility_Simple_StoreOnSender::map_to_update;
            Map_server_MapVisibility_Simple_StoreOnSender::map_to_update=NULL;
        }
}

#ifdef CATCHCHALLENGER_CACHE_HPS
void BaseServer::setSave(const std::string &file)
{
    out_file=new std::ofstream(file, std::ofstream::binary);
    if(!out_file->good() || !out_file->is_open())
    {
        delete out_file;
        out_file=nullptr;
    }
}

void BaseServer::setLoad(const std::string &file)
{
    in_file=new std::ifstream(file, std::ifstream::binary);
    if(!in_file->good() || !in_file->is_open())
    {
        delete in_file;
        in_file=nullptr;
    }
    else
        serialBuffer=new hps::StreamInputBuffer(*in_file);
}

bool BaseServer::binaryCacheIsOpen() const
{
    return in_file!=nullptr;
}

NormalServerSettings BaseServer::loadSettingsFromBinaryCache(std::string &master_host, uint16_t &master_port,
                                                             uint8_t &master_tryInterval,
                                                             uint8_t &master_considerDownAfterNumberOfTry)
{
    *serialBuffer >> GlobalServerData::serverSettings;

    *serialBuffer >> CommonSettingsServer::commonSettingsServer.useSP;
    *serialBuffer >> CommonSettingsServer::commonSettingsServer.autoLearn;
    *serialBuffer >> CommonSettingsServer::commonSettingsServer.useSP;
    *serialBuffer >> CommonSettingsServer::commonSettingsServer.forcedSpeed;
    *serialBuffer >> CommonSettingsServer::commonSettingsServer.dontSendPseudo;
    *serialBuffer >> CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange;
    *serialBuffer >> CommonSettingsServer::commonSettingsServer.mainDatapackCode;
    *serialBuffer >> CommonSettingsServer::commonSettingsServer.subDatapackCode;
    *serialBuffer >> CommonSettingsServer::commonSettingsServer.exportedXml;
    *serialBuffer >> CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer;
    *serialBuffer >> CommonSettingsServer::commonSettingsServer.rates_xp;
    *serialBuffer >> CommonSettingsServer::commonSettingsServer.rates_gold;
    *serialBuffer >> CommonSettingsServer::commonSettingsServer.rates_xp_pow;
    *serialBuffer >> CommonSettingsServer::commonSettingsServer.rates_drop;
    *serialBuffer >> CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick;
    *serialBuffer >> CommonSettingsServer::commonSettingsServer.chat_allow_all;
    *serialBuffer >> CommonSettingsServer::commonSettingsServer.chat_allow_local;
    *serialBuffer >> CommonSettingsServer::commonSettingsServer.chat_allow_private;
    *serialBuffer >> CommonSettingsServer::commonSettingsServer.chat_allow_clan;

    NormalServerSettings normalServerSettings;
    *serialBuffer >> normalServerSettings.proxy;
    *serialBuffer >> normalServerSettings.proxy_port;
    *serialBuffer >> normalServerSettings.server_ip;
    *serialBuffer >> normalServerSettings.server_port;
    *serialBuffer >> normalServerSettings.useSsl;

    setSettings(GlobalServerData::serverSettings);

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    (void)master_host;
    (void)master_port;
    (void)master_tryInterval;
    (void)master_considerDownAfterNumberOfTry;
    #else
    *serialBuffer >> master_host;
    *serialBuffer >> master_port;
    *serialBuffer >> master_tryInterval;
    *serialBuffer >> master_considerDownAfterNumberOfTry;
    #endif

    return normalServerSettings;
}
#endif

void BaseServer::closeDB()
{
    if(GlobalServerData::serverPrivateVariables.db_server!=NULL)
        GlobalServerData::serverPrivateVariables.db_server->syncDisconnect();
    if(GlobalServerData::serverPrivateVariables.db_common!=NULL)
        GlobalServerData::serverPrivateVariables.db_common->syncDisconnect();
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    if(GlobalServerData::serverPrivateVariables.db_base!=NULL)
        GlobalServerData::serverPrivateVariables.db_base->syncDisconnect();
    if(GlobalServerData::serverPrivateVariables.db_login!=NULL)
        GlobalServerData::serverPrivateVariables.db_login->syncDisconnect();
    #endif
}

void BaseServer::initAll()
{
}

//////////////////////////////////////////// server starting //////////////////////////////////////

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
        for(unsigned int i=0; i<MapServer::mapListSize; i++)
        {
            std::string string;
            uint32_t id=0;
            *serialBuffer >> id;
            if(id>4000000000)
                abort();
            *serialBuffer >> string;
            MapServer * map=static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.flat_map_list[i]);
            *serialBuffer >> *map;
            GlobalServerData::serverPrivateVariables.id_map_to_map[id]=string;
            GlobalServerData::serverPrivateVariables.map_list[string]=map;
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
                hps::to_stream(id, *out_file);
                idSize+=((uint32_t)out_file->tellp()-(uint32_t)lastSize);lastSize=out_file->tellp();
                hps::to_stream(string, *out_file);
                pathSize+=((uint32_t)out_file->tellp()-(uint32_t)lastSize);lastSize=out_file->tellp();
                hps::to_stream(*map, *out_file);
                mapSize+=((uint32_t)out_file->tellp()-(uint32_t)lastSize);lastSize=out_file->tellp();
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

void BaseServer::criticalDatabaseQueryFailed()
{
    unload_the_data();
    quitForCriticalDatabaseQueryFailed();
}

#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
void BaseServer::SQL_common_load_finish()
{
    std::cout << dictionary_reputation_database_to_internal.size() << " SQL reputation dictionary" << std::endl;

    /*DictionaryLogin::dictionary_allow_database_to_internal=this->dictionary_allow_database_to_internal;
    DictionaryLogin::dictionary_allow_internal_to_database=this->dictionary_allow_internal_to_database;*/
    DictionaryLogin::dictionary_reputation_database_to_internal=this->dictionary_reputation_database_to_internal;//negative == not found
    DictionaryLogin::dictionary_skin_database_to_internal=this->dictionary_skin_database_to_internal;
    DictionaryLogin::dictionary_skin_internal_to_database=this->dictionary_skin_internal_to_database;
    DictionaryLogin::dictionary_starter_database_to_internal=this->dictionary_starter_database_to_internal;
    DictionaryLogin::dictionary_starter_internal_to_database=this->dictionary_starter_internal_to_database;

    preload_profile();
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    load_sql_monsters_max_id();
    #else
    preload_industries();
    #endif
}
#endif

void BaseServer::preload_finish()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(CommonDatapack::commonDatapack.craftingRecipesMaxId==0)
    {
        std::cerr << "CommonDatapack::commonDatapack.crafingRecipesMaxId==0" << std::endl;
        abort();
    }
    if(CommonDatapack::commonDatapack.monstersMaxId==0)
    {
        std::cerr << "CommonDatapack::commonDatapack.monstersMaxId==0" << std::endl;
        abort();
    }
    if(CommonDatapack::commonDatapack.items.itemMaxId==0)
    {
        std::cerr << "CommonDatapack::commonDatapack.items.itemMaxId==0" << std::endl;
        abort();
    }
    #endif

    std::cout << plant_on_the_map << " SQL plant on map" << std::endl;
    std::cout << GlobalServerData::serverPrivateVariables.marketItemList.size() << " SQL market item" << std::endl;
    const auto &now = msFrom1970();
    std::cout << "Loaded the server SQL datapack into " << (now-timeDatapack) << "ms" << std::endl;
    preload_other();
    #if defined(EPOLLCATCHCHALLENGERSERVER) && ! defined(CATCHCHALLENGER_CLIENT)

    //delete content of Map_loader::getXmlCondition()
    CommonDatapack::commonDatapack.xmlLoadedFile.clear();

    Map_loader::teleportConditionsUnparsed.clear();
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    entryListZone.clear();
    #endif
    CommonSettingsCommon::commonSettingsCommon.datapackHashBase.clear();
    CommonSettingsServer::commonSettingsServer.datapackHashServerMain.clear();
    CommonSettingsServer::commonSettingsServer.datapackHashServerSub.clear();
    preload_market_monsters_prices_call=false;
    preload_industries_call=false;
    preload_market_items_call=false;

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    switch(GlobalServerData::serverSettings.database_login.tryOpenType)
    {
        #ifdef CATCHCHALLENGER_DB_POSTGRESQL
        case DatabaseBase::DatabaseType::PostgreSQL:
            static_cast<EpollPostgresql *>(GlobalServerData::serverPrivateVariables.db_login)->setMaxDbQueries(100);
        break;
        #endif
        default:
        break;
    }
    switch(GlobalServerData::serverSettings.database_base.tryOpenType)
    {
        #ifdef CATCHCHALLENGER_DB_POSTGRESQL
        case DatabaseBase::DatabaseType::PostgreSQL:
            static_cast<EpollPostgresql *>(GlobalServerData::serverPrivateVariables.db_base)->setMaxDbQueries(100);
        break;
        #endif
        default:
        break;
    }
    #endif
    switch(GlobalServerData::serverSettings.database_common.tryOpenType)
    {
        #ifdef CATCHCHALLENGER_DB_POSTGRESQL
        case DatabaseBase::DatabaseType::PostgreSQL:
            static_cast<EpollPostgresql *>(GlobalServerData::serverPrivateVariables.db_common)->setMaxDbQueries(100);
        break;
        #endif
        default:
        break;
    }
    switch(GlobalServerData::serverSettings.database_server.tryOpenType)
    {
        #ifdef CATCHCHALLENGER_DB_POSTGRESQL
        case DatabaseBase::DatabaseType::PostgreSQL:
            static_cast<EpollPostgresql *>(GlobalServerData::serverPrivateVariables.db_server)->setMaxDbQueries(100);
        break;
        #endif
        default:
        break;
    }
}

#ifndef EPOLLCATCHCHALLENGERSERVER
bool BaseServer::load_next_city_capture()
{
    GlobalServerData::serverPrivateVariables.time_city_capture=FacilityLib::nextCaptureTime(GlobalServerData::serverSettings.city);
    std::time_t result = std::time(nullptr);
    const int64_t &time=GlobalServerData::serverPrivateVariables.time_city_capture-result;
    GlobalServerData::serverPrivateVariables.timer_city_capture->start(static_cast<int>(time));
    return true;
}
#endif

void BaseServer::parseJustLoadedMap(const Map_to_send &,const std::string &)
{
}

bool BaseServer::initialize_the_database()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    #ifdef CATCHCHALLENGER_SOLO
    if(GlobalServerData::serverPrivateVariables.db_server!=NULL && GlobalServerData::serverPrivateVariables.db_server->isConnected())
        if(GlobalServerData::serverPrivateVariables.db_server->databaseType()!=DatabaseBase::DatabaseType::SQLite)
        {
            std::cerr << "Disconnected to incorrect database type for solo: " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_server->databaseType()) << std::endl;
            abort();
        }
    if(GlobalServerData::serverPrivateVariables.db_common!=NULL && GlobalServerData::serverPrivateVariables.db_common->isConnected())
        if(GlobalServerData::serverPrivateVariables.db_common->databaseType()!=DatabaseBase::DatabaseType::SQLite)
        {
            std::cerr << "Disconnected to incorrect database type for solo: " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_common->databaseType()) << std::endl;
            abort();
        }
    if(GlobalServerData::serverPrivateVariables.db_login!=NULL && GlobalServerData::serverPrivateVariables.db_login->isConnected())
        if(GlobalServerData::serverPrivateVariables.db_login->databaseType()!=DatabaseBase::DatabaseType::SQLite)
        {
            std::cerr << "Disconnected to incorrect database type for solo: " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_login->databaseType()) << std::endl;
            abort();
        }
    if(GlobalServerData::serverPrivateVariables.db_base!=NULL && GlobalServerData::serverPrivateVariables.db_base->isConnected())
        if(GlobalServerData::serverPrivateVariables.db_base->databaseType()!=DatabaseBase::DatabaseType::SQLite)
        {
            std::cerr << "Disconnected to incorrect database type for solo: " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_base->databaseType()) << std::endl;
            abort();
        }
    #endif
    #endif
    if(GlobalServerData::serverPrivateVariables.db_server->isConnected())
    {
        std::cout << "Disconnected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_server->databaseType())
                  << " at " << GlobalServerData::serverSettings.database_server.host << std::endl;
        GlobalServerData::serverPrivateVariables.db_server->syncDisconnect();
    }
    if(GlobalServerData::serverPrivateVariables.db_common->isConnected())
    {
        std::cout << "Disconnected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_common->databaseType())
                  << " at " << GlobalServerData::serverSettings.database_common.host << std::endl;
        GlobalServerData::serverPrivateVariables.db_common->syncDisconnect();
    }
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    if(GlobalServerData::serverPrivateVariables.db_login->isConnected())
    {
        std::cout << "Disconnected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_login->databaseType())
                  << " at " << GlobalServerData::serverSettings.database_login.host << std::endl;
        GlobalServerData::serverPrivateVariables.db_login->syncDisconnect();
    }
    if(GlobalServerData::serverPrivateVariables.db_base->isConnected())
    {
        std::cout << "Disconnected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_base->databaseType())
                  << " at " << GlobalServerData::serverSettings.database_base.host << std::endl;
        GlobalServerData::serverPrivateVariables.db_base->syncDisconnect();
    }
    #endif


    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    switch(GlobalServerData::serverSettings.database_login.tryOpenType)
    {
        default:
        std::cerr << "database type unknown" << std::endl;
        return false;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::Mysql:
        if(!GlobalServerData::serverPrivateVariables.db_login->syncConnectMysql(
                    GlobalServerData::serverSettings.database_login.host.c_str(),
                    GlobalServerData::serverSettings.database_login.db.c_str(),
                    GlobalServerData::serverSettings.database_login.login.c_str(),
                    GlobalServerData::serverSettings.database_login.pass.c_str()
                    ))
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_login->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_login->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_login.host
                      << " (" << GlobalServerData::serverSettings.database_login.db << ")"
                      << std::endl;
        break;

        case DatabaseBase::DatabaseType::SQLite:
        if(!GlobalServerData::serverPrivateVariables.db_login->syncConnectSqlite(GlobalServerData::serverSettings.database_login.file.c_str()))
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_login->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_login->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_login.file << std::endl;
        break;
        #endif

        case DatabaseBase::DatabaseType::PostgreSQL:
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(!GlobalServerData::serverPrivateVariables.db_login->syncConnectPostgresql(
                    GlobalServerData::serverSettings.database_login.host.c_str(),
                    GlobalServerData::serverSettings.database_login.db.c_str(),
                    GlobalServerData::serverSettings.database_login.login.c_str(),
                    GlobalServerData::serverSettings.database_login.pass.c_str()
                    ))
        #else
        if(!GlobalServerData::serverPrivateVariables.db_login->syncConnect(
                    GlobalServerData::serverSettings.database_login.host.c_str(),
                    GlobalServerData::serverSettings.database_login.db.c_str(),
                    GlobalServerData::serverSettings.database_login.login.c_str(),
                    GlobalServerData::serverSettings.database_login.pass.c_str()
                    ))
        #endif
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_login->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_login->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_login.host
                      << " (" << GlobalServerData::serverSettings.database_login.db << ")"
                      << std::endl;
        break;
    }
    if(!GlobalServerData::serverPrivateVariables.db_login->asyncWrite("SELECT * FROM account"))
    {
        std::cerr << "Basic test failed: " << GlobalServerData::serverPrivateVariables.db_login->errorMessage() << std::endl;
        return false;
    }

    switch(GlobalServerData::serverSettings.database_base.tryOpenType)
    {
        default:
        std::cerr << "database type unknown" << std::endl;
        return false;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::Mysql:
        if(!GlobalServerData::serverPrivateVariables.db_base->syncConnectMysql(
                    GlobalServerData::serverSettings.database_base.host.c_str(),
                    GlobalServerData::serverSettings.database_base.db.c_str(),
                    GlobalServerData::serverSettings.database_base.login.c_str(),
                    GlobalServerData::serverSettings.database_base.pass.c_str()
                    ))
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_base->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_base->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_base.host
                      << " (" << GlobalServerData::serverSettings.database_base.db << ")"
                      << std::endl;
        break;

        case DatabaseBase::DatabaseType::SQLite:
        if(!GlobalServerData::serverPrivateVariables.db_base->syncConnectSqlite(GlobalServerData::serverSettings.database_base.file.c_str()))
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_base->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_base->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_base.file << std::endl;
        break;
        #endif

        case DatabaseBase::DatabaseType::PostgreSQL:
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(!GlobalServerData::serverPrivateVariables.db_base->syncConnectPostgresql(
                    GlobalServerData::serverSettings.database_base.host.c_str(),
                    GlobalServerData::serverSettings.database_base.db.c_str(),
                    GlobalServerData::serverSettings.database_base.login.c_str(),
                    GlobalServerData::serverSettings.database_base.pass.c_str()
                    ))
        #else
        if(!GlobalServerData::serverPrivateVariables.db_base->syncConnect(
                    GlobalServerData::serverSettings.database_base.host.c_str(),
                    GlobalServerData::serverSettings.database_base.db.c_str(),
                    GlobalServerData::serverSettings.database_base.login.c_str(),
                    GlobalServerData::serverSettings.database_base.pass.c_str()
                    ))
        #endif
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_base->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_base->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_base.host
                      << " (" << GlobalServerData::serverSettings.database_base.db << ")"
                      << std::endl;
        break;
    }
    if(!GlobalServerData::serverPrivateVariables.db_base->asyncWrite("SELECT * FROM dictionary_reputation"))
    {
        std::cerr << "Basic test failed: " << GlobalServerData::serverPrivateVariables.db_base->errorMessage() << std::endl;
        return false;
    }
    #endif

    switch(GlobalServerData::serverSettings.database_common.tryOpenType)
    {
        default:
        std::cerr << "database type unknown" << std::endl;
        return false;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::Mysql:
        if(!GlobalServerData::serverPrivateVariables.db_common->syncConnectMysql(
                    GlobalServerData::serverSettings.database_common.host.c_str(),
                    GlobalServerData::serverSettings.database_common.db.c_str(),
                    GlobalServerData::serverSettings.database_common.login.c_str(),
                    GlobalServerData::serverSettings.database_common.pass.c_str()
                    ))
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_common->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_common.host
                      << " (" << GlobalServerData::serverSettings.database_common.db << ")"
                      << std::endl;
        break;

        case DatabaseBase::DatabaseType::SQLite:
        if(!GlobalServerData::serverPrivateVariables.db_common->syncConnectSqlite(GlobalServerData::serverSettings.database_common.file.c_str()))
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_common->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_common.file << std::endl;
        break;
        #endif

        case DatabaseBase::DatabaseType::PostgreSQL:
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(!GlobalServerData::serverPrivateVariables.db_common->syncConnectPostgresql(
                    GlobalServerData::serverSettings.database_common.host.c_str(),
                    GlobalServerData::serverSettings.database_common.db.c_str(),
                    GlobalServerData::serverSettings.database_common.login.c_str(),
                    GlobalServerData::serverSettings.database_common.pass.c_str()
                    ))
        #else
        if(!GlobalServerData::serverPrivateVariables.db_common->syncConnect(
                    GlobalServerData::serverSettings.database_common.host.c_str(),
                    GlobalServerData::serverSettings.database_common.db.c_str(),
                    GlobalServerData::serverSettings.database_common.login.c_str(),
                    GlobalServerData::serverSettings.database_common.pass.c_str()
                    ))
        #endif
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_common->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_common.host
                      << " (" << GlobalServerData::serverSettings.database_common.db << ")"
                      << std::endl;
        break;
    }
    if(!GlobalServerData::serverPrivateVariables.db_common->asyncWrite("SELECT * FROM character"))
    {
        std::cerr << "Basic test failed: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
        return false;
    }
    switch(GlobalServerData::serverSettings.database_server.tryOpenType)
    {
        default:
        std::cerr << "database type unknown" << std::endl;
        return false;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case DatabaseBase::DatabaseType::Mysql:
        if(!GlobalServerData::serverPrivateVariables.db_server->syncConnectMysql(
                    GlobalServerData::serverSettings.database_server.host.c_str(),
                    GlobalServerData::serverSettings.database_server.db.c_str(),
                    GlobalServerData::serverSettings.database_server.login.c_str(),
                    GlobalServerData::serverSettings.database_server.pass.c_str()
                    ))
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_server->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_server.host
                      << " (" << GlobalServerData::serverSettings.database_server.db << ")"
                      << std::endl;
        break;

        case DatabaseBase::DatabaseType::SQLite:
        if(!GlobalServerData::serverPrivateVariables.db_server->syncConnectSqlite(GlobalServerData::serverSettings.database_server.file.c_str()))
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_server->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_server.file << std::endl;
        break;
        #endif

        case DatabaseBase::DatabaseType::PostgreSQL:
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(!GlobalServerData::serverPrivateVariables.db_server->syncConnectPostgresql(
                    GlobalServerData::serverSettings.database_server.host.c_str(),
                    GlobalServerData::serverSettings.database_server.db.c_str(),
                    GlobalServerData::serverSettings.database_server.login.c_str(),
                    GlobalServerData::serverSettings.database_server.pass.c_str()
                    ))
        #else
        if(!GlobalServerData::serverPrivateVariables.db_server->syncConnect(
                    GlobalServerData::serverSettings.database_server.host.c_str(),
                    GlobalServerData::serverSettings.database_server.db.c_str(),
                    GlobalServerData::serverSettings.database_server.login.c_str(),
                    GlobalServerData::serverSettings.database_server.pass.c_str()
                    ))
        #endif
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_server->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_server.host
                      << " (" << GlobalServerData::serverSettings.database_server.db << ")"
                      << std::endl;
        break;
    }
    if(!GlobalServerData::serverPrivateVariables.db_server->asyncWrite("SELECT * FROM character_forserver"))
    {
        std::cerr << "Basic test failed: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
        return false;
    }

    initialize_the_database_prepared_query();
    return true;
}

void BaseServer::initialize_the_database_prepared_query()
{
    #if ! defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER) || defined(CATCHCHALLENGER_CLIENT)
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        //test memory leak around prepared string
        StringWithReplacement test;
        test="SELECT id,encode(password,'hex') FROM account WHERE login=decode('%1','hex')";
    }
    #endif
    PreparedDBQueryLogin::initDatabaseQueryLogin(GlobalServerData::serverPrivateVariables.db_login->databaseType(),GlobalServerData::serverPrivateVariables.db_login);
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommonForLogin.initDatabaseQueryCommonForLogin(GlobalServerData::serverPrivateVariables.db_common->databaseType(),GlobalServerData::serverPrivateVariables.db_common);
    #endif
    //PreparedDBQueryBase::initDatabaseQueryBase(GlobalServerData::serverPrivateVariables.db_base->databaseType());//don't exist, allow dictionary and loaded without cache
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.initDatabaseQueryCommonWithoutSP(GlobalServerData::serverPrivateVariables.db_common->databaseType(),GlobalServerData::serverPrivateVariables.db_common);
    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.initDatabaseQueryCommonWithSP(GlobalServerData::serverPrivateVariables.db_common->databaseType(),CommonSettingsServer::commonSettingsServer.useSP,GlobalServerData::serverPrivateVariables.db_common);
    GlobalServerData::serverPrivateVariables.preparedDBQueryServer.initDatabaseQueryServer(GlobalServerData::serverPrivateVariables.db_server->databaseType(),GlobalServerData::serverPrivateVariables.db_server);
}

#ifdef CATCHCHALLENGER_CACHE_HPS
#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
void BaseServer::setMaster(
    const std::string &master_host,
    const uint16_t &master_port,
    const uint8_t &master_tryInterval,
    const uint8_t &master_considerDownAfterNumberOfTry)
{
    hps::to_stream(master_host, *out_file);
    hps::to_stream(master_port, *out_file);
    hps::to_stream(master_tryInterval, *out_file);
    hps::to_stream(master_considerDownAfterNumberOfTry, *out_file);
}
#endif
#endif

void BaseServer::setSettings(const GameServerSettings &settings)
{
    //load it
    GlobalServerData::serverSettings=settings;
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    memcpy(Client::private_token_statclient,settings.private_token_statclient,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
    #endif

    loadAndFixSettings();

    #ifdef CATCHCHALLENGER_CACHE_HPS
    if(out_file!=nullptr)
    {
        hps::to_stream(GlobalServerData::serverSettings, *out_file);

        hps::to_stream(CommonSettingsServer::commonSettingsServer.useSP, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.autoLearn, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.useSP, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.forcedSpeed, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.dontSendPseudo, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.mainDatapackCode, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.subDatapackCode, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.exportedXml, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.rates_xp, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.rates_gold, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.rates_xp_pow, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.rates_drop, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.chat_allow_all, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.chat_allow_local, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.chat_allow_private, *out_file);
        hps::to_stream(CommonSettingsServer::commonSettingsServer.chat_allow_clan, *out_file);
    }
    #endif
}

#ifdef CATCHCHALLENGER_CACHE_HPS
void BaseServer::setNormalSettings(const NormalServerSettings &normalServerSettings)
{
    if(out_file!=nullptr)
    {
        hps::to_stream(normalServerSettings.proxy, *out_file);
        hps::to_stream(normalServerSettings.proxy_port, *out_file);
        hps::to_stream(normalServerSettings.server_ip, *out_file);
        hps::to_stream(normalServerSettings.server_port, *out_file);
        hps::to_stream(normalServerSettings.useSsl, *out_file);
    }
}
#endif

GameServerSettings BaseServer::getSettings() const
{
    return GlobalServerData::serverSettings;
}
