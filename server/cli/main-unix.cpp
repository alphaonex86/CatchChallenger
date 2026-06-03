#include <iostream>
#include <fstream>
#include <chrono>
#include <signal.h>
#ifdef CATCHCHALLENGER_BENCHMARK
#include <time.h>
#endif
#include "win32_compat.hpp"
#ifndef _WIN32
#include <unistd.h>
#include <netdb.h>
#include <netinet/tcp.h>
#else
#include <direct.h>
#endif
#if defined(CATCHCHALLENGER_DB_FILE) || defined(CATCHCHALLENGER_CACHE_HPS)
#include <sys/stat.h>
//dictionary_serialBuffer and server_serialBuffer now open as DB
#endif
#include "../base/ServerStructures.hpp"
#ifdef CATCHCHALLENGER_DB_INTERNAL_VARS
#include "../base/DbInternalVars.hpp"
#endif
#ifndef CATCHCHALLENGER_NOXML
#include "../base/TinyXMLSettings.hpp"
#endif
#include "../base/GlobalServerData.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/FacilityLib.hpp"
#include "../../general/base/ProtocolParsing.hpp"
#include "EventLoopServer.hpp"
#include "EventLoopClientList.hpp"
#include "EventLoop.hpp"
#if defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_SQLITE)
// EventLoopDatabase is the polymorphic base; the SQL-event dispatch at
// the bottom of main() casts events[i].data.ptr to EventLoopDatabase*
// when the EventLoopObjectType is Database. Without this include the
// SQLITE backend (which has no dedicated UnixSQLite.hpp wrapping
// the include) builds with `EventLoopDatabase was not declared in this
// scope`. PostgreSQL/MySQL paths pull this header transitively via
// their own subclass headers below.
#include "db/EventLoopDatabase.hpp"
#endif
#ifdef CATCHCHALLENGER_DB_POSTGRESQL
#include "db/EventLoopPostgresql.hpp"
#endif
#ifdef CATCHCHALLENGER_DB_MYSQL
#include "db/EventLoopMySQL.hpp"
#endif
#include "../base/NormalServerGlobal.hpp"
#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "../game-server-alone/LinkToMaster.hpp"
#include "SocketUtil.hpp"
#include "timer/TimerPurgeTokenAuthList.hpp"
#endif

#include "timer/TimerCityCapture.hpp"
#include "timer/TimerDdos.hpp"
#include "timer/TimerPositionSync.hpp"
#include "timer/TimerSendInsertMoveRemove.hpp"
#include "timer/PlayerUpdaterEventLoop.hpp"

#define MAXEVENTS 512

using namespace CatchChallenger;

EventLoopServer *server=NULL;
bool bench_exit_after_bind=false;
#ifndef CATCHCHALLENGER_NOXML
TinyXMLSettings *settings=NULL;
#endif

std::string master_host;
uint16_t master_port;
uint8_t master_tryInterval;
uint8_t master_considerDownAfterNumberOfTry;

#ifndef CATCHCHALLENGER_NOXML
#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
void generateTokenStatClient(TinyXMLSettings &settings,char * const data)
{
#ifdef __DJGPP__
    //MS-DOS has no /dev/urandom: fill with rand() in a loop, same as the
    //non-linux token path in ClientNetworkRead.cpp. The server RNG is already
    //seeded at startup; poor-quality randomness is fine for this single-machine
    //DOS server's stat-client token.
    int index=0;
    while(index<CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER)
    {
        data[index]=static_cast<uint8_t>(rand()%256);
        index++;
    }
#else
    FILE *fpRandomFile = fopen(RANDOMFILEDEVICE,"rb");
    if(fpRandomFile==NULL)
    {
        std::cerr << "Unable to open " << RANDOMFILEDEVICE << " to generate random token" << std::endl;
        abort();
    }
    const size_t &returnedSize=fread(data,1,CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER,fpRandomFile);
    if(returnedSize!=CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER)
    {
        std::cerr << "Unable to read the " << CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER << " needed to do the token from " << RANDOMFILEDEVICE << std::endl;
        abort();
    }
    fclose(fpRandomFile);
#endif
    settings.setValue("token",binarytoHexa(reinterpret_cast<char *>(data)
                                           ,CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER).c_str());
    settings.sync();
}
#endif
#endif

std::vector<void *> elementsToDelete[16];
size_t elementsToDeleteSize=0;
uint8_t elementsToDeleteIndex=0;

/* Catch Signal Handler functio */
void signal_callback_handler(int signum){

        printf("Caught signal SIGPIPE %d\n",signum);
}

#ifdef CATCHCHALLENGER_BENCHMARK
//Benchmark-only counters. Compiled out of production builds entirely
//(no counters, no per-event clock read, no handler, zero cost), see
//benchmark/CLAUDE.md "Zero impact on production binaries".
//
// * bench_packets_in   -- client read-events processed (one per
//                         parseIncommingData()); /wall => requests/s.
// * bench_lat_hist[i]   -- log2 histogram of per-read-event processing
//                         time in ns (bucket i = [2^i, 2^(i+1)) ns).
//                         benchmark*.py reconstructs p50/p95/p99/p999
//                         (latency) and stddev (jitter) from the buckets.
//
//On SIGINT/SIGTERM the handler dumps everything as "BENCH <k>=<v>" lines
//(async-signal-safe integer writes only) so the requests/s headline is
//always paired with its latency tail + jitter, never reported alone.
#define BENCH_LAT_BUCKETS 48
volatile unsigned long bench_packets_in=0;
volatile unsigned long bench_lat_hist[BENCH_LAT_BUCKETS];

static void bench_write_kv(const char *key,unsigned long v)
{
    //async-signal-safe: hand-format "<key>=<v>\n" and write() it; printf /
    //std::cout are NOT safe to call from a signal handler.
    char buf[96];
    int p=0;
    while(*key!='\0'){buf[p++]=*key;key++;}
    buf[p++]='=';
    char num[20];
    int n=0;
    if(v==0)
        num[n++]='0';
    while(v>0){num[n++]=static_cast<char>('0'+(v%10));v/=10;}
    while(n>0){buf[p++]=num[--n];}
    buf[p++]='\n';
    const ssize_t w=::write(STDOUT_FILENO,buf,static_cast<size_t>(p));
    (void)w;
}

static void bench_signal_dump_and_exit(int)
{
    bench_write_kv("BENCH packets_in",bench_packets_in);
    int i=0;
    while(i<BENCH_LAT_BUCKETS)
    {
        const unsigned long c=bench_lat_hist[i];
        if(c>0)
        {
            //"BENCH lat_hist_<i>" -- build the key inline (no snprintf).
            char key[24];
            int k=0;
            const char pre[]="BENCH lat_hist_";
            while(pre[k]!='\0'){key[k]=pre[k];k++;}
            if(i>=10)
                key[k++]=static_cast<char>('0'+(i/10));
            key[k++]=static_cast<char>('0'+(i%10));
            key[k]='\0';
            bench_write_kv(key,c);
        }
        i++;
    }
    _exit(0);
}
#endif

/*void CatchChallenger::recordDisconnectByServer(void * client)
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
}*/

#ifndef CATCHCHALLENGER_NOXML
void send_settings(
        EventLoopServer *server
        ,TinyXMLSettings *settings,
        std::string &master_host,
        uint16_t &master_port,
        uint8_t &master_tryInterval,
        uint8_t &master_considerDownAfterNumberOfTry
        );
#endif

int main(int argc, char *argv[])
{
    //capture wall start ASAP so the "correctly bind" log can show ms-since-start
    EventLoopGenericServer::processStart=std::chrono::steady_clock::now();
    memset(ProtocolParsingBase::tempBigBufferForOutput,0,sizeof(ProtocolParsingBase::tempBigBufferForOutput));
#ifdef _WIN32
    if(!cc_winsock_init())
    {
        std::cerr << "WSAStartup failed" << std::endl;
        return EXIT_FAILURE;
    }
#endif
#ifndef _WIN32
    /* Catch Signal Handler SIGPIPE */
    if(signal(SIGPIPE, signal_callback_handler)==SIG_ERR)
    {
        std::cerr << "signal(SIGPIPE, signal_callback_handler)==SIG_ERR, errno: " << std::to_string(errno) << std::endl;
        abort();
    }
#ifdef CATCHCHALLENGER_BENCHMARK
    //benchmark build: SIGINT/SIGTERM dump the throughput counter then
    //exit cleanly so benchmark*.py can read "BENCH packets_in=<N>".
    signal(SIGINT, bench_signal_dump_and_exit);
    signal(SIGTERM, bench_signal_dump_and_exit);
#endif
#endif

    NormalServerGlobal::displayInfo();
    if(argc<1)
    {
        std::cerr << "argc<1: wrong arg count" << std::endl;
        return EXIT_FAILURE;
    }
    {
        int ai=1;
        while(ai<argc)
        {
            if(strcmp(argv[ai],"--bench-exit")==0)
                bench_exit_after_bind=true;
            ++ai;
        }
    }
    if(argc>=2 && (strcmp(argv[1],"--help")==0 || strcmp(argv[1],"-h")==0 || strcmp(argv[1],"help")==0))
    {
        std::cout << "Usage: " << argv[0] << " [OPTION]" << std::endl;
        std::cout << std::endl;
        std::cout << "CatchChallenger epoll game server." << std::endl;
        std::cout << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  (none)              Start the server normally (loads datapack cache if available)" << std::endl;
#ifdef CATCHCHALLENGER_CACHE_HPS
        std::cout << "  save, --save, -s    Parse the datapack, write datapack-cache.bin, then exit" << std::endl;
#endif
        std::cout << "  --bench-exit        Exit cleanly right after the listen socket is bound (no event loop)" << std::endl;
        std::cout << "  help, --help, -h    Display this help message and exit" << std::endl;
        std::cout << std::endl;
        std::cout << "Files:" << std::endl;
        std::cout << "  server-properties.xml   Server configuration (port, databases, mirrors, ...)" << std::endl;
        std::cout << "  datapack/               Datapack directory (must contain informations.xml)" << std::endl;
#ifdef CATCHCHALLENGER_CACHE_HPS
        std::cout << "  datapack-cache.bin      Pre-computed datapack cache (created by 'save')" << std::endl;
#endif
#ifdef CATCHCHALLENGER_DB_FILE
        std::cout << "  database/               File-based database directory (accounts, characters, ...)" << std::endl;
#endif
        return EXIT_SUCCESS;
    }
    CatchChallenger::FacilityLibGeneral::applicationDirPath=argv[0];

    srand(static_cast<unsigned int>(time(NULL)));

    #ifdef CATCHCHALLENGER_DB_FILE
    #ifdef CATCHCHALLENGER_DB_INTERNAL_VARS
    // In-memory mode: every DB record is a key in a std::unordered_map
    // (no files, no directories, no syscall). init() just starts from an
    // empty store; the "database/..." paths are map keys, not real dirs.
    CatchChallenger::DbInternalVars::init();
    #else
    #ifdef _WIN32
    #define CC_MKDIR(p,m) ::_mkdir(p)
    #else
    #define CC_MKDIR(p,m) ::mkdir(p,m)
    #endif
    CC_MKDIR("database",0700);
    CC_MKDIR("database/login",0700);
    CC_MKDIR("database/common",0700);
    CC_MKDIR("database/common/accounts",0700);
    CC_MKDIR("database/common/characters",0700);
    CC_MKDIR("database/server",0700);
    CC_MKDIR("database/server/characters",0700);
    CC_MKDIR("database/server/clans",0700);
    CC_MKDIR("database/server/zone",0700);
    #undef CC_MKDIR
    #endif
    #endif

    #ifndef CATCHCHALLENGER_DB_BLACKHOLE
    #ifndef CATCHCHALLENGER_DB_FILE
    bool datapack_loaded=false;
    #endif
    #endif

    server=new EventLoopServer();

    #ifdef CATCHCHALLENGER_CACHE_HPS
    const bool save=argc==2 && (strcmp(argv[1],"save")==0 || strcmp(argv[1],"--save")==0 || strcmp(argv[1],"-s")==0);
    if(save)
    {
        const std::string &datapackCache=FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/datapack-cache.bin.tmp";
        server->setSave(datapackCache);
    }
    else
    {
        const std::string &basePath=FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath);
        const std::string &datapackCache=basePath+"/datapack-cache.bin";
        struct stat cacheStat,xmlStat;
        if(::stat(datapackCache.c_str(),&cacheStat)==0 &&
           ::stat((basePath+"/server-properties.xml").c_str(),&xmlStat)==0 &&
           cacheStat.st_mtime==xmlStat.st_mtime)
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

    #ifndef CATCHCHALLENGER_NOXML
    #if defined(CATCHCHALLENGER_CACHE_HPS) && !defined(CATCHCHALLENGER_CLASS_ONLYGAMESERVER)
    if(save || !server->binaryCacheIsOpen())
    #endif
    {
        settings=new TinyXMLSettings(FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/server-properties.xml");
        NormalServerGlobal::checkSettingsFile(settings,FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/datapack/");
    }
    #endif

    if(!EventLoop::loop.init())
        return EPOLLERR;

    //before linkToMaster->registerGameServer() to have the correct settings loaded
    //after server to have the settings
    #ifdef CATCHCHALLENGER_CACHE_HPS
    if(!save && server->binaryInputCacheIsOpen())
        server->setNormalSettings(server->loadSettingsFromBinaryCache(master_host,master_port,master_tryInterval,master_considerDownAfterNumberOfTry));
    else
    #endif
    {
        #ifndef CATCHCHALLENGER_NOXML
        send_settings(server,settings,master_host,master_port,master_tryInterval,master_considerDownAfterNumberOfTry);
        #else
        std::cerr << "CATCHCHALLENGER_NOXML defined but no binary cache available (abort)" << std::endl;
        abort();
        #endif
    }

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
        LinkToMaster::linkToMaster=new LinkToMaster(linkfd);
        LinkToMaster::linkToMaster->stat=LinkToMaster::Stat::Connected;
        LinkToMaster::linkToMaster->setSettings(settings);
        // The 1-byte SSL/cleartext preamble that this used to wait on
        // is gone; LinkToMaster now sends its protocol header straight
        // away. See server/game-server-alone/LinkToMaster.cpp.
        LinkToMaster::linkToMaster->sendProtocolHeader();
        LinkToMaster::linkToMaster->setConnexionSettings(master_tryInterval,master_considerDownAfterNumberOfTry);
        const int &s = SocketUtil::make_non_blocking(linkfd);
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
                    #ifdef CATCHCHALLENGER_DB_SQLITE
                    && formatedServerSettings.database_login.tryOpenType!=DatabaseBase::DatabaseType::SQLite
                    #endif
                    )
            {
                #ifndef CATCHCHALLENGER_NOXML
                settings->beginGroup("db-login");
                std::cerr << "Database type not supported for now: " << settings->value("type") << std::endl;
                settings->endGroup();
                #else
                std::cerr << "Database type not supported for now: db-login" << std::endl;
                #endif
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
                    #ifdef CATCHCHALLENGER_DB_SQLITE
                    && formatedServerSettings.database_base.tryOpenType!=DatabaseBase::DatabaseType::SQLite
                    #endif
                    )
            {
                #ifndef CATCHCHALLENGER_NOXML
                settings->beginGroup("db-base");
                std::cerr << "Database type not supported for now: " << settings->value("type") << std::endl;
                settings->endGroup();
                #else
                std::cerr << "Database type not supported for now: db-base" << std::endl;
                #endif
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
                    #ifdef CATCHCHALLENGER_DB_SQLITE
                    && formatedServerSettings.database_common.tryOpenType!=DatabaseBase::DatabaseType::SQLite
                    #endif
                    )
            {
                #ifndef CATCHCHALLENGER_NOXML
                settings->beginGroup("db-common");
                std::cerr << "Database type not supported for now: " << settings->value("type") << std::endl;
                settings->endGroup();
                #else
                std::cerr << "Database type not supported for now: db-common" << std::endl;
                #endif
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
                    #ifdef CATCHCHALLENGER_DB_SQLITE
                    && formatedServerSettings.database_server.tryOpenType!=DatabaseBase::DatabaseType::SQLite
                    #endif
                    )
            {
                #ifndef CATCHCHALLENGER_NOXML
                settings->beginGroup("db-server");
                std::cerr << "Database type not supported for now: " << settings->value("type") << std::endl;
                settings->endGroup();
                #else
                std::cerr << "Database type not supported for now: db-server" << std::endl;
                #endif
                return EXIT_FAILURE;
            }
            #elif CATCHCHALLENGER_DB_BLACKHOLE
            #elif CATCHCHALLENGER_DB_FILE
            #else
            #error Define what do here
            #endif
        }


        // useSsl gate removed — the protocol no longer includes the
        // SSL/cleartext preamble byte, so binaries compiled with or
        // without CATCHCHALLENGER_SERVER_SSL no longer need to assert
        // a matching server-properties.xml entry.
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
            static_cast<EventLoopPostgresql *>(GlobalServerData::serverPrivateVariables.db_login)->setMaxDbQueries(2000000000);
        break;
        #endif
        default:
        break;
    }
    switch(GlobalServerData::serverSettings.database_base.tryOpenType)
    {
        #ifdef CATCHCHALLENGER_DB_POSTGRESQL
        case DatabaseBase::DatabaseType::PostgreSQL:
            static_cast<EventLoopPostgresql *>(GlobalServerData::serverPrivateVariables.db_base)->setMaxDbQueries(2000000000);
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
            static_cast<EventLoopPostgresql *>(GlobalServerData::serverPrivateVariables.db_common)->setMaxDbQueries(2000000000);
        break;
        #endif
        default:
        break;
    }
    switch(GlobalServerData::serverSettings.database_server.tryOpenType)
    {
        #ifdef CATCHCHALLENGER_DB_POSTGRESQL
        case DatabaseBase::DatabaseType::PostgreSQL:
            static_cast<EventLoopPostgresql *>(GlobalServerData::serverPrivateVariables.db_server)->setMaxDbQueries(2000000000);
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
    server->preload_1_the_data();
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    server->preload_1_the_data();
    #elif CATCHCHALLENGER_DB_FILE
    {
        struct stat sb;
        if(::stat(CATCHCHALLENGER_DB_FILE_PATH(std::string("database/server/dictionary_map")).c_str(),&sb)==0)
        {
            BaseServer::dictionary_in_file=new std::ifstream(CATCHCHALLENGER_DB_FILE_PATH(std::string("database/server/dictionary_map")), std::ifstream::binary);
            if(BaseServer::dictionary_in_file->good() && BaseServer::dictionary_in_file->is_open())
                BaseServer::dictionary_serialBuffer=new hps::StreamInputBuffer(*BaseServer::dictionary_in_file);
            else
            {
                delete BaseServer::dictionary_in_file;
                BaseServer::dictionary_in_file=nullptr;
            }
        }
        if(::stat(CATCHCHALLENGER_DB_FILE_PATH(std::string("database/common/dictionary_reputation")).c_str(),&sb)==0)
        {
            BaseServer::dictionary_reputation_in_file=new std::ifstream(CATCHCHALLENGER_DB_FILE_PATH(std::string("database/common/dictionary_reputation")), std::ifstream::binary);
            if(!BaseServer::dictionary_reputation_in_file->good() || !BaseServer::dictionary_reputation_in_file->is_open())
            {
                delete BaseServer::dictionary_reputation_in_file;
                BaseServer::dictionary_reputation_in_file=nullptr;
            }
        }
        if(::stat(CATCHCHALLENGER_DB_FILE_PATH(std::string("database/common/dictionary_skin")).c_str(),&sb)==0)
        {
            BaseServer::dictionary_skin_in_file=new std::ifstream(CATCHCHALLENGER_DB_FILE_PATH(std::string("database/common/dictionary_skin")), std::ifstream::binary);
            if(!BaseServer::dictionary_skin_in_file->good() || !BaseServer::dictionary_skin_in_file->is_open())
            {
                delete BaseServer::dictionary_skin_in_file;
                BaseServer::dictionary_skin_in_file=nullptr;
            }
        }
        if(::stat(CATCHCHALLENGER_DB_FILE_PATH(std::string("database/common/dictionary_starter")).c_str(),&sb)==0)
        {
            BaseServer::dictionary_starter_in_file=new std::ifstream(CATCHCHALLENGER_DB_FILE_PATH(std::string("database/common/dictionary_starter")), std::ifstream::binary);
            if(!BaseServer::dictionary_starter_in_file->good() || !BaseServer::dictionary_starter_in_file->is_open())
            {
                delete BaseServer::dictionary_starter_in_file;
                BaseServer::dictionary_starter_in_file=nullptr;
            }
        }
    }
    server->preload_1_the_data();
    #else
    #error Define what do here
    #endif

    if(bench_exit_after_bind)
    {
        const auto endTs=std::chrono::steady_clock::now();
        const auto totalMs=std::chrono::duration_cast<std::chrono::milliseconds>(endTs-EventLoopGenericServer::processStart).count();
        std::cout << "BENCH startup_total_ms=" << totalMs << std::endl;
        server->close();
        server->unload_the_data();
        delete server;
        return EXIT_SUCCESS;
    }

    EventLoopClientList *unixClientList=new EventLoopClientList();
    ClientList::list=unixClientList;
    GlobalServerData::serverPrivateVariables.player_updater=new PlayerUpdaterEventLoop();
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
            if(!static_cast<PlayerUpdaterEventLoop *>(GlobalServerData::serverPrivateVariables.player_updater)->start())
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
    #ifndef CATCHCHALLENGER_NOXML
    delete settings;
    #endif

    char buf[4096];
    memset(buf,0,4096);
    /* Buffer where events are returned */
    epoll_event events[MAXEVENTS];

    bool acceptSocketWarningShow=false;
    int numberOfConnectedClient=0;
    /* The event loop */
    #ifdef CATCHCHALLENGER_HARDENED
    unsigned int clientnumberToDebug=0;
    #endif
    int number_of_events, i;
    while(1)
    {

        number_of_events = EventLoop::loop.wait(events, MAXEVENTS);
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
                    //Character persistence is handled in Client::disconnectClient(),
                    //before setToDefault() wipes the data. By the time we reach
                    //this delete queue the fields are already reset.
                    delete c;
                    index++;
                }
            }
            elementsToDeleteSize-=elementsToDeleteSub.size();
            elementsToDelete[elementsToDeleteIndex].clear();
        }
        for(i = 0; i < number_of_events; i++)
        {
            #ifdef PROTOCOLPARSINGDEBUG
            //std::cout << "new event: " << events[i].data.ptr << " event: " << events[i].events << std::endl;
            #endif
            switch(static_cast<BaseClassSwitch *>(events[i].data.ptr)->getType())
            {
                case BaseClassSwitch::EventLoopObjectType::Server:
                {
                    if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occurred on this fd, or the socket is not
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
                            cc_close_socket(infd);
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
                            cc_close_socket(infd);
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
                            cc_close_socket(infd);
                            break;
                        }

                        /* Make the incoming socket non-blocking and add it to the
                        list of fds to monitor. */
                        numberOfConnectedClient++;

                        //const int s = SocketUtil::make_non_blocking(infd);->do problem with large datapack from interne protocol
                        const int s = 0;
                        if(s == -1)
                            std::cerr << "unable to make to socket non blocking" << std::endl;
                        else if(!EventLoopClientList::list->haveFreeSlot())
                            std::cerr << "!EventLoopClientList::haveFreeSlot()" << std::endl;
                        else
                        {
                            ClientWithMapEventLoop &client=unixClientList->getByReference();
                            client.reset(infd);
                            #ifdef CATCHCHALLENGER_HARDENED
                            ++clientnumberToDebug;
                            if(client.getType()!=BaseClassSwitch::EventLoopObjectType::Client)
                            {
                                std::cerr << "Wrong post check type (abort)" << std::endl;
                                abort();
                            }
                            #endif
                            #ifdef PROTOCOLPARSINGDEBUG
                            std::cout << "new MapVisibilityAlgorithm: " << infd << " " << (void*)&client << std::endl;
                            #endif
                            //populate client.socketString so headerOutput()/normalOutput()/errorOutput()
                            //display a real "<ip>:<port>:" prefix instead of the empty string fallback.
                            //Mirrors server/gateway/main-unix-gateway.cpp accept path.
                            {
                                if(client.socketString!=NULL)
                                {
                                    delete[] client.socketString;
                                    client.socketString=NULL;
                                    client.socketStringSize=0;
                                }
                                char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
                                const int s2 = getnameinfo(&in_addr, in_len,
                                                           hbuf, sizeof hbuf,
                                                           sbuf, sizeof sbuf,
                                                           NI_NUMERICHOST | NI_NUMERICSERV);
                                if(s2 == 0)
                                {
                                    client.socketStringSize=static_cast<int>(strlen(hbuf))+static_cast<int>(strlen(sbuf))+1+1;
                                    client.socketString=new char[client.socketStringSize];
                                    strcpy(client.socketString,hbuf);
                                    strcat(client.socketString,":");
                                    strcat(client.socketString,sbuf);
                                    client.socketString[client.socketStringSize-1]='\0';
                                }
                            }
                            #ifdef CATCHCHALLENGER_IO_URING
                            if(EventLoop::loop.multishotEnabled())
                            {
                                if(!EventLoop::loop.armRecvMultishot(infd,
                                        static_cast<BaseClassSwitch *>(&client)))
                                {
                                    std::cerr << "armRecvMultishot failed for fd " << infd << std::endl;
                                    client.disconnectClient();
                                    client.setToDefault();
                                    unixClientList->remove(client);
                                }
                            }
                            else
                            #endif
                            {
                                epoll_event event;
                                memset(&event,0,sizeof(event));
                                event.data.ptr = &client;
                                event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET | EPOLLRDHUP | EPOLLOUT;
                                const int s2 = EventLoop::loop.ctl(EPOLL_CTL_ADD, infd, &event);
                                if(s2 == -1)
                                {
                                    std::cerr << "epoll_ctl on socket error" << std::endl;
                                    client.disconnectClient();
                                    client.setToDefault();
                                    unixClientList->remove(client);
                                }
                            }
                            // SSL preamble byte removed; the server no
                            // longer writes a 0x00/0x01 sentinel — every
                            // byte on the wire is protocol payload.
                            #ifdef PROTOCOLPARSINGDEBUG
                            std::cout << "client accepted: " << infd << std::endl;
                            #endif
                        }
                    }
                }
                break;
                case BaseClassSwitch::EventLoopObjectType::Client:
                {
                    ClientWithMapEventLoop *client=static_cast<ClientWithMapEventLoop *>(events[i].data.ptr);
                    #ifdef PROTOCOLPARSINGDEBUG
                    std::cout << "client " << events[i].data.ptr << " event: " << events[i].events << std::endl;
                    #endif
                    if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occurred on this fd, or the socket is not
                        ready for reading (why were we notified then?) */
                        if(!(events[i].events & EPOLLHUP))
                            std::cerr << "client epoll error: " << events[i].events << std::endl;
                        numberOfConnectedClient--;

                        client->disconnectClient();
                        client->setToDefault();
                        unixClientList->remove(*client);

                        continue;
                    }
                    //ready to read
                    if(events[i].events & EPOLLIN)
                    {
                        #ifdef CATCHCHALLENGER_BENCHMARK
                        struct timespec benchT0;
                        clock_gettime(CLOCK_MONOTONIC,&benchT0);
                        #endif
                        client->parseIncommingData();
                        #ifdef CATCHCHALLENGER_BENCHMARK
                        struct timespec benchT1;
                        clock_gettime(CLOCK_MONOTONIC,&benchT1);
                        //plain assignment (not ++) so a C++20+ build
                        //doesn't warn -Wvolatile on a volatile RMW.
                        bench_packets_in = bench_packets_in + 1;
                        unsigned long long benchNs=
                            static_cast<unsigned long long>(benchT1.tv_sec-benchT0.tv_sec)*1000000000ULL
                            +static_cast<unsigned long long>(benchT1.tv_nsec-benchT0.tv_nsec);
                        int benchBucket=0;
                        if(benchNs>0)
                            benchBucket=63-__builtin_clzll(benchNs);
                        if(benchBucket>=BENCH_LAT_BUCKETS)
                            benchBucket=BENCH_LAT_BUCKETS-1;
                        bench_lat_hist[benchBucket]=bench_lat_hist[benchBucket]+1;
                        #endif
                    }
                    if(events[i].events & EPOLLRDHUP || events[i].events & EPOLLHUP || !client->isValid())
                    {
                        // Crash at 51th: /usr/bin/php -f loginserver-json-generator.php 127.0.0.1 39034
                        numberOfConnectedClient--;

                        client->disconnectClient();
                        client->setToDefault();
                        unixClientList->remove(*client);
                    }
                }
                break;
                case BaseClassSwitch::EventLoopObjectType::Timer:
                {
                    static_cast<EventLoopTimer *>(events[i].data.ptr)->exec();
                    static_cast<EventLoopTimer *>(events[i].data.ptr)->validateTheTimer();
                }
                break;
                #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
                case BaseClassSwitch::EventLoopObjectType::Database:
                {
                    EventLoopDatabase * const db=static_cast<EventLoopDatabase *>(events[i].data.ptr);
                    db->unixEvent(events[i].events);
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
                case BaseClassSwitch::EventLoopObjectType::MasterLink:
                {
                    LinkToMaster * const client=static_cast<LinkToMaster *>(events[i].data.ptr);
                    if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occurred on this fd, or the socket is not
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
