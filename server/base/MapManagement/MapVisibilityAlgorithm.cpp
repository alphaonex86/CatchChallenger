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
#include <iomanip>

// Wire slots are map-local (cpu/balanced) or recipient-local (network); 255 is reserved.
// Packets: 0x6C self slot, 0x65 clear, 0x6B insert, 0x66 move, 0x69 remove, 0xE3 ping.
using namespace CatchChallenger;

// Scalar DensePlayerState comparisons avoid a separate snapshot prescan.
char MapVisibilityAlgorithm::tempBigBufferForChanges[];
char MapVisibilityAlgorithm::tempBigBufferForRemove[];
uint8_t MapVisibilityAlgorithm::tempInsertSlots[255];
std::vector<MapVisibilityAlgorithm> MapVisibilityAlgorithm::flat_map_list;
DensePlayerState MapVisibilityAlgorithm::tempDenseBuffer[255];
PLAYER_INDEX_FOR_CONNECTED MapVisibilityAlgorithm::tempInsertPlayers[255];
uint8_t MapVisibilityAlgorithm::tempSeenSlot[255];
std::vector<uint8_t> MapVisibilityAlgorithm::tempSlotOfPlayer;
uint32_t MapVisibilityAlgorithm::visibilityTick=0;
// Fallback view extents; resolveViewRange() applies the datapack zoom at load.
uint8_t MapVisibilityAlgorithm::view_x=13;
uint8_t MapVisibilityAlgorithm::view_y=13;

MapVisibilityAlgorithm::MapVisibilityAlgorithm() :
    candidatesCount(0),
    // Must differ from the initial visibilityTick.
    candidatesTick(0xffffffff)
{
    // Seed the shared packet headers once.
    MapVisibilityAlgorithm::tempBigBufferForChanges[0x00]=0x66;
    MapVisibilityAlgorithm::tempBigBufferForRemove[0x00]=0x69;
}

MapVisibilityAlgorithm::~MapVisibilityAlgorithm()
{
}

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

// Catch up from a recipient's private baseline; reuses the shared entry buffers.
void MapVisibilityAlgorithm::sendCoalescedDelta(ClientWithMap &clientWithMap,const CATCHCHALLENGER_TYPE_MAPID &mapIndex,
                                                const unsigned int index_client,const size_t dense_size)
{
    uint8_t changesCount=0;
    uint8_t removeCount=0;
    uint8_t insertCount=0;
    const size_t baseline_size=clientWithMap.sendedStatus.size();
    unsigned int index=0;
    while(index<dense_size)
    {
        if(index_client==index)
        {
        }
        else
        {
            const DensePlayerState &dense=tempDenseBuffer[index];
            if(index<baseline_size)
            {
                const DensePlayerState &sent=clientWithMap.sendedStatus[index];
                if(dense.isEqual(sent))
                {
                }
                else
                {
                    if(dense.isEmpty())
                    {
                        MapVisibilityAlgorithm::tempBigBufferForRemove[1+4+1+removeCount]=static_cast<char>(index);
                        removeCount++;
                    }
                    // Check emptiness before isSameCharacter().
                    else if(sent.isEmpty() || !dense.isSameCharacter(sent))
                    {
                        MapVisibilityAlgorithm::tempInsertSlots[insertCount]=static_cast<uint8_t>(index);
                        insertCount++;
                    }
                    else
                    {
                        char *ce=MapVisibilityAlgorithm::tempBigBufferForChanges+(1+4+1)+changesCount*(1+1+1+1);
                        {const uint32_t _tmp_le=(htole32(dense.wireChangeWord(static_cast<uint8_t>(index))));memcpy(ce,&_tmp_le,sizeof(_tmp_le));}
                        changesCount++;
                    }
                }
            }
            else
            {
                if(!dense.isEmpty())
                {
                    MapVisibilityAlgorithm::tempInsertSlots[insertCount]=static_cast<uint8_t>(index);
                    insertCount++;
                }
            }
        }
        index++;
    }
    if(changesCount==0 && removeCount==0 && insertCount==0)
    {
        return;
    }
    uint32_t posOutput=0;
    posOutput+=1+4+1+2+1;// Reserve the optional insert header.
    if(insertCount>0)
    {
        unsigned int k=0;
        while(k<insertCount)
        {
            const uint8_t insertSlot=MapVisibilityAlgorithm::tempInsertSlots[k];
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<char>(insertSlot);
            posOutput+=1;
            posOutput+=playerToFullInsert(ClientList::list->at(map_clients_id[insertSlot]),ProtocolParsingBase::tempBigBufferForOutput+posOutput);
            k++;
        }
        ProtocolParsingBase::tempBigBufferForOutput[0x00]=0x6B;
        {const uint32_t _tmp_le=(htole32(posOutput-1-4));memcpy(ProtocolParsingBase::tempBigBufferForOutput+1,&_tmp_le,sizeof(_tmp_le));}
        ProtocolParsingBase::tempBigBufferForOutput[1+4]=0x01;
        {const uint16_t _tmp_le=(htole16(mapIndex));memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+4+1,&_tmp_le,sizeof(_tmp_le));}
        if(insertCount<254)
        {
            ProtocolParsingBase::tempBigBufferForOutput[1+4+1+2]=static_cast<uint8_t>(insertCount);
        }
        else
        {
            ProtocolParsingBase::tempBigBufferForOutput[1+4+1+2]=static_cast<uint8_t>(254);
        }
    }
    else
    {
        posOutput=0;
    }
    if(removeCount>0)
    {
        char * const removeOut=ProtocolParsingBase::tempBigBufferForOutput+posOutput;

        memcpy(removeOut,MapVisibilityAlgorithm::tempBigBufferForRemove,1+4+1);
        {const uint32_t _tmp_le=(htole32(1+removeCount));memcpy(removeOut+1,&_tmp_le,sizeof(_tmp_le));}
        removeOut[1+4]=static_cast<char>(removeCount);
        memcpy(removeOut+(1+4+1),MapVisibilityAlgorithm::tempBigBufferForRemove+(1+4+1),removeCount);
        posOutput+=1+4+1+removeCount;
    }
    if(changesCount>0)
    {
        char * const changeOut=ProtocolParsingBase::tempBigBufferForOutput+posOutput;
        memcpy(changeOut,MapVisibilityAlgorithm::tempBigBufferForChanges,1+4+1);
        {const uint32_t _tmp_le=(htole32(1+changesCount*(1+1+1+1)));memcpy(changeOut+1,&_tmp_le,sizeof(_tmp_le));}
        changeOut[1+4]=static_cast<char>(changesCount);
        memcpy(changeOut+(1+4+1),MapVisibilityAlgorithm::tempBigBufferForChanges+(1+4+1),changesCount*(1+1+1+1));
        posOutput+=1+4+1+changesCount*(1+1+1+1);
    }

    #ifdef CATCHCHALLENGER_BENCHMARK
    if(false)
    #else
    if(clientWithMap.pingCountInProgress()<=0)
    #endif
    {
        posOutput+=clientWithMap.sendPing(ProtocolParsingBase::tempBigBufferForOutput+posOutput);
    }
    clientWithMap.sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

// Share one map delta; clients awaiting ACK retain a private baseline.
void MapVisibilityAlgorithm::min_balanced(const CATCHCHALLENGER_TYPE_MAPID &mapIndex)
{
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

    // Map membership and player state stay fixed throughout this broadcast.
    const size_t dense_size=std::min(map_clients_id.size(),static_cast<size_t>(255));
    unsigned int dense_idx=0;
    while(dense_idx<dense_size)
    {
        const PLAYER_INDEX_FOR_CONNECTED &oid=map_clients_id.at(dense_idx);
        if(oid!=PLAYER_INDEX_FOR_CONNECTED_MAX)
        {
            const Client &c=ClientList::list->at(oid);
            #ifdef CATCHCHALLENGER_TESTING
            assertXYInRange(c.getX(),c.getY(),"min_balanced_dense_build");
            #endif
            tempDenseBuffer[dense_idx].set(c.getX(),c.getY(),static_cast<uint8_t>(c.getLastDirection()),c.getPlayerId());
        }
        else
        {
            tempDenseBuffer[dense_idx].setEmpty();
        }
        dense_idx++;
    }

    // Build entries in ascending slot order for the recipient cursors below.
    uint8_t changesCount=0;
    uint8_t removeCount=0;
    uint8_t insertCount=0;
    {
        const size_t previous_size=previousDenseBuffer.size();
        unsigned int index=0;
        while(index<dense_size)
        {
            const DensePlayerState &dense=tempDenseBuffer[index];

            if(index<previous_size)
            {
                const DensePlayerState &sent=previousDenseBuffer[index];
                if(dense.isEqual(sent))
                {
                }

                else
                {
                    if(dense.isEmpty())
                    {
                        MapVisibilityAlgorithm::tempBigBufferForRemove[1+4+1+removeCount]=static_cast<char>(index);
                        removeCount++;
                    }
                    // Check emptiness before isSameCharacter().
                    else if(sent.isEmpty() || !dense.isSameCharacter(sent))
                    {
                        #ifdef CATCHCHALLENGER_TESTING
                        assertXYInRange(dense.getX(),dense.getY(),"min_balanced_path2_replaced");
                        #endif
                        MapVisibilityAlgorithm::tempInsertSlots[insertCount]=static_cast<uint8_t>(index);
                        insertCount++;
                    }

                    else
                    {
                        #ifdef CATCHCHALLENGER_TESTING
                        assertXYInRange(dense.getX(),dense.getY(),"min_balanced_path2_change");
                        #endif
                        // Little-endian memcpy also supports unaligned output.
                        char *ce=MapVisibilityAlgorithm::tempBigBufferForChanges+(1+4+1)+changesCount*(1+1+1+1);
                        {const uint32_t _tmp_le=(htole32(dense.wireChangeWord(static_cast<uint8_t>(index))));memcpy(ce,&_tmp_le,sizeof(_tmp_le));}
                        changesCount++;
                    }
                }
            }

            else
            {
                if(!dense.isEmpty())
                {
                    #ifdef CATCHCHALLENGER_TESTING
                    assertXYInRange(dense.getX(),dense.getY(),"min_balanced_path2_beyond");
                    #endif
                    MapVisibilityAlgorithm::tempInsertSlots[insertCount]=static_cast<uint8_t>(index);
                    insertCount++;
                }
            }
            index++;
        }
    }

    // Prebuild shared headers; only recipients excluding themselves need to patch them.
    if(removeCount>0)
    {
        {const uint32_t _tmp_le=(htole32(1+removeCount));memcpy(MapVisibilityAlgorithm::tempBigBufferForRemove+1,&_tmp_le,sizeof(_tmp_le));}
        MapVisibilityAlgorithm::tempBigBufferForRemove[1+4]=static_cast<char>(removeCount);
    }
    if(changesCount>0)
    {
        {const uint32_t _tmp_le=(htole32(1+changesCount*(1+1+1+1)));memcpy(MapVisibilityAlgorithm::tempBigBufferForChanges+1,&_tmp_le,sizeof(_tmp_le));}
        MapVisibilityAlgorithm::tempBigBufferForChanges[1+4]=static_cast<char>(changesCount);
    }

    // Entries and recipients share slot order, so each cursor advances only once.
    unsigned int cursorChange=0;
    unsigned int cursorRemove=0;
    unsigned int cursorInsert=0;

    bool haveCatchUp=false;
    // An empty shared delta still needs to serve new arrivals and catch-up clients.
    const bool sharedEmpty=(changesCount==0 && removeCount==0 && insertCount==0);

    unsigned int index_client=0;
    while(index_client<map_clients_id.size())
    {
        const PLAYER_INDEX_FOR_CONNECTED &map_c_idP=map_clients_id[index_client];
        if(map_c_idP!=PLAYER_INDEX_FOR_CONNECTED_MAX)
        {
            #ifdef CATCHCHALLENGER_HARDENED
            if(!ClientList::list->isNull(map_c_idP))
            #endif
            {
                ClientWithMap &clientWithMap=ClientList::list->rwWithMap(map_c_idP);
                {
                    // New map: rebuild the recipient's view.
                    if(clientWithMap.sendedMap!=clientWithMap.mapIndex)
                    {
                        clientWithMap.sendedMap=clientWithMap.mapIndex;
                        uint32_t posOutput=0;
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x65;
                        posOutput+=1;
                        posOutput+=send_reinsertAllWithFilter(mapIndex,ProtocolParsingBase::tempBigBufferForOutput+posOutput,clients_size,index_client);

                        #ifdef CATCHCHALLENGER_BENCHMARK
                        if(false)
                        #else
                        if(clientWithMap.pingCountInProgress()<=0)
                        #endif
                        {
                            posOutput+=clientWithMap.sendPing(ProtocolParsingBase::tempBigBufferForOutput+posOutput);
                        }
                        clientWithMap.sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                        // The full insert joins the shared baseline.
                        clientWithMap.sendedStatus.clear();
                    }
                    // Await ACK without advancing this recipient's baseline.
                    else if(clientWithMap.pingCountInProgress()>0)
                    {
                        if(clientWithMap.sendedStatus.empty())
                        {
                            // Preserve last tick's state before previousDenseBuffer is refreshed.
                            const size_t baseline_size=previousDenseBuffer.size();
                            if(baseline_size>0)
                            {
                                clientWithMap.sendedStatus.resize(baseline_size);
                                memcpy(clientWithMap.sendedStatus.data(),previousDenseBuffer.data(),
                                       baseline_size*sizeof(DensePlayerState));
                            }
                        }
                    }
                    // Defer catch-up until the shared buffers are no longer being read.
                    else if(!clientWithMap.sendedStatus.empty())
                    {
                        haveCatchUp=true;
                    }
                    else if(!sharedEmpty)
                    {
                        while(cursorChange<changesCount &&
                              static_cast<uint8_t>(MapVisibilityAlgorithm::tempBigBufferForChanges[(1+4+1)+cursorChange*(1+1+1+1)])<index_client)
                        {
                            cursorChange++;
                        }
                        const bool selfChange=(cursorChange<changesCount &&
                              static_cast<uint8_t>(MapVisibilityAlgorithm::tempBigBufferForChanges[(1+4+1)+cursorChange*(1+1+1+1)])==index_client);
                        while(cursorRemove<removeCount &&
                              static_cast<uint8_t>(MapVisibilityAlgorithm::tempBigBufferForRemove[(1+4+1)+cursorRemove])<index_client)
                        {
                            cursorRemove++;
                        }
                        const bool selfRemove=(cursorRemove<removeCount &&
                              static_cast<uint8_t>(MapVisibilityAlgorithm::tempBigBufferForRemove[(1+4+1)+cursorRemove])==index_client);
                        while(cursorInsert<insertCount &&
                              MapVisibilityAlgorithm::tempInsertSlots[cursorInsert]<index_client)
                        {
                            cursorInsert++;
                        }
                        const bool selfInsert=(cursorInsert<insertCount &&
                              MapVisibilityAlgorithm::tempInsertSlots[cursorInsert]==index_client);
                        // Exclude the recipient's own slot.
                        const uint8_t changesEff=selfChange?static_cast<uint8_t>(changesCount-1):changesCount;
                        const uint8_t removeEff=selfRemove?static_cast<uint8_t>(removeCount-1):removeCount;
                        const uint8_t insertEff=selfInsert?static_cast<uint8_t>(insertCount-1):insertCount;

                        if(changesEff>0 || removeEff>0 || insertEff>0)
                        {
                            uint32_t posOutput=0;
                            posOutput+=1+4+1+2+1;// Reserve the optional insert header.
                            if(insertEff>0)
                            {
                                unsigned int k=0;
                                while(k<insertCount)
                                {
                                    const uint8_t insertSlot=MapVisibilityAlgorithm::tempInsertSlots[k];
                                    if(insertSlot!=index_client)
                                    {
                                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<char>(insertSlot);
                                        posOutput+=1;
                                        posOutput+=playerToFullInsert(ClientList::list->at(map_clients_id[insertSlot]),ProtocolParsingBase::tempBigBufferForOutput+posOutput);
                                    }
                                    k++;
                                }

                                ProtocolParsingBase::tempBigBufferForOutput[0x00]=0x6B;
                                {const uint32_t _tmp_le=(htole32(posOutput-1-4));memcpy(ProtocolParsingBase::tempBigBufferForOutput+1,&_tmp_le,sizeof(_tmp_le));}
                                ProtocolParsingBase::tempBigBufferForOutput[1+4]=0x01;
                                {const uint16_t _tmp_le=(htole16(mapIndex));memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+4+1,&_tmp_le,sizeof(_tmp_le));}
                                if(insertEff<254)
                                {
                                    ProtocolParsingBase::tempBigBufferForOutput[1+4+1+2]=static_cast<uint8_t>(insertEff);
                                }
                                else
                                {
                                    ProtocolParsingBase::tempBigBufferForOutput[1+4+1+2]=static_cast<uint8_t>(254);
                                }
                            }
                            else
                            {
                                posOutput=0;
                            }

                            if(removeEff>0)
                            {
                                char * const removeOut=ProtocolParsingBase::tempBigBufferForOutput+posOutput;
                                if(selfRemove)
                                {
                                    // Patch the header and copy entries on either side of the recipient.
                                    memcpy(removeOut,MapVisibilityAlgorithm::tempBigBufferForRemove,1+4+1);
                                    {const uint32_t _tmp_le=(htole32(1+removeEff));memcpy(removeOut+1,&_tmp_le,sizeof(_tmp_le));}
                                    removeOut[1+4]=static_cast<char>(removeEff);
                                    memcpy(removeOut+(1+4+1),
                                           MapVisibilityAlgorithm::tempBigBufferForRemove+(1+4+1),
                                           cursorRemove);
                                    memcpy(removeOut+(1+4+1)+cursorRemove,
                                           MapVisibilityAlgorithm::tempBigBufferForRemove+(1+4+1)+cursorRemove+1,
                                           removeCount-cursorRemove-1);
                                }
                                else
                                {
                                    memcpy(removeOut,MapVisibilityAlgorithm::tempBigBufferForRemove,1+4+1+removeCount);
                                }
                                posOutput+=1+4+1+removeEff;
                            }

                            if(changesEff>0)
                            {
                                char * const changeOut=ProtocolParsingBase::tempBigBufferForOutput+posOutput;
                                if(selfChange)
                                {
                                    memcpy(changeOut,MapVisibilityAlgorithm::tempBigBufferForChanges,1+4+1);
                                    {const uint32_t _tmp_le=(htole32(1+changesEff*(1+1+1+1)));memcpy(changeOut+1,&_tmp_le,sizeof(_tmp_le));}
                                    changeOut[1+4]=static_cast<char>(changesEff);
                                    memcpy(changeOut+(1+4+1),
                                           MapVisibilityAlgorithm::tempBigBufferForChanges+(1+4+1),
                                           cursorChange*(1+1+1+1));
                                    memcpy(changeOut+(1+4+1)+cursorChange*(1+1+1+1),
                                           MapVisibilityAlgorithm::tempBigBufferForChanges+(1+4+1)+(cursorChange+1)*(1+1+1+1),
                                           (changesCount-cursorChange-1)*(1+1+1+1));
                                }
                                else
                                {
                                    memcpy(changeOut,MapVisibilityAlgorithm::tempBigBufferForChanges,1+4+1+changesCount*(1+1+1+1));
                                }
                                posOutput+=1+4+1+changesEff*(1+1+1+1);
                            }

                            #ifdef CATCHCHALLENGER_BENCHMARK
                            if(false)
                            #else
                            if(clientWithMap.pingCountInProgress()<=0)
                            #endif
                            {
                                posOutput+=clientWithMap.sendPing(ProtocolParsingBase::tempBigBufferForOutput+posOutput);
                            }
                            clientWithMap.sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                        }
                    }
                }
            }
            #ifdef CATCHCHALLENGER_HARDENED
            else
                std::cerr << "MapVisibilityAlgorithm::min_balanced() ClientList::list.empty(): " << map_c_idP << std::endl;
            #endif
        }
        index_client++;
    }

    // Catch-up overwrites shared buffers, so it must follow the shared broadcast.
    index_client=0;
    while(haveCatchUp && index_client<map_clients_id.size())
    {
        const PLAYER_INDEX_FOR_CONNECTED &map_c_idP=map_clients_id[index_client];
        if(map_c_idP!=PLAYER_INDEX_FOR_CONNECTED_MAX)
        {
            #ifdef CATCHCHALLENGER_HARDENED
            if(!ClientList::list->isNull(map_c_idP))
            #endif
            {
                ClientWithMap &clientWithMap=ClientList::list->rwWithMap(map_c_idP);
                #ifdef CATCHCHALLENGER_BENCHMARK
                // Benchmarks bypass ACK throttling, retaining the baseline and map checks.
                if(!clientWithMap.sendedStatus.empty()
                   && clientWithMap.sendedMap==clientWithMap.mapIndex)
                #else
                if(!clientWithMap.sendedStatus.empty() && clientWithMap.pingCountInProgress()<=0
                   && clientWithMap.sendedMap==clientWithMap.mapIndex)
                #endif
                {
                    sendCoalescedDelta(clientWithMap,mapIndex,index_client,dense_size);

                    clientWithMap.sendedStatus.clear();
                }
            }
        }
        index_client++;
    }

    // Publish the new shared baseline after every recipient has been processed.
    if(previousDenseBuffer.size()!=dense_size)
    {
        previousDenseBuffer.resize(dense_size);
    }
    if(dense_size>0)
    {
        memcpy(previousDenseBuffer.data(),tempDenseBuffer,dense_size*sizeof(DensePlayerState));
    }
}

// Translate a border map into this map's coordinates; sides: top, bottom, left, right.
static bool mapSideOffset(const MapVisibilityAlgorithm &map,const uint8_t &side,
                          CATCHCHALLENGER_TYPE_MAPID &otherIndex,int16_t &offset_x,int16_t &offset_y)
{
    switch(side)
    {
        case 0x00:
            otherIndex=map.border.top.mapIndex;
        break;
        case 0x01:
            otherIndex=map.border.bottom.mapIndex;
        break;
        case 0x02:
            otherIndex=map.border.left.mapIndex;
        break;
        default:
            otherIndex=map.border.right.mapIndex;
        break;
    }
    if(otherIndex==65535)
        return false;
    if(otherIndex>=MapVisibilityAlgorithm::flat_map_list.size())
    {
        std::cerr << "mapSideOffset(): border map index out of the map list: " << otherIndex << std::endl;
        return false;
    }
    const MapVisibilityAlgorithm &other=MapVisibilityAlgorithm::flat_map_list.at(otherIndex);
    switch(side)
    {
        case 0x00:
            offset_x=-static_cast<int16_t>(map.border.top.x_offset);
            offset_y=-static_cast<int16_t>(other.height);
        break;
        case 0x01:
            offset_x=-static_cast<int16_t>(map.border.bottom.x_offset);
            offset_y=static_cast<int16_t>(map.height);
        break;
        case 0x02:
            offset_x=-static_cast<int16_t>(other.width);
            offset_y=-static_cast<int16_t>(map.border.left.y_offset);
        break;
        default:
            offset_x=static_cast<int16_t>(map.width);
            offset_y=-static_cast<int16_t>(map.border.right.y_offset);
        break;
    }
    return true;
}

// Match the client's rectTouch(): only touching maps can be displayed.
static void addNeighbour(MapVisibilityAlgorithm &map,const CATCHCHALLENGER_TYPE_MAPID &selfIndex,
                         const CATCHCHALLENGER_TYPE_MAPID &otherIndex,const int16_t &offset_x,const int16_t &offset_y)
{
    if(otherIndex==selfIndex)
        return;
    unsigned int index=0;
    while(index<map.neighbours.size())
    {
        if(map.neighbours.at(index).mapIndex==otherIndex)
            return;// Keep the first path.
        index++;
    }
    const MapVisibilityAlgorithm &other=MapVisibilityAlgorithm::flat_map_list.at(otherIndex);
    // Include touching edges.
    if((offset_x+static_cast<int16_t>(other.width))<0 || static_cast<int16_t>(map.width)<offset_x)
        return;
    if((offset_y+static_cast<int16_t>(other.height))<0 || static_cast<int16_t>(map.height)<offset_y)
        return;
    MapVisibilityAlgorithm::NeighbourMap neighbour;
    neighbour.mapIndex=otherIndex;
    neighbour.offset_x=offset_x;
    neighbour.offset_y=offset_y;
    map.neighbours.push_back(neighbour);
}

// Resolve visible neighbours once at load.
void MapVisibilityAlgorithm::resolveNeighbours()
{
    unsigned int mapIndex=0;
    while(mapIndex<flat_map_list.size())
    {
        MapVisibilityAlgorithm &map=flat_map_list[mapIndex];
        map.neighbours.clear();
        uint8_t side=0;
        while(side<4)
        {
            CATCHCHALLENGER_TYPE_MAPID directIndex=65535;
            int16_t offset_x=0,offset_y=0;
            if(mapSideOffset(map,side,directIndex,offset_x,offset_y))
            {
                addNeighbour(map,static_cast<CATCHCHALLENGER_TYPE_MAPID>(mapIndex),directIndex,offset_x,offset_y);
                // The second hop reaches diagonals; addNeighbour() rejects non-touching maps.
                uint8_t farSide=0;
                while(farSide<4)
                {
                    CATCHCHALLENGER_TYPE_MAPID farIndex=65535;
                    int16_t farOffsetX=0,farOffsetY=0;
                    if(mapSideOffset(flat_map_list.at(directIndex),farSide,farIndex,farOffsetX,farOffsetY))
                        addNeighbour(map,static_cast<CATCHCHALLENGER_TYPE_MAPID>(mapIndex),farIndex,
                                     static_cast<int16_t>(offset_x+farOffsetX),
                                     static_cast<int16_t>(offset_y+farOffsetY));
                    farSide++;
                }
            }
            side++;
        }
        mapIndex++;
    }
}

// Match client scaling; cover portrait orientation and partially visible tiles.
void MapVisibilityAlgorithm::resolveViewRange(const uint8_t &datapackZoom)
{
    uint32_t zoom=datapackZoom;
    if(zoom<1)
        zoom=CATCHCHALLENGER_SERVER_MAP_VIEW_ZOOM_DEFAULT;
    uint32_t screenMin=CATCHCHALLENGER_SERVER_MAP_VIEW_SCREEN_WIDTH;
    if(CATCHCHALLENGER_SERVER_MAP_VIEW_SCREEN_HEIGHT<screenMin)
        screenMin=CATCHCHALLENGER_SERVER_MAP_VIEW_SCREEN_HEIGHT;
    // ceil(screenMin * zoom / 512), clamped to at least 1.
    uint32_t factor=(screenMin*zoom+511)/512;
    if(factor<1)
        factor=1;
    const uint32_t tileScreen=CATCHCHALLENGER_SERVER_MAP_VIEW_TILE_PIXEL*factor;

    uint32_t tiles=(CATCHCHALLENGER_SERVER_MAP_VIEW_SCREEN_WIDTH+tileScreen-1)/tileScreen;
    const uint32_t tilesHeight=(CATCHCHALLENGER_SERVER_MAP_VIEW_SCREEN_HEIGHT+tileScreen-1)/tileScreen;
    if(tilesHeight>tiles)
        tiles=tilesHeight;
    uint32_t resolvedX=tiles/2+1;
    uint32_t resolvedY=tiles/2+1;
    // Maps are bounded to 127 tiles per axis.
    if(resolvedX>127)
        resolvedX=127;
    if(resolvedY>127)
        resolvedY=127;
    view_x=static_cast<uint8_t>(resolvedX);
    view_y=static_cast<uint8_t>(resolvedY);
}

// Finish the insert header for one source map.
static void closeInsertGroup(char * const buffer,const uint32_t &groupStart,const uint32_t &posOutput,
                             const CATCHCHALLENGER_TYPE_MAPID &mapId,const uint8_t &count)
{
    buffer[groupStart]=0x6B;
    {const uint32_t _tmp_le=(htole32(posOutput-groupStart-1-4));memcpy(buffer+groupStart+1,&_tmp_le,sizeof(_tmp_le));}
    buffer[groupStart+1+4]=0x01;
    {const uint16_t _tmp_le=(htole16(mapId));memcpy(buffer+groupStart+1+4+1,&_tmp_le,sizeof(_tmp_le));}
    buffer[groupStart+1+4+1+2]=static_cast<char>(count);
}

// Interaction range includes visible border maps.
bool MapVisibilityAlgorithm::inViewRange(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const COORD_TYPE &x,const COORD_TYPE &y,
                                         const CATCHCHALLENGER_TYPE_MAPID &otherMapIndex,const COORD_TYPE &otherX,const COORD_TYPE &otherY)
{
    if(mapIndex>=flat_map_list.size() || otherMapIndex>=flat_map_list.size())
        return false;
    int16_t offset_x=0;
    int16_t offset_y=0;
    if(otherMapIndex!=mapIndex)
    {
        const MapVisibilityAlgorithm &map=flat_map_list.at(mapIndex);
        unsigned int index=0;
        while(index<map.neighbours.size() && map.neighbours.at(index).mapIndex!=otherMapIndex)
            index++;
        if(index>=map.neighbours.size())
            return false;
        offset_x=map.neighbours.at(index).offset_x;
        offset_y=map.neighbours.at(index).offset_y;
    }
    int16_t dx=static_cast<int16_t>(static_cast<int16_t>(otherX)+offset_x-static_cast<int16_t>(x));
    if(dx<0)
        dx=-dx;
    int16_t dy=static_cast<int16_t>(static_cast<int16_t>(otherY)+offset_y-static_cast<int16_t>(y));
    if(dy<0)
        dy=-dy;
    return dx<=static_cast<int16_t>(view_x) && dy<=static_cast<int16_t>(view_y);
}

// Advance before broadcasting any map: neighbours share the same tick.
void MapVisibilityAlgorithm::beginTick()
{
    visibilityTick++;
}

// Snapshot once per tick; map membership stays fixed during the broadcast.
void MapVisibilityAlgorithm::refreshCandidates()
{
    if(candidatesTick==visibilityTick)
        return;
    candidatesTick=visibilityTick;
    const size_t slotCount=map_clients_id.size();
    // Preserve sparse slots; consumers skip empty entries.
    if(candidates.size()<slotCount)
        candidates.resize(slotCount);
    size_t slot=0;
    while(slot<slotCount)
    {
        CandidateState &entry=candidates[slot];
        const PLAYER_INDEX_FOR_CONNECTED playerIndex=map_clients_id[slot];
        entry.player=playerIndex;
        if(playerIndex!=PLAYER_INDEX_FOR_CONNECTED_MAX)
        {
            #ifdef CATCHCHALLENGER_HARDENED
            if(!ClientList::list->isNull(playerIndex))
            #endif
            {
                const Client &c=ClientList::list->at(playerIndex);
                entry.state.set(c.getX(),c.getY(),static_cast<uint8_t>(c.getLastDirection()),c.getPlayerId());
            }
            #ifdef CATCHCHALLENGER_HARDENED
            else
            {
                std::cerr << "MapVisibilityAlgorithm::refreshCandidates() ClientList::list.empty(): "
                          << playerIndex << std::endl;
                entry.player=PLAYER_INDEX_FOR_CONNECTED_MAX;
            }
            #endif
        }
        slot++;
    }
    candidatesCount=static_cast<uint16_t>(slotCount);
}

// Diff each recipient's view across this map and its visible neighbours.
void MapVisibilityAlgorithm::min_network(const CATCHCHALLENGER_TYPE_MAPID &mapIndex)
{
    unsigned int index_client=0;
    while(index_client<map_clients_id.size())
    {
        const PLAYER_INDEX_FOR_CONNECTED &map_c_idP=map_clients_id[index_client];
        if(map_c_idP!=PLAYER_INDEX_FOR_CONNECTED_MAX)
        {
            #ifdef CATCHCHALLENGER_HARDENED
            if(!ClientList::list->isNull(map_c_idP))
            #endif
            {
                ClientWithMap &clientWithMap=ClientList::list->rwWithMap(map_c_idP);
                // Keep visibleSlots unchanged until the previous broadcast is acknowledged.
                #ifdef CATCHCHALLENGER_BENCHMARK

                if(true)
                #else
                if(clientWithMap.pingCountInProgress()<=0)
                #endif
                    sendViewDelta(clientWithMap,map_c_idP,mapIndex);
            }
            #ifdef CATCHCHALLENGER_HARDENED
            else
                std::cerr << "MapVisibilityAlgorithm::min_network() ClientList::list.empty(): " << map_c_idP << std::endl;
            #endif
        }
        index_client++;
    }
}

// Send removals before inserts so freed wire slots can be reused in this tick.
void MapVisibilityAlgorithm::sendViewDelta(ClientWithMap &recipient,const PLAYER_INDEX_FOR_CONNECTED &recipientIndex,
                                           const CATCHCHALLENGER_TYPE_MAPID &mapIndex)
{
    uint32_t posOutput=0;
    // Rebuild the view on map changes, including border crossings.
    if(recipient.sendedMap!=mapIndex)
    {
        recipient.sendedMap=mapIndex;
        // Avoid sending a clear packet when nothing is displayed.
        bool displaySomething=false;
        unsigned int displayedIndex=0;
        while(displayedIndex<recipient.visibleSlots.size())
        {
            if(recipient.visibleSlots.at(displayedIndex).player!=PLAYER_INDEX_FOR_CONNECTED_MAX)
            {
                displaySomething=true;
                displayedIndex=recipient.visibleSlots.size();
            }
            else
                displayedIndex++;
        }
        recipient.visibleSlots.clear();
        if(displaySomething)
        {
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x65;
            posOutput+=1;
        }
    }
    std::vector<ClientWithMap::VisibleSlot> &slots=recipient.visibleSlots;

    // Index displayed players; zero means absent, otherwise the value is slot + 1.
    uint8_t liveCount=0;
    unsigned int slotIndex=0;
    // Cache sizes across virtual ClientList calls; slots grow only in the insert pass.
    const unsigned int slotCount=static_cast<unsigned int>(slots.size());
    while(slotIndex<slotCount)
    {
        const ClientWithMap::VisibleSlot &visibleSlot=slots[slotIndex];
        tempSeenSlot[slotIndex]=0x00;
        if(visibleSlot.player!=PLAYER_INDEX_FOR_CONNECTED_MAX)
        {
            if(tempSlotOfPlayer.size()<=static_cast<size_t>(visibleSlot.player))
                tempSlotOfPlayer.resize(static_cast<size_t>(visibleSlot.player)+1,0x00);
            tempSlotOfPlayer[visibleSlot.player]=static_cast<uint8_t>(slotIndex+1);
            liveCount++;
        }
        slotIndex++;
    }

    // The visibility cap applies per recipient.
    uint8_t visibleMax=254;
    if(GlobalServerData::serverSettings.mapVisibility.simple.max<254)
        visibleMax=static_cast<uint8_t>(GlobalServerData::serverSettings.mapVisibility.simple.max);
    const int16_t recipientX=static_cast<int16_t>(recipient.getX());
    const int16_t recipientY=static_cast<int16_t>(recipient.getY());
    const int16_t viewX=static_cast<int16_t>(view_x);
    const int16_t viewY=static_cast<int16_t>(view_y);
    // Hysteresis keeps edge movement from alternating inserts and removals.
    const int16_t margin=static_cast<int16_t>(GlobalServerData::serverSettings.mapVisibility.viewMargin);
    const int16_t keepX=static_cast<int16_t>(viewX+margin);
    const int16_t keepY=static_cast<int16_t>(viewY+margin);
    uint8_t changesCount=0;
    uint8_t removeCount=0;
    uint8_t insertCount=0;

    const size_t slotOfPlayerSize=tempSlotOfPlayer.size();
    const uint8_t * const slotOfPlayer=tempSlotOfPlayer.empty()?NULL:tempSlotOfPlayer.data();
    const unsigned int neighbourCount=static_cast<unsigned int>(neighbours.size());
    unsigned int sourceIndex=0;
    while(sourceIndex<=neighbourCount)
    {
        MapVisibilityAlgorithm *sourceMap;
        CATCHCHALLENGER_TYPE_MAPID sourceMapIndex;
        int16_t offset_x;
        int16_t offset_y;
        if(sourceIndex==0)
        {
            sourceMap=this;
            sourceMapIndex=mapIndex;
            offset_x=0;
            offset_y=0;
        }
        else
        {
            const NeighbourMap &neighbour=neighbours[sourceIndex-1];
            sourceMap=&flat_map_list[neighbour.mapIndex];
            sourceMapIndex=neighbour.mapIndex;
            offset_x=neighbour.offset_x;
            offset_y=neighbour.offset_y;
        }
        // Skip maps outside the recipient's keep rectangle.
        if((offset_x+static_cast<int16_t>(sourceMap->width)-1)>=(recipientX-keepX) &&
           (recipientX+keepX)>=offset_x &&
           (offset_y+static_cast<int16_t>(sourceMap->height)-1)>=(recipientY-keepY) &&
           (recipientY+keepY)>=offset_y)
        {
            sourceMap->refreshCandidates();
            unsigned int sourceSlot=0;
            const unsigned int sourceSlotCount=sourceMap->candidatesCount;
            const CandidateState * const sourceCandidates=sourceMap->candidates.data();
            while(sourceSlot<sourceSlotCount)
            {
                const CandidateState &entry=sourceCandidates[sourceSlot];
                const PLAYER_INDEX_FOR_CONNECTED candidateIndex=entry.player;

                if(candidateIndex!=PLAYER_INDEX_FOR_CONNECTED_MAX && candidateIndex!=recipientIndex)
                {
                    int16_t dx=static_cast<int16_t>(static_cast<int16_t>(entry.state.getX())+offset_x-recipientX);
                    if(dx<0)
                        dx=-dx;
                    int16_t dy=static_cast<int16_t>(static_cast<int16_t>(entry.state.getY())+offset_y-recipientY);
                    if(dy<0)
                        dy=-dy;
                    const uint8_t known=(static_cast<size_t>(candidateIndex)<slotOfPlayerSize)?
                                slotOfPlayer[candidateIndex]:static_cast<uint8_t>(0x00);
                    if(known!=0x00)
                    {
                        if(dx<=keepX && dy<=keepY)
                        {
                            const uint8_t slot=static_cast<uint8_t>(known-1);
                            tempSeenSlot[slot]=0x01;
                            ClientWithMap::VisibleSlot &visibleSlot=slots[slot];
                            const DensePlayerState &current=entry.state;
                            if(visibleSlot.map!=sourceMapIndex || !current.isSameCharacter(visibleSlot.state))
                            {
                                // Map or character changes require reinsertion: move packets carry neither.
                                tempInsertPlayers[insertCount]=candidateIndex;
                                tempInsertSlots[insertCount]=slot;
                                insertCount++;
                                visibleSlot.map=sourceMapIndex;
                                visibleSlot.state=current;
                            }
                            else if(!current.isEqual(visibleSlot.state))
                            {
                                char * const changeEntry=MapVisibilityAlgorithm::tempBigBufferForChanges+(1+4+1)+changesCount*(1+1+1+1);
                                {const uint32_t _tmp_le=(htole32(current.wireChangeWord(slot)));memcpy(changeEntry,&_tmp_le,sizeof(_tmp_le));}
                                changesCount++;
                                visibleSlot.state=current;
                            }
                        }
                    }
                    else if(dx<=viewX && dy<=viewY)
                    {
                        // Allocate new slots after removals.
                        if(liveCount<visibleMax && insertCount<254)
                        {
                            tempInsertPlayers[insertCount]=candidateIndex;
                            tempInsertSlots[insertCount]=0xff;// Allocate below.
                            insertCount++;
                            liveCount++;
                        }
                    }
                }
                sourceSlot++;
            }
        }
        sourceIndex++;
    }

    // Clear the shared lookup before returning or serving another recipient.
    slotIndex=0;
    while(slotIndex<slotCount)
    {
        ClientWithMap::VisibleSlot &visibleSlot=slots[slotIndex];
        if(visibleSlot.player!=PLAYER_INDEX_FOR_CONNECTED_MAX)
        {
            tempSlotOfPlayer[visibleSlot.player]=0x00;
            if(tempSeenSlot[slotIndex]==0x00)
            {
                MapVisibilityAlgorithm::tempBigBufferForRemove[1+4+1+removeCount]=static_cast<char>(slotIndex);
                removeCount++;
                visibleSlot.player=PLAYER_INDEX_FOR_CONNECTED_MAX;
                liveCount--;
            }
        }
        slotIndex++;
    }
    if(posOutput==0 && changesCount==0 && removeCount==0 && insertCount==0)
        return;

    // Reuse freed slots before growing the recipient's table.
    unsigned int insertIndex=0;
    unsigned int freeSlot=0;
    while(insertIndex<insertCount)
    {
        if(MapVisibilityAlgorithm::tempInsertSlots[insertIndex]==0xff)
        {
            while(freeSlot<slots.size() && slots[freeSlot].player!=PLAYER_INDEX_FOR_CONNECTED_MAX)
                freeSlot++;
            if(freeSlot>=slots.size())
            {
                ClientWithMap::VisibleSlot newSlot;
                newSlot.player=PLAYER_INDEX_FOR_CONNECTED_MAX;
                newSlot.map=65535;
                newSlot.state.setEmpty();
                slots.push_back(newSlot);
            }
            const PLAYER_INDEX_FOR_CONNECTED &candidateIndex=MapVisibilityAlgorithm::tempInsertPlayers[insertIndex];
            const Client &candidate=ClientList::list->at(candidateIndex);
            ClientWithMap::VisibleSlot &visibleSlot=slots[freeSlot];
            visibleSlot.player=candidateIndex;
            visibleSlot.map=candidate.mapIndex;
            visibleSlot.state.set(candidate.getX(),candidate.getY(),
                                  static_cast<uint8_t>(candidate.getLastDirection()),candidate.getPlayerId());
            MapVisibilityAlgorithm::tempInsertSlots[insertIndex]=static_cast<uint8_t>(freeSlot);
            freeSlot++;
        }
        insertIndex++;
    }

    if(removeCount>0)
    {
        char * const removeOut=ProtocolParsingBase::tempBigBufferForOutput+posOutput;

        memcpy(removeOut,MapVisibilityAlgorithm::tempBigBufferForRemove,1+4+1);
        {const uint32_t _tmp_le=(htole32(1+removeCount));memcpy(removeOut+1,&_tmp_le,sizeof(_tmp_le));}
        removeOut[1+4]=static_cast<char>(removeCount);
        memcpy(removeOut+(1+4+1),MapVisibilityAlgorithm::tempBigBufferForRemove+(1+4+1),removeCount);
        posOutput+=1+4+1+removeCount;
    }
    insertIndex=0;
    while(insertIndex<insertCount)
    {
        // Candidates are already grouped by source map.
        const CATCHCHALLENGER_TYPE_MAPID groupMap=
                ClientList::list->at(MapVisibilityAlgorithm::tempInsertPlayers[insertIndex]).mapIndex;
        const uint32_t groupStart=posOutput;
        posOutput+=1+4+1+2+1;
        uint8_t groupCount=0;
        while(insertIndex<insertCount)
        {
            const Client &candidate=ClientList::list->at(MapVisibilityAlgorithm::tempInsertPlayers[insertIndex]);
            if(candidate.mapIndex!=groupMap)
                break;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<char>(MapVisibilityAlgorithm::tempInsertSlots[insertIndex]);
            posOutput+=1;
            posOutput+=playerToFullInsert(candidate,ProtocolParsingBase::tempBigBufferForOutput+posOutput);
            groupCount++;
            insertIndex++;
        }
        closeInsertGroup(ProtocolParsingBase::tempBigBufferForOutput,groupStart,posOutput,groupMap,groupCount);
    }
    if(changesCount>0)
    {
        char * const changeOut=ProtocolParsingBase::tempBigBufferForOutput+posOutput;

        memcpy(changeOut,MapVisibilityAlgorithm::tempBigBufferForChanges,1+4+1);
        {const uint32_t _tmp_le=(htole32(1+changesCount*(1+1+1+1)));memcpy(changeOut+1,&_tmp_le,sizeof(_tmp_le));}
        changeOut[1+4]=static_cast<char>(changesCount);
        memcpy(changeOut+(1+4+1),MapVisibilityAlgorithm::tempBigBufferForChanges+(1+4+1),changesCount*(1+1+1+1));
        posOutput+=1+4+1+changesCount*(1+1+1+1);
    }

    #ifdef CATCHCHALLENGER_BENCHMARK
    if(false)
    #else
    if(recipient.pingCountInProgress()<=0)
    #endif
    {
        posOutput+=recipient.sendPing(ProtocolParsingBase::tempBigBufferForOutput+posOutput);
    }
    recipient.sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}
