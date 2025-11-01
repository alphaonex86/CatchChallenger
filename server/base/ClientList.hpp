#ifndef CATCHCHALLENGER_CLIENTLIST_H
#define CATCHCHALLENGER_CLIENTLIST_H

#include "../../general/base/GeneralType.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

namespace CatchChallenger {
class Client;
class ClientList
{
public:
    /* access to glocal/map client list
    see ClientWithMapEpoll.hpp or QtClientWithMap
    see BaseServerEpoll.hpp and QtServer.hpp */
    ClientList();
    virtual ~ClientList();
    static ClientList *list;
public:
    //return index into map list
    virtual void remove(const Client &client);

    virtual SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED size() const = 0;
    virtual bool empty(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index) const = 0;
    virtual const Client &at(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index) const = 0;//abort if index is not valid
    virtual Client &rw(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index) = 0;//abort if index is not valid

    SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED global_clients_list_bypseudo(const std::string &pseudo);//return SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX if not found

    SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED insert_characterSelected(const std::string &pseudo);
    SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED insert_StatusWatcher();

    bool haveFreeSlot() const;
protected:
    virtual SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED insert() = 0;
private:
    //std::unordered_map<uint32_t,SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED> playerById_db;//where used?
    std::unordered_map<std::string,SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED> playerByPseudo;//see ClientWithMapEpoll.hpp or QtClientWithMap, only after character is selected
    std::unordered_set<SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED> clientForStatus;//to directly send status update to this specific client (without search into whole list)
    #if CATCHCHALLENGER_DYNAMIC_MAP_LIST
    static std::vector<SIMPLIFIED_PLAYER_ID_FOR_MAP> clients_removed_index;//garbage collector, reuse slot and only grow memory, never remove vector index and have to move whole back data
    #else
    #error todo the static part
    #endif
};
}

#endif // CATCHCHALLENGER_CLIENTLIST_H
