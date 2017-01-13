#include "CheckTimeoutGameServer.h"
#include "EpollClientLoginMaster.h"
#include "../../general/base/cpp11addition.h"
#include <iostream>

using namespace CatchChallenger;

CheckTimeoutGameServer::CheckTimeoutGameServer(const uint32_t checkTimeoutGameServerMSecond)
{
    start(checkTimeoutGameServerMSecond);
}

void CheckTimeoutGameServer::exec()
{
    const uint64_t &msecondFrom1970=msFrom1970();
    if(msecondFrom1970>=CharactersGroup::lastPingStarted)
    {
        const uint64_t diff=(msecondFrom1970-CharactersGroup::lastPingStarted)*100/120;//-20%
        if(diff<CharactersGroup::checkTimeoutGameServerMSecond)
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
                        std::cerr << "CheckTimeoutGameServer::exec(): Locale timedrift detected" << std::endl;
                        gameServer->sendGameServerPing(msecondFrom1970);
                        gameServerInternal->lastPingStarted=msecondFrom1970;
                    }
                    else
                    {
                        if(gameServerInternal->pingInProgress)
                        {
                            const uint64_t diff=(msecondFrom1970-gameServerInternal->lastPingStarted);
                            if(diff>CharactersGroup::gameserverTimeoutms)
                            {
                                if(gameServerInternal->host.empty())
                                    gameServer->errorParsingLayer("Game server don't reponds to ping, remove it, not responds into: "+std::to_string(diff)+", max: "+std::to_string(CharactersGroup::gameserverTimeoutms)+", unique key: "+std::to_string(gameServer->uniqueKey));
                                else if(gameServerInternal->metaData.empty())
                                    gameServer->errorParsingLayer("Game server don't reponds to ping, remove it, not responds into: "+std::to_string(diff)+", max: "+std::to_string(CharactersGroup::gameserverTimeoutms)+", unique key: "+std::to_string(gameServer->uniqueKey)+", host: "+gameServerInternal->host+":"+std::to_string(gameServerInternal->port));
                                else
                                    gameServer->errorParsingLayer("Game server don't reponds to ping, remove it, not responds into: "+std::to_string(diff)+", max: "+std::to_string(CharactersGroup::gameserverTimeoutms)+", unique key: "+std::to_string(gameServer->uniqueKey)+", host: "+gameServerInternal->host+":"+std::to_string(gameServerInternal->port)+", meta data: "+gameServerInternal->metaData);
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
            std::cerr << "CheckTimeoutGameServer::exec(): Timedrift detected (diff " << diff << "> CharactersGroup::checkTimeoutGameServerMSecond " << CharactersGroup::checkTimeoutGameServerMSecond << ")" << std::endl;
            timeDrift();
        }
    }
    else
    {
        std::cerr << "CheckTimeoutGameServer::exec(): Timedrift detected (msecondFrom1970 " << msecondFrom1970 << " < CharactersGroup::lastPingStarted " << CharactersGroup::lastPingStarted << " )" << std::endl;
        timeDrift();
    }
}

void CheckTimeoutGameServer::timeDrift()
{
    const uint64_t &msecondFrom1970=msFrom1970();
    unsigned int index=0;
    while(index<EpollClientLoginMaster::gameServers.size())
    {
        EpollClientLoginMaster * const gameServer=EpollClientLoginMaster::gameServers.at(index);
        CharactersGroup::InternalGameServer * const gameServerInternal=gameServer->charactersGroupForGameServerInformation;
        if(gameServer!=NULL)
        {
            gameServer->sendGameServerPing(msecondFrom1970);
            gameServerInternal->lastPingStarted=msecondFrom1970;
        }
        index++;
    }
    CharactersGroup::lastPingStarted=msFrom1970();
}
