#include <sys/epoll.h>
#include <errno.h>
#include <iostream>
#include <list>
#include <sys/socket.h>
#include <netdb.h>

#include <QSettings>
#include <QString>
#include <QCoreApplication>

#include "EpollClient.h"
#include "EpollSocket.h"
#include "EpollServer.h"
#include "Epoll.h"
#include "EpollTimer.h"
#include "TimerDisplayEventBySeconds.h"
#include "../base/ServerStructures.h"
#include "NormalServer.h"

#define MAXEVENTS 512
#define MAXCLIENT 6000

using namespace CatchChallenger;

EpollServer *server=NULL;
QSettings *settings=NULL;

void send_settings()
{
    ServerSettings formatedServerSettings=server->getSettings();

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
    formatedServerSettings.server_port					= settings->value(QLatin1Literal("server-port")).toUInt();
    formatedServerSettings.server_ip					= settings->value(QLatin1Literal("server-ip")).toString();
    formatedServerSettings.anonymous					= settings->value(QLatin1Literal("anonymous")).toBool();
    formatedServerSettings.server_message				= settings->value(QLatin1Literal("server_message")).toString();
    formatedServerSettings.proxy					    = settings->value(QLatin1Literal("proxy")).toString();
    formatedServerSettings.proxy_port					= settings->value(QLatin1Literal("proxy_port")).toUInt();

    formatedServerSettings.httpDatapackMirror			= settings->value(QLatin1Literal("httpDatapackMirror")).toString();
    formatedServerSettings.datapackCache				= settings->value(QLatin1Literal("datapackCache")).toInt();
    #ifdef Q_OS_LINUX
    settings->beginGroup(QLatin1Literal("Linux"));
    formatedServerSettings.linuxSettings.tcpCork		= settings->value(QLatin1Literal("tcpCork")).toBool();
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

    {
        bool ok;
        settings->beginGroup(QLatin1Literal("bitcoin"));
        formatedServerSettings.bitcoin.address=settings->value(QLatin1Literal("address")).toString();
        if(settings->contains(QLatin1Literal("binaryPath")) && !settings->value(QLatin1Literal("binaryPath")).toString().isEmpty())
            formatedServerSettings.bitcoin.binaryPath=settings->value(QLatin1Literal("binaryPath")).toString();
        else
        {
            #ifdef Q_OS_WIN32
            formatedServerSettings.bitcoin.binaryPath                         = QLatin1Literal("%application_path%/bitcoin/bitcoind.exe");
            #else
            formatedServerSettings.bitcoin.binaryPath                         = QLatin1Literal("/usr/bin/bitcoind");
            #endif
        }
        formatedServerSettings.bitcoin.enabled=(settings->value(QLatin1Literal("enabled")).toString()==QLatin1Literal("true"));
        if(formatedServerSettings.bitcoin.enabled)
        {
            formatedServerSettings.bitcoin.fee=settings->value(QLatin1Literal("fee")).toDouble(&ok);
            if(!ok)
                formatedServerSettings.bitcoin.fee=1.0;
            formatedServerSettings.bitcoin.port=settings->value(QLatin1Literal("port")).toUInt(&ok);
            if(!ok)
                formatedServerSettings.bitcoin.port=46349;
            formatedServerSettings.bitcoin.workingPath=settings->value(QLatin1Literal("workingPath")).toString();
            if(settings->contains(QLatin1Literal("workingPath")) && !settings->value(QLatin1Literal("workingPath")).toString().isEmpty())
                formatedServerSettings.bitcoin.workingPath=settings->value(QLatin1Literal("workingPath")).toString();
            else
            {
                #ifdef Q_OS_WIN32
                formatedServerSettings.bitcoin.workingPath                        = QLatin1Literal("%application_path%/bitcoin-storage/");
                #else
                formatedServerSettings.bitcoin.workingPath                        = QDir::homePath()+QLatin1Literal("/.config/CatchChallenger/server/bitoin/");
                #endif
            }
        }
        settings->endGroup();
    }

    server->setSettings(formatedServerSettings);
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

    server=new EpollServer();
    send_settings();
    if(!server->tryListen())
        return EXIT_FAILURE;
    /*TimerDisplayEventBySeconds timerDisplayEventBySeconds;
    if(!timerDisplayEventBySeconds.init())
        return EXIT_FAILURE;*/

    char buf[512];
    /* Buffer where events are returned */
    epoll_event events[MAXEVENTS];

    EpollClient* clients[MAXCLIENT];
    int clientListSize=0;
    /* The event loop */
    int number_of_events, i;
    while(1)
    {
        number_of_events = Epoll::epoll.wait(events, MAXEVENTS);
        for(i = 0; i < number_of_events; i++)
        {
            //timerDisplayEventBySeconds.addCount();
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
                        fprintf(stderr, "server epoll error\n");
                        continue;
                    }
                    /* We have a notification on the listening socket, which
                    means one or more incoming connections. */
                    while(1)
                    {
                        if(clientListSize>=MAXCLIENT)
                            break;
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
                                perror("accept");
                                break;
                            }
                        }
                        //just for informations
                        {
                            char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
                            const int &s = getnameinfo(&in_addr, in_len,
                            hbuf, sizeof hbuf,
                            sbuf, sizeof sbuf,
                            NI_NUMERICHOST | NI_NUMERICSERV);
                            if(s == 0)
                                printf("Accepted connection on descriptor %d (host=%s, port=%s)\n", infd, hbuf, sbuf);
                        }

                        /* Make the incoming socket non-blocking and add it to the
                        list of fds to monitor. */
                        EpollClient *client=new EpollClient();
                        client->infd=infd;
                        clients[clientListSize]=client;
                        clientListSize++;
                        int s = EpollSocket::make_non_blocking(infd);
                        if(s == -1)
                            return EXIT_FAILURE;
                        epoll_event event;
                        event.data.ptr = client;
                        event.events = EPOLLIN | EPOLLET | EPOLLOUT;
                        s = Epoll::epoll.ctl(EPOLL_CTL_ADD, infd, &event);
                        if(s == -1)
                        {
                            perror("epoll_ctl");
                            return EXIT_FAILURE;
                        }
                    }
                    continue;
                }
                break;
                case BaseClassSwitch::Type::Client:
                {
                    EpollClient *client=static_cast<EpollClient *>(events[i].data.ptr);
                    if ((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN) && !(events[i].events & EPOLLOUT)))
                    {
                        /* An error has occured on this fd, or the socket is not
                        ready for reading (why were we notified then?) */
                        fprintf(stderr, "client epoll error\n");
                        client->close();
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
                        while (1)
                        {
                            const ssize_t &count = client->read(buf, sizeof buf);
                            if(count>0)
                            {
                                //broadcast to all the client
                                int index=0;
                                while(index<clientListSize)
                                {
                                    clients[index]->write(buf,count);
                                    index++;
                                }
                            }
                            else
                                break;
                        }
                    }
                    //ready to write
                    if(events[i].events & EPOLLOUT)
                    {
                        client->flush();
                    }
                }
                break;
                case BaseClassSwitch::Type::Timer:
                    static_cast<EpollTimer *>(events[i].data.ptr)->exec();
                break;
                default:
                    fprintf(stderr, "unknown event\n");
                break;
            }
        }
    }
    server->close();
    delete server;
    delete settings;
    return EXIT_SUCCESS;
}
