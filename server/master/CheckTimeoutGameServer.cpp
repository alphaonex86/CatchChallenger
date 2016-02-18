#include "CheckTimeoutGameServer.h"
#include "EpollClientLoginMaster.h"
#include "../../general/base/cpp11addition.h"
#include <iostream>

using namespace CatchChallenger;

CheckTimeoutGameServer::CheckTimeoutGameServer(const uint32_t pingMSecond)
{
    start(pingMSecond);
}

void CheckTimeoutGameServer::exec()
{
    const uint64_t &msecondFrom1970=msFrom1970();
    if(msecondFrom1970>CharactersGroup::lastPingStarted)
    {
        const uint64_t diff=(msecondFrom1970-CharactersGroup::lastPingStarted)*100/120;//-20%
        if(diff<CharactersGroup::pingMSecond)
        {
            unsigned int index=0;
            while(index<EpollClientLoginMaster::gameServers.size())
            {
                EpollClientLoginMaster * const gameServer=EpollClientLoginMaster::gameServers.at(index);
                CharactersGroup::InternalGameServer * const gameServerInternal=gameServer->charactersGroupForGameServerInformation;
                if(gameServerInternal!=NULL)
                {
                    if(msecondFrom1970<gameServerInternal->lastPingStarted)
                    {
                        std::cerr << "Locale timedrift detected" << std::endl;
                        gameServer->sendGameServerPing();
                        gameServerInternal->lastPingStarted=msecondFrom1970;
                    }
                    else
                    {
                        if(gameServerInternal->pingInProgress)
                        {
                            const uint64_t diff=(msecondFrom1970-gameServerInternal->lastPingStarted);
                            if(diff>CharactersGroup::gameserverTimeoutms)
                            {
                                gameServer->errorParsingLayer("Game server don't reponds to ping, remove it, not responds into: "+std::to_string(diff)+", max: "+std::to_string(CharactersGroup::gameserverTimeoutms));
                                gameServer->passUniqueKeyToNextGameServer();
                            }
                        }
                    }
                }
                index++;
            }
            CharactersGroup::lastPingStarted=msFrom1970();
        }
        else
        {
            std::cerr << "Timedrift detected (diff " << diff << "> CharactersGroup::pingMSecond " << CharactersGroup::pingMSecond << ")" << std::endl;
            timeDrift();
        }
    }
    else
    {
        std::cerr << "Timedrift detected (msecondFrom1970<CharactersGroup::lastPingStarted)" << std::endl;
        timeDrift();
    }
}

void CheckTimeoutGameServer::timeDrift()
{
    unsigned int index=0;
    while(index<EpollClientLoginMaster::gameServers.size())
    {
        EpollClientLoginMaster * const gameServer=EpollClientLoginMaster::gameServers.at(index);
        CharactersGroup::InternalGameServer * const gameServerInternal=gameServer->charactersGroupForGameServerInformation;
        if(gameServer!=NULL)
        {
            gameServer->sendGameServerPing();
            gameServerInternal->lastPingStarted=msFrom1970();
        }
        index++;
    }
    CharactersGroup::lastPingStarted=msFrom1970();
}
