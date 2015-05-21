#include "Api_client_virtual.h"

using namespace CatchChallenger;

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#endif

//need host + port here to have datapack base

Api_client_virtual::Api_client_virtual(ConnectedSocket *socket, const QString &forcedDatapack, const QString &mainDatapackCode, const QString &subDatapackCode) :
    Api_protocol(socket)
{
    this->forcedDatapackBase=forcedDatapack;
    this->forcedDatapackMain=this->forcedDatapackBase+"map/main/"+mainDatapackCode+"/";
    this->forcedDatapackSub=this->forcedDatapackMain+"sub/"+subDatapackCode+"/";
    mDatapackBase=QStringLiteral("%1/datapack/").arg(QCoreApplication::applicationDirPath());
    mDatapackMain=mDatapackBase+"map/main/"+mainDatapackCode+"/";
    mDatapackSub=mDatapackMain+"sub/"+subDatapackCode+"/";
    this->mMainDatapackCode=mainDatapackCode;
    this->mSubDatapackCode=subDatapackCode;
}

Api_client_virtual::~Api_client_virtual()
{
}

void Api_client_virtual::sendDatapackContent()
{
    /*emit */haveTheDatapack();
}

void Api_client_virtual::tryDisconnect()
{
    if(socket!=NULL)
        socket->disconnectFromHost();
}

QString Api_client_virtual::mainDatapackCode() const
{
    return mMainDatapackCode;
}

QString Api_client_virtual::subDatapackCode() const
{
    return mMainDatapackCode;
}

QString Api_client_virtual::datapackPathBase() const
{
    return forcedDatapackBase;
}

QString Api_client_virtual::datapackPathMain() const
{
    return forcedDatapackMain;
}

QString Api_client_virtual::datapackPathSub() const
{
    return forcedDatapackSub;
}

//general data
void Api_client_virtual::defineMaxPlayers(const quint16 &)
{
}
