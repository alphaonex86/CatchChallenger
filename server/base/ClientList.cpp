#include "ClientList.hpp"
#include "Client.hpp"

using namespace CatchChallenger;

ClientList * ClientList::list=nullptr;

ClientList::ClientList() :
    maxIndex(0)
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

void ClientList::insert_characterSelected(const Client &c)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(playerByPseudo.find(c.getPseudo())!=playerByPseudo.cend())
    {
        std::cerr << "this pseudo is already found" << std::endl;
        abort();
    }
    #endif
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    for (const auto& n : playerByPseudo)
    {
        if(n.second==c.getIndexConnect())
        {
            std::cerr << "this playerByPseudo index is already found" << std::endl;
            abort();
        }
    }
    #endif
    playerByPseudo[c.getPseudo()]=c.getIndexConnect();
    return;
}

void ClientList::insert_StatusWatcher(const Client &c)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(clientForStatus.find(c.getIndexConnect())!=clientForStatus.cend())
    {
        std::cerr << "this clientForStatus is already found" << std::endl;
        abort();
    }
    #endif
    clientForStatus.insert(c.getIndexConnect());
    return;
}

bool ClientList::haveFreeSlot() const
{
    if(!clients_removed_index.empty())
        return true;
    if((playerByPseudo.size()+clientForStatus.size())<254)
        return true;
    return false;
}
