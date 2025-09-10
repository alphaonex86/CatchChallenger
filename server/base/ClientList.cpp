#include "ClientList.hpp"
#include "Client.hpp"

using namespace CatchChallenger;

ClientList * ClientList::list=nullptr;

ClientList::ClientList()
{
}

ClientList::~ClientList()
{
}

void ClientList::remove(const Client &client)
{
    if(client.index_connected_player==SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
        return;
    switch(client.stat)
    {
    case Client::LoggedStatClient:
        if(clientForStatus.remove(client.index_connected_player)==clientForStatus.cend())
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            std::cerr << "this clientForStatus not found for remove" << std::endl;
            abort();
            #endif
        }
        break;
    case Client::CharacterSelected:
        if(playerByPseudo.remove(client.public_and_private_informations.public_informations.pseudo)==playerByPseudo.cend())
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            std::cerr << "this clientForStatus not found for remove" << std::endl;
            abort();
            #endif
        }
        break;
    default:
        break;
    }
}

SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED ClientList::global_clients_list_bypseudo(const std::string &pseudo)//return SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX if not found
{
    if(playerByPseudo.find(pseudo)==playerByPseudo.cend())
        return SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX;
    playerByPseudo.at(pseudo);
}

SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED ClientList::insert_characterSelected(const std::string &pseudo)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(playerByPseudo.find(pseudo)!=playerByPseudo.cend())
    {
        std::cerr << "this pseudo is already found" << std::endl;
        abort();
    }
    #endif
    const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED alloc_id=insert();
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    for (const auto& n : playerByPseudo)
    {
        if(n.second==alloc_id)
        {
            std::cerr << "this playerByPseudo index is already found" << std::endl;
            abort();
        }
    }
    #endif
    playerByPseudo[pseudo]=alloc_id;
    return alloc_id;
}

SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED ClientList::insert_StatusWatcher()
{
    const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED alloc_id=insert();
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(clientForStatus.find(alloc_id)!=clientForStatus.cend())
    {
        std::cerr << "this clientForStatus is already found" << std::endl;
        abort();
    }
    #endif
    clientForStatus.insert(alloc_id);
    return alloc_id;
}
