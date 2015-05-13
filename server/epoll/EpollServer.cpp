#ifndef SERVERSSL

#include "EpollServer.h"
#include "EpollSocket.h"
#include "Epoll.h"

#include "../base/ServerStructures.h"
#include "../base/GlobalServerData.h"

using namespace CatchChallenger;

EpollServer::EpollServer()
{
    ready=false;

    normalServerSettings.server_ip      = QString();
    normalServerSettings.server_port    = 42489;
    normalServerSettings.useSsl         = true;
    #ifdef Q_OS_LINUX
    CommonSettingsServer::commonSettingsServer.tcpCork                      = true;
    #endif
    GlobalServerData::serverPrivateVariables.db=new EpollPostgresql();
}

EpollServer::~EpollServer()
{
    delete GlobalServerData::serverPrivateVariables.db;
}

void EpollServer::preload_finish()
{
    BaseServer::preload_finish();
    qDebug() << QStringLiteral("Waiting connection on port %1").arg(normalServerSettings.server_port);
    ready=true;
}

bool EpollServer::isReady()
{
    return ready;
}

bool EpollServer::tryListen()
{
    if(!normalServerSettings.server_ip.isEmpty())
        return tryListenInternal(normalServerSettings.server_ip.toUtf8().constData(), QString::number(normalServerSettings.server_port).toUtf8().constData());
    else
        return tryListenInternal(NULL, QString::number(normalServerSettings.server_port).toUtf8().constData());
}

void EpollServer::quitForCriticalDatabaseQueryFailed()
{
    abort();
}

void EpollServer::preload_the_data()
{
    BaseServer::preload_the_data();
}

void EpollServer::unload_the_data()
{
    BaseServer::unload_the_data();
}

void EpollServer::setNormalSettings(const NormalServerSettings &settings)
{
    normalServerSettings=settings;
    loadAndFixSettings();
}

NormalServerSettings EpollServer::getNormalSettings() const
{
    return normalServerSettings;
}

void EpollServer::loadAndFixSettings()
{
    if(normalServerSettings.server_port<=0)
        normalServerSettings.server_port=42489;
    if(normalServerSettings.proxy_port<=0)
        normalServerSettings.proxy=QString();
}
#endif
