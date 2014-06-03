#include <iostream>
#include <netdb.h>
#include <unistd.h>
#include <cstring>

#include "epoll/EpollSocket.h"
#include "epoll/EpollSslClient.h"
#include "epoll/EpollSslServer.h"
#include "epoll/EpollClient.h"
#include "epoll/EpollServer.h"
#include "epoll/Epoll.h"
#include "epoll/EpollTimer.h"
#include "epoll/TimerDisplayEventBySeconds.h"
#include "base/ServerStructures.h"
#include "NormalServer.h"
#include "base/GlobalServerData.h"
#include "base/Client.h"

#define MAXEVENTS 512

using namespace CatchChallenger;

#ifndef SERVERNOSSL
EpollSslServer *server=NULL;
#else
Epollerver *server=NULL;
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

    CommonSettings::commonSettings.forcedSpeed                      = settings->value(QLatin1Literal("forcedSpeed")).toUInt();
    CommonSettings::commonSettings.dontSendPseudo					= settings->value(QLatin1Literal("dontSendPseudo")).toBool();
    CommonSettings::commonSettings.forceClientToSendAtMapChange		= settings->value(QLatin1Literal("forceClientToSendAtMapChange")).toBool();
    formatedServerSettings.dontSendPlayerType                       = settings->value(QLatin1Literal("dontSendPlayerType")).toBool();

    //the listen
    formatedServerNormalSettings.server_port			= settings->value(QLatin1Literal("server-port")).toUInt();
    formatedServerNormalSettings.server_ip				= settings->value(QLatin1Literal("server-ip")).toString();
    formatedServerNormalSettings.proxy					= settings->value(QLatin1Literal("proxy")).toString();
    formatedServerNormalSettings.proxy_port				= settings->value(QLatin1Literal("proxy_port")).toUInt();

    formatedServerSettings.anonymous					= settings->value(QLatin1Literal("anonymous")).toBool();
    formatedServerSettings.server_message				= settings->value(QLatin1Literal("server_message")).toString();
    formatedServerSettings.httpDatapackMirror			= settings->value(QLatin1Literal("httpDatapackMirror")).toString();
    formatedServerSettings.datapackCache				= settings->value(QLatin1Literal("datapackCache")).toInt();
    #ifdef Q_OS_LINUX
    settings->beginGroup(QLatin1Literal("Linux"));
    formatedServerNormalSettings.linuxSettings.tcpCork	= settings->value(QLatin1Literal("tcpCork")).toBool();
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
        formatedServerSettings.mapVisibility.simple.storeOnSender   = settings->value(QLatin1Literal("StoreOnSender")).toBool();
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
        formatedServerSettings.mapVisibility.withBorder.storeOnSender	= settings->value(QLatin1Literal("StoreOnSender")).toBool();
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

    const QString &configFile=QCoreApplication::applicationDirPath()+QLatin1Literal("/server.properties");
    settings=new QSettings(configFile,QSettings::IniFormat);
    if(settings->status()!=QSettings::NoError)
    {
        qDebug() << "Error settings (1): " << settings->status();
        return EXIT_FAILURE;
    }
    CatchChallenger::NormalServer::checkSettingsFile(settings);

    if(settings->status()!=QSettings::NoError)
    {
        qDebug() << "Error settings (2): " << settings->status();
        return EXIT_FAILURE;
    }
    if(!Epoll::epoll.init())
        return EPOLLERR;

    #ifndef SERVERNOSSL
    server=new EpollSslServer();
    #else
    server=new EpollServer();
    #endif
    send_settings();
    if(!server->tryListen())
        return EPOLLERR;
    server->preload_the_data();

    TimerDisplayEventBySeconds timerDisplayEventBySeconds;
    if(!timerDisplayEventBySeconds.init())
        return EXIT_FAILURE;

    #ifndef SERVERNOBUFFER
    #ifndef SERVERNOSSL
    EpollSslClient::staticInit();
    #endif
    #endif
    char buf[4096];
    memset(buf,0,4096);
    /* Buffer where events are returned */
    epoll_event events[MAXEVENTS];

    bool tcpCork;
    {
        const ServerSettings &formatedServerSettings=server->getSettings();
        const NormalServerSettings &formatedServerNormalSettings=server->getNormalSettings();
        tcpCork=formatedServerNormalSettings.linuxSettings.tcpCork;

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
            qDebug() << "Only postgresql is supported for now: " << settings->value(QLatin1Literal("type")).toString();
            return EXIT_FAILURE;
        }
    }

    bool closed;
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
                        if(numberOfConnectedClient>=GlobalServerData::serverSettings.max_players)
                        {
                            ::close(infd);
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
                                std::cout << "Accepted connection on descriptor " << infd << "(host=" << hbuf << ", port=" << sbuf << ")" << std::endl;
                        }

                        /* Make the incoming socket non-blocking and add it to the
                        list of fds to monitor. */
                        #ifndef SERVERNOSSL
                        EpollSslClient *epollClient=new EpollSslClient(infd,server->getCtx(),tcpCork);
                        #else
                        EpollClient *epollClient=new EpollClient(infd,server->getCtx(),tcpCork);
                        #endif
                        numberOfConnectedClient++;
                        if(!epollClient->init())
                            delete epollClient;
                        else
                            /*Client *client=*/new Client(new ConnectedSocket(epollClient),server->getClientMapManagement());
                    }
                    continue;
                }
                break;
                case BaseClassSwitch::Type::Client:
                {
                    closed=true;
                    //timerDisplayEventBySeconds.addCount();
                    #ifndef SERVERNOSSL
                    EpollSslClient *client=static_cast<EpollSslClient *>(events[i].data.ptr);
                    #else
                    EpollClient *client=static_cast<EpollClient *>(events[i].data.ptr);
                    #endif
                    if((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occured on this fd, or the socket is not
                        ready for reading (why were we notified then?) */
                        std::cerr << "client epoll error: " << events[i].events << std::endl;
                        delete client;
                        continue;
                    }
                    //ready to read
                    if(events[i].events & EPOLLIN)
                    {
                        /* We have data on the fd waiting to be read. Read and
                        display it. We must read whatever data is available
                        completely, as we are running in edge-triggered mode
                        and won't get a notification again for the same
                        data. */
                        const ssize_t &count = client->read(buf,sizeof(buf));
                        //bug or close, or buffer full
                        if(count<0)
                        {
                            delete client;
                            numberOfConnectedClient--;
                        }
                        else
                        {
                            if(client->write(buf,count)!=count)
                            {
                                //buffer full, we disconnect this client
                                delete client;
                                numberOfConnectedClient--;
                            }
                            else
                                closed=false;
                        }
                    }
                    #ifndef SERVERNOBUFFER
                    //ready to write
                    if(events[i].events & EPOLLOUT)
                        if(!closed)
                            client->flush();
                    #endif
                }
                break;
                case BaseClassSwitch::Type::Timer:
                    static_cast<EpollTimer *>(events[i].data.ptr)->exec();
                break;
                default:
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
