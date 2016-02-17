#include "CheckTimeoutGameServer.h"
#include "EpollClientLoginMaster.h"
#include "../../general/base/cpp11addition.h"
#include <iostream>

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
                EpollClientLoginMaster * const gameServer=EpollClientLoginMaster::gameServers.at(index);
                CharactersGroup::InternalGameServer * const gameServerInternal=gameServer->charactersGroupForGameServerInformation;
                if(gameServerInternal!=NULL)
                {
                    if(msecondFrom1970>gameServerInternal->lastPingStarted)
                    {
                        std::cerr << "Locale timedrift detected" << std::endl;
                        gameServer->sendGameServerPing();
                        gameServerInternal->lastPingStarted=msecondFrom1970;
                    }
                    else
                    {
                        const uint64_t diff=(msecondFrom1970-gameServerInternal->lastPingStarted);
                        if(diff>CharactersGroup::gameserverTimeoutms)
                        {
                            gameServer->errorParsingLayer("Game server don't reponds to ping, remove it");

                            if(!gameServer->secondServerInConflict.empty())
                            {
                                EpollClientLoginMaster * newServerToBeMaster=gameServer->secondServerInConflict.front();
                                gameServer->secondServerInConflict.erase(gameServer->secondServerInConflict.cbegin());

                                newServerToBeMaster->removeFromQueryReceived(newServerToBeMaster->queryNumberInConflicWithTheMainServer);
                                newServerToBeMaster->sendGameServerRegistrationReply(false);

                                CharactersGroup::InternalGameServer tempData;
                                if(newServerToBeMaster->charactersGroupForGameServerInformation!=NULL)
                                {
                                    tempData=*newServerToBeMaster->charactersGroupForGameServerInformation;
                                    delete newServerToBeMaster->charactersGroupForGameServerInformation;
                                    newServerToBeMaster->charactersGroupForGameServerInformation=NULL;
                                }
                                else
                                {
                                    std::cerr << "newServerToBeMaster->charactersGroupForGameServerInformation==NULL at " << __FILE__ << ":" << __LINE__ << std::endl;
                                    abort();
                                }
                                newServerToBeMaster->charactersGroupForGameServerInformation=newServerToBeMaster->charactersGroupForGameServer->addGameServerUniqueKey(
                                            newServerToBeMaster,tempData.uniqueKey,tempData.host,tempData.port,tempData.metaData,tempData.logicalGroupIndex,tempData.currentPlayer,tempData.maxPlayer,tempData.lockedAccountByGameserver);


                                if(!gameServer->secondServerInConflict.empty())
                                {
                                    newServerToBeMaster->secondServerInConflict=gameServer->secondServerInConflict;

                                    unsigned int indexServerToUpdate=0;
                                    while(indexServerToUpdate<newServerToBeMaster->secondServerInConflict.size())
                                    {
                                        EpollClientLoginMaster * const serverToMasterUpdate=newServerToBeMaster->secondServerInConflict.at(indexServerToUpdate);
                                        serverToMasterUpdate->inConflicWithTheMainServer=newServerToBeMaster;
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
