#ifdef CATCHCHALLENGER_TESTING
// Test stubs must precede the production headers.
#include "../../../test/testingmapmanagement/Stubs.hpp"
#endif
#include "MapVisibilityAlgorithm.hpp"
#include <cstring>
#include "ClientWithMap.hpp"
#include "../GlobalServerData.hpp"
#include "../Client.hpp"
#include "../ClientList.hpp"

#include <iostream>

using namespace CatchChallenger;

// Full insert: [code:1][size:4][maps:1][map:2][players:1][entries...].
unsigned int MapVisibilityAlgorithm::send_reinsertAll(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,char *output,const size_t &clients_size)
{
    if(clients_size<=1)
    {
        return 0;
    }
    uint32_t posOutput=0;
    output[posOutput]=0x6B;
    posOutput+=1+4;// Reserve code and size.

    output[posOutput]=0x01;// One map.
    posOutput+=1;
    {const uint16_t _tmp_le=(htole16(mapIndex));memcpy(output+posOutput,&_tmp_le,sizeof(_tmp_le));}
    posOutput+=2;
    posOutput+=1;// Reserve player count.
    unsigned int count=0;
    unsigned int index=0;
    // Walk sparse slots, including live players above holes; emit at most 254.
    const size_t slot_count=(map_clients_id.size()<255)?map_clients_id.size():255;
    while(index<slot_count && count<254)
    {
        const PLAYER_INDEX_FOR_CONNECTED &index_c=map_clients_id[index];
        if(index_c!=PLAYER_INDEX_FOR_CONNECTED_MAX)
        {
            output[posOutput]=static_cast<uint8_t>(index);
            posOutput+=1;
            const Client &c=ClientList::list->at(index_c);
            #ifdef CATCHCHALLENGER_TESTING
            assertXYInRange(c.getX(),c.getY(),"send_reinsertAll");
            #endif
            posOutput+=playerToFullInsert(c,output+posOutput);
            count++;
        }
        index++;
    }
    {const uint32_t _tmp_le=(htole32(posOutput-1-4));memcpy(output+1,&_tmp_le,sizeof(_tmp_le));}
    if(count<254)
    {
        output[1+4+1+2]=static_cast<uint8_t>(count);
    }
    else
    {
        output[1+4+1+2]=static_cast<uint8_t>(254);
    }
    return posOutput;
}

// Full insert excluding the recipient's local slot.
unsigned int MapVisibilityAlgorithm::send_reinsertAllWithFilter(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,char *output,const size_t &clients_size,const size_t &skipped_id)
{
    if(clients_size<=1)
    {
        return 0;
    }
    if(skipped_id>=255)
    {
        return send_reinsertAll(mapIndex,output,clients_size);
    }
    uint32_t posOutput=0;
    output[posOutput]=0x6B;
    posOutput+=1+4;

    output[posOutput]=0x01;
    posOutput+=1;
    {const uint16_t _tmp_le=(htole16(mapIndex));memcpy(output+posOutput,&_tmp_le,sizeof(_tmp_le));}
    posOutput+=2;
    posOutput+=1;
    unsigned int count=0;
    unsigned int index=0;

    const size_t slot_count=(map_clients_id.size()<255)?map_clients_id.size():255;
    while(index<slot_count && count<254)
    {
        const PLAYER_INDEX_FOR_CONNECTED &index_c=map_clients_id[index];
        if(index_c!=PLAYER_INDEX_FOR_CONNECTED_MAX && index!=skipped_id)
        {
            output[posOutput]=static_cast<uint8_t>(index);
            posOutput+=1;
            const Client &c=ClientList::list->at(index_c);
            #ifdef CATCHCHALLENGER_TESTING
            assertXYInRange(c.getX(),c.getY(),"send_reinsertAllWithFilter");
            #endif
            posOutput+=playerToFullInsert(c,output+posOutput);
            count++;
        }
        index++;
    }
    {const uint32_t _tmp_le=(htole32(posOutput-1-4));memcpy(output+1,&_tmp_le,sizeof(_tmp_le));}
    if(count<254)
    {
        output[1+4+1+2]=static_cast<uint8_t>(count);
    }
    else
    {
        output[1+4+1+2]=static_cast<uint8_t>(254);
    }
    return posOutput;
}

// Broadcast one cached clear/full-insert block to every client on this map.
void MapVisibilityAlgorithm::min_CPU(const CATCHCHALLENGER_TYPE_MAPID &mapIndex)
{
    uint32_t posOutput=0;
    uint32_t baseOutput=0;
    uint32_t cachedEndOutput=0;
    bool cached=false;
    // Slot 255 is reserved; map_removed_index holds only freed slots below it.
    size_t clients_size=map_clients_id.size();
    if(clients_size>254)
    {
        clients_size=254;
    }
    clients_size-=map_removed_index.size();
    if(clients_size>=GlobalServerData::serverSettings.mapVisibility.simple.max)
    {
        return;
    }
    if(clients_size<=1)
    {
        return;
    }

    unsigned int index_client=0;
    while(index_client<map_clients_id.size())
    {
        const PLAYER_INDEX_FOR_CONNECTED &map_c_idP=map_clients_id.at(index_client);
        if(map_c_idP!=PLAYER_INDEX_FOR_CONNECTED_MAX)
        {
            #ifdef CATCHCHALLENGER_HARDENED
            if(!ClientList::list->isNull(map_c_idP))
            #endif
            {
                Client &client=ClientList::list->rw(map_c_idP);
                ClientWithMap &clientWithMap=ClientList::list->rwWithMap(map_c_idP);
                {
                    if(clientWithMap.sendedMap!=client.mapIndex)
                    {
                        clientWithMap.sendedMap=client.mapIndex;
                        posOutput=0;
                        baseOutput=0;

                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x6C;
                        posOutput+=1;
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)index_client;
                        posOutput+=1;
                    }
                    else
                    {
                        // Skip the first-insert header.
                        posOutput=2;
                        baseOutput=2;
                    }

                    if(cached==false)
                    {
                        cached=true;

                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x65;
                        posOutput+=1;

                        posOutput+=send_reinsertAll(mapIndex,ProtocolParsingBase::tempBigBufferForOutput+posOutput,clients_size);
                        cachedEndOutput=posOutput;
                    }
                    else
                    {
                        posOutput=cachedEndOutput;
                    }

                    #ifdef CATCHCHALLENGER_BENCHMARK
                    // Benchmarks omit pings and ACK throttling to measure server capacity.
                    if(false)
                    #else
                    if(client.pingCountInProgress()<=0)
                    #endif
                    {
                        posOutput+=client.sendPing(ProtocolParsingBase::tempBigBufferForOutput+posOutput);
                    }
                    client.sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput+baseOutput,posOutput-baseOutput);
                }
            }
            #ifdef CATCHCHALLENGER_HARDENED
            else
                std::cerr << "MapVisibilityAlgorithm::min_CPU() ClientList::list.empty(): " << map_c_idP << std::endl;
            #endif
        }
        index_client++;
    }
}
