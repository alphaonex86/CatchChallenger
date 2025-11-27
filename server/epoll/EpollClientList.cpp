#include "EpollClientList.hpp"

EpollClientList::EpollClientList()
{
    size_t max=maxplayer;
    if(max>SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
        max=SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX-1;

    size_t tmax=max;
    clients_removed_index.resize(tmax);
    do
    {
        clients_removed_index[max-tmax]=tmax;
    } while(tmax>0);

    clients.resize(tmax);
    int index=0;
    while(index<tmax)
    {
        clients[index].reset();
        clients[index].pos=index;
        index++;
    }
}

//return index into map list
EpollClientList EpollClientList::insert()
{
    if(!clients_removed_index.empty())
    {
        const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED b=clients_removed_index.back();
        clients_removed_index.pop_back();
        clients[b]=index_global;
        return b;
    }
    else
    {
        const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED b=clients.size();
        clients.push_back(new ClientWithMapEpoll);
        return b;
    }
}

void EpollClientList::remove(const CatchChallenger::Client &client)
{
    const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED index_map=client.getIndexConnect();
    if(index_map==SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
        return;
    if(empty(index_map))
        return;
    clients[index_map]=SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX;
    clients_removed_index.push_back(index_map);
    CatchChallenger::ClientList::remove(index_global);
}

SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED EpollClientList::size() const
{
    return clients.size();
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
    return clients.at(index)!=SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX;
}

//abort if index is not valid
const CatchChallenger::Client &EpollClientList::at(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(index>=clients.size())
    {
        std::cerr << "MapServer::map_clients_list_isValid out of range: " << index << "/" << clients.size() << std::endl;
        abort();
    }
    if(clients.at(index)==SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
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
    if(clients.at(index)==SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
    {
        std::cerr << "MapServer::map_clients_list_isValid wrong value: " << index << std::endl;
        abort();
    }
    #endif
    return clients[index];
}

SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED EpollClientList::connected_size() const
{
    return clients.size()-clients_removed_index.size();
}

CatchChallenger::ClientWithMap &rwWithMap(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index) const
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(index>=clients.size())
    {
        std::cerr << "MapServer::map_clients_list_isValid out of range: " << index << "/" << clients.size() << std::endl;
        abort();
    }
    if(clients.at(index)==SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
    {
        std::cerr << "MapServer::map_clients_list_isValid wrong value: " << index << std::endl;
        abort();
    }
    #endif
    return clients[index];
}
