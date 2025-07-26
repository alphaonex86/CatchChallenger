#include "BaseServer.hpp"
#include "../GlobalServerData.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/CommonMap.hpp"
#include "../../general/base/cpp11addition.hpp"
#include "../../general/base/ProtocolParsing.hpp"
#include <iostream>

using namespace CatchChallenger;

void BaseServer::loadAndFixSettings()
{
    GlobalServerData::serverPrivateVariables.server_message=stringsplit(GlobalServerData::serverSettings.server_message,'\n');
    while(GlobalServerData::serverPrivateVariables.server_message.size()>16)
    {
        std::cerr << "GlobalServerData::serverPrivateVariables.server_message too big, remove: " << GlobalServerData::serverPrivateVariables.server_message.at(GlobalServerData::serverPrivateVariables.server_message.size()-1) << std::endl;
        GlobalServerData::serverPrivateVariables.server_message.pop_back();
    }
    unsigned int index=0;
    while(index<GlobalServerData::serverPrivateVariables.server_message.size())
    {
        if(GlobalServerData::serverPrivateVariables.server_message.at(index).size()>128)
        {
            std::cerr << "GlobalServerData::serverPrivateVariables.server_message too big, truncate: " << GlobalServerData::serverPrivateVariables.server_message.at(index) << std::endl;
            GlobalServerData::serverPrivateVariables.server_message[index].resize(128);
        }
        index++;
    }

    //drop the empty \n at the begin
    bool removeTheLastList;
    do
    {
        removeTheLastList=GlobalServerData::serverPrivateVariables.server_message.size()>0 && GlobalServerData::serverPrivateVariables.server_message.back().size()==0;
        if(removeTheLastList)
            GlobalServerData::serverPrivateVariables.server_message.pop_back();
    } while(removeTheLastList);

    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    if(GlobalServerData::serverSettings.database_login.tryInterval<1)
    {
        std::cerr << "GlobalServerData::serverSettings.database_login.tryInterval<1" << std::endl;
        GlobalServerData::serverSettings.database_login.tryInterval=5;
    }
    if(GlobalServerData::serverSettings.database_login.considerDownAfterNumberOfTry<1)
    {
        std::cerr << "GlobalServerData::serverSettings.database_login.considerDownAfterNumberOfTry<1" << std::endl;
        GlobalServerData::serverSettings.database_login.considerDownAfterNumberOfTry=3;
    }
    if(GlobalServerData::serverSettings.database_login.tryInterval*GlobalServerData::serverSettings.database_login.considerDownAfterNumberOfTry>(60*10)/*10mins*/)
    {
        std::cerr << "GlobalServerData::serverSettings.database_login.tryInterval*GlobalServerData::serverSettings.database_login.considerDownAfterNumberOfTry>(60*10)" << std::endl;
        GlobalServerData::serverSettings.database_login.tryInterval=5;
        GlobalServerData::serverSettings.database_login.considerDownAfterNumberOfTry=3;
    }

    if(GlobalServerData::serverSettings.database_base.tryInterval<1)
    {
        std::cerr << "GlobalServerData::serverSettings.database_base.tryInterval<1" << std::endl;
        GlobalServerData::serverSettings.database_base.tryInterval=5;
    }
    if(GlobalServerData::serverSettings.database_base.considerDownAfterNumberOfTry<1)
    {
        std::cerr << "GlobalServerData::serverSettings.database_base.considerDownAfterNumberOfTry<1" << std::endl;
        GlobalServerData::serverSettings.database_base.considerDownAfterNumberOfTry=3;
    }
    if(GlobalServerData::serverSettings.database_base.tryInterval*GlobalServerData::serverSettings.database_base.considerDownAfterNumberOfTry>(60*10)/*10mins*/)
    {
        std::cerr << "GlobalServerData::serverSettings.database_base.tryInterval*GlobalServerData::serverSettings.database_base.considerDownAfterNumberOfTry>(60*10)" << std::endl;
        GlobalServerData::serverSettings.database_base.tryInterval=5;
        GlobalServerData::serverSettings.database_base.considerDownAfterNumberOfTry=3;
    }
    #endif

    if(GlobalServerData::serverSettings.database_common.tryInterval<1)
    {
        std::cerr << "GlobalServerData::serverSettings.database_common.tryInterval<1" << std::endl;
        GlobalServerData::serverSettings.database_common.tryInterval=5;
    }
    if(GlobalServerData::serverSettings.database_common.considerDownAfterNumberOfTry<1)
    {
        std::cerr << "GlobalServerData::serverSettings.database_common.considerDownAfterNumberOfTry<1" << std::endl;
        GlobalServerData::serverSettings.database_common.considerDownAfterNumberOfTry=3;
    }
    if(GlobalServerData::serverSettings.database_common.tryInterval*GlobalServerData::serverSettings.database_common.considerDownAfterNumberOfTry>(60*10)/*10mins*/)
    {
        std::cerr << "GlobalServerData::serverSettings.database_common.tryInterval*GlobalServerData::serverSettings.database_common.considerDownAfterNumberOfTry>(60*10)" << std::endl;
        GlobalServerData::serverSettings.database_common.tryInterval=5;
        GlobalServerData::serverSettings.database_common.considerDownAfterNumberOfTry=3;
    }

    if(GlobalServerData::serverSettings.database_server.tryInterval<1)
    {
        std::cerr << "GlobalServerData::serverSettings.database_server.tryInterval<1" << std::endl;
        GlobalServerData::serverSettings.database_server.tryInterval=5;
    }
    if(GlobalServerData::serverSettings.database_server.considerDownAfterNumberOfTry<1)
    {
        std::cerr << "GlobalServerData::serverSettings.database_server.considerDownAfterNumberOfTry<1" << std::endl;
        GlobalServerData::serverSettings.database_server.considerDownAfterNumberOfTry=3;
    }
    if(GlobalServerData::serverSettings.database_server.tryInterval*GlobalServerData::serverSettings.database_server.considerDownAfterNumberOfTry>(60*10)/*10mins*/)
    {
        std::cerr << "GlobalServerData::serverSettings.database_server.tryInterval*GlobalServerData::serverSettings.database_server.considerDownAfterNumberOfTry>(60*10)" << std::endl;
        GlobalServerData::serverSettings.database_server.tryInterval=5;
        GlobalServerData::serverSettings.database_server.considerDownAfterNumberOfTry=3;
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif

    if(CommonSettingsServer::commonSettingsServer.mainDatapackCode.size()==0)
    {
        std::cerr << "mainDatapackCode is empty, please put it into the settings" << std::endl;
        abort();
    }
    if(!regex_search(CommonSettingsServer::commonSettingsServer.mainDatapackCode, std::regex(CATCHCHALLENGER_CHECK_MAINDATAPACKCODE)))
    {
        std::cerr << "CommonSettingsServer::commonSettingsServer.mainDatapackCode not match CATCHCHALLENGER_CHECK_MAINDATAPACKCODE "
                  << CommonSettingsServer::commonSettingsServer.mainDatapackCode
                  << " not contain "
                  << CATCHCHALLENGER_CHECK_MAINDATAPACKCODE << std::endl;
        abort();
    }
    if(!CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
    {
        if(!regex_search(CommonSettingsServer::commonSettingsServer.subDatapackCode,std::regex(CATCHCHALLENGER_CHECK_SUBDATAPACKCODE)))
        {
            std::cerr << "CommonSettingsServer::commonSettingsServer.subDatapackCode " << CommonSettingsServer::commonSettingsServer.subDatapackCode << " not match " << CATCHCHALLENGER_CHECK_SUBDATAPACKCODE << std::endl;
            abort();
        }
    }
    const std::string &mainDir=GlobalServerData::serverSettings.datapack_basePath+std::string("map/main/")+CommonSettingsServer::commonSettingsServer.mainDatapackCode+std::string("/");
    if(!FacilityLibGeneral::isDir(mainDir))
    {
        std::cerr << mainDir << " don't exists" << std::endl;
        abort();
    }
    if(!CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
    {
        const std::string &subDatapackFolder=GlobalServerData::serverSettings.datapack_basePath+std::string("map/main/")+CommonSettingsServer::commonSettingsServer.mainDatapackCode+std::string("/")+
                std::string("sub/")+CommonSettingsServer::commonSettingsServer.subDatapackCode+std::string("/");
        if(!FacilityLibGeneral::isDir(subDatapackFolder))
        {
            std::cerr << subDatapackFolder << " don't exists, drop spec" << std::endl;
            CommonSettingsServer::commonSettingsServer.subDatapackCode.clear();
        }
    }

    if(GlobalServerData::serverSettings.ddos.computeAverageValueTimeInterval<1)
    {
        std::cerr << "GlobalServerData::serverSettings.ddos.computeAverageValueTimeInterval<1" << std::endl;
        GlobalServerData::serverSettings.ddos.computeAverageValueTimeInterval=1;
    }

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    /*if(CommonSettingsCommon::commonSettingsCommon.min_character<0)
    {
        std::cerr << "CommonSettingsCommon::commonSettingsCommon.min_character<0" << std::endl;
        CommonSettingsCommon::commonSettingsCommon.min_character=0;
    }*/
    if(CommonSettingsCommon::commonSettingsCommon.max_character<1)
    {
        std::cerr << "CommonSettingsCommon::commonSettingsCommon.max_character<1" << std::endl;
        CommonSettingsCommon::commonSettingsCommon.max_character=1;
    }
    if(CommonSettingsCommon::commonSettingsCommon.max_character<CommonSettingsCommon::commonSettingsCommon.min_character)
    {
        std::cerr << "CommonSettingsCommon::commonSettingsCommon.max_character<CommonSettingsCommon::commonSettingsCommon.min_character" << std::endl;
        CommonSettingsCommon::commonSettingsCommon.max_character=CommonSettingsCommon::commonSettingsCommon.min_character;
    }
    if(CommonSettingsCommon::commonSettingsCommon.character_delete_time<=0)
    {
        std::cerr << "CommonSettingsCommon::commonSettingsCommon.character_delete_time<=0" << std::endl;
        CommonSettingsCommon::commonSettingsCommon.character_delete_time=7*24*3600;
    }
    #endif

    if(GlobalServerData::serverSettings.datapackCache<-1)
    {
        std::cerr << "GlobalServerData::serverSettings.datapackCache<-1" << std::endl;
        GlobalServerData::serverSettings.datapackCache=-1;
    }
    {
        std::vector<std::string> newMirrorList;
        std::regex httpMatch("^https?://.+$");
        const std::vector<std::string> &mirrorList=stringsplit(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer,';');
        unsigned int index=0;
        while(index<mirrorList.size())
        {
            const std::string &mirror=mirrorList.at(index);
            if(!regex_search(mirror,httpMatch))
            {}//std::cerr << "Mirror wrong: " << mirror.toLocal8Bit() << std::endl; -> single player
            else
            {
                if(stringEndsWith(mirror,"/"))
                    newMirrorList.push_back(mirror);
                else
                    newMirrorList.push_back(mirror+"/");
            }
            index++;
        }
        CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer=stringimplode(newMirrorList,';');
        CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase=CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer;
    }

    //check the settings here
    if(GlobalServerData::serverSettings.max_players<1)
    {
        std::cerr << "GlobalServerData::serverSettings.max_players<1" << std::endl;
        GlobalServerData::serverSettings.max_players=200;
    }
    if(GlobalServerData::serverSettings.max_players>65533)
    {
        std::cerr << "GlobalServerData::serverSettings.max_players>65533: " << GlobalServerData::serverSettings.max_players << std::endl;
        GlobalServerData::serverSettings.max_players=65533;
    }
    ProtocolParsing::setMaxPlayers(GlobalServerData::serverSettings.max_players);

/*    uint8_t player_list_size;
    if(GlobalServerData::serverSettings.max_players<=255)
        player_list_size=sizeof(uint8_t);
    else
        player_list_size=sizeof(uint16_t);*/
    uint8_t map_list_size;
    if(CommonMap::flat_map_list_count<=255)
        map_list_size=sizeof(uint8_t);
    else if(CommonMap::flat_map_list_count<=65534)
        map_list_size=sizeof(uint16_t);
    else
    {
        std::cerr << "too many map, you need less than 65534 map count" << std::endl;
        abort();
    }
    GlobalServerData::serverPrivateVariables.sizeofInsertRequest=
            //mutualised
            sizeof(uint8_t)+map_list_size+/*player_list_size same with move, delete, ...*/
            //of the player
            /*player_list_size same with move, delete, ...*/+sizeof(uint8_t)+sizeof(uint8_t)+sizeof(uint8_t)+sizeof(uint8_t)+sizeof(uint8_t)+0/*pseudo size put directy*/+sizeof(uint8_t);
    if(GlobalServerData::serverSettings.mapVisibility.simple.max<5)
    {
        std::cerr << "GlobalServerData::serverSettings.mapVisibility.simple.max<5" << std::endl;
        GlobalServerData::serverSettings.mapVisibility.simple.max=5;
    }
    if(GlobalServerData::serverSettings.mapVisibility.simple.reshow<3)
    {
        std::cerr << "GlobalServerData::serverSettings.mapVisibility.simple.reshow<3" << std::endl;
        GlobalServerData::serverSettings.mapVisibility.simple.reshow=3;
    }

    if(GlobalServerData::serverSettings.mapVisibility.simple.reshow>GlobalServerData::serverSettings.max_players)
    {
        std::cerr << "GlobalServerData::serverSettings.mapVisibility.simple.reshow>GlobalServerData::serverSettings.max_players" << std::endl;
        GlobalServerData::serverSettings.mapVisibility.simple.reshow=GlobalServerData::serverSettings.max_players;
    }
    if(GlobalServerData::serverSettings.mapVisibility.simple.max>GlobalServerData::serverSettings.max_players)
    {
        std::cerr << "GlobalServerData::serverSettings.mapVisibility.simple.max>GlobalServerData::serverSettings.max_players" << std::endl;
        GlobalServerData::serverSettings.mapVisibility.simple.max=GlobalServerData::serverSettings.max_players;
    }
    if(GlobalServerData::serverSettings.mapVisibility.simple.reshow>GlobalServerData::serverSettings.mapVisibility.simple.max)
    {
        std::cerr << "GlobalServerData::serverSettings.mapVisibility.simple.reshow>GlobalServerData::serverSettings.mapVisibility.simple.max" << std::endl;
        GlobalServerData::serverSettings.mapVisibility.simple.reshow=GlobalServerData::serverSettings.mapVisibility.simple.max;
    }

    /*do the coding part...if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime>GlobalServerData::serverSettings.mapVisibility.simple.max)
        GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime=GlobalServerData::serverSettings.mapVisibility.simple.max;*/

    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    switch(GlobalServerData::serverSettings.database_login.tryOpenType)
    {
        #if defined(CATCHCHALLENGER_DB_SQLITE) || defined(CATCHCHALLENGER_CLASS_QT)
        case CatchChallenger::DatabaseBase::DatabaseType::SQLite:
        #endif
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_CLASS_QT)
        case CatchChallenger::DatabaseBase::DatabaseType::Mysql:
        #endif
        #if defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_CLASS_QT)
        case CatchChallenger::DatabaseBase::DatabaseType::PostgreSQL:
        #endif
        break;
        default:
            std::cerr << "Wrong db type, line: " << __LINE__ << std::endl;
            GlobalServerData::serverSettings.database_login.tryOpenType=CatchChallenger::DatabaseBase::DatabaseType::Unknown;
            abort();
        break;
    }
    switch(GlobalServerData::serverSettings.database_base.tryOpenType)
    {
        #if defined(CATCHCHALLENGER_DB_SQLITE) || defined(CATCHCHALLENGER_CLASS_QT)
        case CatchChallenger::DatabaseBase::DatabaseType::SQLite:
        #endif
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_CLASS_QT)
        case CatchChallenger::DatabaseBase::DatabaseType::Mysql:
        #endif
        #if defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_CLASS_QT)
        case CatchChallenger::DatabaseBase::DatabaseType::PostgreSQL:
        #endif
        break;
        default:
            std::cerr << "Wrong db type, line: " << __LINE__ << std::endl;
            GlobalServerData::serverSettings.database_base.tryOpenType=CatchChallenger::DatabaseBase::DatabaseType::Unknown;
            abort();
        break;
    }
    #endif
    switch(GlobalServerData::serverSettings.database_common.tryOpenType)
    {
        #if defined(CATCHCHALLENGER_DB_SQLITE) || defined(CATCHCHALLENGER_CLASS_QT)
        case CatchChallenger::DatabaseBase::DatabaseType::SQLite:
        #endif
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_CLASS_QT)
        case CatchChallenger::DatabaseBase::DatabaseType::Mysql:
        #endif
        #if defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_CLASS_QT)
        case CatchChallenger::DatabaseBase::DatabaseType::PostgreSQL:
        #endif
        break;
        default:
            std::cerr << "Wrong db type, line: " << __LINE__ << std::endl;
            GlobalServerData::serverSettings.database_common.tryOpenType=CatchChallenger::DatabaseBase::DatabaseType::Unknown;
            abort();
        break;
    }
    switch(GlobalServerData::serverSettings.database_server.tryOpenType)
    {
        #if defined(CATCHCHALLENGER_DB_SQLITE) || defined(CATCHCHALLENGER_CLASS_QT)
        case CatchChallenger::DatabaseBase::DatabaseType::SQLite:
        #endif
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_CLASS_QT)
        case CatchChallenger::DatabaseBase::DatabaseType::Mysql:
        #endif
        #if defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_CLASS_QT)
        case CatchChallenger::DatabaseBase::DatabaseType::PostgreSQL:
        #endif
        break;
        default:
            std::cerr << "Wrong db type, line: " << __LINE__ << std::endl;
            GlobalServerData::serverSettings.database_server.tryOpenType=CatchChallenger::DatabaseBase::DatabaseType::Unknown;
            abort();
        break;
    }
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
    
    switch(GlobalServerData::serverSettings.city.capture.frenquency)
    {
        case City::Capture::Frequency_week:
        case City::Capture::Frequency_month:
        break;
        default:
            GlobalServerData::serverSettings.city.capture.frenquency=City::Capture::Frequency_week;
            std::cerr << "Wrong City::Capture::Frequency";
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
            std::cerr << "Wrong City::Capture::Day" << std::endl;
        break;
    }
    if(GlobalServerData::serverSettings.city.capture.hour>24)
    {
        std::cerr << "GlobalServerData::serverSettings.city.capture.hours out of range" << std::endl;
        GlobalServerData::serverSettings.city.capture.hour=0;
    }
    if(GlobalServerData::serverSettings.city.capture.minute>60)
    {
        std::cerr << "GlobalServerData::serverSettings.city.capture.minutes out of range" << std::endl;
        GlobalServerData::serverSettings.city.capture.minute=0;
    }

    if(CompressionProtocol::compressionLevel<1 || CompressionProtocol::compressionLevel>9)
        CompressionProtocol::compressionLevel=6;
    switch(GlobalServerData::serverSettings.compressionType)
    {
        case CompressionProtocol::CompressionType::None:
            GlobalServerData::serverSettings.compressionType      = CompressionProtocol::CompressionType::None;
            CompressionProtocol::compressionTypeServer=CompressionProtocol::CompressionType::None;
        break;
        default:
    case CompressionProtocol::CompressionType::Zstandard:
            GlobalServerData::serverSettings.compressionType      = CompressionProtocol::CompressionType::Zstandard;
            CompressionProtocol::compressionTypeServer=CompressionProtocol::CompressionType::Zstandard;
        break;
    }
}
