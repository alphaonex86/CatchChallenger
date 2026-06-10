#ifdef CATCHCHALLENGER_TESTING
//testingmapmanagement.py builds with -DCATCHCHALLENGER_TESTING to inject
//stub Client/ClientList/etc. via Stubs.hpp; it pre-defines the header
//guards of the heavy server-side includes that follow so the rest of
//this translation unit compiles against the stubs. The define also
//enables a handful of assertXYInRange(...) coordinate sanity checks
//further down — production builds compile those out entirely.
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

/** CRITICAL CODE: performance-sensitive hot path that drives how every player
 *  sees the others on the map. Touch with care — this runs on every tick for
 *  every visible player on every map.
 *
 *  Two broadcast strategies are implemented, picked per server configuration:
 *
 *  - min_CPU (stateless): rebroadcast every player's x, y and direction every
 *    tick regardless of whether anything changed. The server keeps no per-
 *    recipient state; clients filter out their own and unchanged data on
 *    receipt. Cheapest possible CPU on the server, at the cost of higher
 *    network usage.
 *
 *  - min_network (stateful): for each recipient, remember what was last sent
 *    (sendedStatus) and only emit the players whose data actually changed
 *    (insert / remove / move). Much heavier on CPU (per-recipient diff every
 *    tick) but uses far less network.
 *
 *  limit visible 254 player at time (store internal index, 255, 65535 if not found), drop branch, /2 network, less code
 *  way to do:
 *  1) dropall + just do full insert and send all vector
 *  2) store last state and do a diff for each player
 * when diff, some entry in list can be null, allow quick compare, some entry will just be replaced, ELSE need database id or pseudo std::map resolution
 * when broadcast, can if > 2M {while SIMD} else {C}
 *
 * Packet codes used:
 *   0x6C = first insert on this map (fixed 2 bytes: code + self slot index)
 *   0x65 = drop all players on map (fixed 1 byte: code only)
 *   0x6B = full insert players (dynamic: code + size4 + map_count1 + mapId2 + player_count1 + player_data)
 *   0x66 = player position/direction changes (dynamic: code + size4 + count1 + entries[slot+x+y+dir])
 *   0x69 = remove players from map (dynamic: code + size4 + count1 + slot_indices)
 *   0xE3 = ping (fixed 2 bytes: code + query_number)
 *
 * Player identifiers in all packets use local slot index (position in map_clients_id vector).
*/

using namespace CatchChallenger;

//tempDenseBuffer (the per-tick snapshot) and ClientWithMap::sendedStatus (the
//per-recipient last-sent state) are the SAME DensePlayerState type, so the
//diff loop compares a slot with one isEqual() and the sent-state is refreshed
//with a single flat memcpy of the dense snapshot. The slot layout (full
//8-byte db id, or 4-byte truncated via
//CATCHCHALLENGER_VISIBILITY_TRUNCATED_DB_ID) lives entirely in
//DensePlayerState.hpp — this file only uses its inline helpers.
//
//SIMD / skip-gate re-test on this packed layout (2026-06-10, owner-set 40%
//movers workload, all byte-identical output): SSE2 4-slot prescan −4..−6%,
//generic memcmp(16) group prescan −7..−15%, whole-snapshot memcmp gate
//−3..−12% — ALL slower than the plain scalar isEqual() loop. At 40% movement
//a 4-slot group is all-equal only 0.6^4≈13% of the time, so a prescan pays
//its compare on ~87% of groups and then walks them scalar anyway; the
//whole-snapshot gate hits only when NO player moved (0.6^N). Don't re-add
//without first changing the workload model (a skip strategy needs <15%
//movement to break even).

//to prevent allocate memory
char MapVisibilityAlgorithm::tempBigBufferForChanges[];
char MapVisibilityAlgorithm::tempBigBufferForRemove[];
std::vector<MapVisibilityAlgorithm> MapVisibilityAlgorithm::flat_map_list;
DensePlayerState MapVisibilityAlgorithm::tempDenseBuffer[255];

MapVisibilityAlgorithm::MapVisibilityAlgorithm()
{
    MapVisibilityAlgorithm::tempBigBufferForChanges[0x00]=0x66;
    MapVisibilityAlgorithm::tempBigBufferForRemove[0x00]=0x69;
}

MapVisibilityAlgorithm::~MapVisibilityAlgorithm()
{
}

/// Build a 0x6B packet listing ALL players on this map.
/// Buffer layout: [0x6B][size:4][map_count:1][mapId:2][player_count:1][player_entries...]
/// Each player entry: [slot:1][x:1][y:1][dir|type:1][pseudo_len:1][pseudo:N][skinId:1][monsterId:2]
/// Returns total bytes written to output (including the 0x6B header).
unsigned int MapVisibilityAlgorithm::send_reinsertAll(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,char *output,const size_t &clients_size)
{
    if(clients_size<=1)
    {
        return 0;
    }
    uint32_t posOutput=0;
    output[posOutput]=0x6B;
    posOutput+=1+4;//skip code(1) + size(4), size filled at end
    //////////////////////////// insert //////////////////////////
    // can be only this map with this algo, then 1 map
    output[posOutput]=0x01;//map list count
    posOutput+=1;
    {const uint16_t _tmp_le=(htole16(mapIndex));memcpy(output+posOutput,&_tmp_le,sizeof(_tmp_le));}//map id
    posOutput+=2;
    posOutput+=1;//skip player count, filled at end
    unsigned int count=0;
    unsigned int index=0;
    while(index<clients_size && index<255)
    {
        const PLAYER_INDEX_FOR_CONNECTED &index_c=map_clients_id.at(index);
        if(index_c!=PLAYER_INDEX_FOR_CONNECTED_MAX)
        {
            output[posOutput]=static_cast<uint8_t>(index);//local slot index, consistent with changes/removes
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
    {const uint32_t _tmp_le=(htole32(posOutput-1-4));memcpy(output+1,&_tmp_le,sizeof(_tmp_le));}//set the dynamic size (data bytes after code+size)
    if(count<254)
    {
        output[1+4+1+2]=static_cast<uint8_t>(count);//player count
    }
    else
    {
        output[1+4+1+2]=static_cast<uint8_t>(254);//player count
    }
    return posOutput;
}

/// Same as send_reinsertAll but skips the player at local slot skipped_id (the recipient).
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
    //////////////////////////// insert //////////////////////////
    // can be only this map with this algo, then 1 map
    output[posOutput]=0x01;//map list count
    posOutput+=1;
    {const uint16_t _tmp_le=(htole16(mapIndex));memcpy(output+posOutput,&_tmp_le,sizeof(_tmp_le));}//map id
    posOutput+=2;
    posOutput+=1;
    unsigned int count=0;
    unsigned int index=0;
    while(index<clients_size && index<255)
    {
        const PLAYER_INDEX_FOR_CONNECTED &index_c=map_clients_id.at(index);
        if(index_c!=PLAYER_INDEX_FOR_CONNECTED_MAX && index!=skipped_id)//compare local slot, not global id
        {
            output[posOutput]=static_cast<uint8_t>(index);//local slot index, consistent with changes/removes
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
    {const uint32_t _tmp_le=(htole32(posOutput-1-4));memcpy(output+1,&_tmp_le,sizeof(_tmp_le));}//set the dynamic size
    if(count<254)
    {
        output[1+4+1+2]=static_cast<uint8_t>(count);//player count
    }
    else
    {
        output[1+4+1+2]=static_cast<uint8_t>(254);//player count
    }
    return posOutput;
}

/// min_CPU (stateless broadcast): resend every player's x, y and direction
/// every tick whether or not anything actually changed. The server keeps no
/// per-recipient state; clients are responsible for filtering out themselves
/// and ignoring unchanged entries. Minimises CPU at the cost of more network.
/// Every tick (150ms), sends [0x65 drop_all][0x6B full_insert][0xE3 ping] to each client.
/// First tick for a client also prepends [0x6C first_insert self_slot].
/// Caches the 0x65+0x6B block across clients on the same map (same data, different ping).
void MapVisibilityAlgorithm::min_CPU(const CATCHCHALLENGER_TYPE_MAPID &mapIndex)
{
    uint32_t posOutput=0;//if > 0 then cache created
    uint32_t baseOutput=0;
    uint32_t cachedEndOutput=0;
    bool cached=false;
    //if too many player then just stop update
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
                    //first time on this map: prepend 0x6C header with self slot index
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
                        //same map as last tick: skip the 0x6C header (positions [0..1])
                        posOutput=2;
                        baseOutput=2;
                    }
                    //build the 0x65+0x6B block once, reuse for all clients on this map
                    if(cached==false)
                    {
                        cached=true;

                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x65;//drop all player on map
                        posOutput+=1;//drop all

                        posOutput+=send_reinsertAll(mapIndex,ProtocolParsingBase::tempBigBufferForOutput+posOutput,clients_size);
                        cachedEndOutput=posOutput;
                    }
                    else
                    {
                        //reuse cached 0x65+0x6B data at [2..cachedEndOutput)
                        posOutput=cachedEndOutput;
                    }
                    //only append ping if none pending, to avoid exhausting query numbers
                    if(client.pingCountInProgress()<=0)
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

/// min_network (stateful diff): only send the other players that actually
/// changed since the last tick, by remembering per recipient what was last
/// sent (ClientWithMap::sendedStatus / sendedMap). Costs significantly more
/// CPU than min_CPU (diff per recipient per tick) but uses far less network.
/// On each tick:
///   - Path 1 (new map): sends [0x65 drop_all][0x6B full_insert_filtered][0xE3 ping], populates sendedStatus
///   - Path 2 (same map): compares current state with sendedStatus, sends only:
///       * 0x6B for new/replaced players (inserts)
///       * 0x69 for removed players (removes)
///       * 0x66 for moved players (changes = x/y/direction diff)
///     then updates sendedStatus
void MapVisibilityAlgorithm::min_network(const CATCHCHALLENGER_TYPE_MAPID &mapIndex)
{
    //if too many player then just stop update
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
    //Same guard as min_CPU(): nothing to broadcast with 0 or 1 client on the
    //map, and send_reinsertAllWithFilter() would return 0 leaving the buffer
    //half-composed.
    if(clients_size<=1)
    {
        return;
    }

    // Compose dense buffer of current player states ONCE per call: the map's
    // current x/y/db_id/direction snapshot is identical for every recipient,
    // so build it once (N scattered Client reads) and reuse it across all N
    // recipient diffs (N contiguous cache-friendly reads each) instead of
    // re-reading the scattered Client objects N*N times. Map state does not
    // change during the loop below (no insert/remove, only sends), so the
    // snapshot stays valid for the whole call.
    const size_t dense_size=std::min(map_clients_id.size(),static_cast<size_t>(255));
    unsigned int dense_idx=0;
    while(dense_idx<dense_size)
    {
        const PLAYER_INDEX_FOR_CONNECTED &oid=map_clients_id.at(dense_idx);
        if(oid!=PLAYER_INDEX_FOR_CONNECTED_MAX)
        {
            const Client &c=ClientList::list->at(oid);
            #ifdef CATCHCHALLENGER_TESTING
            assertXYInRange(c.getX(),c.getY(),"min_network_dense_build");
            #endif
            tempDenseBuffer[dense_idx].set(c.getX(),c.getY(),static_cast<uint8_t>(c.getLastDirection()),c.getPlayerId());
        }
        else
        {
            tempDenseBuffer[dense_idx].setEmpty();
        }
        dense_idx++;
    }

    unsigned int index_client=0;
    while(index_client<map_clients_id.size())
    {
        if(index_client==0)
            {const uint16_t _tmp_le=(htole16(mapIndex));memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+4+1,&_tmp_le,sizeof(_tmp_le));}//map id (pre-set for path 2 insert header)
        const PLAYER_INDEX_FOR_CONNECTED &map_c_idP=map_clients_id.at(index_client);
        if(map_c_idP!=PLAYER_INDEX_FOR_CONNECTED_MAX)
        {
            #ifdef CATCHCHALLENGER_HARDENED
            if(!ClientList::list->isNull(map_c_idP))
            #endif
            {
                ClientWithMap &clientWithMap=ClientList::list->rwWithMap(map_c_idP);
                {
                    //see /doc/algo/visibility/constant-time-player-visibility.png

                    /// PATH 1: client changed map (or first time) -> full reload
                    /// Sends: [0x65][0x6B filtered_insert][0xE3 ping?]
                    if(clientWithMap.sendedMap!=clientWithMap.mapIndex)//async multiple map change to more performance
                    {
                        clientWithMap.sendedMap=clientWithMap.mapIndex;
                        uint32_t posOutput=0;
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x65;//drop all player on map
                        posOutput+=1;//drop all
                        posOutput+=send_reinsertAllWithFilter(mapIndex,ProtocolParsingBase::tempBigBufferForOutput+posOutput,clients_size,index_client);
                        //only append ping if none pending, to avoid exhausting query numbers
                        if(clientWithMap.pingCountInProgress()<=0)
                        {
                            posOutput+=clientWithMap.sendPing(ProtocolParsingBase::tempBigBufferForOutput+posOutput);
                        }
                        clientWithMap.sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                        //populate sendedStatus from the shared dense buffer (current state).
                        //The new sent-state IS the dense snapshot; empty slots carry the
                        //canonical setEmpty() marker. memcpy is internally vectorised.
                        clientWithMap.sendedStatus.resize(dense_size);
                        if(dense_size>0)
                            memcpy(clientWithMap.sendedStatus.data(),tempDenseBuffer,dense_size*sizeof(DensePlayerState));
                    }
                    /// PATH 2: same map as last tick -> diff update
                    /// Compare sendedStatus with current state to detect inserts/changes/removes.
                    /// Buffer layout when inserts exist:
                    ///   [0x6B insert_header+data][0x69 removes?][0x66 changes?][0xE3 ping]
                    /// Buffer layout when no inserts:
                    ///   [0x69 removes?][0x66 changes?][0xE3 ping]
                    else
                    {
                        uint8_t changesCount=0;
                        uint8_t removeCount=0;
                        uint8_t insertCount=0;
                        uint32_t posOutput=0;
                        posOutput+=1+4+1+2+1;//reserve [0..8] for 0x6B header (used only if insertCount>0)

                        //compare the shared dense buffer (built once above) against this
                        //recipient's sendedStatus to detect inserts/changes/removes.
                        //Hoist both loop bounds: dense_size already = min(map_clients_id.size(),255)
                        //from above, and sendedStatus is not resized until after this loop. The
                        //compiler can't hoist them itself — the rare insert path calls
                        //playerToFullInsert(), which it can't prove leaves the vectors untouched —
                        //so without this it reloads .size() every iteration of the N*N path.
                        const size_t ss_size=clientWithMap.sendedStatus.size();
                        unsigned int index=0;
                        while(index<dense_size)
                        {
                            if(index_client==index)//skip self
                            {
                                index++;
                                continue;
                            }
                            //slot is within sendedStatus range -> can compare with last-sent state
                            if(index<ss_size)
                            {
                                const DensePlayerState &sent=clientWithMap.sendedStatus[index];
                                const DensePlayerState &dense=tempDenseBuffer[index];
                                if(dense.isEqual(sent))
                                {
                                }//no change, nothing to send: one compare covers
                                 //x+y+direction+db-id AND the empty==empty case
                                 //(setEmpty() is canonical)
                                else
                                {
                                    //slot emptied since last send -> 0x69 remove entry.
                                    //(dense!=sent here, so the recipient still has a
                                    //player in this slot; the already-removed-last-tick
                                    //case is swallowed by the equality fast path above.)
                                    if(dense.isEmpty())
                                    {
                                        MapVisibilityAlgorithm::tempBigBufferForRemove[1+4+1+removeCount]=static_cast<uint8_t>(index);
                                        removeCount++;
                                    }
                                    //slot was empty at last send, or a DIFFERENT character
                                    //moved in -> full re-insert. The empty checks MUST run
                                    //before isSameCharacter(): see DensePlayerState.hpp.
                                    else if(sent.isEmpty() || !dense.isSameCharacter(sent))
                                    {
                                        #ifdef CATCHCHALLENGER_TESTING
                                        assertXYInRange(dense.getX(),dense.getY(),"min_network_path2_replaced");
                                        #endif
                                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=index;//local slot
                                        posOutput+=1;
                                        posOutput+=playerToFullInsert(ClientList::list->at(map_clients_id.at(index)),ProtocolParsingBase::tempBigBufferForOutput+posOutput);
                                        insertCount++;
                                    }
                                    //same character, position or direction changed -> 0x66 change entry
                                    else
                                    {
                                        #ifdef CATCHCHALLENGER_TESTING
                                        assertXYInRange(dense.getX(),dense.getY(),"min_network_path2_change");
                                        #endif
                                        //only send partial changes: slot + x + y + direction (4 bytes per
                                        //entry). The 0x66 wire entry is the byte sequence
                                        //[slot][x][y][direction]; compose it in a register
                                        //(wireChangeWord) and flush with ONE 32-bit little-endian store
                                        //instead of 4 byte stores. htole32+memcpy keeps it endian-neutral.
                                        char *ce=MapVisibilityAlgorithm::tempBigBufferForChanges+(1+4+1)+changesCount*(1+1+1+1);
                                        {const uint32_t _tmp_le=(htole32(dense.wireChangeWord(static_cast<uint8_t>(index))));memcpy(ce,&_tmp_le,sizeof(_tmp_le));}
                                        changesCount++;
                                    }
                                }
                            }
                            //slot is beyond sendedStatus range -> new slot appeared (map grew)
                            else
                            {
                                const DensePlayerState &dense=tempDenseBuffer[index];
                                if(!dense.isEmpty())
                                {
                                    //new player in a slot we haven't seen before -> full insert
                                    {
                                        #ifdef CATCHCHALLENGER_TESTING
                                        const Client &nc=ClientList::list->at(map_clients_id.at(index));
                                        assertXYInRange(nc.getX(),nc.getY(),"min_network_path2_beyond");
                                        #endif
                                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=index;//local slot
                                        posOutput+=1;
                                        posOutput+=playerToFullInsert(ClientList::list->at(map_clients_id.at(index)),ProtocolParsingBase::tempBigBufferForOutput+posOutput);
                                        insertCount++;
                                    }
                                }
                            }
                            index++;
                        }

                        //flush the buffer: compose and send all detected differences
                        if(changesCount>0 || removeCount>0 || insertCount>0)
                        {
                            if(insertCount>0)
                            {
                                //fill the reserved 0x6B header at [0..8] now that we know insert data size
                                ProtocolParsingBase::tempBigBufferForOutput[0x00]=0x6B;//full Insert player on map
                                {const uint32_t _tmp_le=(htole32(posOutput-1-4));memcpy(ProtocolParsingBase::tempBigBufferForOutput+1,&_tmp_le,sizeof(_tmp_le));}//set the dynamic size (data bytes after code+size)
                                ProtocolParsingBase::tempBigBufferForOutput[1+4]=0x01;//map list count
                                {const uint16_t _tmp_le=(htole16(mapIndex));memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+4+1,&_tmp_le,sizeof(_tmp_le));}//map id
                                if(insertCount<254)
                                {
                                    ProtocolParsingBase::tempBigBufferForOutput[1+4+1+2]=static_cast<uint8_t>(insertCount);//player count
                                }
                                else
                                {
                                    ProtocolParsingBase::tempBigBufferForOutput[1+4+1+2]=static_cast<uint8_t>(254);//player count
                                }
                            }
                            else
                            {
                                posOutput=0;//no inserts: reset to 0, don't send the unused reserved 0x6B header space
                            }

                            //append 0x69 remove packet (if any) after the insert data (or at position 0 if no inserts)
                            if(removeCount>0)
                            {
                                MapVisibilityAlgorithm::tempBigBufferForRemove[1+4]=static_cast<uint8_t>(removeCount);//player count
                                {const uint32_t _tmp_le=(htole32(1+removeCount));memcpy(MapVisibilityAlgorithm::tempBigBufferForRemove+1,&_tmp_le,sizeof(_tmp_le));}//dynamic size = count_byte + indices
                                //copy the pre-built remove buffer [0x69][size4][count1][indices...] into main output
                                memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,MapVisibilityAlgorithm::tempBigBufferForRemove,1+4+1+removeCount);
                                posOutput+=1+4+1+removeCount;
                            }
                            //append 0x66 changes packet (if any) after removes
                            if(changesCount>0)
                            {
                                MapVisibilityAlgorithm::tempBigBufferForChanges[1+4]=static_cast<uint8_t>(changesCount);//player count
                                {const uint32_t _tmp_le=(htole32(1+changesCount*(1+1+1+1)));memcpy(MapVisibilityAlgorithm::tempBigBufferForChanges+1,&_tmp_le,sizeof(_tmp_le));}//dynamic size = count_byte + count * 4 bytes per entry
                                //copy the pre-built changes buffer [0x66][size4][count1][entries...] into main output
                                memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,MapVisibilityAlgorithm::tempBigBufferForChanges,1+4+1+changesCount*(1+1+1+1));
                                posOutput+=1+4+1+changesCount*(1+1+1+1);
                            }
                            //only append ping if none pending, to avoid exhausting query numbers
                            if(clientWithMap.pingCountInProgress()<=0)
                            {
                                posOutput+=clientWithMap.sendPing(ProtocolParsingBase::tempBigBufferForOutput+posOutput);
                            }
                            clientWithMap.sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                            //update sendedStatus from the shared dense buffer so next tick can
                            //detect differences. Flat memcpy: the dense snapshot IS the new
                            //sent-state (see PATH 1 note), internally vectorised by libc.
                            clientWithMap.sendedStatus.resize(dense_size);
                            if(dense_size>0)
                                memcpy(clientWithMap.sendedStatus.data(),tempDenseBuffer,dense_size*sizeof(DensePlayerState));
                        }
                    }
                }
            }
            #ifdef CATCHCHALLENGER_HARDENED
            else
                std::cerr << "MapVisibilityAlgorithm::min_network() ClientList::list.empty(): " << map_c_idP << std::endl;
            #endif
        }
        index_client++;
    }
}
