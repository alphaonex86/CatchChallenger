#include "BaseServer.hpp"
#include "GlobalServerData.hpp"
#include "DictionaryLogin.hpp"
#include "ClientMapManagement/Map_server_MapVisibility_Simple_StoreOnSender.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/ProtocolParsing.hpp"
#include <fstream>

using namespace CatchChallenger;

BaseServer::BaseServer() :
    stat(Down),
    dataLoaded(false)
{
    #ifdef CATCHCHALLENGER_DB_FILE
    //mostly static
    dictionary_in_file=nullptr;
    dictionary_serialBuffer=nullptr;
    dictionary_haveChange=false;
    //mostly dynamic
    server_in_file=nullptr;
    server_serialBuffer=nullptr;
    #endif

    ProtocolParsing::initialiseTheVariable();
    timeDatapack=0;
    entryListIndex=0;
    preload_industries_call=false;

    CompressionProtocol::compressionTypeServer                                = CompressionProtocol::CompressionType::Zstandard;

    GlobalServerData::serverPrivateVariables.connected_players      = 0;
    GlobalServerData::serverPrivateVariables.number_of_bots_logged  = 0;
    GlobalServerData::serverPrivateVariables.time_city_capture      = 0;

    GlobalServerData::serverPrivateVariables.botSpawnIndex          = 0;
    GlobalServerData::serverPrivateVariables.datapack_rightFileName	= std::regex(DATAPACK_FILE_REGEX,std::regex_constants::optimize);

    GlobalServerData::serverSettings.automatic_account_creation             = false;
    GlobalServerData::serverSettings.max_players                            = 1;
    GlobalServerData::serverSettings.sendPlayerNumber                       = true;
    GlobalServerData::serverSettings.pvp                                    = true;

    GlobalServerData::serverSettings.datapackCache                              = -1;
    if(FacilityLibGeneral::applicationDirPath.empty())
    {
        std::cerr << "FacilityLibGeneral::applicationDirPath is empty" << std::endl;
        abort();
    }
    GlobalServerData::serverSettings.datapack_basePath                          = FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/datapack/";
    GlobalServerData::serverSettings.compressionType                            = CompressionProtocol::CompressionType::Zstandard;
    GlobalServerData::serverSettings.dontSendPlayerType                         = false;
    CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange = true;
    CommonSettingsServer::commonSettingsServer.forcedSpeed            = CATCHCHALLENGER_SERVER_NORMAL_SPEED;
    CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters            = 8;
    CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters   = 30;
    CommonSettingsCommon::commonSettingsCommon.maxPlayerItems               = 30;
    CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems      = 150;
    GlobalServerData::serverSettings.anonymous            = false;
    GlobalServerData::serverSettings.everyBodyIsRoot=0;
    GlobalServerData::serverSettings.teleportIfMapNotFoundOrOutOfMap=0;
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
    GlobalServerData::serverSettings.mapVisibility.simple.max                   = 30;
    GlobalServerData::serverSettings.mapVisibility.simple.reshow                = 20;
    GlobalServerData::serverSettings.mapVisibility.simple.enable                = true;
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
    #ifndef CATCHCHALLENGER_DB_FILE
    GlobalServerData::serverPrivateVariables.maxMonsterId=1;
    #endif
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    GlobalServerData::serverSettings.database_login.tryInterval=5;
    GlobalServerData::serverSettings.database_login.considerDownAfterNumberOfTry=3;
    GlobalServerData::serverSettings.database_base.tryInterval=5;
    GlobalServerData::serverSettings.database_base.considerDownAfterNumberOfTry=3;
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #else
    #endif
    #endif
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    GlobalServerData::serverSettings.database_common.tryInterval=5;
    GlobalServerData::serverSettings.database_common.considerDownAfterNumberOfTry=3;
    GlobalServerData::serverSettings.database_server.tryInterval=5;
    GlobalServerData::serverSettings.database_server.considerDownAfterNumberOfTry=3;
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #else
    #endif

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
    if(Map_server_MapVisibility_Simple_StoreOnSender::map_to_update!=NULL)
    {
        delete Map_server_MapVisibility_Simple_StoreOnSender::map_to_update;
        Map_server_MapVisibility_Simple_StoreOnSender::map_to_update=NULL;
    }
    Map_server_MapVisibility_Simple_StoreOnSender::map_to_update_size=0;
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
    return in_file!=nullptr || out_file!=nullptr;
}

bool BaseServer::binaryInputCacheIsOpen() const
{
    return in_file!=nullptr;
}

bool BaseServer::binaryOutputCacheIsOpen() const
{
    return out_file!=nullptr;
}

NormalServerSettings BaseServer::loadSettingsFromBinaryCache(std::string &master_host, uint16_t &master_port,
                                                             uint8_t &master_tryInterval,
                                                             uint8_t &master_considerDownAfterNumberOfTry)
{
    *serialBuffer >> GlobalServerData::serverSettings;

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
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
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
    #endif
}

void BaseServer::initAll()
{
}

//////////////////////////////////////////// server starting //////////////////////////////////////

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

    preload_18_sync_profile();
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    preload_19_async_sql_monsters_max_id();
    #else
    preload_industries();
    #endif
}
#endif

void BaseServer::preload_finish()//call after preload_industries_return(), after SQL, preload_industries_return() save the cache/and db file
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(CommonDatapack::commonDatapack.get_craftingRecipesMaxId()==0)
    {
        std::cerr << "CommonDatapack::commonDatapack.crafingRecipesMaxId==0" << std::endl;
        abort();
    }
    if(CommonDatapack::commonDatapack.get_monstersMaxId()==0)
    {
        std::cerr << "CommonDatapack::commonDatapack.monstersMaxId==0" << std::endl;
        abort();
    }
    if(CommonDatapack::commonDatapack.get_items().itemMaxId==0)
    {
        std::cerr << "CommonDatapack::commonDatapack.items.itemMaxId==0" << std::endl;
        abort();
    }
    #endif

    std::cout << GlobalServerData::serverPrivateVariables.marketItemList.size() << " SQL market item" << std::endl;
    const auto &now = msFrom1970();
    std::cout << "Loaded the server SQL datapack into " << (now-timeDatapack) << "ms" << std::endl;
    preload_30_sync_other();
    #if defined(EPOLLCATCHCHALLENGERSERVER) && ! defined(CATCHCHALLENGER_CLIENT)

    //delete content of Map_loader::getXmlCondition()
    CommonDatapack::commonDatapack.get_xmlLoadedFile_rw().clear();

    Map_loader::teleportConditionsUnparsed.clear();
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    entryListZone.clear();
    #endif
    CommonSettingsCommon::commonSettingsCommon.datapackHashBase.clear();
    CommonSettingsServer::commonSettingsServer.datapackHashServerMain.clear();
    CommonSettingsServer::commonSettingsServer.datapackHashServerSub.clear();
    preload_industries_call=false;

    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    switch(GlobalServerData::serverSettings.database_login.tryOpenType)
    {
        #ifdef CATCHCHALLENGER_DB_POSTGRESQL
        case DatabaseBase::DatabaseType::PostgreSQL:
            GlobalServerData::serverPrivateVariables.db_login->setMaxDbQueries(100);
        break;
        #endif
        default:
        break;
    }
    switch(GlobalServerData::serverSettings.database_base.tryOpenType)
    {
        #ifdef CATCHCHALLENGER_DB_POSTGRESQL
        case DatabaseBase::DatabaseType::PostgreSQL:
            GlobalServerData::serverPrivateVariables.db_base->setMaxDbQueries(100);
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
            GlobalServerData::serverPrivateVariables.db_common->setMaxDbQueries(100);
        break;
        #endif
        default:
        break;
    }
    switch(GlobalServerData::serverSettings.database_server.tryOpenType)
    {
        #ifdef CATCHCHALLENGER_DB_POSTGRESQL
        case DatabaseBase::DatabaseType::PostgreSQL:
            GlobalServerData::serverPrivateVariables.db_server->setMaxDbQueries(100);
        break;
        #endif
        default:
        break;
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    {
        {
            #ifdef CATCHCHALLENGER_DB_FILE
            std::ifstream in_file("database/server", std::ifstream::binary);
            if(!in_file.good() || !in_file.is_open())
            {
                GlobalServerData::serverPrivateVariables.maxClanId=0;
                GlobalServerData::serverPrivateVariables.maxAccountId=0;
                GlobalServerData::serverPrivateVariables.maxCharacterId=0;
            }
            else
            {
                hps::StreamInputBuffer s(in_file);
                s >> GlobalServerData::serverPrivateVariables.maxClanId;
                s >> GlobalServerData::serverPrivateVariables.maxAccountId;
                s >> GlobalServerData::serverPrivateVariables.maxCharacterId;
                s >> GlobalServerData::serverPrivateVariables.industriesStatus;
                s >> GlobalServerData::serverSettings.city;
            }
            #endif
        }
        {
            std::ofstream out_file("database/server", std::ofstream::binary);
            if(!out_file.good() || !out_file.is_open())
            {
                std::cerr << "Unable to open data base file " << "database/server (abort)" << std::endl;
                abort();
                return;
            }
            hps::to_stream(GlobalServerData::serverPrivateVariables.maxClanId, out_file);
            hps::to_stream(GlobalServerData::serverPrivateVariables.maxAccountId, out_file);
            hps::to_stream(GlobalServerData::serverPrivateVariables.maxCharacterId, out_file);
            hps::to_stream(GlobalServerData::serverPrivateVariables.industriesStatus, out_file);
            hps::to_stream(GlobalServerData::serverSettings.city, out_file);

            //std::cerr << "createAccount_return) for: database/server" << std::endl;
        }
    }
    #else
    #endif
}
