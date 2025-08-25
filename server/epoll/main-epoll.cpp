#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/tcp.h>
#ifdef CATCHCHALLENGER_DB_FILE
#include <sys/stat.h>
#warning dictionary_serialBuffer and server_serialBuffer not open as DB
#endif
#include "../base/ServerStructures.hpp"
#include "../base/TinyXMLSettings.hpp"
#include "../base/GlobalServerData.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/FacilityLib.hpp"
#include "EpollServer.hpp"
#include "Epoll.hpp"
#include "ClientMapManagementEpoll.hpp"
#ifdef CATCHCHALLENGER_DB_POSTGRESQL
#include "db/EpollPostgresql.hpp"
#endif
#ifdef CATCHCHALLENGER_DB_MYSQL
#include "db/EpollMySQL.hpp"
#endif
#include "../base/NormalServerGlobal.hpp"
#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "../game-server-alone/LinkToMaster.hpp"
#include "EpollSocket.hpp"
#include "timer/TimerPurgeTokenAuthList.hpp"
#endif

#include "timer/TimerCityCapture.hpp"
#include "timer/TimerDdos.hpp"
#include "timer/TimerPositionSync.hpp"
#include "timer/TimerSendInsertMoveRemove.hpp"
#include "timer/PlayerUpdaterEpoll.hpp"

#define MAXEVENTS 512

using namespace CatchChallenger;

#ifdef SERVERSSL
EpollSslServer *server=NULL;
#else
EpollServer *server=NULL;
#endif
TinyXMLSettings *settings=NULL;

std::string master_host;
uint16_t master_port;
uint8_t master_tryInterval;
uint8_t master_considerDownAfterNumberOfTry;

#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
void generateTokenStatClient(TinyXMLSettings &settings,char * const data)
{
    FILE *fpRandomFile = fopen(RANDOMFILEDEVICE,"rb");
    if(fpRandomFile==NULL)
    {
        std::cerr << "Unable to open " << RANDOMFILEDEVICE << " to generate random token" << std::endl;
        abort();
    }
    const size_t &returnedSize=fread(data,1,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT,fpRandomFile);
    if(returnedSize!=TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)
    {
        std::cerr << "Unable to read the " << TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT << " needed to do the token from " << RANDOMFILEDEVICE << std::endl;
        abort();
    }
    settings.setValue("token",binarytoHexa(reinterpret_cast<char *>(data)
                                           ,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT).c_str());
    fclose(fpRandomFile);
    settings.sync();
}
#endif

std::vector<void *> elementsToDelete[16];
size_t elementsToDeleteSize=0;
uint8_t elementsToDeleteIndex=0;

/* Catch Signal Handler functio */
void signal_callback_handler(int signum){

        printf("Caught signal SIGPIPE %d\n",signum);
}

void CatchChallenger::recordDisconnectByServer(void * client)
{
    unsigned int mainIndex=0;
    while(mainIndex<16)
    {
        const std::vector<void *> &elementsToDeleteSub=elementsToDelete[mainIndex];
        if(!elementsToDeleteSub.empty())
        {
            unsigned int index=0;
            while(index<elementsToDeleteSub.size())
            {
                if(elementsToDeleteSub.at(index)==client)
                    return;
                index++;
            }
        }
        mainIndex++;
    }
    elementsToDelete[elementsToDeleteIndex].push_back(client);
    elementsToDeleteSize++;
}

void send_settings(
        #ifdef SERVERSSL
        EpollSslServer *server
        #else
        EpollServer *server
        #endif
        ,TinyXMLSettings *settings,
        std::string &master_host,
        uint16_t &master_port,
        uint8_t &master_tryInterval,
        uint8_t &master_considerDownAfterNumberOfTry
        );

int main(int argc, char *argv[])
{
    memset(ProtocolParsingBase::tempBigBufferForOutput,0,sizeof(ProtocolParsingBase::tempBigBufferForOutput));
    /* Catch Signal Handler SIGPIPE */
    if(signal(SIGPIPE, signal_callback_handler)==SIG_ERR)
    {
        std::cerr << "signal(SIGPIPE, signal_callback_handler)==SIG_ERR, errno: " << std::to_string(errno) << std::endl;
        abort();
    }

    NormalServerGlobal::displayInfo();
    if(argc<1)
    {
        std::cerr << "argc<1: wrong arg count" << std::endl;
        return EXIT_FAILURE;
    }
    CatchChallenger::FacilityLibGeneral::applicationDirPath=argv[0];

    srand(static_cast<unsigned int>(time(NULL)));

    #ifdef CATCHCHALLENGER_DB_FILE
    ::mkdir("database",0700);
    ::mkdir("database/accounts",0700);
    ::mkdir("database/characters",0700);
    ::mkdir("database/clans",0700);
    #endif

    #ifndef CATCHCHALLENGER_DB_BLACKHOLE
    #ifndef CATCHCHALLENGER_DB_FILE
    bool datapack_loaded=false;
    #endif
    #endif

    #ifdef SERVERSSL
    server=new EpollSslServer();
    #else
    server=new EpollServer();
    #endif

    #ifdef CATCHCHALLENGER_CACHE_HPS
    const bool save=argc==2 && (strcmp(argv[1],"save")==0 || strcmp(argv[1],"--save")==0 || strcmp(argv[1],"-s")==0);
    if(save)
    {
        const std::string &datapackCache=FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/datapack-cache.bin.tmp";
        server->setSave(datapackCache);
    }
    else
    {
        const std::string &datapackCache=FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/datapack-cache.bin";
        server->setLoad(datapackCache);
    }
    #endif

    #ifdef CATCHCHALLENGER_CACHE_HPS
    if(save || !server->binaryInputCacheIsOpen())
    #endif
        if(!CatchChallenger::FacilityLibGeneral::isFile(FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/datapack/informations.xml"))
        {
            std::cerr << "No datapack found into: " << FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath) << "/datapack/" << std::endl;
            return EXIT_FAILURE;
        }

    #if defined(CATCHCHALLENGER_CACHE_HPS) && !defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER)
    if(save || !server->binaryCacheIsOpen())
    #endif
    {
        settings=new TinyXMLSettings(FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/server-properties.xml");
        NormalServerGlobal::checkSettingsFile(settings,FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/datapack/");
    }

    if(!Epoll::epoll.init())
        return EPOLLERR;

    //before linkToMaster->registerGameServer() to have the correct settings loaded
    //after server to have the settings
    #ifdef CATCHCHALLENGER_CACHE_HPS
    if(!save && server->binaryInputCacheIsOpen())
        server->setNormalSettings(server->loadSettingsFromBinaryCache(master_host,master_port,master_tryInterval,master_considerDownAfterNumberOfTry));
    else
    #endif
        send_settings(server,settings,master_host,master_port,master_tryInterval,master_considerDownAfterNumberOfTry);

    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    {
        const int &linkfd=LinkToMaster::tryConnect(
                    master_host.c_str(),
                    master_port,
                    master_tryInterval,
                    master_considerDownAfterNumberOfTry
                    );
        if(linkfd<0)
        {
            std::cerr << "Unable to connect on master" << std::endl;
            abort();
        }
        #ifdef SERVERSSL
        ctx from what?
        LoginLinkToMaster::loginLinkToMaster=new LoginLinkToMaster(linkfd,ctx);
        #else
        LinkToMaster::linkToMaster=new LinkToMaster(linkfd);
        #endif
        LinkToMaster::linkToMaster->stat=LinkToMaster::Stat::Connected;
        LinkToMaster::linkToMaster->setSettings(settings);
        LinkToMaster::linkToMaster->readTheFirstSslHeader();
        LinkToMaster::linkToMaster->setConnexionSettings(master_tryInterval,master_considerDownAfterNumberOfTry);
        const int &s = EpollSocket::make_non_blocking(linkfd);
        if(s == -1)
            std::cerr << "unable to make to socket non blocking" << std::endl;

        LinkToMaster::linkToMaster->baseServer=server;
    }
    #endif

    {
        const GameServerSettings &formatedServerSettings=server->getSettings();
        const NormalServerSettings &formatedServerNormalSettings=server->getNormalSettings();

        if(!formatedServerNormalSettings.proxy.empty())
        {
            std::cerr << "Proxy not supported: " << formatedServerNormalSettings.proxy << std::endl;
            return EXIT_FAILURE;
        }
        if(!formatedServerNormalSettings.proxy.empty())
        {
            std::cerr << "Proxy not supported" << std::endl;
            return EXIT_FAILURE;
        }
        {
            #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
            #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
            if(
                    true
                    #ifdef CATCHCHALLENGER_DB_POSTGRESQL
                    && formatedServerSettings.database_login.tryOpenType!=DatabaseBase::DatabaseType::PostgreSQL
                    #endif
                    #ifdef CATCHCHALLENGER_DB_MYSQL
                    && formatedServerSettings.database_login.tryOpenType!=DatabaseBase::DatabaseType::Mysql
                    #endif
                    )
            {
                settings->beginGroup("db-login");
                std::cerr << "Database type not supported for now: " << settings->value("type") << std::endl;
                settings->endGroup();
                return EXIT_FAILURE;
            }
            if(
                    true
                    #ifdef CATCHCHALLENGER_DB_POSTGRESQL
                    && formatedServerSettings.database_base.tryOpenType!=DatabaseBase::DatabaseType::PostgreSQL
                    #endif
                    #ifdef CATCHCHALLENGER_DB_MYSQL
                    && formatedServerSettings.database_base.tryOpenType!=DatabaseBase::DatabaseType::Mysql
                    #endif
                    )
            {
                settings->beginGroup("db-base");
                std::cerr << "Database type not supported for now: " << settings->value("type") << std::endl;
                settings->endGroup();
                return EXIT_FAILURE;
            }
            #endif
            if(
                    true
                    #ifdef CATCHCHALLENGER_DB_POSTGRESQL
                    && formatedServerSettings.database_common.tryOpenType!=DatabaseBase::DatabaseType::PostgreSQL
                    #endif
                    #ifdef CATCHCHALLENGER_DB_MYSQL
                    && formatedServerSettings.database_common.tryOpenType!=DatabaseBase::DatabaseType::Mysql
                    #endif
                    )
            {
                settings->beginGroup("db-common");
                std::cerr << "Database type not supported for now: " << settings->value("type") << std::endl;
                settings->endGroup();
                return EXIT_FAILURE;
            }
            if(
                    true
                    #ifdef CATCHCHALLENGER_DB_POSTGRESQL
                    && formatedServerSettings.database_server.tryOpenType!=DatabaseBase::DatabaseType::PostgreSQL
                    #endif
                    #ifdef CATCHCHALLENGER_DB_MYSQL
                    && formatedServerSettings.database_server.tryOpenType!=DatabaseBase::DatabaseType::Mysql
                    #endif
                    )
            {
                settings->beginGroup("db-server");
                std::cerr << "Database type not supported for now: " << settings->value("type") << std::endl;
                settings->endGroup();
                return EXIT_FAILURE;
            }
            #elif CATCHCHALLENGER_DB_BLACKHOLE
            #elif CATCHCHALLENGER_DB_FILE
            #else
            #error Define what do here
            #endif
        }


        #ifdef SERVERSSL
        if(!formatedServerNormalSettings.useSsl)
        {
            qDebug() << "Ssl connexion requested but server not compiled with ssl support!";
            return EXIT_FAILURE;
        }
        #else
        if(formatedServerNormalSettings.useSsl)
        {
            std::cerr << "Clear connexion requested but server compiled with ssl support!" << std::endl;
            return EXIT_FAILURE;
        }
        #endif
        if(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.empty())
        {
            #ifdef CATCHCHALLENGERSERVERBLOCKCLIENTTOSERVERPACKETDECOMPRESSION
            qDebug() << "Need mirror because CATCHCHALLENGERSERVERBLOCKCLIENTTOSERVERPACKETDECOMPRESSION is def, need decompression to datapack list input";
            return EXIT_FAILURE;
            #endif
            #ifdef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
            std::cerr << "CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR defined, httpDatapackMirrorBase can't be empty" << std::endl;
            return EXIT_FAILURE;
            #endif
        }
        else
        {
            std::vector<std::string> newMirrorList;
            std::regex httpMatch("^https?://.+$");
            const std::vector<std::string> &mirrorList=stringsplit(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer,';');
            unsigned int index=0;
            while(index<mirrorList.size())
            {
                const std::string &mirror=mirrorList.at(index);
                if(!regex_search(mirror,httpMatch))
                {
                    std::cerr << "Mirror wrong: " << mirror << std::endl;
                    return EXIT_FAILURE;
                }
                if(stringEndsWith(mirror,'/'))
                    newMirrorList.push_back(mirror);
                else
                    newMirrorList.push_back(mirror+'/');
                index++;
            }
            CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer=stringimplode(newMirrorList,';');
            CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase=CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer;
        }
    }
    server->initTheDatabase();
    server->loadAndFixSettings();

    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    switch(GlobalServerData::serverSettings.database_login.tryOpenType)
    {
        #ifdef CATCHCHALLENGER_DB_POSTGRESQL
        case DatabaseBase::DatabaseType::PostgreSQL:
            static_cast<EpollPostgresql *>(GlobalServerData::serverPrivateVariables.db_login)->setMaxDbQueries(2000000000);
        break;
        #endif
        default:
        break;
    }
    switch(GlobalServerData::serverSettings.database_base.tryOpenType)
    {
        #ifdef CATCHCHALLENGER_DB_POSTGRESQL
        case DatabaseBase::DatabaseType::PostgreSQL:
            static_cast<EpollPostgresql *>(GlobalServerData::serverPrivateVariables.db_base)->setMaxDbQueries(2000000000);
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
            static_cast<EpollPostgresql *>(GlobalServerData::serverPrivateVariables.db_common)->setMaxDbQueries(2000000000);
        break;
        #endif
        default:
        break;
    }
    switch(GlobalServerData::serverSettings.database_server.tryOpenType)
    {
        #ifdef CATCHCHALLENGER_DB_POSTGRESQL
        case DatabaseBase::DatabaseType::PostgreSQL:
            static_cast<EpollPostgresql *>(GlobalServerData::serverPrivateVariables.db_server)->setMaxDbQueries(2000000000);
        break;
        #endif
        default:
        break;
    }

    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    if(!GlobalServerData::serverPrivateVariables.db_login->syncConnect(
                GlobalServerData::serverSettings.database_login.host,
                GlobalServerData::serverSettings.database_login.db,
                GlobalServerData::serverSettings.database_login.login,
                GlobalServerData::serverSettings.database_login.pass))
    {
        std::cerr << "Unable to connect to database login:" << GlobalServerData::serverPrivateVariables.db_login->errorMessage() << std::endl;
        return EXIT_FAILURE;
    }
    if(!GlobalServerData::serverPrivateVariables.db_base->syncConnect(
                GlobalServerData::serverSettings.database_base.host,
                GlobalServerData::serverSettings.database_base.db,
                GlobalServerData::serverSettings.database_base.login,
                GlobalServerData::serverSettings.database_base.pass))
    {
        std::cerr << "Unable to connect to database base:" << GlobalServerData::serverPrivateVariables.db_base->errorMessage() << std::endl;
        return EXIT_FAILURE;
    }
    #endif
    if(!GlobalServerData::serverPrivateVariables.db_common->syncConnect(
                GlobalServerData::serverSettings.database_common.host,
                GlobalServerData::serverSettings.database_common.db,
                GlobalServerData::serverSettings.database_common.login,
                GlobalServerData::serverSettings.database_common.pass))
    {
        std::cerr << "Unable to connect to database common:" << GlobalServerData::serverPrivateVariables.db_common->errorMessage() << std::endl;
        return EXIT_FAILURE;
    }
    if(!GlobalServerData::serverPrivateVariables.db_server->syncConnect(
                GlobalServerData::serverSettings.database_server.host,
                GlobalServerData::serverSettings.database_server.db,
                GlobalServerData::serverSettings.database_server.login,
                GlobalServerData::serverSettings.database_server.pass))
    {
        std::cerr << "Unable to connect to database server:" << GlobalServerData::serverPrivateVariables.db_server->errorMessage() << std::endl;
        return EXIT_FAILURE;
    }
    server->initialize_the_database_prepared_query();
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    server->preload_1_the_data();
    #elif CATCHCHALLENGER_DB_FILE
    server->preload_1_the_data();
    #else
    #error Define what do here
    #endif

    GlobalServerData::serverPrivateVariables.player_updater=new PlayerUpdaterEpoll();
    TimerCityCapture timerCityCapture;
    TimerDdos timerDdos;
    TimerPositionSync timerPositionSync;
    TimerSendInsertMoveRemove timerSendInsertMoveRemove;
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    TimerPurgeTokenAuthList timerPurgeTokenAuthList;
    #endif
    {
        GlobalServerData::serverPrivateVariables.time_city_capture=CatchChallenger::FacilityLib::nextCaptureTime(GlobalServerData::serverSettings.city);
        const int64_t &time=GlobalServerData::serverPrivateVariables.time_city_capture-sFrom1970();
        timerCityCapture.setSingleShot(true);
        if(!timerCityCapture.start(time))
        {
            std::cerr << "timerCityCapture fail to set" << std::endl;
            return EXIT_FAILURE;
        }
    }
    {
        if(!timerDdos.start(GlobalServerData::serverSettings.ddos.computeAverageValueTimeInterval*1000))
        {
            std::cerr << "timerDdos fail to set" << std::endl;
            return EXIT_FAILURE;
        }
    }
    {
        if(GlobalServerData::serverSettings.secondToPositionSync>0)
            if(!timerPositionSync.start(GlobalServerData::serverSettings.secondToPositionSync*1000))
            {
                std::cerr << "timerPositionSync fail to set" << std::endl;
                return EXIT_FAILURE;
            }
    }
    {
        if(!timerSendInsertMoveRemove.start(CATCHCHALLENGER_SERVER_MAP_TIME_TO_SEND_MOVEMENT))
        {
            std::cerr << "timerSendInsertMoveRemove fail to set" << std::endl;
            return EXIT_FAILURE;
        }
    }
    {
        if(GlobalServerData::serverSettings.sendPlayerNumber)
        {
            if(!static_cast<PlayerUpdaterEpoll *>(GlobalServerData::serverPrivateVariables.player_updater)->start())
            {
                std::cerr << "player_updater timer fail to set" << std::endl;
                return EXIT_FAILURE;
            }
            #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
            if(!PlayerUpdaterToMaster::player_updater_to_master.start())
            {
                std::cerr << "player_updater_to_master timer fail to set" << std::endl;
                return EXIT_FAILURE;
            }
            #endif
        }
    }
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    {
        if(!timerPurgeTokenAuthList.start(LinkToMaster::purgeLockPeriod))
        {
            std::cerr << "timerPurgeTokenAuthList fail to set" << std::endl;
            return EXIT_FAILURE;
        }
    }
    #endif
    delete settings;

    char buf[4096];
    memset(buf,0,4096);
    /* Buffer where events are returned */
    epoll_event events[MAXEVENTS];

    char encodingBuff[1];
    #ifdef SERVERSSL
    encodingBuff[0]=0x01;
    #else
    encodingBuff[0]=0x00;
    #endif

    bool acceptSocketWarningShow=false;
    int numberOfConnectedClient=0;
    /* The event loop */
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    unsigned int clientnumberToDebug=0;
    #endif
    int number_of_events, i;
    while(1)
    {

        number_of_events = Epoll::epoll.wait(events, MAXEVENTS);
        if(elementsToDeleteSize>0 && number_of_events<MAXEVENTS)
        {
            if(elementsToDeleteIndex>=15)
                elementsToDeleteIndex=0;
            else
                ++elementsToDeleteIndex;
            const std::vector<void *> &elementsToDeleteSub=elementsToDelete[elementsToDeleteIndex];
            if(!elementsToDeleteSub.empty())
            {
                unsigned int index=0;
                while(index<elementsToDeleteSub.size())
                {
                    Client * c=static_cast<Client *>(elementsToDeleteSub.at(index));
#ifdef CATCHCHALLENGER_DB_FILE
const std::string &hexa=binarytoHexa(c->public_and_private_informations.public_informations.pseudo.c_str(),
                                     c->public_and_private_informations.public_informations.pseudo.size());
std::ofstream out_file("characters/"+hexa, std::ofstream::binary);

if(c->getPlayerId()>0)
    hps::to_stream(*c, out_file);
#endif
                    delete c;
                    index++;
                }
            }
            elementsToDeleteSize-=elementsToDeleteSub.size();
            elementsToDelete[elementsToDeleteIndex].clear();
        }
        #ifdef SERVERBENCHMARK
        EpollUnixSocketClientFinal::start = std::chrono::high_resolution_clock::now();
        #endif
        for(i = 0; i < number_of_events; i++)
        {
            #ifdef PROTOCOLPARSINGDEBUG
            //std::cout << "new event: " << events[i].data.ptr << " event: " << events[i].events << std::endl;
            #endif
            switch(static_cast<BaseClassSwitch *>(events[i].data.ptr)->getType())
            {
                case BaseClassSwitch::EpollObjectType::Server:
                {
                    if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occured on this fd, or the socket is not
                        ready for reading (why were we notified then?) */
                        std::cerr << "server epoll error" << std::endl;
                        continue;
                    }
                    /* We have a notification on the listening socket, which
                    means one or more incoming connections. */
                    while(1)
                    {
                        sockaddr in_addr;
                        socklen_t in_len = sizeof(in_addr);
                        const int &infd = server->accept(&in_addr, &in_len);
                        #ifdef PROTOCOLPARSINGDEBUG
                        std::cout << "new client infd: " << infd << std::endl;
                        #endif
                        if(!server->isReady())
                        {
                            /// \todo dont clean error on client into this case
                            std::cerr << "client connect when the server is not ready" << std::endl;
                            ::close(infd);
                            break;
                        }
                        if(elementsToDeleteSize>64
                                #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                                || BaseServerLogin::tokenForAuthSize>=CATCHCHALLENGER_SERVER_MAXNOTLOGGEDCONNECTION
                                #else
                                || Client::tokenAuthList.size()>=CATCHCHALLENGER_SERVER_MAXNOTLOGGEDCONNECTION
                                #endif
                                )
                        {
                            /// \todo dont clean error on client into this case
                            std::cerr << "server overload" << std::endl;
                            ::close(infd);
                            break;
                        }
                        if(infd == -1)
                        {
                            if((errno == EAGAIN) ||
                            (errno == EWOULDBLOCK))
                            {
                                /* We have processed all incoming
                                connections. */
                                if(!acceptSocketWarningShow)
                                {
                                    acceptSocketWarningShow=true;
                                    std::cerr << "::accept() return -1 and errno: " << std::to_string(errno) << " event or socket ignored: " << strerror(errno) << std::endl;
                                }
                                break;
                            }
                            else
                            {
                                std::cout << "connexion accepted" << std::endl;
                                break;
                            }
                        }
                        // do at the protocol negociation to send the reason
                        if(numberOfConnectedClient>=GlobalServerData::serverSettings.max_players)
                        {
                            #ifdef PROTOCOLPARSINGDEBUG
                            std::cout << "numberOfConnectedClient>=GlobalServerData::serverSettings.max_players: " << infd << std::endl;
                            #endif
                            ::close(infd);
                            break;
                        }

                        /* Make the incoming socket non-blocking and add it to the
                        list of fds to monitor. */
                        numberOfConnectedClient++;

                        //const int s = EpollSocket::make_non_blocking(infd);->do problem with large datapack from interne protocol
                        const int s = 0;
                        if(s == -1)
                            std::cerr << "unable to make to socket non blocking" << std::endl;
                        else
                        {
                            int *socketStringSize=nullptr;
                            char **socketString=nullptr;

                            search stat(ClientStat::Free) into std::vector<Client> Client::clientBroadCastList;
                            and change ClientStat::Free
                                    set index_connected_player

                            switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
                            {
                                case MapVisibilityAlgorithmSelection_Simple:
                                {
                                    MapVisibilityAlgorithm_Simple_StoreOnSenderEpoll *c=new MapVisibilityAlgorithm_Simple_StoreOnSenderEpoll(infd);
                                    socketStringSize=&c->socketStringSize;
                                    socketString=&c->socketString;
                                    client=c;
                                }
                                break;
                                case MapVisibilityAlgorithmSelection_WithBorder:
                                {
                                    MapVisibilityAlgorithm_WithBorder_StoreOnSenderEpoll *c=new MapVisibilityAlgorithm_WithBorder_StoreOnSenderEpoll(infd);
                                    socketStringSize=&c->socketStringSize;
                                    socketString=&c->socketString;
                                    client=c;
                                }
                                break;
                                default:
                                case MapVisibilityAlgorithmSelection_None:
                                {
                                    MapVisibilityAlgorithm_NoneEpoll *c=new MapVisibilityAlgorithm_NoneEpoll(infd);
                                    socketStringSize=&c->socketStringSize;
                                    socketString=&c->socketString;
                                    client=c;
                                }
                                break;
                            }
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            ++clientnumberToDebug;
                            if(static_cast<BaseClassSwitch *>(client)->getType()!=BaseClassSwitch::EpollObjectType::Client)
                            {
                                std::cerr << "Wrong post check type (abort)" << std::endl;
                                abort();
                            }
                            #endif
                            #ifdef PROTOCOLPARSINGDEBUG
                            std::cout << "new MapVisibilityAlgorithm: " << infd << " " << client << std::endl;
                            #endif
                            //just for informations
                            {
                                char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
                                const int &s2 = getnameinfo(&in_addr, in_len,
                                hbuf, sizeof hbuf,
                                sbuf, sizeof sbuf,
                                NI_NUMERICHOST | NI_NUMERICSERV);
                                if(s2 == 0)
                                {
                                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                                    //std::cout << "Accepted connection on descriptor " << infd << "(host=" << hbuf << ", port=" << sbuf << "), client: " << client << ", clientnumberToDebug: " << clientnumberToDebug << std::endl;
                                    #else
                                    //std::cout << "Accepted connection on descriptor " << infd << "(host=" << hbuf << ", port=" << sbuf << "), client: " << client << std::endl;
                                    #endif
                                    *socketStringSize=static_cast<int>(strlen(hbuf))+static_cast<int>(strlen(sbuf))+1+1;
                                    (*socketString)=new char[*socketStringSize];
                                    strcpy(*socketString,hbuf);
                                    strcat(*socketString,":");
                                    strcat(*socketString,sbuf);
                                    (*socketString)[*socketStringSize-1]='\0';
                                }
                                /*else
                                    std::cout << "Accepted connection on descriptor " << infd << ", client: " << client << std::endl;*/
                            }
                            epoll_event event;
                            memset(&event,0,sizeof(event));
                            event.data.ptr = client;
                            event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET | EPOLLRDHUP | EPOLLOUT;
                            const int s2 = Epoll::epoll.ctl(EPOLL_CTL_ADD, infd, &event);
                            if(s2 == -1)
                            {
                                std::cerr << "epoll_ctl on socket error" << std::endl;
                                switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
                                {
                                    case MapVisibilityAlgorithmSelection_Simple:
                                        delete static_cast<MapVisibilityAlgorithm_Simple_StoreOnSenderEpoll *>(client);
                                    break;
                                    case MapVisibilityAlgorithmSelection_WithBorder:
                                        delete static_cast<MapVisibilityAlgorithm_WithBorder_StoreOnSenderEpoll *>(client);
                                    break;
                                    default:
                                    case MapVisibilityAlgorithmSelection_None:
                                        delete static_cast<MapVisibilityAlgorithm_NoneEpoll *>(client);
                                    break;
                                }
                            }
                            else
                            {
                                if(::write(infd,encodingBuff,sizeof(encodingBuff))!=sizeof(encodingBuff))
                                {
                                    std::cerr << "socket first byte write error" << std::endl;
                                    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
                                    {
                                        case MapVisibilityAlgorithmSelection_Simple:
                                            delete static_cast<MapVisibilityAlgorithm_Simple_StoreOnSenderEpoll *>(client);
                                        break;
                                        case MapVisibilityAlgorithmSelection_WithBorder:
                                            delete static_cast<MapVisibilityAlgorithm_WithBorder_StoreOnSenderEpoll *>(client);
                                        break;
                                        default:
                                        case MapVisibilityAlgorithmSelection_None:
                                            delete static_cast<MapVisibilityAlgorithm_NoneEpoll *>(client);
                                        break;
                                    }
                                }
                            }
                            #ifdef PROTOCOLPARSINGDEBUG
                            std::cout << "first write: " << infd << std::endl;
                            #endif
                        }
                    }
                }
                break;
                case BaseClassSwitch::EpollObjectType::Client:
                {
                    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
                    {
                        case MapVisibilityAlgorithmSelection_Simple:
                        {
                            MapVisibilityAlgorithm_Simple_StoreOnSenderEpoll * const client=static_cast<MapVisibilityAlgorithm_Simple_StoreOnSenderEpoll *>(events[i].data.ptr);
                            #ifdef PROTOCOLPARSINGDEBUG
                            std::cout << "client " << events[i].data.ptr << " event: " << events[i].events << std::endl;
                            #endif
                            if((events[i].events & EPOLLERR) ||
                            (events[i].events & EPOLLHUP) ||
                            (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                            {
                                /* An error has occured on this fd, or the socket is not
                                ready for reading (why were we notified then?) */
                                if(!(events[i].events & EPOLLHUP))
                                    std::cerr << "client epoll error: " << events[i].events << std::endl;
                                numberOfConnectedClient--;

                                client->disconnectClient();

                                continue;
                            }
                            //ready to read
                            if(events[i].events & EPOLLIN)
                            {
                                #ifdef PROTOCOLPARSINGDEBUG
                                std::cout << "client " << events[i].data.ptr << " client->parseIncommingData()" << std::endl;
                                #endif
                                client->parseIncommingData();
                            }
                            if(events[i].events & EPOLLRDHUP || events[i].events & EPOLLHUP || !client->isValid())
                            {
                                // Crash at 51th: /usr/bin/php -f loginserver-json-generator.php 127.0.0.1 39034
                                numberOfConnectedClient--;
                                client->disconnectClient();
                            }
                        }
                        break;
                        case MapVisibilityAlgorithmSelection_WithBorder:
                        {
                            MapVisibilityAlgorithm_WithBorder_StoreOnSenderEpoll * const client=static_cast<MapVisibilityAlgorithm_WithBorder_StoreOnSenderEpoll *>(events[i].data.ptr);
                            #ifdef PROTOCOLPARSINGDEBUG
                            std::cout << "client " << events[i].data.ptr << " event: " << events[i].events << std::endl;
                            #endif
                            if((events[i].events & EPOLLERR) ||
                            (events[i].events & EPOLLHUP) ||
                            (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                            {
                                /* An error has occured on this fd, or the socket is not
                                ready for reading (why were we notified then?) */
                                if(!(events[i].events & EPOLLHUP))
                                    std::cerr << "client epoll error: " << events[i].events << std::endl;
                                numberOfConnectedClient--;

                                client->disconnectClient();

                                continue;
                            }
                            //ready to read
                            if(events[i].events & EPOLLIN)
                            {
                                #ifdef PROTOCOLPARSINGDEBUG
                                std::cout << "client " << events[i].data.ptr << " client->parseIncommingData()" << std::endl;
                                #endif
                                client->parseIncommingData();
                            }
                            if(events[i].events & EPOLLRDHUP || events[i].events & EPOLLHUP || !client->isValid())
                            {
                                // Crash at 51th: /usr/bin/php -f loginserver-json-generator.php 127.0.0.1 39034
                                numberOfConnectedClient--;
                                client->disconnectClient();
                            }
                        }
                        break;
                        default:
                        case MapVisibilityAlgorithmSelection_None:
                        {
                            MapVisibilityAlgorithm_NoneEpoll * const client=static_cast<MapVisibilityAlgorithm_NoneEpoll *>(events[i].data.ptr);
                            #ifdef PROTOCOLPARSINGDEBUG
                            std::cout << "client " << events[i].data.ptr << " event: " << events[i].events << std::endl;
                            #endif
                            if((events[i].events & EPOLLERR) ||
                            (events[i].events & EPOLLHUP) ||
                            (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                            {
                                /* An error has occured on this fd, or the socket is not
                                ready for reading (why were we notified then?) */
                                if(!(events[i].events & EPOLLHUP))
                                    std::cerr << "client epoll error: " << events[i].events << std::endl;
                                numberOfConnectedClient--;

                                client->disconnectClient();

                                continue;
                            }
                            //ready to read
                            if(events[i].events & EPOLLIN)
                            {
                                #ifdef PROTOCOLPARSINGDEBUG
                                std::cout << "client " << events[i].data.ptr << " client->parseIncommingData()" << std::endl;
                                #endif
                                client->parseIncommingData();
                            }
                            if(events[i].events & EPOLLRDHUP || events[i].events & EPOLLHUP || !client->isValid())
                            {
                                // Crash at 51th: /usr/bin/php -f loginserver-json-generator.php 127.0.0.1 39034
                                numberOfConnectedClient--;
                                client->disconnectClient();
                            }
                        }
                        break;
                    }
                }
                break;
                case BaseClassSwitch::EpollObjectType::Timer:
                {
                    static_cast<EpollTimer *>(events[i].data.ptr)->exec();
                    static_cast<EpollTimer *>(events[i].data.ptr)->validateTheTimer();
                }
                break;
                #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
                case BaseClassSwitch::EpollObjectType::Database:
                {
                    EpollDatabase * const db=static_cast<EpollDatabase *>(events[i].data.ptr);
                    db->epollEvent(events[i].events);
                    if(!datapack_loaded)
                    {
                        if(db->isConnected())
                        {
                            std::cout << "datapack_loaded not loaded: start preload data " << std::endl;
                            server->preload_1_the_data();
                            datapack_loaded=true;
                        }
                        else
                            std::cerr << "datapack_loaded not loaded: but database seam don't be connected" << std::endl;
                    }
                    if(!db->isConnected())
                    {
                        std::cerr << "database disconnect, quit now" << std::endl;
                        return EXIT_FAILURE;
                    }
                    //std::cerr << "epoll database type return not supported: " << CatchChallenger::DatabaseBase::databaseTypeToString(static_cast<CatchChallenger::DatabaseBase *>(events[i].data.ptr)->databaseType()) << " for " << events[i].data.ptr << std::endl;
                }
                break;
                #elif CATCHCHALLENGER_DB_BLACKHOLE
                #elif CATCHCHALLENGER_DB_FILE
                #else
                #error Define what do here
                #endif
                #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                case BaseClassSwitch::EpollObjectType::MasterLink:
                {
                    LinkToMaster * const client=static_cast<LinkToMaster *>(events[i].data.ptr);
                    if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occured on this fd, or the socket is not
                        ready for reading (why were we notified then?) */
                        if(!(events[i].events & EPOLLHUP))
                            std::cerr << "client epoll error: " << events[i].events << std::endl;
                        client->tryReconnect();
                    }
                    else
                    {
                        //ready to read
                        if(events[i].events & EPOLLIN)
                            client->parseIncommingData();
                        if(events[i].events & EPOLLHUP || events[i].events & EPOLLRDHUP)
                            client->tryReconnect();
                    }
                }
                break;
                #endif
            default:
                #ifdef PROTOCOLPARSINGDEBUG
                std::cout << "unknown type: " << events[i].data.ptr << " event: " << events[i].events << std::endl;
                #endif
                break;
            }
        }
        #ifdef SERVERBENCHMARK
        std::chrono::duration<unsigned long long int,std::nano> elapsed_seconds = std::chrono::high_resolution_clock::now()-EpollUnixSocketClientFinal::start;
        EpollUnixSocketClientFinal::timeUsed+=elapsed_seconds.count();
        #endif
    }
    server->close();
    server->unload_the_data();
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    LinkToMaster::linkToMaster->closeSocket();
    #endif
    delete server;
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    delete LinkToMaster::linkToMaster;
    #endif
    return EXIT_SUCCESS;
}
