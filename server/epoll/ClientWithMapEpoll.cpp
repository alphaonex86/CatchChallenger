#include "ClientWithMapEpoll.hpp"

ClientWithMapEpoll::MapVisibilityAlgorithm_NoneEpoll(const int &infd) :
    CatchChallenger::EpollClient(infd)
{
}

void ClientWithMapEpoll::closeSocket()
{
    CatchChallenger::EpollClient::closeSocket();
}

bool ClientWithMapEpoll::isValid()
{
    return CatchChallenger::EpollClient::isValid();
}

ssize_t ClientWithMapEpoll::readFromSocket(char * data, const size_t &size)
{
    return CatchChallenger::EpollClient::read(data,size);
}

ssize_t ClientWithMapEpoll::writeToSocket(const char * const data, const size_t &size)
{
    return CatchChallenger::EpollClient::write(data,size);
}

ClientWithMapEpoll::MapVisibilityAlgorithm_WithBorder_StoreOnSenderEpoll(const int &infd) :
    CatchChallenger::EpollClient(infd)
{
}

void ClientWithMapEpoll::closeSocket()
{
    CatchChallenger::EpollClient::closeSocket();
}

bool ClientWithMapEpoll::isValid()
{
    return CatchChallenger::EpollClient::isValid();
}

ssize_t ClientWithMapEpoll::readFromSocket(char * data, const size_t &size)
{
    return CatchChallenger::EpollClient::read(data,size);
}

ssize_t ClientWithMapEpoll::writeToSocket(const char * const data, const size_t &size)
{
    return CatchChallenger::EpollClient::write(data,size);
}

ClientWithMapEpoll::MapVisibilityAlgorithm_Simple_StoreOnSenderEpoll(const int &infd) :
    CatchChallenger::EpollClient(infd)
{
}

void ClientWithMapEpoll::closeSocket()
{
    CatchChallenger::EpollClient::closeSocket();
}

bool ClientWithMapEpoll::isValid()
{
    return CatchChallenger::EpollClient::isValid();
}

ssize_t ClientWithMapEpoll::readFromSocket(char * data, const size_t &size)
{
    return CatchChallenger::EpollClient::read(data,size);
}

ssize_t ClientWithMapEpoll::writeToSocket(const char * const data, const size_t &size)
{
    return CatchChallenger::EpollClient::write(data,size);
}

//return index into map list
ClientWithMapEpoll MapServer::insert()
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

void ClientWithMapEpoll::remove(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index_global)
{
    clients[index_map]=SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX;
    clients_removed_index.push_back(index_map);
}

SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED ClientWithMapEpoll::global_clients_list_size() const
{
    return clients.size();
}

bool ClientWithMapEpoll::global_clients_list_isValid(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index) const
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
Client &ClientWithMapEpoll::global_clients_list_at(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index)
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
