#ifdef SERVERSSL

#include "EpollSslServer.h"
#include "EpollSocket.h"
#include "Epoll.h"

#include "../base/ServerStructures.h"
#include "../base/GlobalServerData.h"

using namespace CatchChallenger;

void eventLoop();

EpollSslServer::EpollSslServer()
{
    ready=false;

    normalServerSettings.server_ip      = std::string();
    normalServerSettings.server_port    = 42489;
    normalServerSettings.useSsl         = true;
    #ifdef __linux__
    CommonSettings::commonSettings.tcpCork                      = false;
    #endif
    GlobalServerData::serverPrivateVariables.db=new EpollPostgresql();
}

EpollSslServer::~EpollSslServer()
{
    delete GlobalServerData::serverPrivateVariables.db;
}

void EpollSslServer::preload_finish()
{
    BaseServer::preload_finish();
    if(!ready)
    {
        qDebug() << std::stringLiteral("Waiting connection on port %1").arg(normalServerSettings.server_port);
        ready=true;

        if(!tryListen())
            abort();
    }
    else
        qDebug() << std::stringLiteral("EpollSslServer::preload_finish() double event dropped");
}

bool EpollSslServer::isReady()
{
    return ready;
}

void EpollSslServer::quitForCriticalDatabaseQueryFailed()
{
    abort();
}

bool EpollSslServer::tryListen()
{
    if(!normalServerSettings.server_ip.isEmpty())
        return trySslListen(normalServerSettings.server_ip.toUtf8().constData(), std::to_string(normalServerSettings.server_port).toUtf8().constData(),"server.crt", "server.key");
    else
        return trySslListen(NULL, std::to_string(normalServerSettings.server_port).toUtf8().constData(),"server.crt", "server.key");
}

void EpollSslServer::preload_the_data()
{
    BaseServer::preload_the_data();
}

void EpollSslServer::unload_the_data()
{
    close();
    ready=false;
    BaseServer::unload_the_data();
}


void EpollSslServer::setNormalSettings(const NormalServerSettings &settings)
{
    normalServerSettings=settings;
    loadAndFixSettings();
}

NormalServerSettings EpollSslServer::getNormalSettings() const
{
    return normalServerSettings;
}

void EpollSslServer::loadAndFixSettings()
{
    if(normalServerSettings.server_port<=0)
        normalServerSettings.server_port=42489;
    if(normalServerSettings.proxy_port<=0)
        normalServerSettings.proxy=std::string();
}
#endif
