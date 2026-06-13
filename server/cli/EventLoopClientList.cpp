#include "EventLoopClientList.hpp"
#include <iostream>
#include "../base/GlobalServerData.hpp"

EventLoopClientList::EventLoopClientList()
{
    maxIndex=0;
    size_t max=CatchChallenger::GlobalServerData::serverSettings.max_players;
    if(max>PLAYER_INDEX_FOR_CONNECTED_MAX)
        max=PLAYER_INDEX_FOR_CONNECTED_MAX-1;
    clients_removed_index.reserve(max);

    size_t tmax=max;
    clients_removed_index.resize(tmax);
    {
        //Loop index must be wide enough to hold tmax. Was previously
        //SIMPLIFIED_PLAYER_ID_FOR_MAP (uint8_t); with tmax>=256 the
        //index wraps to 0 after 255 and the loop never terminates,
        //hanging the gsa startup right after master auth.
        PLAYER_INDEX_FOR_CONNECTED index=0;
        while(index<tmax)
        {
            //Free-slot stack holds the indices of clients[] entries, which has
            //exactly tmax elements (valid indices 0..tmax-1). The value MUST
            //stay in that range: the old `tmax-index` produced 1..tmax, so the
            //OOB value tmax sat on the stack (and index 0 was never reusable),
            //and once max_players slots were taken getByReference() returned
            //clients[tmax] -> out-of-bounds vector access (reachable just by
            //opening max_players concurrent connections). Use tmax-1-index.
            clients_removed_index[index]=tmax-1-index;
            index++;
        }
        maxIndex=max;
    }

    clients.reserve(tmax);
    unsigned int index=0;
    while(index<tmax)
    {
        clients.emplace_back(index);
        ClientWithMapEventLoop &c=clients[index];
        c.resetAll();
        c.setToDefault();
        c.reset(-1);
        index++;
    }
}

//return index into map list
ClientWithMapEventLoop &EventLoopClientList::getByReference()
{
    if(!clients_removed_index.empty())
    {
        const PLAYER_INDEX_FOR_CONNECTED b=clients_removed_index.back();
        clients_removed_index.pop_back();
        return clients[b];
    }
    else
    {
        if(maxIndex>=65535)
        {
            std::cout << "EventLoopClientList::getByReference() maxIndex>=65535" << std::endl;
            abort();
        }
        maxIndex++;
        return clients[maxIndex-1];
    }
}

void EventLoopClientList::remove(const CatchChallenger::Client &client)
{
    const PLAYER_INDEX_FOR_CONNECTED index_global=client.getIndexConnect();
    if(isNull(index_global))
        return;
    clients_removed_index.push_back(index_global);
    CatchChallenger::ClientList::remove(client);
}

PLAYER_INDEX_FOR_CONNECTED EventLoopClientList::size() const
{
    return maxIndex;
}

bool EventLoopClientList::isNull(const PLAYER_INDEX_FOR_CONNECTED &index) const
{
    #ifdef CATCHCHALLENGER_HARDENED
    if(index>=clients.size())
    {
        std::cerr << "MapServer::map_clients_list_isValid out of range: " << index << "/" << clients.size() << " then fix the caller, check before into the backtrace" << std::endl;
        abort();
    }
    #endif
    const ClientWithMapEventLoop &c=clients.at(index);
    switch(c.getClientStat())
    {
        case CatchChallenger::Client::CharacterSelected:
            break;
        default:
            return true;
            break;
    }

    return false;
}

//abort if index is not valid
const CatchChallenger::Client &EventLoopClientList::at(const PLAYER_INDEX_FOR_CONNECTED &index) const
{
    #ifdef CATCHCHALLENGER_HARDENED
    if(index>=clients.size())
    {
        std::cerr << "MapServer::map_clients_list_isValid out of range: " << index << "/" << clients.size() << std::endl;
        abort();
    }
    if(clients.at(index).getIndexConnect()==PLAYER_INDEX_FOR_CONNECTED_MAX)
    {
        std::cerr << "MapServer::map_clients_list_isValid wrong value: " << index << std::endl;
        abort();
    }
    #endif
    //operator[] not .at(): the HARDENED block above already bounds-checks,
    //and rw()/rwWithMap() return clients[index] too -- the redundant
    //std::vector::at() bounds check on this hot visibility read path is
    //pure overhead in the production (non-HARDENED) build.
    return clients[index];
}

//abort if index is not valid
CatchChallenger::Client &EventLoopClientList::rw(const PLAYER_INDEX_FOR_CONNECTED &index)
{
    #ifdef CATCHCHALLENGER_HARDENED
    if(index>=clients.size())
    {
        std::cerr << "MapServer::map_clients_list_isValid out of range: " << index << "/" << clients.size() << std::endl;
        abort();
    }
    if(clients.at(index).getIndexConnect()==PLAYER_INDEX_FOR_CONNECTED_MAX)
    {
        std::cerr << "MapServer::map_clients_list_isValid wrong value: " << index << std::endl;
        abort();
    }
    #endif
    return clients[index];
}

PLAYER_INDEX_FOR_CONNECTED EventLoopClientList::connected_size() const
{
    return maxIndex-clients_removed_index.size();
}

CatchChallenger::ClientWithMap &EventLoopClientList::rwWithMap(const PLAYER_INDEX_FOR_CONNECTED &index)
{
    #ifdef CATCHCHALLENGER_HARDENED
    if(index>=clients.size())
    {
        std::cerr << "MapServer::map_clients_list_isValid out of range: " << index << "/" << clients.size() << std::endl;
        abort();
    }
    if(clients.at(index).getIndexConnect()==PLAYER_INDEX_FOR_CONNECTED_MAX)
    {
        std::cerr << "MapServer::map_clients_list_isValid wrong value: " << index << std::endl;
        abort();
    }
    #endif
    return clients[index];
}
