#include "BaseServer.h"
#include "GlobalServerData.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonDatapackServerSpec.h"
#include "../../general/base/DatapackGeneralLoader.h"
#include "ClientMapManagement/MapVisibilityAlgorithm_None.h"
#include "ClientMapManagement/MapVisibilityAlgorithm_Simple_StoreOnSender.h"
#include "ClientMapManagement/MapVisibilityAlgorithm_WithBorder_StoreOnSender.h"
#include "ClientMapManagement/Map_server_MapVisibility_Simple_StoreOnSender.h"
#include "ClientMapManagement/Map_server_MapVisibility_WithBorder_StoreOnSender.h"
#include "LocalClientHandlerWithoutSender.h"
#include "ClientNetworkReadWithoutSender.h"
#include "SqlFunction.h"
#include "DictionaryServer.h"
#include "DictionaryLogin.h"
#include "PreparedDBQuery.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/CommonSettingsServer.h"

#include <QFile>
#include <QByteArray>
#include <QDateTime>
#include <QTime>
#include <QCryptographicHash>
#include <time.h>
#include <iostream>
#ifndef EPOLLCATCHCHALLENGERSERVER
#include <QTimer>
#endif

using namespace CatchChallenger;

QRegularExpression BaseServer::regexXmlFile=QRegularExpression(QLatin1String("^[a-zA-Z0-9\\- _]+\\.xml$"));
const std::basic_string<char> BaseServer::text_dotxml=".xml";
const std::basic_string<char> BaseServer::text_zone="zone";
const std::basic_string<char> BaseServer::text_capture="capture";
const std::basic_string<char> BaseServer::text_fightId="fightId";
const std::basic_string<char> BaseServer::text_dotcomma=";";
const std::basic_string<char> BaseServer::text_male="male";
const std::basic_string<char> BaseServer::text_female="female";
const std::basic_string<char> BaseServer::text_unknown="unknown";
const std::basic_string<char> BaseServer::text_slash="/";
const std::basic_string<char> BaseServer::text_antislash="\\";
const std::basic_string<char> BaseServer::text_type="type";
const std::basic_string<char> BaseServer::text_shop="shop";
const std::basic_string<char> BaseServer::text_learn="learn";
const std::basic_string<char> BaseServer::text_heal="heal";
const std::basic_string<char> BaseServer::text_market="market";
const std::basic_string<char> BaseServer::text_zonecapture="zonecapture";
const std::basic_string<char> BaseServer::text_fight="fight";
const std::basic_string<char> BaseServer::text_fightid="fightid";
const std::basic_string<char> BaseServer::text_lookAt="lookAt";
const std::basic_string<char> BaseServer::text_left="left";
const std::basic_string<char> BaseServer::text_right="right";
const std::basic_string<char> BaseServer::text_top="top";
const std::basic_string<char> BaseServer::text_bottom="bottom";
const std::basic_string<char> BaseServer::text_fightRange="fightRange";
const std::basic_string<char> BaseServer::text_bots="bots";
const std::basic_string<char> BaseServer::text_bot="bot";
const std::basic_string<char> BaseServer::text_id="id";
const std::basic_string<char> BaseServer::text_name="name";
const std::basic_string<char> BaseServer::text_step="step";
const std::basic_string<char> BaseServer::text_arrow="->";
const std::basic_string<char> BaseServer::text_dottmx=".tmx";
const std::basic_string<char> BaseServer::text_shops="shops";
const std::basic_string<char> BaseServer::text_product="product";
const std::basic_string<char> BaseServer::text_itemId="itemId";
const std::basic_string<char> BaseServer::text_overridePrice="overridePrice";
const std::basic_string<char> BaseServer::text_list="list";
const std::basic_string<char> BaseServer::text_monster="monster";
const std::basic_string<char> BaseServer::text_monsters="monsters";
const std::basic_string<char> BaseServer::text_drops="drops";
const std::basic_string<char> BaseServer::text_drop="drop";
const std::basic_string<char> BaseServer::text_item="item";
const std::basic_string<char> BaseServer::text_quantity_min="quantity_min";
const std::basic_string<char> BaseServer::text_quantity_max="quantity_max";
const std::basic_string<char> BaseServer::text_luck="luck";
const std::basic_string<char> BaseServer::text_percent="percent";

BaseServer::BaseServer() :
    stat(Down),
    dataLoaded(false)
{
    ProtocolParsing::initialiseTheVariable();

    dictionary_pointOnMap_maxId=0;
    ProtocolParsing::compressionTypeServer                                = ProtocolParsing::CompressionType::Zlib;

    GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue=0;
    GlobalServerData::serverPrivateVariables.connected_players      = 0;
    GlobalServerData::serverPrivateVariables.number_of_bots_logged  = 0;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    GlobalServerData::serverPrivateVariables.timer_city_capture     = NULL;
    #endif

    GlobalServerData::serverPrivateVariables.botSpawnIndex          = 0;
    GlobalServerData::serverPrivateVariables.datapack_rightFileName	= QRegularExpression(DATAPACK_FILE_REGEX);
    GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove=NULL;

    GlobalServerData::serverSettings.automatic_account_creation             = false;
    GlobalServerData::serverSettings.max_players                            = 1;
    GlobalServerData::serverSettings.tolerantMode                           = false;
    GlobalServerData::serverSettings.sendPlayerNumber                       = true;
    GlobalServerData::serverSettings.pvp                                    = true;
    GlobalServerData::serverSettings.benchmark                              = false;

    GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm       = CatchChallenger::MapVisibilityAlgorithmSelection_None;
    GlobalServerData::serverSettings.datapackCache                              = -1;
    GlobalServerData::serverSettings.datapack_basePath                          = (QCoreApplication::applicationDirPath()+QLatin1String("/datapack/")).toStdString();
    GlobalServerData::serverSettings.compressionType                            = CompressionType_Zlib;
    GlobalServerData::serverSettings.dontSendPlayerType                         = false;
    CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange = true;
    CommonSettingsServer::commonSettingsServer.forcedSpeed            = CATCHCHALLENGER_SERVER_NORMAL_SPEED;
    #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer=true;
    #else
    CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer=false;
    #endif
    CommonSettingsServer::commonSettingsServer.useSP                  = true;
    CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters            = 8;
    CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters   = 30;
    CommonSettingsCommon::commonSettingsCommon.maxPlayerItems               = 30;
    CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems      = 150;
    GlobalServerData::serverSettings.anonymous            = false;
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
    Client::indexOfItemOnMap=0;
    #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    Client::indexOfDirtOnMap=0;//index of plant on map, ordened by map and x,y ordened into the xml file, less bandwith than send map,x,y
    #endif
    CommonSettingsServer::commonSettingsServer.rates_gold             = 1.0;
    CommonSettingsServer::commonSettingsServer.rates_drop             = 1.0;
    CommonSettingsServer::commonSettingsServer.rates_xp               = 1.0;
    CommonSettingsServer::commonSettingsServer.rates_xp_pow           = 1.0;
    CommonSettingsServer::commonSettingsServer.factoryPriceChange     = 20;
    CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick=30;
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
    GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue      = 3;
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
    #endif


    initAll();

    srand(time(NULL));
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
}

void BaseServer::closeDB()
{
    GlobalServerData::serverPrivateVariables.db_server->syncDisconnect();
    GlobalServerData::serverPrivateVariables.db_common->syncDisconnect();
    GlobalServerData::serverPrivateVariables.db_base->syncDisconnect();
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    GlobalServerData::serverPrivateVariables.db_login->syncDisconnect();
    #endif
}

void BaseServer::initAll()
{
}

//////////////////////////////////////////// server starting //////////////////////////////////////

void BaseServer::preload_the_data()
{
    std::cout << "Preload data for server version " << CATCHCHALLENGER_VERSION << std::endl;

    if(dataLoaded)
        return;
    dataLoaded=true;
    GlobalServerData::serverPrivateVariables.stopIt=false;

    {
        QTime time;
        time.restart();
        CommonDatapack::commonDatapack.parseDatapack(GlobalServerData::serverSettings.datapack_basePath);
        CommonDatapackServerSpec::commonDatapackServerSpec.parseDatapack(GlobalServerData::serverSettings.datapack_basePath,CommonSettingsServer::commonSettingsServer.mainDatapackCode);
        std::cout << "Loaded the common datapack into " << time.elapsed() << "ms" << std::endl;
    }
    timeDatapack.restart();
    preload_the_randomData();
    preload_randomBlock();
    preload_the_events();
    preload_the_ddos();
    preload_the_datapack();
    preload_the_skin();
    preload_the_players();
    preload_monsters_drops();
    preload_the_map();
    baseServerMasterSendDatapack.load(GlobalServerData::serverSettings.datapack_basePath);
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
    load_sql_monsters_warehouse_max_id();
    load_sql_monsters_market_max_id();
    */
}

void BaseServer::criticalDatabaseQueryFailed()
{
    unload_the_data();
    quitForCriticalDatabaseQueryFailed();
}

void BaseServer::SQL_common_load_finish()
{
    std::cout << dictionary_reputation_database_to_internal.size() << " SQL reputation dictionary" << std::endl;

    DictionaryLogin::dictionary_allow_database_to_internal=this->dictionary_allow_database_to_internal;
    DictionaryLogin::dictionary_allow_internal_to_database=this->dictionary_allow_internal_to_database;
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

void BaseServer::preload_finish()
{
    std::cout << plant_on_the_map << " SQL plant on map" << std::endl;
    std::cout << GlobalServerData::serverPrivateVariables.marketItemList.size() << " SQL market item" << std::endl;
    std::cout << "Loaded the server SQL datapack into " << timeDatapack.elapsed() << "ms" << std::endl;
}

bool BaseServer::load_next_city_capture()
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    GlobalServerData::serverPrivateVariables.time_city_capture=FacilityLib::nextCaptureTime(GlobalServerData::serverSettings.city);
    const qint64 &time=GlobalServerData::serverPrivateVariables.time_city_capture.toMSecsSinceEpoch()-QDateTime::currentMSecsSinceEpoch();
    GlobalServerData::serverPrivateVariables.timer_city_capture->start(time);
    #endif
    return true;
}

void BaseServer::parseJustLoadedMap(const Map_to_send &,const std::basic_string<char> &)
{
}

bool BaseServer::initialize_the_database()
{
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
    #endif
    if(GlobalServerData::serverPrivateVariables.db_base->isConnected())
    {
        std::cout << "Disconnected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_base->databaseType())
                  << " at " << GlobalServerData::serverSettings.database_base.host << std::endl;
        GlobalServerData::serverPrivateVariables.db_base->syncDisconnect();
    }
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
                      << " at " << GlobalServerData::serverSettings.database_login.host << std::endl;
        break;

        case DatabaseBase::DatabaseType::SQLite:
        if(!GlobalServerData::serverPrivateVariables.db_login->syncConnectSqlite(GlobalServerData::serverSettings.database_login.file.c_str()))
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_login->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_login->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_login.host << std::endl;
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
                      << " at " << GlobalServerData::serverSettings.database_login.host << std::endl;
        break;
    }
    #endif
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
                      << " at " << GlobalServerData::serverSettings.database_base.host << std::endl;
        break;

        case DatabaseBase::DatabaseType::SQLite:
        if(!GlobalServerData::serverPrivateVariables.db_base->syncConnectSqlite(GlobalServerData::serverSettings.database_base.file.c_str()))
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_base->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_base->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_base.host << std::endl;
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
                      << " at " << GlobalServerData::serverSettings.database_base.host << std::endl;
        break;
    }
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
                      << " at " << GlobalServerData::serverSettings.database_common.host << std::endl;
        break;

        case DatabaseBase::DatabaseType::SQLite:
        if(!GlobalServerData::serverPrivateVariables.db_common->syncConnectSqlite(GlobalServerData::serverSettings.database_common.file.c_str()))
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_common->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_common.host << std::endl;
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
                      << " at " << GlobalServerData::serverSettings.database_common.host << std::endl;
        break;
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
                      << " at " << GlobalServerData::serverSettings.database_server.host << std::endl;
        break;

        case DatabaseBase::DatabaseType::SQLite:
        if(!GlobalServerData::serverPrivateVariables.db_server->syncConnectSqlite(GlobalServerData::serverSettings.database_server.file.c_str()))
        {
            std::cerr << "Unable to connect to the database: " << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
            return false;
        }
        else
            std::cout << "Connected to " << DatabaseBase::databaseTypeToString(GlobalServerData::serverPrivateVariables.db_server->databaseType())
                      << " at " << GlobalServerData::serverSettings.database_server.host << std::endl;
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
                      << " at " << GlobalServerData::serverSettings.database_server.host << std::endl;
        break;
    }

    initialize_the_database_prepared_query();
    return true;
}

void BaseServer::initialize_the_database_prepared_query()
{
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    PreparedDBQueryLogin::initDatabaseQueryLogin(GlobalServerData::serverPrivateVariables.db_login->databaseType());
    #endif
    //PreparedDBQueryBase::initDatabaseQueryBase(GlobalServerData::serverPrivateVariables.db_base->databaseType());//don't exist, allow dictionary and loaded without cache
    PreparedDBQueryCommon::initDatabaseQueryCommonWithoutSP(GlobalServerData::serverPrivateVariables.db_common->databaseType());
    PreparedDBQueryCommon::initDatabaseQueryCommonWithSP(GlobalServerData::serverPrivateVariables.db_common->databaseType(),CommonSettingsServer::commonSettingsServer.useSP);
    PreparedDBQueryServer::initDatabaseQueryServer(GlobalServerData::serverPrivateVariables.db_server->databaseType());
}

void BaseServer::setSettings(const GameServerSettings &settings)
{
    //load it
    GlobalServerData::serverSettings=settings;

    loadAndFixSettings();
}

GameServerSettings BaseServer::getSettings() const
{
    return GlobalServerData::serverSettings;
}

void BaseServer::loadAndFixSettings()
{
    GlobalServerData::serverPrivateVariables.server_message=GlobalServerData::serverSettings.server_message.split(QRegularExpression("[\n\r]+"));
    while(GlobalServerData::serverPrivateVariables.server_message.size()>16)
    {
        qDebug() << std::basic_string<char>Literal("GlobalServerData::serverPrivateVariables.server_message too big, remove: ")+GlobalServerData::serverPrivateVariables.server_message.last();
        GlobalServerData::serverPrivateVariables.server_message.removeLast();
    }
    int index=0;
    const int &listsize=GlobalServerData::serverPrivateVariables.server_message.size();
    while(index<listsize)
    {
        if(GlobalServerData::serverPrivateVariables.server_message.at(index).size()>128)
        {
            qDebug() << std::basic_string<char>Literal("GlobalServerData::serverPrivateVariables.server_message too big, truncate: ")+GlobalServerData::serverPrivateVariables.server_message.at(index);
            GlobalServerData::serverPrivateVariables.server_message[index].truncate(128);
        }
        index++;
    }

    //drop the empty \n
    bool removeTheLastList;
    do
    {
        removeTheLastList=!GlobalServerData::serverPrivateVariables.server_message.isEmpty();
        if(removeTheLastList)
            removeTheLastList=GlobalServerData::serverPrivateVariables.server_message.last().isEmpty();
        if(removeTheLastList)
            GlobalServerData::serverPrivateVariables.server_message.removeLast();
    } while(removeTheLastList);

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    if(GlobalServerData::serverSettings.database_login.tryInterval<1)
    {
        qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.database_login.tryInterval<1");
        GlobalServerData::serverSettings.database_login.tryInterval=5;
    }
    if(GlobalServerData::serverSettings.database_login.considerDownAfterNumberOfTry<1)
    {
        qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.database_login.considerDownAfterNumberOfTry<1");
        GlobalServerData::serverSettings.database_login.considerDownAfterNumberOfTry=3;
    }
    if(GlobalServerData::serverSettings.database_login.tryInterval*GlobalServerData::serverSettings.database_login.considerDownAfterNumberOfTry>(60*10)/*10mins*/)
    {
        qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.database_login.tryInterval*GlobalServerData::serverSettings.database_login.considerDownAfterNumberOfTry>(60*10)");
        GlobalServerData::serverSettings.database_login.tryInterval=5;
        GlobalServerData::serverSettings.database_login.considerDownAfterNumberOfTry=3;
    }
    #endif

    if(GlobalServerData::serverSettings.database_base.tryInterval<1)
    {
        qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.database_base.tryInterval<1");
        GlobalServerData::serverSettings.database_base.tryInterval=5;
    }
    if(GlobalServerData::serverSettings.database_base.considerDownAfterNumberOfTry<1)
    {
        qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.database_base.considerDownAfterNumberOfTry<1");
        GlobalServerData::serverSettings.database_base.considerDownAfterNumberOfTry=3;
    }
    if(GlobalServerData::serverSettings.database_base.tryInterval*GlobalServerData::serverSettings.database_base.considerDownAfterNumberOfTry>(60*10)/*10mins*/)
    {
        qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.database_base.tryInterval*GlobalServerData::serverSettings.database_base.considerDownAfterNumberOfTry>(60*10)");
        GlobalServerData::serverSettings.database_base.tryInterval=5;
        GlobalServerData::serverSettings.database_base.considerDownAfterNumberOfTry=3;
    }

    if(GlobalServerData::serverSettings.database_common.tryInterval<1)
    {
        qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.database_common.tryInterval<1");
        GlobalServerData::serverSettings.database_common.tryInterval=5;
    }
    if(GlobalServerData::serverSettings.database_common.considerDownAfterNumberOfTry<1)
    {
        qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.database_common.considerDownAfterNumberOfTry<1");
        GlobalServerData::serverSettings.database_common.considerDownAfterNumberOfTry=3;
    }
    if(GlobalServerData::serverSettings.database_common.tryInterval*GlobalServerData::serverSettings.database_common.considerDownAfterNumberOfTry>(60*10)/*10mins*/)
    {
        qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.database_common.tryInterval*GlobalServerData::serverSettings.database_common.considerDownAfterNumberOfTry>(60*10)");
        GlobalServerData::serverSettings.database_common.tryInterval=5;
        GlobalServerData::serverSettings.database_common.considerDownAfterNumberOfTry=3;
    }

    if(GlobalServerData::serverSettings.database_server.tryInterval<1)
    {
        qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.database_server.tryInterval<1");
        GlobalServerData::serverSettings.database_server.tryInterval=5;
    }
    if(GlobalServerData::serverSettings.database_server.considerDownAfterNumberOfTry<1)
    {
        qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.database_server.considerDownAfterNumberOfTry<1");
        GlobalServerData::serverSettings.database_server.considerDownAfterNumberOfTry=3;
    }
    if(GlobalServerData::serverSettings.database_server.tryInterval*GlobalServerData::serverSettings.database_server.considerDownAfterNumberOfTry>(60*10)/*10mins*/)
    {
        qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.database_server.tryInterval*GlobalServerData::serverSettings.database_server.considerDownAfterNumberOfTry>(60*10)");
        GlobalServerData::serverSettings.database_server.tryInterval=5;
        GlobalServerData::serverSettings.database_server.considerDownAfterNumberOfTry=3;
    }

    if(CommonSettingsServer::commonSettingsServer.mainDatapackCode.isEmpty())
    {
        DebugClass::debugConsole(std::basic_string<char>Literal("mainDatapackCode is empty, please put it into the settings"));
        abort();
    }
    if(!CommonSettingsServer::commonSettingsServer.mainDatapackCode.contains(QRegularExpression(CATCHCHALLENGER_CHECK_MAINDATAPACKCODE)))
    {
        DebugClass::debugConsole(
                    std::basic_string<char>Literal("CommonSettingsServer::commonSettingsServer.mainDatapackCode not match CATCHCHALLENGER_CHECK_MAINDATAPACKCODE %1 not contain %2")
                    .arg(CommonSettingsServer::commonSettingsServer.mainDatapackCode)
                    .arg(std::basic_string<char>Literal(CATCHCHALLENGER_CHECK_MAINDATAPACKCODE))
                    );
        abort();
    }
    if(!CommonSettingsServer::commonSettingsServer.subDatapackCode.isEmpty())
    {
        if(!CommonSettingsServer::commonSettingsServer.subDatapackCode.contains(QRegularExpression(CATCHCHALLENGER_CHECK_SUBDATAPACKCODE)))
        {
            DebugClass::debugConsole(std::basic_string<char>Literal("CommonSettingsServer::commonSettingsServer.subDatapackCode not match CATCHCHALLENGER_CHECK_SUBDATAPACKCODE"));
            abort();
        }
    }
    const std::basic_string<char> &mainDir=GlobalServerData::serverSettings.datapack_basePath+std::basic_string<char>Literal("map/main/")+CommonSettingsServer::commonSettingsServer.mainDatapackCode+std::basic_string<char>Literal("/");
    if(!QDir(mainDir).exists())
    {
        DebugClass::debugConsole(mainDir+std::basic_string<char>Literal(" don't exists"));
        abort();
    }
    if(!CommonSettingsServer::commonSettingsServer.subDatapackCode.isEmpty())
    {
        const std::basic_string<char> &subDatapackFolder=GlobalServerData::serverSettings.datapack_basePath+std::basic_string<char>Literal("map/main/")+CommonSettingsServer::commonSettingsServer.mainDatapackCode+std::basic_string<char>Literal("/")+
                std::basic_string<char>Literal("sub/")+CommonSettingsServer::commonSettingsServer.subDatapackCode+std::basic_string<char>Literal("/");
        if(!QDir(subDatapackFolder).exists())
        {
            DebugClass::debugConsole(subDatapackFolder+std::basic_string<char>Literal(" don't exists, drop spec"));
            CommonSettingsServer::commonSettingsServer.subDatapackCode.clear();
        }
    }

    if(GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue>9)
    {
        qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue>9");
        GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue=9;
    }
    if(GlobalServerData::serverSettings.ddos.computeAverageValueTimeInterval<1)
    {
        qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.ddos.computeAverageValueTimeInterval<1");
        GlobalServerData::serverSettings.ddos.computeAverageValueTimeInterval=1;
    }

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    /*if(CommonSettingsCommon::commonSettingsCommon.min_character<0)
    {
        qDebug() << std::basic_string<char>Literal("CommonSettingsCommon::commonSettingsCommon.min_character<0");
        CommonSettingsCommon::commonSettingsCommon.min_character=0;
    }*/
    if(CommonSettingsCommon::commonSettingsCommon.max_character<1)
    {
        qDebug() << std::basic_string<char>Literal("CommonSettingsCommon::commonSettingsCommon.max_character<1");
        CommonSettingsCommon::commonSettingsCommon.max_character=1;
    }
    if(CommonSettingsCommon::commonSettingsCommon.max_character<CommonSettingsCommon::commonSettingsCommon.min_character)
    {
        qDebug() << std::basic_string<char>Literal("CommonSettingsCommon::commonSettingsCommon.max_character<CommonSettingsCommon::commonSettingsCommon.min_character");
        CommonSettingsCommon::commonSettingsCommon.max_character=CommonSettingsCommon::commonSettingsCommon.min_character;
    }
    if(CommonSettingsCommon::commonSettingsCommon.character_delete_time<=0)
    {
        qDebug() << std::basic_string<char>Literal("CommonSettingsCommon::commonSettingsCommon.character_delete_time<=0");
        CommonSettingsCommon::commonSettingsCommon.character_delete_time=7*24*3600;
    }
    #endif
    if(CommonSettingsServer::commonSettingsServer.useSP)
    {
        if(CommonSettingsServer::commonSettingsServer.autoLearn)
        {
            qDebug() << "Auto-learn disable when SP enabled";
            CommonSettingsServer::commonSettingsServer.autoLearn=false;
        }
    }

    if(GlobalServerData::serverSettings.datapackCache<-1)
    {
        qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.datapackCache<-1");
        GlobalServerData::serverSettings.datapackCache=-1;
    }
    {
        std::basic_string<char>List newMirrorList;
        QRegularExpression httpMatch("^https?://.+$");
        const std::basic_string<char>List &mirrorList=CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.split(BaseServer::text_dotcomma);
        int index=0;
        while(index<mirrorList.size())
        {
            const std::basic_string<char> &mirror=mirrorList.at(index);
            if(!mirror.contains(httpMatch))
            {}//qDebug() << "Mirror wrong: " << mirror.toLocal8Bit(); -> single player
            else
            {
                if(mirror.endsWith(BaseServer::text_slash))
                    newMirrorList << mirror;
                else
                    newMirrorList << mirror+BaseServer::text_slash;
            }
            index++;
        }
        CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer=newMirrorList.join(BaseServer::text_dotcomma);
        CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase=CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer;
    }

    //check the settings here
    if(GlobalServerData::serverSettings.max_players<1)
    {
        qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.max_players<1");
        GlobalServerData::serverSettings.max_players=200;
    }
    ProtocolParsing::setMaxPlayers(GlobalServerData::serverSettings.max_players);

/*    quint8 player_list_size;
    if(GlobalServerData::serverSettings.max_players<=255)
        player_list_size=sizeof(quint8);
    else
        player_list_size=sizeof(quint16);*/
    quint8 map_list_size;
    if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
        map_list_size=sizeof(quint8);
    else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
        map_list_size=sizeof(quint16);
    else
        map_list_size=sizeof(quint32);
    GlobalServerData::serverPrivateVariables.sizeofInsertRequest=
            //mutualised
            sizeof(quint8)+map_list_size+/*player_list_size same with move, delete, ...*/
            //of the player
            /*player_list_size same with move, delete, ...*/+sizeof(quint8)+sizeof(quint8)+sizeof(quint8)+sizeof(quint8)+sizeof(quint8)+0/*pseudo size put directy*/+sizeof(quint8);
    if(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm==CatchChallenger::MapVisibilityAlgorithmSelection_Simple)
    {
        if(GlobalServerData::serverSettings.mapVisibility.simple.max<5)
        {
            qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.mapVisibility.simple.max<5");
            GlobalServerData::serverSettings.mapVisibility.simple.max=5;
        }
        if(GlobalServerData::serverSettings.mapVisibility.simple.reshow<3)
        {
            qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.mapVisibility.simple.reshow<3");
            GlobalServerData::serverSettings.mapVisibility.simple.reshow=3;
        }

        if(GlobalServerData::serverSettings.mapVisibility.simple.reshow>GlobalServerData::serverSettings.max_players)
        {
            qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.mapVisibility.simple.reshow>GlobalServerData::serverSettings.max_players");
            GlobalServerData::serverSettings.mapVisibility.simple.reshow=GlobalServerData::serverSettings.max_players;
        }
        if(GlobalServerData::serverSettings.mapVisibility.simple.max>GlobalServerData::serverSettings.max_players)
        {
            qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.mapVisibility.simple.max>GlobalServerData::serverSettings.max_players");
            GlobalServerData::serverSettings.mapVisibility.simple.max=GlobalServerData::serverSettings.max_players;
        }
        if(GlobalServerData::serverSettings.mapVisibility.simple.reshow>GlobalServerData::serverSettings.mapVisibility.simple.max)
        {
            qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.mapVisibility.simple.reshow>GlobalServerData::serverSettings.mapVisibility.simple.max");
            GlobalServerData::serverSettings.mapVisibility.simple.reshow=GlobalServerData::serverSettings.mapVisibility.simple.max;
        }

        /*do the coding part...if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime>GlobalServerData::serverSettings.mapVisibility.simple.max)
            GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime=GlobalServerData::serverSettings.mapVisibility.simple.max;*/
    }
    else if(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm==CatchChallenger::MapVisibilityAlgorithmSelection_WithBorder)
    {
        if(GlobalServerData::serverSettings.mapVisibility.withBorder.maxWithBorder<3)
        {
            qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.mapVisibility.withBorder.maxWithBorder<3");
            GlobalServerData::serverSettings.mapVisibility.withBorder.maxWithBorder=3;
        }
        if(GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder<2)
        {
            qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder<2");
            GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder=2;
        }
        if(GlobalServerData::serverSettings.mapVisibility.withBorder.max<5)
        {
            qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.mapVisibility.withBorder.max<5");
            GlobalServerData::serverSettings.mapVisibility.withBorder.max=5;
        }
        if(GlobalServerData::serverSettings.mapVisibility.withBorder.reshow<3)
        {
            qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.mapVisibility.withBorder.reshow<3");
            GlobalServerData::serverSettings.mapVisibility.withBorder.reshow=3;
        }

        if(GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder>GlobalServerData::serverSettings.max_players)
        {
            qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder>GlobalServerData::serverSettings.max_players");
            GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder=GlobalServerData::serverSettings.max_players;
        }
        if(GlobalServerData::serverSettings.mapVisibility.withBorder.maxWithBorder>GlobalServerData::serverSettings.max_players)
        {
            qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.mapVisibility.withBorder.maxWithBorder>GlobalServerData::serverSettings.max_players");
            GlobalServerData::serverSettings.mapVisibility.withBorder.maxWithBorder=GlobalServerData::serverSettings.max_players;
        }
        if(GlobalServerData::serverSettings.mapVisibility.withBorder.reshow>GlobalServerData::serverSettings.max_players)
        {
            qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.mapVisibility.withBorder.reshow>GlobalServerData::serverSettings.max_players");
            GlobalServerData::serverSettings.mapVisibility.withBorder.reshow=GlobalServerData::serverSettings.max_players;
        }
        if(GlobalServerData::serverSettings.mapVisibility.withBorder.max>GlobalServerData::serverSettings.max_players)
        {
            qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.mapVisibility.withBorder.max>GlobalServerData::serverSettings.max_players");
            GlobalServerData::serverSettings.mapVisibility.withBorder.max=GlobalServerData::serverSettings.max_players;
        }

        if(GlobalServerData::serverSettings.mapVisibility.withBorder.reshow>GlobalServerData::serverSettings.mapVisibility.withBorder.max)
        {
            qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.mapVisibility.withBorder.reshow>GlobalServerData::serverSettings.mapVisibility.withBorder.max");
            GlobalServerData::serverSettings.mapVisibility.withBorder.reshow=GlobalServerData::serverSettings.mapVisibility.withBorder.max;
        }
        if(GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder>GlobalServerData::serverSettings.mapVisibility.withBorder.maxWithBorder)
        {
            qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder>GlobalServerData::serverSettings.mapVisibility.withBorder.maxWithBorder");
            GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder=GlobalServerData::serverSettings.mapVisibility.withBorder.maxWithBorder;
        }
        if(GlobalServerData::serverSettings.mapVisibility.withBorder.maxWithBorder>GlobalServerData::serverSettings.mapVisibility.withBorder.max)
        {
            qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.mapVisibility.withBorder.maxWithBorder>GlobalServerData::serverSettings.mapVisibility.withBorder.max");
            GlobalServerData::serverSettings.mapVisibility.withBorder.maxWithBorder=GlobalServerData::serverSettings.mapVisibility.withBorder.max;
        }
        if(GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder>GlobalServerData::serverSettings.mapVisibility.withBorder.reshow)
        {
            qDebug() << std::basic_string<char>Literal("GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder>GlobalServerData::serverSettings.mapVisibility.withBorder.reshow");
            GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder=GlobalServerData::serverSettings.mapVisibility.withBorder.reshow;
        }
    }

    if(GlobalServerData::serverSettings.secondToPositionSync==0)
    {
        #ifndef EPOLLCATCHCHALLENGERSERVER
        GlobalServerData::serverPrivateVariables.positionSync.stop();
        #endif
    }
    else
    {
        GlobalServerData::serverPrivateVariables.positionSync.start(GlobalServerData::serverSettings.secondToPositionSync*1000);
    }
    GlobalServerData::serverPrivateVariables.ddosTimer.start(GlobalServerData::serverSettings.ddos.computeAverageValueTimeInterval*1000);

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    switch(GlobalServerData::serverSettings.database_login.tryOpenType)
    {
        case CatchChallenger::DatabaseBase::DatabaseType::SQLite:
        case CatchChallenger::DatabaseBase::DatabaseType::Mysql:
        case CatchChallenger::DatabaseBase::DatabaseType::PostgreSQL:
        break;
        default:
            qDebug() << "Wrong db type";
            GlobalServerData::serverSettings.database_login.tryOpenType=CatchChallenger::DatabaseBase::DatabaseType::Mysql;
        break;
    }
    #endif
    switch(GlobalServerData::serverSettings.database_base.tryOpenType)
    {
        case CatchChallenger::DatabaseBase::DatabaseType::SQLite:
        case CatchChallenger::DatabaseBase::DatabaseType::Mysql:
        case CatchChallenger::DatabaseBase::DatabaseType::PostgreSQL:
        break;
        default:
            qDebug() << "Wrong db type";
            GlobalServerData::serverSettings.database_base.tryOpenType=CatchChallenger::DatabaseBase::DatabaseType::Mysql;
        break;
    }
    switch(GlobalServerData::serverSettings.database_common.tryOpenType)
    {
        case CatchChallenger::DatabaseBase::DatabaseType::SQLite:
        case CatchChallenger::DatabaseBase::DatabaseType::Mysql:
        case CatchChallenger::DatabaseBase::DatabaseType::PostgreSQL:
        break;
        default:
            qDebug() << "Wrong db type";
            GlobalServerData::serverSettings.database_common.tryOpenType=CatchChallenger::DatabaseBase::DatabaseType::Mysql;
        break;
    }
    switch(GlobalServerData::serverSettings.database_server.tryOpenType)
    {
        case CatchChallenger::DatabaseBase::DatabaseType::SQLite:
        case CatchChallenger::DatabaseBase::DatabaseType::Mysql:
        case CatchChallenger::DatabaseBase::DatabaseType::PostgreSQL:
        break;
        default:
            qDebug() << "Wrong db type";
            GlobalServerData::serverSettings.database_server.tryOpenType=CatchChallenger::DatabaseBase::DatabaseType::Mysql;
        break;
    }
    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
    {
        case CatchChallenger::MapVisibilityAlgorithmSelection_None:
        case CatchChallenger::MapVisibilityAlgorithmSelection_Simple:
        case CatchChallenger::MapVisibilityAlgorithmSelection_WithBorder:
        break;
        default:
            GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm=CatchChallenger::MapVisibilityAlgorithmSelection_Simple;
            qDebug() << "Wrong visibility algorithm";
        break;
    }

    switch(GlobalServerData::serverSettings.city.capture.frenquency)
    {
        case City::Capture::Frequency_week:
        case City::Capture::Frequency_month:
        break;
        default:
            GlobalServerData::serverSettings.city.capture.frenquency=City::Capture::Frequency_week;
            qDebug() << "Wrong City::Capture::Frequency";
        break;
    }
    switch(GlobalServerData::serverSettings.city.capture.day)
    {
        case City::Capture::Monday:
        case City::Capture::Tuesday:
        case City::Capture::Wednesday:
        case City::Capture::Thursday:
        case City::Capture::Friday:
        case City::Capture::Saturday:
        case City::Capture::Sunday:
        break;
        default:
            GlobalServerData::serverSettings.city.capture.day=City::Capture::Monday;
            qDebug() << "Wrong City::Capture::Day";
        break;
    }
    if(GlobalServerData::serverSettings.city.capture.hour>24)
    {
        qDebug() << "GlobalServerData::serverSettings.city.capture.hours out of range";
        GlobalServerData::serverSettings.city.capture.hour=0;
    }
    if(GlobalServerData::serverSettings.city.capture.minute>60)
    {
        qDebug() << "GlobalServerData::serverSettings.city.capture.minutes out of range";
        GlobalServerData::serverSettings.city.capture.minute=0;
    }

    if(GlobalServerData::serverSettings.compressionLevel<1 || GlobalServerData::serverSettings.compressionLevel>9)
        GlobalServerData::serverSettings.compressionLevel=6;
    switch(GlobalServerData::serverSettings.compressionType)
    {
        case CatchChallenger::CompressionType_None:
            GlobalServerData::serverSettings.compressionType      = CompressionType_None;
            ProtocolParsing::compressionTypeServer=ProtocolParsing::CompressionType::None;
        break;
        default:
        case CatchChallenger::CompressionType_Zlib:
            GlobalServerData::serverSettings.compressionType      = CompressionType_Zlib;
            ProtocolParsing::compressionTypeServer=ProtocolParsing::CompressionType::Zlib;
        break;
        case CatchChallenger::CompressionType_Xz:
            GlobalServerData::serverSettings.compressionType      = CompressionType_Xz;
            ProtocolParsing::compressionTypeServer=ProtocolParsing::CompressionType::Xz;
        break;
        case CatchChallenger::CompressionType_Lz4:
            GlobalServerData::serverSettings.compressionType      = CompressionType_Lz4;
            ProtocolParsing::compressionTypeServer=ProtocolParsing::CompressionType::Lz4;
        break;
    }
}
