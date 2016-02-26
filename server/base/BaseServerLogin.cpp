#include "BaseServerLogin.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/GeneralVariable.h"
#include "../VariableServer.h"
#include "../VariableServer.h"

#include <iostream>

using namespace CatchChallenger;

#ifdef __linux__
FILE * BaseServerLogin::fpRandomFile=NULL;
#endif
#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
BaseServerLogin::TokenLink BaseServerLogin::tokenForAuth[];
uint32_t BaseServerLogin::tokenForAuthSize=0;
#endif

BaseServerLogin::BaseServerLogin()
    #ifndef CATCHCHALLENGER_CLASS_LOGIN
    : databaseBaseLogin(NULL)
    #endif
{
}

BaseServerLogin::~BaseServerLogin()
{
}

void BaseServerLogin::preload_the_randomData()
{
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    BaseServerLogin::tokenForAuthSize=0;
    #endif
    #ifdef __linux__
    if(BaseServerLogin::fpRandomFile!=NULL)
        fclose(BaseServerLogin::fpRandomFile);
    BaseServerLogin::fpRandomFile = fopen(RANDOMFILEDEVICE,"rb");
    if(fpRandomFile==NULL)
    {
        std::cerr << "Unable to open " << RANDOMFILEDEVICE << " to generate random token" << std::endl;
        /* allow poor quality number:
         * 1) more easy to run, allow start include if RANDOMFILEDEVICE can't be read
         * 2) it's for very small server (Lan) or internal communication */
        #if ! defined(CATCHCHALLENGER_CLIENT) && ! defined(CATCHCHALLENGER_SOLO)
        abort();
        #endif
    }
    #endif

    //to have previsible data
    /*if(GlobalServerData::serverSettings.benchmark)
    {
        srand(time(NULL));
        QDataStream randomDataStream(&GlobalServerData::serverPrivateVariables.randomData, QIODevice::WriteOnly);
        randomDataStream.setVersion(QDataStream::Qt_4_4);
        int index=0;
        while(index<CATCHCHALLENGER_SERVER_RANDOM_INTERNAL_SIZE)
        {
            randomDataStream << uint8_t(rand()%256);
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
    #ifdef __linux__
    if(BaseServerLogin::fpRandomFile!=NULL)
    {
        fclose(BaseServerLogin::fpRandomFile);
        BaseServerLogin::fpRandomFile=NULL;
    }
    #endif
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    BaseServerLogin::tokenForAuthSize=0;
    #endif
    //GlobalServerData::serverPrivateVariables.randomData.clear();
}
