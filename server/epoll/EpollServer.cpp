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
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    GlobalServerData::serverPrivateVariables.db_login=new EpollDatabaseAsync();
    #endif
    GlobalServerData::serverPrivateVariables.db_base=new EpollDatabaseAsync();
    GlobalServerData::serverPrivateVariables.db_common=new EpollDatabaseAsync();
    GlobalServerData::serverPrivateVariables.db_server=new EpollDatabaseAsync();
}

EpollServer::~EpollServer()
{
    delete GlobalServerData::serverPrivateVariables.db_server;
    delete GlobalServerData::serverPrivateVariables.db_common;
    delete GlobalServerData::serverPrivateVariables.db_base;
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    delete GlobalServerData::serverPrivateVariables.db_login;
    #endif
}

void EpollServer::preload_finish()
{
    BaseServer::preload_finish();
    if(!ready)
    {
        qDebug() << QStringLiteral("Waiting connection on port %1").arg(normalServerSettings.server_port);
        ready=true;

        if(!tryListen())
            abort();
    }
    else
        qDebug() << QStringLiteral("EpollServer::preload_finish() double event dropped");
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
    close();
    ready=false;
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
