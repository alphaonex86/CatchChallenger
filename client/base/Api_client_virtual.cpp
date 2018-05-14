#include "Api_client_virtual.h"

using namespace CatchChallenger;

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#endif
#include <QString>

//need host + port here to have datapack base

Api_client_virtual::Api_client_virtual(ConnectedSocket *socket) :
    Api_protocol(socket)
{
    mDatapackBase=QStringLiteral("%1/datapack/").arg(QCoreApplication::applicationDirPath()).toStdString();
}

Api_client_virtual::~Api_client_virtual()
{
}

void Api_client_virtual::sendDatapackContentBase(const std::string &hashBase)
{
    Q_UNUSED(hashBase);
    /*emit */haveTheDatapack();
}

void Api_client_virtual::sendDatapackContentMainSub(const std::string &hashMain,const std::string &hashSub)
{
    Q_UNUSED(hashMain);
    Q_UNUSED(hashSub);
    /*emit */haveTheDatapackMainSub();
}

void Api_client_virtual::tryDisconnect()
{
    if(socket!=NULL)
        socket->disconnectFromHost();
}

//general data
void Api_client_virtual::defineMaxPlayers(const uint16_t &)
{
}
