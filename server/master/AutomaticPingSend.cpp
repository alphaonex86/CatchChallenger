#include "AutomaticPingSend.hpp"
#include "EpollClientLoginMaster.hpp"
#include "../../general/base/cpp11addition.hpp"
#include <iostream>

using namespace CatchChallenger;

AutomaticPingSend::AutomaticPingSend(const uint32_t pingMSecond)
{
    start(pingMSecond);
}

void AutomaticPingSend::exec()
{
    const uint64_t &msecondFrom1970=msFrom1970();
    if(msecondFrom1970>=CharactersGroup::lastPingStarted)
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
                        std::cerr << "AutomaticPingSend(): Locale timedrift detected" << std::endl;
                        gameServer->sendGameServerPing(msecondFrom1970);
                        gameServerInternal->lastPingStarted=msecondFrom1970;
                    }
                    else
                        gameServer->sendGameServerPing(msecondFrom1970);
                }
                index++;
            }
            CharactersGroup::lastPingStarted=msFrom1970();
        }
        else
        {
            std::cerr << "AutomaticPingSend(): Timedrift detected (diff " << diff << "> CharactersGroup::pingMSecond " << CharactersGroup::pingMSecond << ")" << std::endl;
            timeDrift();
        }
    }
    else
    {
        std::cerr << "AutomaticPingSend(): Timedrift detected (msecondFrom1970 " << msecondFrom1970 << " < CharactersGroup::lastPingStarted " << CharactersGroup::lastPingStarted << " )" << std::endl;
        timeDrift();
    }
}

void AutomaticPingSend::timeDrift()
{
    const uint64_t &msecondFrom1970=msFrom1970();
    unsigned int index=0;
    while(index<EpollClientLoginMaster::gameServers.size())
    {
        EpollClientLoginMaster * const gameServer=EpollClientLoginMaster::gameServers.at(index);
        if(gameServer!=NULL)
            gameServer->sendGameServerPing(msecondFrom1970);
        index++;
    }
    CharactersGroup::lastPingStarted=msFrom1970();
}
