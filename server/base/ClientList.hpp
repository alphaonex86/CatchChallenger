#ifndef CATCHCHALLENGER_CLIENTLIST_H
#define CATCHCHALLENGER_CLIENTLIST_H

#include "../../general/base/GeneralType.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

namespace CatchChallenger {
class Client;
class ClientWithMap;
class ClientList
{
public:
    /* access to glocal/map client list
    see ClientWithMapEpoll.hpp or QtClientWithMap
    see BaseServerEpoll.hpp and QtServer.hpp */
    ClientList();
    virtual ~ClientList();
    static ClientList *list;
    size_t maxIndex;
public:
    //return index into map list
    virtual void remove(const Client &client);
    virtual PLAYER_INDEX_FOR_CONNECTED size() const = 0;
    virtual PLAYER_INDEX_FOR_CONNECTED connected_size() const = 0;
    virtual bool empty(const PLAYER_INDEX_FOR_CONNECTED &index) const = 0;
    virtual const Client &at(const PLAYER_INDEX_FOR_CONNECTED &index) const = 0;//abort if index is not valid
    virtual ClientWithMap &rwWithMap(const PLAYER_INDEX_FOR_CONNECTED &index) = 0;//abort if index is not valid
    virtual Client &rw(const PLAYER_INDEX_FOR_CONNECTED &index) = 0;//abort if index is not valid

    PLAYER_INDEX_FOR_CONNECTED global_clients_list_bypseudo(const std::string &pseudo) const;//return SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX if not found

    void insert_characterSelected(const Client &c);
    void insert_StatusWatcher(const Client &c);

    bool haveFreeSlot() const;
protected:
    //virtual SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED insert() = 0;//replace by getbyRef

    std::vector<SIMPLIFIED_PLAYER_ID_FOR_MAP> clients_removed_index;//garbage collector, reuse slot and only grow memory, never remove vector index and have to move whole back data
private:
    //std::unordered_map<uint32_t,PLAYER_INDEX_FOR_CONNECTED> playerById_db;//where used?
    std::unordered_map<std::string,PLAYER_INDEX_FOR_CONNECTED> playerByPseudo;//see ClientWithMapEpoll.hpp or QtClientWithMap, only after character is selected
    std::unordered_set<PLAYER_INDEX_FOR_CONNECTED> clientForStatus;//to directly send status update to this specific client (without search into whole list)
};
}

#endif // CATCHCHALLENGER_CLIENTLIST_H
