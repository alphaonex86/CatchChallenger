#include <iostream>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>

#include "epoll/EpollSocket.h"
#include "epoll/EpollSslClient.h"
#include "epoll/EpollSslServer.h"
#include "epoll/EpollClient.h"
#include "epoll/EpollServer.h"
#include "epoll/Epoll.h"
#include "epoll/EpollTimer.h"
#include "epoll/timer/TimerCityCapture.h"
#include "epoll/timer/TimerDdos.h"
#include "epoll/timer/TimerDisplayEventBySeconds.h"
#include "epoll/timer/TimerPositionSync.h"
#include "epoll/timer/TimerSendInsertMoveRemove.h"
#include "base/ServerStructures.h"
#include "NormalServerGlobal.h"
#include "base/GlobalServerData.h"
#include "base/Client.h"
#include "base/ClientMapManagement/MapVisibilityAlgorithm_None.h"
#include "base/ClientMapManagement/MapVisibilityAlgorithm_Simple_StoreOnSender.h"
#include "base/ClientMapManagement/MapVisibilityAlgorithm_WithBorder_StoreOnSender.h"
#include "../general/base/FacilityLib.h"
#include "../general/base/GeneralVariable.h"

#define MAXEVENTS 512

using namespace CatchChallenger;

#ifdef SERVERSSL
EpollSslServer *server=NULL;
#else
EpollServer *server=NULL;
#endif
QSettings *settings=NULL;

void send_settings()
{
    ServerSettings formatedServerSettings=server->getSettings();
    NormalServerSettings formatedServerNormalSettings=server->getNormalSettings();

    //common var
    CommonSettings::commonSettings.min_character					= settings->value(QLatin1Literal("min_character")).toUInt();
    CommonSettings::commonSettings.max_character					= settings->value(QLatin1Literal("max_character")).toUInt();
    CommonSettings::commonSettings.max_pseudo_size					= settings->value(QLatin1Literal("max_pseudo_size")).toUInt();
    CommonSettings::commonSettings.character_delete_time			= settings->value(QLatin1Literal("character_delete_time")).toUInt();
    CommonSettings::commonSettings.useSP                            = settings->value(QLatin1Literal("useSP")).toBool();
    CommonSettings::commonSettings.autoLearn                        = settings->value(QLatin1Literal("autoLearn")).toBool() && !CommonSettings::commonSettings.useSP;
    CommonSettings::commonSettings.forcedSpeed                      = settings->value(QLatin1Literal("forcedSpeed")).toUInt();
    CommonSettings::commonSettings.dontSendPseudo					= settings->value(QLatin1Literal("dontSendPseudo")).toBool();
    CommonSettings::commonSettings.forceClientToSendAtMapChange		= settings->value(QLatin1Literal("forceClientToSendAtMapChange")).toBool();
    formatedServerSettings.dontSendPlayerType                       = settings->value(QLatin1Literal("dontSendPlayerType")).toBool();

    //the listen
    formatedServerNormalSettings.server_port			= settings->value(QLatin1Literal("server-port")).toUInt();
    formatedServerNormalSettings.server_ip				= settings->value(QLatin1Literal("server-ip")).toString();
    formatedServerNormalSettings.proxy					= settings->value(QLatin1Literal("proxy")).toString();
    formatedServerNormalSettings.proxy_port				= settings->value(QLatin1Literal("proxy_port")).toUInt();
    formatedServerNormalSettings.useSsl					= settings->value(QLatin1Literal("useSsl")).toBool();

    CommonSettings::commonSettings.anonymous					= settings->value(QLatin1Literal("anonymous")).toBool();
    formatedServerSettings.server_message				= settings->value(QLatin1Literal("server_message")).toString();
    CommonSettings::commonSettings.httpDatapackMirror	= settings->value(QLatin1Literal("httpDatapackMirror")).toString();
    formatedServerSettings.datapackCache				= settings->value(QLatin1Literal("datapackCache")).toInt();
    #ifdef Q_OS_LINUX
    settings->beginGroup(QLatin1Literal("Linux"));
    CommonSettings::commonSettings.tcpCork	= settings->value(QLatin1Literal("tcpCork")).toBool();
    formatedServerNormalSettings.tcpNodelay= settings->value(QLatin1Literal("tcpNodelay")).toBool();
    settings->endGroup();
    #endif

    //fight
    //CommonSettings::commonSettings.pvp			= settings->value(QLatin1Literal("pvp")).toBool();
    formatedServerSettings.sendPlayerNumber         = settings->value(QLatin1Literal("sendPlayerNumber")).toBool();

    //rates
    settings->beginGroup(QLatin1Literal("rates"));
    CommonSettings::commonSettings.rates_xp             = settings->value(QLatin1Literal("xp_normal")).toReal();
    CommonSettings::commonSettings.rates_gold			= settings->value(QLatin1Literal("gold_normal")).toReal();
    CommonSettings::commonSettings.rates_xp_pow			= settings->value(QLatin1Literal("xp_pow_normal")).toReal();
    CommonSettings::commonSettings.rates_drop			= settings->value(QLatin1Literal("drop_normal")).toReal();
    //formatedServerSettings.rates_xp_premium                         = settings->value(QLatin1Literal("xp_premium")).toReal();
    //formatedServerSettings.rates_gold_premium                       = settings->value(QLatin1Literal("gold_premium")).toReal();
    /*CommonSettings::commonSettings.rates_shiny		= settings->value(QLatin1Literal("shiny_normal")).toReal();
    formatedServerSettings.rates_shiny_premium                      = settings->value(QLatin1Literal("shiny_premium")).toReal();*/
    settings->endGroup();

    settings->beginGroup(QLatin1Literal("DDOS"));
    CommonSettings::commonSettings.waitBeforeConnectAfterKick         = settings->value(QLatin1Literal("waitBeforeConnectAfterKick")).toUInt();
    formatedServerSettings.ddos.computeAverageValueNumberOfValue      = settings->value(QLatin1Literal("computeAverageValueNumberOfValue")).toUInt();
    formatedServerSettings.ddos.computeAverageValueTimeInterval       = settings->value(QLatin1Literal("computeAverageValueTimeInterval")).toUInt();
    formatedServerSettings.ddos.kickLimitMove                         = settings->value(QLatin1Literal("kickLimitMove")).toUInt();
    formatedServerSettings.ddos.kickLimitChat                         = settings->value(QLatin1Literal("kickLimitChat")).toUInt();
    formatedServerSettings.ddos.kickLimitOther                        = settings->value(QLatin1Literal("kickLimitOther")).toUInt();
    formatedServerSettings.ddos.dropGlobalChatMessageGeneral          = settings->value(QLatin1Literal("dropGlobalChatMessageGeneral")).toUInt();
    formatedServerSettings.ddos.dropGlobalChatMessageLocalClan        = settings->value(QLatin1Literal("dropGlobalChatMessageLocalClan")).toUInt();
    formatedServerSettings.ddos.dropGlobalChatMessagePrivate          = settings->value(QLatin1Literal("dropGlobalChatMessagePrivate")).toUInt();
    settings->endGroup();

    //chat allowed
    settings->beginGroup(QLatin1Literal("chat"));
    CommonSettings::commonSettings.chat_allow_all         = settings->value(QLatin1Literal("allow-all")).toBool();
    CommonSettings::commonSettings.chat_allow_local		= settings->value(QLatin1Literal("allow-local")).toBool();
    CommonSettings::commonSettings.chat_allow_private		= settings->value(QLatin1Literal("allow-private")).toBool();
    //CommonSettings::commonSettings.chat_allow_aliance		= settings->value(QLatin1Literal("allow-aliance")).toBool();
    CommonSettings::commonSettings.chat_allow_clan		= settings->value(QLatin1Literal("allow-clan")).toBool();
    settings->endGroup();

    settings->beginGroup(QLatin1Literal("db"));
    if(settings->value(QLatin1Literal("type")).toString()==QLatin1Literal("mysql"))
        formatedServerSettings.database.type					= CatchChallenger::ServerSettings::Database::DatabaseType_Mysql;
    else if(settings->value(QLatin1Literal("type")).toString()==QLatin1Literal("sqlite"))
        formatedServerSettings.database.type					= CatchChallenger::ServerSettings::Database::DatabaseType_SQLite;
    else if(settings->value(QLatin1Literal("type")).toString()==QLatin1Literal("postgresql"))
        formatedServerSettings.database.type					= CatchChallenger::ServerSettings::Database::DatabaseType_PostgreSQL;
    else
        formatedServerSettings.database.type					= CatchChallenger::ServerSettings::Database::DatabaseType_Mysql;
    switch(formatedServerSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            formatedServerSettings.database.mysql.host				= settings->value(QLatin1Literal("mysql_host")).toString();
            formatedServerSettings.database.mysql.db				= settings->value(QLatin1Literal("mysql_db")).toString();
            formatedServerSettings.database.mysql.login				= settings->value(QLatin1Literal("mysql_login")).toString();
            formatedServerSettings.database.mysql.pass				= settings->value(QLatin1Literal("mysql_pass")).toString();
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            formatedServerSettings.database.sqlite.file				= settings->value(QLatin1Literal("file")).toString();
        break;
    }
    if(settings->value(QLatin1Literal("db_fight_sync")).toString()==QLatin1Literal("FightSync_AtEachTurn"))
        formatedServerSettings.database.fightSync                       = CatchChallenger::ServerSettings::Database::FightSync_AtEachTurn;
    else if(settings->value(QLatin1Literal("db_fight_sync")).toString()==QLatin1Literal("FightSync_AtTheDisconnexion"))
        formatedServerSettings.database.fightSync                       = CatchChallenger::ServerSettings::Database::FightSync_AtTheDisconnexion;
    else
        formatedServerSettings.database.fightSync                       = CatchChallenger::ServerSettings::Database::FightSync_AtTheEndOfBattle;
    formatedServerSettings.database.positionTeleportSync=settings->value(QLatin1Literal("positionTeleportSync")).toBool();
    formatedServerSettings.database.secondToPositionSync=settings->value(QLatin1Literal("secondToPositionSync")).toUInt();
    settings->endGroup();

    //connection
    formatedServerSettings.automatic_account_creation   = settings->value(QLatin1Literal("automatic_account_creation")).toBool();
    formatedServerSettings.max_players					= settings->value(QLatin1Literal("max-players")).toUInt();
    formatedServerSettings.tolerantMode                 = settings->value(QLatin1Literal("tolerantMode")).toBool();

    //visibility algorithm
    settings->beginGroup(QLatin1Literal("MapVisibilityAlgorithm"));
    switch(settings->value(QLatin1Literal("MapVisibilityAlgorithm")).toUInt())
    {
        case 0:
            formatedServerSettings.mapVisibility.mapVisibilityAlgorithm		= MapVisibilityAlgorithmSelection_Simple;
        break;
        case 1:
            formatedServerSettings.mapVisibility.mapVisibilityAlgorithm		= MapVisibilityAlgorithmSelection_None;
        break;
        case 2:
            formatedServerSettings.mapVisibility.mapVisibilityAlgorithm		= MapVisibilityAlgorithmSelection_WithBorder;
        break;
        default:
            formatedServerSettings.mapVisibility.mapVisibilityAlgorithm		= MapVisibilityAlgorithmSelection_Simple;
        break;
    }
    settings->endGroup();
    if(formatedServerSettings.mapVisibility.mapVisibilityAlgorithm==MapVisibilityAlgorithmSelection_Simple)
    {
        settings->beginGroup(QLatin1Literal("MapVisibilityAlgorithm-Simple"));
        formatedServerSettings.mapVisibility.simple.max				= settings->value(QLatin1Literal("Max")).toUInt();
        formatedServerSettings.mapVisibility.simple.reshow			= settings->value(QLatin1Literal("Reshow")).toUInt();
        formatedServerSettings.mapVisibility.simple.reemit          = settings->value(QLatin1Literal("Reemit")).toBool();
        settings->endGroup();
    }
    else if(formatedServerSettings.mapVisibility.mapVisibilityAlgorithm==MapVisibilityAlgorithmSelection_WithBorder)
    {
        settings->beginGroup(QLatin1Literal("MapVisibilityAlgorithm-WithBorder"));
        formatedServerSettings.mapVisibility.withBorder.maxWithBorder	= settings->value(QLatin1Literal("MaxWithBorder")).toUInt();
        formatedServerSettings.mapVisibility.withBorder.reshowWithBorder= settings->value(QLatin1Literal("ReshowWithBorder")).toUInt();
        formatedServerSettings.mapVisibility.withBorder.max				= settings->value(QLatin1Literal("Max")).toUInt();
        formatedServerSettings.mapVisibility.withBorder.reshow			= settings->value(QLatin1Literal("Reshow")).toUInt();
        settings->endGroup();
    }

    {
        settings->beginGroup(QLatin1Literal("programmedEvent"));
            const QStringList &tempListType=settings->childGroups();
            int indexType=0;
            while(indexType<tempListType.size())
            {
                const QString &type=tempListType.at(indexType);
                settings->beginGroup(type);
                    const QStringList &tempList=settings->childGroups();
                    int index=0;
                    while(index<tempList.size())
                    {
                        const QString &groupName=tempList.at(index);
                        settings->beginGroup(groupName);
                        if(settings->contains(QLatin1Literal("value")) && settings->contains(QLatin1Literal("cycle")) && settings->contains(QLatin1Literal("offset")))
                        {
                            ServerSettings::ProgrammedEvent event;
                            event.value=settings->value(QLatin1Literal("value")).toString();
                            bool ok;
                            event.cycle=settings->value(QLatin1Literal("cycle")).toUInt(&ok);
                            if(!ok)
                                event.cycle=0;
                            event.offset=settings->value(QLatin1Literal("offset")).toUInt(&ok);
                            if(!ok)
                                event.offset=0;
                            if(event.cycle>0)
                                formatedServerSettings.programmedEventList[type][groupName]=event;
                        }
                        settings->endGroup();
                        index++;
                    }
                settings->endGroup();
                indexType++;
            }
        settings->endGroup();
    }

    settings->beginGroup(QLatin1Literal("city"));
    if(settings->value(QLatin1Literal("capture_frequency")).toString()==QLatin1Literal("week"))
        formatedServerSettings.city.capture.frenquency=City::Capture::Frequency_week;
    else
        formatedServerSettings.city.capture.frenquency=City::Capture::Frequency_month;

    if(settings->value(QLatin1Literal("capture_day")).toString()==QLatin1Literal("monday"))
        formatedServerSettings.city.capture.day=City::Capture::Monday;
    else if(settings->value(QLatin1Literal("capture_day")).toString()==QLatin1Literal("tuesday"))
        formatedServerSettings.city.capture.day=City::Capture::Tuesday;
    else if(settings->value(QLatin1Literal("capture_day")).toString()==QLatin1Literal("wednesday"))
        formatedServerSettings.city.capture.day=City::Capture::Wednesday;
    else if(settings->value(QLatin1Literal("capture_day")).toString()==QLatin1Literal("thursday"))
        formatedServerSettings.city.capture.day=City::Capture::Thursday;
    else if(settings->value(QLatin1Literal("capture_day")).toString()==QLatin1Literal("friday"))
        formatedServerSettings.city.capture.day=City::Capture::Friday;
    else if(settings->value(QLatin1Literal("capture_day")).toString()==QLatin1Literal("saturday"))
        formatedServerSettings.city.capture.day=City::Capture::Saturday;
    else if(settings->value(QLatin1Literal("capture_day")).toString()==QLatin1Literal("sunday"))
        formatedServerSettings.city.capture.day=City::Capture::Sunday;
    else
        formatedServerSettings.city.capture.day=City::Capture::Monday;
    formatedServerSettings.city.capture.hour=0;
    formatedServerSettings.city.capture.minute=0;
    const QStringList &capture_time_string_list=settings->value(QLatin1Literal("capture_time")).toString().split(QLatin1Literal(":"));
    if(capture_time_string_list.size()==2)
    {
        bool ok;
        formatedServerSettings.city.capture.hour=capture_time_string_list.first().toUInt(&ok);
        if(!ok)
            formatedServerSettings.city.capture.hour=0;
        else
        {
            formatedServerSettings.city.capture.minute=capture_time_string_list.last().toUInt(&ok);
            if(!ok)
                formatedServerSettings.city.capture.minute=0;
        }
    }
    settings->endGroup();

    server->setSettings(formatedServerSettings);
    server->setNormalSettings(formatedServerNormalSettings);
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    Q_UNUSED(a);

    bool datapack_loaded=false;

    QFileInfo datapackFolder(QCoreApplication::applicationDirPath()+QLatin1Literal("/datapack/informations.xml"));
    if(!datapackFolder.isFile())
    {
        qDebug() << "No datapack found into: " << datapackFolder.absoluteFilePath();
        return EXIT_FAILURE;
    }

    const QString &configFile=QCoreApplication::applicationDirPath()+QLatin1Literal("/server.properties");
    settings=new QSettings(configFile,QSettings::IniFormat);
    if(settings->status()!=QSettings::NoError)
    {
        qDebug() << "Error settings (1): " << settings->status();
        return EXIT_FAILURE;
    }
    NormalServerGlobal::checkSettingsFile(settings);

    if(settings->status()!=QSettings::NoError)
    {
        qDebug() << "Error settings (2): " << settings->status();
        return EXIT_FAILURE;
    }
    if(!Epoll::epoll.init())
        return EPOLLERR;

    #ifdef SERVERSSL
    server=new EpollSslServer();
    #else
    server=new EpollServer();
    #endif
    send_settings();

    if(!GlobalServerData::serverPrivateVariables.db.syncConnect(
                GlobalServerData::serverSettings.database.mysql.host.toLatin1(),
                GlobalServerData::serverSettings.database.mysql.db.toLatin1(),
                GlobalServerData::serverSettings.database.mysql.login.toLatin1(),
                GlobalServerData::serverSettings.database.mysql.pass.toLatin1()))
    {
        qDebug() << "Unable to connect to database:" << GlobalServerData::serverPrivateVariables.db.errorMessage();
        return EXIT_FAILURE;
    }

    if(!server->tryListen())
        return EPOLLERR;

    TimerCityCapture timerCityCapture;
    TimerDdos timerDdos;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    TimerDisplayEventBySeconds timerDisplayEventBySeconds;
    #endif
    TimerPositionSync timerPositionSync;
    TimerSendInsertMoveRemove timerSendInsertMoveRemove;
    {
        GlobalServerData::serverPrivateVariables.time_city_capture=FacilityLib::nextCaptureTime(GlobalServerData::serverSettings.city);
        const qint64 &time=GlobalServerData::serverPrivateVariables.time_city_capture.toMSecsSinceEpoch()-QDateTime::currentMSecsSinceEpoch();
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
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        if(!timerDisplayEventBySeconds.start(60*1000))
        {
            std::cerr << "timerDisplayEventBySeconds fail to set" << std::endl;
            return EXIT_FAILURE;
        }
    }
    #endif
    {
        if(GlobalServerData::serverSettings.database.secondToPositionSync>0)
            if(!timerPositionSync.start(GlobalServerData::serverSettings.database.secondToPositionSync*1000))
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
            if(!GlobalServerData::serverPrivateVariables.player_updater.start())
            {
                std::cerr << "player_updater timer fail to set" << std::endl;
                return EXIT_FAILURE;
            }
    }

    #ifndef SERVERNOBUFFER
    #ifdef SERVERSSL
    EpollSslClient::staticInit();
    #endif
    #endif
    char buf[4096];
    memset(buf,0,4096);
    /* Buffer where events are returned */
    epoll_event events[MAXEVENTS];

    server->loadAndFixSettings();
    server->initialize_the_database_prepared_query();
    bool tcpCork,tcpNodelay;
    {
        const ServerSettings &formatedServerSettings=server->getSettings();
        const NormalServerSettings &formatedServerNormalSettings=server->getNormalSettings();
        tcpCork=CommonSettings::commonSettings.tcpCork;
        tcpNodelay=formatedServerNormalSettings.tcpNodelay;

        if(!formatedServerNormalSettings.proxy.isEmpty())
        {
            qDebug() << "Proxy not supported: " << settings->status();
            return EXIT_FAILURE;
        }
        if(!formatedServerNormalSettings.proxy.isEmpty())
        {
            qDebug() << "Proxy not supported";
            return EXIT_FAILURE;
        }
        if(formatedServerSettings.database.type!=CatchChallenger::ServerSettings::Database::DatabaseType_PostgreSQL)
        {
            settings->beginGroup(QLatin1Literal("db"));
            qDebug() << "Only postgresql is supported for now:" << settings->value(QLatin1Literal("type")).toString();
            settings->endGroup();
            return EXIT_FAILURE;
        }
        if(formatedServerSettings.database.type!=CatchChallenger::ServerSettings::Database::DatabaseType_PostgreSQL)
        {
            settings->beginGroup(QLatin1Literal("db"));
            qDebug() << "Only postgresql is supported for now:" << settings->value(QLatin1Literal("type")).toString();
            settings->endGroup();
            return EXIT_FAILURE;
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
            qDebug() << "Clear connexion requested but server compiled with ssl support!";
            return EXIT_FAILURE;
        }
        #endif
        if(CommonSettings::commonSettings.httpDatapackMirror.isEmpty())
        {
            #ifdef CATCHCHALLENGERSERVERBLOCKCLIENTTOSERVERPACKETDECOMPRESSION
            qDebug() << "Need mirror because CATCHCHALLENGERSERVERBLOCKCLIENTTOSERVERPACKETDECOMPRESSION is def, need decompression to datapack list input";
            return EXIT_FAILURE;
            #endif
        }
        else
        {
            QStringList newMirrorList;
            QRegularExpression httpMatch("^https?://.+$");
            const QStringList &mirrorList=CommonSettings::commonSettings.httpDatapackMirror.split(";");
            int index=0;
            while(index<mirrorList.size())
            {
                const QString &mirror=mirrorList.at(index);
                if(!mirror.contains(httpMatch))
                {
                    qDebug() << "Mirror wrong: " << mirror.toLocal8Bit();
                    return EXIT_FAILURE;
                }
                if(mirror.endsWith("/"))
                    newMirrorList << mirror;
                else
                    newMirrorList << mirror+"/";
                index++;
            }
            CommonSettings::commonSettings.httpDatapackMirror=newMirrorList.join(";");
        }
    }

    char encodingBuff[1];
    #ifdef SERVERSSL
    encodingBuff[0]=0x01;
    #else
    encodingBuff[0]=0x00;
    #endif

    int numberOfConnectedClient=0;
    /* The event loop */
    int number_of_events, i;
    while(1)
    {
        number_of_events = Epoll::epoll.wait(events, MAXEVENTS);
        for(i = 0; i < number_of_events; i++)
        {
            switch(static_cast<BaseClassSwitch *>(events[i].data.ptr)->getType())
            {
                case BaseClassSwitch::Type::Server:
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    timerDisplayEventBySeconds.addServerCount();
                    #endif
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
                        if(!server->isReady())
                        {
                            /// \todo dont clean error on client into this case
                            std::cerr << "client connect when the server is not ready" << std::endl;
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
                                break;
                            }
                            else
                            {
                                std::cout << "connexion accepted" << std::endl;
                                break;
                            }
                        }
                        /* do at the protocol negociation to send the reason
                        if(numberOfConnectedClient>=GlobalServerData::serverSettings.max_players)
                        {
                            ::close(infd);
                            break;
                        }*/

                        /* Make the incoming socket non-blocking and add it to the
                        list of fds to monitor. */
                        numberOfConnectedClient++;

                        int s = EpollSocket::make_non_blocking(infd);
                        if(s == -1)
                            std::cerr << "unable to make to socket non blocking" << std::endl;
                        else
                        {
                            if(tcpCork)
                            {
                                //set cork for CatchChallener because don't have real time part
                                int state = 1;
                                if(setsockopt(infd, IPPROTO_TCP, TCP_CORK, &state, sizeof(state))!=0)
                                    std::cerr << "Unable to apply tcp cork" << std::endl;
                            }
                            else if(tcpNodelay)
                            {
                                //set no delay to don't try group the packet and improve the performance
                                int state = 1;
                                if(setsockopt(infd, IPPROTO_TCP, TCP_NODELAY, &state, sizeof(state))!=0)
                                    std::cerr << "Unable to apply tcp no delay" << std::endl;
                            }

                            Client *client;
                            switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
                            {
                                case MapVisibilityAlgorithmSelection_Simple:
                                    client=new MapVisibilityAlgorithm_Simple_StoreOnSender(infd
                                                       #ifdef SERVERSSL
                                                       ,server->getCtx()
                                                       #endif
                                                                                           );
                                break;
                                case MapVisibilityAlgorithmSelection_WithBorder:
                                    client=new MapVisibilityAlgorithm_WithBorder_StoreOnSender(infd
                                                           #ifdef SERVERSSL
                                                           ,server->getCtx()
                                                           #endif
                                                                                               );
                                break;
                                default:
                                case MapVisibilityAlgorithmSelection_None:
                                    client=new MapVisibilityAlgorithm_None(infd
                                       #ifdef SERVERSSL
                                       ,server->getCtx()
                                       #endif
                                                                           );
                                break;
                            }
                            //just for informations
                            {
                                char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
                                const int &s = getnameinfo(&in_addr, in_len,
                                hbuf, sizeof hbuf,
                                sbuf, sizeof sbuf,
                                NI_NUMERICHOST | NI_NUMERICSERV);
                                if(s == 0)
                                {
                                    //std::cout << "Accepted connection on descriptor " << infd << "(host=" << hbuf << ", port=" << sbuf << ")" << std::endl;
                                    client->socketStringSize=strlen(hbuf)+strlen(sbuf)+1+1;
                                    client->socketString=new char[client->socketStringSize];
                                    strcpy(client->socketString,hbuf);
                                    strcat(client->socketString,":");
                                    strcat(client->socketString,sbuf);
                                    client->socketString[client->socketStringSize-1]='\0';
                                }
                            }
                            epoll_event event;
                            event.data.ptr = client;
                            event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;//EPOLLET | EPOLLOUT
                            s = Epoll::epoll.ctl(EPOLL_CTL_ADD, infd, &event);
                            if(s == -1)
                            {
                                std::cerr << "epoll_ctl on socket error" << std::endl;
                                delete client;
                            }
                            else
                                ::write(infd,encodingBuff,sizeof(encodingBuff));
                        }
                    }
                    continue;
                }
                break;
                case BaseClassSwitch::Type::Client:
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    timerDisplayEventBySeconds.addClientCount();
                    #endif
                    Client *client=static_cast<Client *>(events[i].data.ptr);
                    if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occured on this fd, or the socket is not
                        ready for reading (why were we notified then?) */
                        std::cerr << "client epoll error: " << events[i].events << std::endl;
                        numberOfConnectedClient--;
                        client->disconnectClient();
                        delete client;
                        continue;
                    }
                    //ready to read
                    if(events[i].events & EPOLLIN)
                        client->parseIncommingData();
                    #ifndef SERVERNOBUFFER
                    //ready to write
                    if(events[i].events & EPOLLOUT)
                        if(!closed)
                            client->flush();
                    #endif
                    if(events[i].events & EPOLLHUP || events[i].events & EPOLLRDHUP)
                    {
                        numberOfConnectedClient--;
                        client->disconnectClient();
                        delete client;//disconnected, remove the object
                    }
                }
                break;
                case BaseClassSwitch::Type::Timer:
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    timerDisplayEventBySeconds.addTimerCount();
                    #endif
                    static_cast<EpollTimer *>(events[i].data.ptr)->exec();
                    static_cast<EpollTimer *>(events[i].data.ptr)->validateTheTimer();
                break;
                case BaseClassSwitch::Type::Database:
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    timerDisplayEventBySeconds.addDbCount();
                    #endif
                    EpollPostgresql *db=static_cast<EpollPostgresql *>(events[i].data.ptr);
                    db->epollEvent(events[i].events);
                    if(!datapack_loaded)
                    {
                        if(db->isConnected())
                        {
                            std::cout << "datapack_loaded not loaded: start preload data " << std::endl;
                            server->preload_the_data();
                            datapack_loaded=true;
                        }
                        else
                            std::cerr << "datapack_loaded not loaded: but database seam don't be connected" << std::endl;
                    }
                }
                break;
                default:
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    timerDisplayEventBySeconds.addOtherCount();
                    #endif
                    std::cerr << "unknown event" << std::endl;
                break;
            }
        }
    }
    server->close();
    server->unload_the_data();
    delete server;
    return EXIT_SUCCESS;
}
