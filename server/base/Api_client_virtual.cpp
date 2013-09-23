#include "Api_client_virtual.h"

using namespace CatchChallenger;

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#endif

//need host + port here to have datapack base

Api_client_virtual::Api_client_virtual(ConnectedSocket *socket, const QString &forcedDatapack) :
    Api_protocol(socket)
{
    this->forcedDatapack=forcedDatapack;
    datapack_base_name=QString("%1/datapack/").arg(QCoreApplication::applicationDirPath());
}

Api_client_virtual::~Api_client_virtual()
{
}

void Api_client_virtual::sendDatapackContent()
{
    emit haveTheDatapack();
}

void Api_client_virtual::tryDisconnect()
{
    socket->disconnectFromHost();
}

QString Api_client_virtual::get_datapack_base_name() const
{
    return forcedDatapack;
}

//general data
void Api_client_virtual::defineMaxPlayers(const quint16 &)
{
}
