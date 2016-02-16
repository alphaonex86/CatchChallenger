#include "CheckTimeoutGameServer.h"
#include "EpollClientLoginMaster.h"

using namespace CatchChallenger;

CheckTimeoutGameServer::CheckTimeoutGameServer(const uint16_t pingSecond)
{
    setInterval(pingSecond*1000);
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
                CharactersGroup::InternalGameServer * gameServer=EpollClientLoginMaster::gameServers.at(index).charactersGroupForGameServerInformation;
                if(gameServer!=NULL)
                {
                    if(msecondFrom1970>gameServer->lastPingStarted)
                    {
                        std::cerr << "Locale timedrift detected" << std::endl;
                        if(gameserver->pingInProgress==false)
                        {
                            gameserver->pingInProgress=true;
                            gameserver->sendGameServerPing();
                        }
                        gameServer->lastPingStarted=msecondFrom1970;
                    }
                    else
                    {
                        const uint64_t diff=(msecondFrom1970-gameServer->lastPingStarted);
                        if(diff>CharactersGroup::gameserverTimeoutms)
                        {
                            std::cerr << "Game server don't reponds to ping, remove it" << std::endl;
                            gameServer->close();

                            removeFromQueryReceived(queryNumber);
                            newServerToBeMaster->sendGameServerRegistrationReply(false);

                            if(!gameServer->secondServerInConflict.empty())
                            {
                                EpollClientLoginMaster * newServerToBeMaster=gameserver->secondServerInConflict->front();
                                gameserver->secondServerInConflict->removeFront();

                                if(!gameserver->secondServerInConflict.empty())
                                {
                                    newServerToBeMaster->secondServerInConflict=gameserver->secondServerInConflict;

                                    unsigned int indexServerToUpdate=0;
                                    while(indexServerToUpdate<newServerToBeMaster->secondServerInConflict.size())
                                    {
                                        newServerToBeMaster->secondServerInConflict->at(indexServerToUpdate).inConflicWithTheMainServer=newServerToBeMaster;
                                        indexServerToUpdate++;
                                    }

                                    newServerToBeMaster->sendGameServerPing();
                                }
                            }
                        }
                    }
                }
                index++;
            }
        }
        else
            timeDrift();
    }
    else
        timeDrift();
}

void CheckTimeoutGameServer::timeDrift()
{
    std::cerr << "Timedrift detected" << std::endl;
    unsigned int index=0;
    while(index<EpollClientLoginMaster::gameServers.size())
    {
        CharactersGroup::InternalGameServer * gameServer=EpollClientLoginMaster::gameServers.at(index).charactersGroupForGameServerInformation;
        if(gameServer!=NULL)
        {
            if(gameserver->pingInProgress==false)
            {
                gameserver->pingInProgress=true;
                gameserver->sendGameServerPing();
            }
            gameServer->lastPingStarted=msFrom1970();
        }
        index++;
    }
    CharactersGroup::lastPingStarted=msFrom1970();
}
