#include "Map_server_MapVisibility_Simple_StoreOnSender.hpp"
#include "../GlobalServerData.hpp"
#include "../Client.hpp"

/** limit visible 254 player at time (store internal index, 255, 65535 if not found), drop branch, /2 network, less code
 *  way to do:
 *  1) dropall + just do full insert and send all vector
 *  2) store last state and do a diff for each player
 * when diff, some entry in list can be null, allow quick compare, some entry will just be replaced, ELSE need database id or pseudo std::map resolution
 * when broadcast, can if > 2M {while SIMD} else {C}
*/

using namespace CatchChallenger;

std::vector<Map_server_MapVisibility_Simple_StoreOnSender> Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list;

Map_server_MapVisibility_Simple_StoreOnSender::Map_server_MapVisibility_Simple_StoreOnSender()
{
}

Map_server_MapVisibility_Simple_StoreOnSender::~Map_server_MapVisibility_Simple_StoreOnSender()
{
}

/*void Map_server_MapVisibility_Simple_StoreOnSender::send_dropAll()
{
    unsigned const char mainCode[]={0x65};
    unsigned int index=0;
    while(index<clients.size())
    {
        ClientWithMap * client=clients[index];
        //clientdropAllClients();
        if(client->pingCountInProgress()<=0 && client->mapSyncMiss==false)
            client->sendRawBlock(reinterpret_cast<const char *>(mainCode),sizeof(mainCode));
        else
            client->mapSyncMiss=true;
        index++;
    }
    send_drop_all=false;
}*/

void Map_server_MapVisibility_Simple_StoreOnSender::send_reinsertAll()
{
    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x6B;
    posOutput+=1+4;
    //prepare the data
    {
        //////////////////////////// insert //////////////////////////
        // can be only this map with this algo, then 1 map
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
        posOutput+=1;
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(id);
        posOutput+=2;
        if(GlobalServerData::serverSettings.max_players<=255)
        {
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(clients.size()-1);
            posOutput+=1;
        }
        else
        {
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(clients.size()-1);
            posOutput+=2;
        }
        if(clients.size()>1)
        {
            unsigned int index=0;
            while(index<clients.size())
            {
                ClientWithMap * const client=clients.at(index);
                posOutput+=playerToFullInsert(client,ProtocolParsingBase::tempBigBufferForOutput+posOutput);
                ++index;
            }
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
            index=0;
            while(index<clients.size())
            {
                ClientWithMap * const client=clients.at(index);
                client->to_send_insert=false;
                if(client->pingCountInProgress()<=0 && client->mapSyncMiss==false)
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    MapServer::check6B(ProtocolParsingBase::tempBigBufferForOutput+1+4,posOutput-1-4);
                    #endif
                    client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                }
                else
                    client->mapSyncMiss=true;
                ++index;
            }
        }
    }

    send_SYNCAll();
    send_reinsert_all=false;
}

/*void Map_server_MapVisibility_Simple_StoreOnSender::send_SYNCAll()
{
    unsigned int index=0;
    while(index<clients.size())
    {
        ClientWithMap * const client=clients.at(index);
        client->sendPing();
        index++;
    }
}*/

// broadcast all, no filter then resend same data
void Map_server_MapVisibility_Simple_StoreOnSender::min_CPU()
{
    //if to many player then just stop update
    if()
        return;
    unsigned int index_client=0;
    while(index_client<ClientList::list.size())
    {
        ClientWithMap * const client=clients.at(index);
        if(flag ping return)
        {
        //client->sendPing();
        }
        index_client++;
    }
}

// filter if already send, then consume CPU
void Map_server_MapVisibility_Simple_StoreOnSender::min_network()
{
}
