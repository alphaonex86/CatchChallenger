#include "BaseServerLogin.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/GeneralVariable.h"
#include "../VariableServer.h"

#include <QDebug>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <iostream>

using namespace CatchChallenger;

#ifdef Q_OS_LINUX
FILE * BaseServerLogin::fpRandomFile=NULL;
#endif
quint32 BaseServerLogin::tokenForAuthSize=0;

BaseServerLogin::BaseServerLogin() :
    databaseBaseLogin(NULL)
{
}

BaseServerLogin::~BaseServerLogin()
{
}

void BaseServerLogin::preload_the_randomData()
{
    GlobalServerData::serverPrivateVariables.tokenForAuthSize=0;
    GlobalServerData::serverPrivateVariables.randomData.clear();

    if(GlobalServerData::serverSettings.benchmark)
    {
        srand(0);
        QDataStream randomDataStream(&GlobalServerData::serverPrivateVariables.randomData, QIODevice::WriteOnly);
        randomDataStream.setVersion(QDataStream::Qt_4_4);
        int index=0;
        while(index<CATCHCHALLENGER_SERVER_RANDOM_INTERNAL_SIZE)
        {
            randomDataStream << quint8(rand()%256);
            index++;
        }
    }

    #ifdef Q_OS_LINUX
    if(GlobalServerData::serverPrivateVariables.fpRandomFile!=NULL)
        fclose(GlobalServerData::serverPrivateVariables.fpRandomFile);
    GlobalServerData::serverPrivateVariables.fpRandomFile = fopen("/dev/urandom","rb");
    if(GlobalServerData::serverPrivateVariables.fpRandomFile==NULL)
        qDebug() << "Unable to open /dev/urandom to have trusted number generator";
    //fclose(GlobalServerData::serverPrivateVariables.fpRandomFile);-> used later into ./base/ClientNetworkRead.cpp for token
    GlobalServerData::serverPrivateVariables.randomData.resize(CATCHCHALLENGER_SERVER_RANDOM_INTERNAL_SIZE);
    const int &returnedSize=fread(GlobalServerData::serverPrivateVariables.randomData.data(),1,CATCHCHALLENGER_SERVER_RANDOM_INTERNAL_SIZE,GlobalServerData::serverPrivateVariables.fpRandomFile);
    if(returnedSize!=CATCHCHALLENGER_SERVER_RANDOM_INTERNAL_SIZE)
    {
        qDebug() << QStringLiteral("CATCHCHALLENGER_SERVER_RANDOM_INTERNAL_SIZE don't match with urandom size: %1").arg(returnedSize);
        abort();
    }
    if(GlobalServerData::serverPrivateVariables.randomData.size()!=CATCHCHALLENGER_SERVER_RANDOM_INTERNAL_SIZE)
    {
        qDebug() << QStringLiteral("GlobalServerData::serverPrivateVariables.randomData.size() don't match with urandom size");
        abort();
    }
    #else
    QDataStream randomDataStream(&GlobalServerData::serverPrivateVariables.randomData, QIODevice::WriteOnly);
    randomDataStream.setVersion(QDataStream::Qt_4_4);
    int index=0;
    while(index<CATCHCHALLENGER_SERVER_RANDOM_INTERNAL_SIZE)
    {
        randomDataStream << quint8(rand()%256);
        index++;
    }
    #endif
}

void BaseServerLogin::unload()
{
    unload_the_randomData();
}

void BaseServerLogin::unload_the_randomData()
{
    #ifdef Q_OS_LINUX
    if(GlobalServerData::serverPrivateVariables.fpRandomFile!=NULL)
    {
        fclose(GlobalServerData::serverPrivateVariables.fpRandomFile);
        GlobalServerData::serverPrivateVariables.fpRandomFile=NULL;
    }
    #endif
    GlobalServerData::serverPrivateVariables.tokenForAuthSize=0;
    GlobalServerData::serverPrivateVariables.randomData.clear();
}
