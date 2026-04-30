#include "QtClientList.hpp"
#include "../base/GlobalServerData.hpp"

using namespace CatchChallenger;

#if CATCHCHALLENGER_DYNAMIC_MAP_LIST
std::vector<QtClientWithMap *> QtClientList::clients;
#endif

QtClientList::QtClientList()
{
    maxIndex=0;
}

PLAYER_INDEX_FOR_CONNECTED QtClientList::insert(QtClientWithMap *client)
{
    if(!clients_removed_index.empty())
    {
        const PLAYER_INDEX_FOR_CONNECTED b=clients_removed_index.back();
        clients_removed_index.pop_back();
        clients[b]=client;
        return b;
    }
    else
    {
        if(maxIndex>=PLAYER_INDEX_FOR_CONNECTED_MAX)
        {
            std::cerr << "QtClientList::insert() maxIndex>=PLAYER_INDEX_FOR_CONNECTED_MAX" << std::endl;
            abort();
        }
        if(maxIndex>=clients.size())
            clients.resize(maxIndex+1,nullptr);
        clients[maxIndex]=client;
        PLAYER_INDEX_FOR_CONNECTED b=maxIndex;
        maxIndex++;
        return b;
    }
}

void QtClientList::remove(const Client &client)
{
    const PLAYER_INDEX_FOR_CONNECTED index_global=client.getIndexConnect();
    if(index_global>=clients.size() || clients[index_global]==nullptr)
        return;
    clients[index_global]=nullptr;
    clients_removed_index.push_back(index_global);
    ClientList::remove(client);
}

PLAYER_INDEX_FOR_CONNECTED QtClientList::size() const
{
    return maxIndex;
}

bool QtClientList::isNull(const PLAYER_INDEX_FOR_CONNECTED &index) const
{
    #ifdef CATCHCHALLENGER_HARDENED
    if(index>=clients.size())
    {
        std::cerr << "QtClientList::isNull() out of range: " << index << "/" << clients.size() << " then fix the caller, check before into the backtrace" << std::endl;
        abort();
    }
    #endif
    if(clients.at(index)==nullptr)
        return true;
    switch(clients.at(index)->getClientStat())
    {
        case Client::CharacterSelected:
            break;
        default:
            return true;
            break;
    }
    return false;
}

const Client &QtClientList::at(const PLAYER_INDEX_FOR_CONNECTED &index) const
{
    #ifdef CATCHCHALLENGER_HARDENED
    if(index>=clients.size())
    {
        std::cerr << "QtClientList::at() out of range: " << index << "/" << clients.size() << std::endl;
        abort();
    }
    if(clients.at(index)==nullptr)
    {
        std::cerr << "QtClientList::at() null at: " << index << std::endl;
        abort();
    }
    #endif
    return *clients.at(index);
}

Client &QtClientList::rw(const PLAYER_INDEX_FOR_CONNECTED &index)
{
    #ifdef CATCHCHALLENGER_HARDENED
    if(index>=clients.size())
    {
        std::cerr << "QtClientList::rw() out of range: " << index << "/" << clients.size() << std::endl;
        abort();
    }
    if(clients.at(index)==nullptr)
    {
        std::cerr << "QtClientList::rw() null at: " << index << std::endl;
        abort();
    }
    #endif
    return *clients[index];
}

PLAYER_INDEX_FOR_CONNECTED QtClientList::connected_size() const
{
    return maxIndex-clients_removed_index.size();
}

ClientWithMap &QtClientList::rwWithMap(const PLAYER_INDEX_FOR_CONNECTED &index)
{
    #ifdef CATCHCHALLENGER_HARDENED
    if(index>=clients.size())
    {
        std::cerr << "QtClientList::rwWithMap() out of range: " << index << "/" << clients.size() << std::endl;
        abort();
    }
    if(clients.at(index)==nullptr)
    {
        std::cerr << "QtClientList::rwWithMap() null at: " << index << std::endl;
        abort();
    }
    #endif
    return *clients[index];
}
