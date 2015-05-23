#include "BaseServerLogin.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/GeneralVariable.h"
#include "../VariableServer.h"
#include "../VariableServer.h"

#include <QDebug>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <iostream>

using namespace CatchChallenger;

#ifdef Q_OS_LINUX
FILE * BaseServerLogin::fpRandomFile=NULL;
#endif
BaseServerLogin::TokenLink BaseServerLogin::tokenForAuth[];
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
    BaseServerLogin::tokenForAuthSize=0;

    //to have previsible data
    /*if(GlobalServerData::serverSettings.benchmark)
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
    else
    {*/
    //}
}

void BaseServerLogin::unload()
{
    unload_the_randomData();
}

void BaseServerLogin::unload_the_randomData()
{
    #ifdef Q_OS_LINUX
    if(BaseServerLogin::fpRandomFile!=NULL)
    {
        fclose(BaseServerLogin::fpRandomFile);
        BaseServerLogin::fpRandomFile=NULL;
    }
    #endif
    BaseServerLogin::tokenForAuthSize=0;
    //GlobalServerData::serverPrivateVariables.randomData.clear();
}
