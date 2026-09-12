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
