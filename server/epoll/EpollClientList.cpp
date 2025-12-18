#include "EpollClientList.hpp"
#include "../base/GlobalServerData.hpp"

EpollClientList::EpollClientList()
{
    maxIndex=0;
    size_t max=CatchChallenger::GlobalServerData::serverSettings.max_players;
    if(max>SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
        max=SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX-1;

    size_t tmax=max;
    clients_removed_index.resize(tmax);
    do
    {
        clients_removed_index[max-tmax]=tmax;
    } while(tmax>0);

    clients.reserve(tmax);
    unsigned int index=0;
    while(index<tmax)
    {
        clients.push_back(ClientWithMapEpoll(index));
        ClientWithMapEpoll &c=clients[index];
        c.resetAll();
        c.setToDefault();
        c.reset(-1);
        index++;
    }
}

//return index into map list
ClientWithMapEpoll &EpollClientList::getByReference()
{
    if(!clients_removed_index.empty())
    {
        const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED b=clients_removed_index.back();
        clients_removed_index.pop_back();
        return clients[b];
    }
    else
    {
        if(maxIndex>=65535)
        {
            std::cout << "EpollClientList::getByReference() maxIndex>=65535" << std::endl;
            abort();
        }
        maxIndex++;
        return clients[maxIndex-1];
    }
}

void EpollClientList::remove(const CatchChallenger::Client &client)
{
    const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED index_global=client.getIndexConnect();
    if(empty(index_global))
        return;
    clients_removed_index.push_back(index_global);
    CatchChallenger::ClientList::remove(client);
}

SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED EpollClientList::size() const
{
    return maxIndex;
}

bool EpollClientList::empty(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index) const
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(index>=clients.size())
    {
        std::cerr << "MapServer::map_clients_list_isValid out of range: " << index << "/" << clients.size() << std::endl;
        abort();
    }
    #endif
    const ClientWithMapEpoll &c=clients.at(index);
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
const CatchChallenger::Client &EpollClientList::at(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index) const
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(index>=clients.size())
    {
        std::cerr << "MapServer::map_clients_list_isValid out of range: " << index << "/" << clients.size() << std::endl;
        abort();
    }
    if(clients.at(index).getIndexConnect()==SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
    {
        std::cerr << "MapServer::map_clients_list_isValid wrong value: " << index << std::endl;
        abort();
    }
    #endif
    return clients.at(index);
}

//abort if index is not valid
CatchChallenger::Client &EpollClientList::rw(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(index>=clients.size())
    {
        std::cerr << "MapServer::map_clients_list_isValid out of range: " << index << "/" << clients.size() << std::endl;
        abort();
    }
    if(clients.at(index).getIndexConnect()==SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
    {
        std::cerr << "MapServer::map_clients_list_isValid wrong value: " << index << std::endl;
        abort();
    }
    #endif
    return clients[index];
}

SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED EpollClientList::connected_size() const
{
    return maxIndex-clients_removed_index.size();
}

CatchChallenger::ClientWithMap &EpollClientList::rwWithMap(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(index>=clients.size())
    {
        std::cerr << "MapServer::map_clients_list_isValid out of range: " << index << "/" << clients.size() << std::endl;
        abort();
    }
    if(clients.at(index).getIndexConnect()==SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
    {
        std::cerr << "MapServer::map_clients_list_isValid wrong value: " << index << std::endl;
        abort();
    }
    #endif
    return clients[index];
}
