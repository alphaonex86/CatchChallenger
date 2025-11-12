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
    if(client.getIndexConnect()==SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
        return;
    switch(client.getClientStat())
    {
    case Client::LoggedStatClient:
        clientForStatus.erase(client.getIndexConnect());
        break;
    case Client::CharacterSelected:
        playerByPseudo.erase(client.public_and_private_informations.public_informations.pseudo);
        break;
    default:
        break;
    }
}

SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED ClientList::global_clients_list_bypseudo(const std::string &pseudo) const//return SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX if not found
{
    if(playerByPseudo.find(pseudo)==playerByPseudo.cend())
        return SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX;
    return playerByPseudo.at(pseudo);
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

bool ClientList::haveFreeSlot() const
{
    if(!clients_removed_index.empty())
        return true;
    if((playerByPseudo.size()+clientForStatus.size())<254)
        return true;
    return false;
}
