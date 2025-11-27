#include "Map_server_MapVisibility_Simple_StoreOnSender.hpp"
#include "ClientWithMap.hpp"
#include "../GlobalServerData.hpp"
#include "../Client.hpp"
#include "../ClientList.hpp"

/** limit visible 254 player at time (store internal index, 255, 65535 if not found), drop branch, /2 network, less code
 *  way to do:
 *  1) dropall + just do full insert and send all vector
 *  2) store last state and do a diff for each player
 * when diff, some entry in list can be null, allow quick compare, some entry will just be replaced, ELSE need database id or pseudo std::map resolution
 * when broadcast, can if > 2M {while SIMD} else {C}
*/

using namespace CatchChallenger;

//to prevent allocate memory
char Map_server_MapVisibility_Simple_StoreOnSender::tempBigBufferForChanges[];
char Map_server_MapVisibility_Simple_StoreOnSender::tempBigBufferForRemove[];
std::vector<Map_server_MapVisibility_Simple_StoreOnSender> Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list;

Map_server_MapVisibility_Simple_StoreOnSender::Map_server_MapVisibility_Simple_StoreOnSender()
{
    Map_server_MapVisibility_Simple_StoreOnSender::tempBigBufferForChanges[0x00]=0x66;
    Map_server_MapVisibility_Simple_StoreOnSender::tempBigBufferForRemove[0x00]=0x69;
}

Map_server_MapVisibility_Simple_StoreOnSender::~Map_server_MapVisibility_Simple_StoreOnSender()
{
}

unsigned int Map_server_MapVisibility_Simple_StoreOnSender::send_reinsertAll(char *output,const size_t &clients_size)
{
    uint32_t posOutput=0;
    output[posOutput]=0x6B;
    posOutput+=1+4;
    //////////////////////////// insert //////////////////////////
    // can be only this map with this algo, then 1 map
    output[posOutput]=0x01;//map list count
    posOutput+=1;
    *reinterpret_cast<uint16_t *>(output+posOutput)=htole16(id);//map id
    posOutput+=2;
    posOutput+=1;
    unsigned int count=0;
    if(clients_size>1)
    {
        unsigned int index=0;
        while(index<clients_size && index<255)
        {
            const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index_c=map_clients_id.at(index);
            if(index_c!=SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
            {
                Client &c=ClientList::list->rw(index_c);
                posOutput+=playerToFullInsert(c,output+posOutput);
                count++;
            }
            index++;
        }
        *reinterpret_cast<uint32_t *>(output+1)=htole32(posOutput-1-4);//set the dynamic size
    }
    if(count<254)
        output[1+4+1+2]=static_cast<uint8_t>(count);//player count
    else
        output[1+4+1+2]=static_cast<uint8_t>(254);//player count
    return posOutput;
}

unsigned int Map_server_MapVisibility_Simple_StoreOnSender::send_reinsertAllWithFilter(char *output,const size_t &clients_size,const size_t &skipped_id)
{
    if(skipped_id>=255)
        return send_reinsertAll(output,clients_size);
    uint32_t posOutput=0;
    output[posOutput]=0x6B;
    posOutput+=1+4;
    //////////////////////////// insert //////////////////////////
    // can be only this map with this algo, then 1 map
    output[posOutput]=0x01;//map list count
    posOutput+=1;
    *reinterpret_cast<uint16_t *>(output+posOutput)=htole16(id);//map id
    posOutput+=2;
    posOutput+=1;
    unsigned int count=0;
    if(clients_size>1)
    {
        unsigned int index=0;
        while(index<clients_size && index<255)
        {
            const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index_c=map_clients_id.at(index);
            if(index_c!=SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX && index_c!=skipped_id)
            {
                Client &c=ClientList::list->rw(index_c);
                posOutput+=playerToFullInsert(c,output+posOutput);
                count++;
            }
            index++;
        }
        *reinterpret_cast<uint32_t *>(output+1)=htole32(posOutput-1-4);//set the dynamic size
    }
    if(count<254)
        output[1+4+1+2]=static_cast<uint8_t>(count);//player count
    else
        output[1+4+1+2]=static_cast<uint8_t>(254);//player count
    return posOutput;
}

// broadcast all, no filter then resend same data
void Map_server_MapVisibility_Simple_StoreOnSender::min_CPU()
{
    uint32_t posOutput=0;//if > 0 then cache created
    uint32_t baseOutput=0;
    bool cached=false;
    //if too many player then just stop update
    size_t clients_size=map_clients_id.size();
    if(clients_size>254)
        clients_size=254;
    clients_size-=map_removed_index.size();
    if(clients_size>=GlobalServerData::serverSettings.mapVisibility.simple.max)
        return;

    unsigned int index_client=0;
    while(index_client<map_clients_id.size())
    {
        const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &map_c_idP=map_clients_id.size();
        if(map_c_idP!=SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(!ClientList::list->empty(map_c_idP))
            #endif
            {
                Client &client=ClientList::list->rw(map_c_idP);
                ClientWithMap &clientWithMap=ClientList::list->rwWithMap(map_c_idP);
                if(client.pingCountInProgress()<=0)
                {
                    if(clientWithMap.sendedMap!=client.mapIndex)//async multiple map change to more performance
                    {
                        clientWithMap.sendedMap=client.mapIndex;
                        posOutput=0;
                        baseOutput=0;

                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x6C;//ignore id, first insert on this map
                        posOutput+=1;
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)index_client;
                        posOutput+=1;
                    }
                    else
                    {
                        posOutput=2;
                        baseOutput=2;
                    }
                    if(cached==false)
                    {
                        cached=true;

                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x65;//drop all player on map
                        posOutput+=1;//drop all

                        posOutput+=send_reinsertAll(ProtocolParsingBase::tempBigBufferForOutput+1,clients_size);
                    }
                    posOutput+=client.sendPing(ProtocolParsingBase::tempBigBufferForOutput);//pingInProgress++ into this
                    client.sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput+baseOutput,posOutput-baseOutput);
                }
            }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            else
                std::cerr << "Map_server_MapVisibility_Simple_StoreOnSender::min_CPU() ClientList::list.empty(): " << map_c_idP << std::endl;
            #endif
        }
        index_client++;
    }
}

// filter if already send, then consume CPU and use MapManagement/ClientWithMap sendedStatus/sendedMap
void Map_server_MapVisibility_Simple_StoreOnSender::min_network()
{
    //if too many player then just stop update
    size_t clients_size=map_clients_id.size();
    if(clients_size>254)
        clients_size=254;
    clients_size-=map_removed_index.size();
    if(clients_size>=GlobalServerData::serverSettings.mapVisibility.simple.max)
        return;

    unsigned int index_client=0;
    while(index_client<map_clients_id.size())
    {
        if(index_client==0)
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+4+1)=htole16(id);//map id
        const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &map_c_idP=map_clients_id.size();
        if(map_c_idP!=SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(!ClientList::list->empty(map_c_idP))
            #endif
            {
                ClientWithMap &clientWithMap=ClientList::list->rwWithMap(map_c_idP);
                if(clientWithMap.pingCountInProgress()<=0)
                {
                    //see /doc/algo/visibility/constant-time-player-visibility.png
                    if(clientWithMap.sendedMap!=clientWithMap.mapIndex)//async multiple map change to more performance
                    {
                        clientWithMap.sendedMap=clientWithMap.mapIndex;
                        uint32_t posOutput=0;
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x65;//drop all player on map
                        posOutput+=1;//drop all
                        posOutput+=send_reinsertAllWithFilter(ProtocolParsingBase::tempBigBufferForOutput+1,clients_size,index_client);
                        posOutput+=clientWithMap.sendPing(ProtocolParsingBase::tempBigBufferForOutput);//pingInProgress++ into this
                        clientWithMap.sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                        clientWithMap.sendedStatus.clear();
                    }
                    else
                    {
                        //Map_server_MapVisibility_Simple_StoreOnSender::tempBigBufferForChanges;
                        //Map_server_MapVisibility_Simple_StoreOnSender::tempBigBufferForRemove;
                        uint8_t changesCount=0;
                        uint8_t removeCount=0;
                        uint8_t insertCount=0;
                        uint32_t posOutput=0;
                        posOutput+=1+4+1+2+1;

                        //maybe can be optimized via preallocate
                        unsigned int index=0;
                        while(index<map_clients_id.size() && index<255)
                        {
                            if(index_client==index)
                                continue;
                            //new entry
                            if(index<clientWithMap.sendedStatus.size())
                            {
                                ClientWithMap::SendedStatus &c_stat=clientWithMap.sendedStatus[index];
                                const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &other_client_id=map_clients_id.at(index);
                                if(other_client_id!=SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
                                {
                                    const Client &other_client=ClientList::list->at(other_client_id);
                                    if(other_client.getPlayerId()==c_stat.characterId_db)
                                    {
                                        if(c_stat.direction==other_client.getLastDirection() && c_stat.x==other_client.getX() && c_stat.y==other_client.getY())
                                        {}//no change, nothing to send
                                        else
                                        {
                                            //if(changesCount<255)//implied into while(index<map_clients_id.size() && index<255)
                                            {
                                                //only send partial changes
                                                Map_server_MapVisibility_Simple_StoreOnSender::tempBigBufferForChanges[1+4+1+changesCount*(1+1+1+1)]=static_cast<uint8_t>(index);
                                                Map_server_MapVisibility_Simple_StoreOnSender::tempBigBufferForChanges[1+4+1+changesCount*(1+1+1+1)+1]=static_cast<uint8_t>(other_client.getX());
                                                Map_server_MapVisibility_Simple_StoreOnSender::tempBigBufferForChanges[1+4+1+changesCount*(1+1+1+1)+1+1]=static_cast<uint8_t>(other_client.getY());
                                                Map_server_MapVisibility_Simple_StoreOnSender::tempBigBufferForChanges[1+4+1+changesCount*(1+1+1+1)+1+1+1]=static_cast<uint8_t>(other_client.getLastDirection());
                                                changesCount++;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        //delete old other player on map
                                        //full insert should override delete RemovePlayer.push_back(index);
                                        //full insert other player on map (another player)
                                        //if(insertCount<255)//implied into while(index<map_clients_id.size() && index<255)
                                        //if(!ClientList::empty(other_client_id)) should not be needed
                                        {
                                            posOutput+=playerToFullInsert(ClientList::list->at(other_client_id),ProtocolParsingBase::tempBigBufferForOutput+posOutput);
                                            insertCount++;
                                        }
                                    }
                                }
                                else
                                {
                                    if(c_stat.characterId_db==0xffffffff)
                                    {}//donothing, already deleted
                                    else
                                    {
                                        //delete other player on map
                                        //if(removeCount<254)//implied into while(index<map_clients_id.size() && index<255)
                                        {
                                            Map_server_MapVisibility_Simple_StoreOnSender::tempBigBufferForRemove[1+4+1+removeCount]=static_cast<uint8_t>(index);//player count
                                            removeCount++;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &other_client_id=map_clients_id.at(index);
                                if(other_client_id!=SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
                                {
                                    //if(insertCount<255)//implied into while(index<map_clients_id.size() && index<255)
                                    //if(!ClientList::empty(other_client_id)) should not be needed
                                    {
                                        posOutput+=playerToFullInsert(ClientList::list->at(other_client_id),ProtocolParsingBase::tempBigBufferForOutput+posOutput);
                                        insertCount++;
                                    }
                                }
                            }
                            index++;
                        }

                        //flush the buffer
                        if(changesCount>0 || removeCount>0 || insertCount>0)
                        {
                            if(insertCount>0)
                            {
                                if(insertCount<254)
                                    ProtocolParsingBase::tempBigBufferForOutput[1+4+1+2]=static_cast<uint8_t>(insertCount);//player count
                                else
                                    ProtocolParsingBase::tempBigBufferForOutput[1+4+1+2]=static_cast<uint8_t>(254);//player count
                                memcpy(ProtocolParsingBase::tempBigBufferForOutput+1,&posOutput,4);
                            }
                            //merge memory block to minimize the kernel syscall count
                            if(removeCount>0)
                            {
                                Map_server_MapVisibility_Simple_StoreOnSender::tempBigBufferForRemove[1+4]=static_cast<uint8_t>(removeCount);//player count

                                memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,Map_server_MapVisibility_Simple_StoreOnSender::tempBigBufferForRemove,1+4+1+removeCount);
                                posOutput+=1+4+1+removeCount;
                            }
                            if(changesCount>0)
                            {
                                Map_server_MapVisibility_Simple_StoreOnSender::tempBigBufferForChanges[1+4]=static_cast<uint8_t>(changesCount);//player count

                                memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,Map_server_MapVisibility_Simple_StoreOnSender::tempBigBufferForChanges,1+4+1+changesCount*(1+1+1+1));
                                posOutput+=1+4+1+changesCount*(1+1+1+1);
                            }
                            posOutput+=clientWithMap.sendPing(ProtocolParsingBase::tempBigBufferForOutput);//pingInProgress++ into this
                            clientWithMap.sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                        }
                    }
                }
            }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            else
                std::cerr << "Map_server_MapVisibility_Simple_StoreOnSender::min_CPU() ClientList::list.empty(): " << map_c_idP << std::endl;
            #endif
        }
        index_client++;
    }
}
