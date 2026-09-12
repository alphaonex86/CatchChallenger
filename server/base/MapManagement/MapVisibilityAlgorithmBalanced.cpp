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
