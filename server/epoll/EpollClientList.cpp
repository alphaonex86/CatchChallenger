#include "EpollClientList.hpp"

//return index into map list
EpollClientList MapServer::insert()
{
    if(!clients_removed_index.empty())
    {
        SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED b=clients_removed_index.back();
        clients_removed_index.pop_back();
        clients[b]=index_global;
        return b;
    }
    else
    {
        SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED b=clients.size();
        clients.resize(b+1);
        clients[b]=index_global;
        return b;
    }
}

void EpollClientList::remove(const Client &client)
{
    clients[index_map]=SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX;
    clients_removed_index.push_back(index_map);
    ClientList::remove(index_global);
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
const Client &EpollClientList::at(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index)
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
Client &EpollClientList::rw(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index)
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
