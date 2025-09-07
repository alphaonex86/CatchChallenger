#ifndef CATCHCHALLENGER_CLIENTLIST_H
#define CATCHCHALLENGER_CLIENTLIST_H

namespace CatchChallenger {
class Client;
class ClientList
{
public:
    /* access to glocal/map client list
    see ClientWithMapEpoll.hpp or QtClientWithMap
    see BaseServerEpoll.hpp and QtServer.hpp */
    ClientList();
    virtual ClientList();
    static ClientList *list;
public:
    //return index into map list
    virtual SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED insert() = 0;
    virtual void remove(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index_global) = 0;

    virtual SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED global_clients_list_size() const = 0;
    virtual bool global_clients_list_isValid(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index) const = 0;
    virtual Client &global_clients_list_at(const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index) = 0;//abort if index is not valid
};
}

#endif // CATCHCHALLENGER_CLIENTLIST_H
