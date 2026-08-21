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
 *  - min_balanced (stateful): remember what was last broadcast and only emit
 *    the players whose data actually changed (insert / remove / move). The
 *    last-broadcast state is kept ONCE PER MAP (previousDenseBuffer): every
 *    recipient on a map is at the same state, so the diff is computed once
 *    and each recipient just gets it with its own entry cut out. A recipient
 *    that has not ACKed the previous broadcast is held back and keeps a
 *    PRIVATE baseline (ClientWithMap::sendedStatus) until it catches up, so
 *    a slow link is never handed bytes it cannot drain.
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

//tempDenseBuffer (the per-tick snapshot), previousDenseBuffer (the per-map
//last-broadcast state) and ClientWithMap::sendedStatus (a held-back
//recipient's private baseline) are all the SAME DensePlayerState type, so the
//diff loop compares a slot with one isEqual() and a state is refreshed
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
uint8_t MapVisibilityAlgorithm::tempInsertSlots[255];
std::vector<MapVisibilityAlgorithm> MapVisibilityAlgorithm::flat_map_list;
DensePlayerState MapVisibilityAlgorithm::tempDenseBuffer[255];
PLAYER_INDEX_FOR_CONNECTED MapVisibilityAlgorithm::tempInsertPlayers[255];
uint8_t MapVisibilityAlgorithm::tempSeenSlot[255];
std::vector<uint8_t> MapVisibilityAlgorithm::tempSlotOfPlayer;
uint32_t MapVisibilityAlgorithm::visibilityTick=0;
//half extents for the fallback zoom 2: 1920x1080 scaled by
//ceil(1080*2/512)=5 -> 80px by tile -> 24x14 tiles -> 24/2+1=13.
//resolveViewRange() replaces them at load with the real datapack zoom.
uint8_t MapVisibilityAlgorithm::view_x=13;
uint8_t MapVisibilityAlgorithm::view_y=13;

MapVisibilityAlgorithm::MapVisibilityAlgorithm() :
    candidatesCount(0),
    //NOT 0: visibilityTick starts at 0 too, and a map that looks already
    //snapshotted before anything was written would show nobody to anybody
    candidatesTick(0xffffffff)
{
    //Packet code bytes are constants: set once here so the hot path never
    //re-writes them. min_balanced() only fills the size+count that actually
    //vary, and copies the rest straight out.
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
    //Bound by the SLOT count, NOT the live-player count. map_clients_id is
    //sparse: removeOnMap() leaves a hole and insertOnMap() refills it LIFO,
    //so a live player can sit ABOVE a hole. Bounding by the live count
    //stopped short of him and dropped him from the full insert PERMANENTLY:
    //0x66 carries only [slot][x][y][direction], no pseudo/skin/monster, so
    //the client cannot recompose a player it never received an insert for
    //(reinsert_player_final() drops the entry outright), and the PATH2 diff
    //then sees no change and never re-sends. Holes still cost ZERO bytes --
    //they are skipped below, only live slots are emitted.
    //Slots 255+ are unreachable by the 8-bit wire slot index, and the packet
    //carries at most 254 players (count clamp below), so stop at either.
    const size_t slot_count=(map_clients_id.size()<255)?map_clients_id.size():255;
    while(index<slot_count && count<254)
    {
        const PLAYER_INDEX_FOR_CONNECTED &index_c=map_clients_id[index];
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
    //Same slot-count bound as send_reinsertAll() -- see the comment there
    //for why the live-player count silently dropped players above a hole.
    const size_t slot_count=(map_clients_id.size()<255)?map_clients_id.size():255;
    while(index<slot_count && count<254)
    {
        const PLAYER_INDEX_FOR_CONNECTED &index_c=map_clients_id[index];
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
    //255 is the SPECIAL value of the 8-bit wire slot index
    //(SIMPLIFIED_PLAYER_ID_FOR_MAP): the client uses it as its "no
    //exclusion" sentinel (Api_protocol::playerExcludeIndex starts at 255),
    //so a real player may only occupy slots 0..254 -- hence the clamp to
    //254 here and the 254 player-count cap in the insert packets.
    //map_removed_index only ever holds freed slots <=254 (the >254 ones go
    //to map_removed_index_greater_than_254), which is what makes this
    //subtraction line up with the clamp above.
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
                    #ifdef CATCHCHALLENGER_BENCHMARK
                    //Benchmark builds drop the pending-ping gate on purpose. In
                    //normal operation it is ACK-based flow control: the server
                    //waits for the client's ping reply, so throughput is bounded
                    //by the client's RTT and the client has to be fast. That is
                    //correct for real players and fatal for a measurement -- it
                    //makes the SERVER's throughput a function of the LOAD
                    //GENERATOR's speed. Ungated, mapmanagement floods and the
                    //server is driven by its own capacity instead.
                    //The ping is SKIPPED entirely rather than sent ungated:
                    //ungated pings would consume query numbers faster than the
                    //client retires them and get it kicked for "no free query
                    //number" -- exactly what the original comment warns about.
                    //Nothing needs the ping here: it exists to measure RTT for
                    //this throttle, and the protocol is otherwise async, so the
                    //cases that truly need sync are very rare.
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

/// Emit ONE delta between clientWithMap.sendedStatus (this recipient's
/// PRIVATE baseline: the last state it actually acknowledged) and the
/// current tempDenseBuffer, skipping its own slot. Same packet shapes and
/// same ordering as the shared path -- [0x6B inserts][0x69 removes]
/// [0x66 changes][0xE3 ping] -- so a client cannot tell whether it was
/// served from the shared snapshot or from its own baseline.
///
/// Used only for a recipient that fell behind and has now ACKed, so every
/// tick it missed is folded into this single packet instead of one packet
/// per tick. Reuses the shared entry buffers, hence the separate pass.
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
        if(index_client==index)//never tell a recipient about itself
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
                }//unchanged since this recipient last heard about the slot
                else
                {
                    if(dense.isEmpty())
                    {
                        MapVisibilityAlgorithm::tempBigBufferForRemove[1+4+1+removeCount]=static_cast<char>(index);
                        removeCount++;
                    }
                    //empty checks MUST precede isSameCharacter(), see DensePlayerState.hpp
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
        return;//nothing happened while it was away
    }
    uint32_t posOutput=0;
    posOutput+=1+4+1+2+1;//reserve [0..8] for the 0x6B header (used only if insertCount>0)
    if(insertCount>0)
    {
        unsigned int k=0;
        while(k<insertCount)
        {
            const uint8_t insertSlot=MapVisibilityAlgorithm::tempInsertSlots[k];
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<char>(insertSlot);//local slot
            posOutput+=1;
            posOutput+=playerToFullInsert(ClientList::list->at(map_clients_id[insertSlot]),ProtocolParsingBase::tempBigBufferForOutput+posOutput);
            k++;
        }
        ProtocolParsingBase::tempBigBufferForOutput[0x00]=0x6B;//full Insert player on map
        {const uint32_t _tmp_le=(htole32(posOutput-1-4));memcpy(ProtocolParsingBase::tempBigBufferForOutput+1,&_tmp_le,sizeof(_tmp_le));}//dynamic size
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
        posOutput=0;//no inserts: don't send the unused reserved 0x6B header space
    }
    if(removeCount>0)
    {
        char * const removeOut=ProtocolParsingBase::tempBigBufferForOutput+posOutput;
        //the 0x69 code byte comes from the constructor-seeded shared buffer
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
    //only append ping if none pending, to avoid exhausting query numbers
    #ifdef CATCHCHALLENGER_BENCHMARK
    if(false)//benchmark: skip the ping entirely (see min_CPU above)
    #else
    if(clientWithMap.pingCountInProgress()<=0)
    #endif
    {
        posOutput+=clientWithMap.sendPing(ProtocolParsingBase::tempBigBufferForOutput+posOutput);
    }
    clientWithMap.sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

/// min_balanced (stateful diff): only send the other players that actually
/// changed since the last broadcast. Uses far less network than min_CPU.
/// On each tick:
///   - Path 1 (new map): sends [0x65 drop_all][0x6B full_insert_filtered][0xE3 ping]
///   - Path 2 (same map): ONE diff of the map's previousDenseBuffer against
///     the current snapshot, reused by every recipient with its own entry
///     cut out, sending only:
///       * 0x6B for new/replaced players (inserts)
///       * 0x69 for removed players (removes)
///       * 0x66 for moved players (changes = x/y/direction diff)
///   - Path 2, held back (previous ping unanswered): sends NOTHING and keeps
///     a private baseline, then one coalesced delta once the client ACKs
///     (sendCoalescedDelta above)
/// previousDenseBuffer is refreshed once at the end of the tick.
void MapVisibilityAlgorithm::min_balanced(const CATCHCHALLENGER_TYPE_MAPID &mapIndex)
{
    //if too many player then just stop update
    //255 is the SPECIAL value of the 8-bit wire slot index, so a real
    //player only ever occupies slots 0..254 -- see the longer note on the
    //same clamp in min_CPU() above.
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

    //====== flow control: hold back a recipient that has not ACKed ======
    //
    // A client answers every 0xE3 immediately (Api_protocol_query.cpp), and
    // its own comment says the ping exists "to not flood for map
    // visibility". pingCountInProgress()>0 therefore means the previous
    // broadcast has not completed its round trip yet -- the link is slower
    // than the 150ms tick, which is the normal case on 2G / ADSL / TOR.
    //
    // Emitting another delta at that point puts bytes on a link that cannot
    // drain them, and they are stale by the time they arrive. Instead the
    // recipient keeps a PRIVATE baseline of what it last really received
    // (ClientWithMap::sendedStatus) and is sent NOTHING until it ACKs; then
    // it gets ONE delta covering everything that happened meanwhile. A
    // client lagging for K ticks costs 1 delta instead of K, and the
    // broadcast rate self-adapts to what each link can carry.
    //
    // Clients that keep up (the common case) own no private baseline at
    // all: they share the map's previousDenseBuffer, which is what makes
    // the diff below O(slots) instead of O(recipients*slots).

    //====== ONE diff for the WHOLE MAP, not one per recipient ======
    //
    // Every recipient used to diff this dense snapshot against its own
    // ClientWithMap::sendedStatus -- but all of those copies were refreshed
    // by memcpy FROM this same snapshot, so they were identical apart from
    // each recipient's OWN slot, which is exactly the slot it never
    // compares. The insert/remove/change SET is therefore the same for
    // everybody, and computing it once turns O(recipients*slots) into
    // O(slots).
    //
    // The invariant holds inductively: a recipient that received nothing
    // last tick agreed with the snapshot on every slot but its own (that is
    // precisely why it received nothing), and one that did receive
    // something was refreshed from it. A client that just arrived takes
    // PATH 1 below and does not read this diff at all.
    //
    // Entries are appended in ASCENDING slot order; the per-recipient
    // composition below relies on that.
    uint8_t changesCount=0;
    uint8_t removeCount=0;
    uint8_t insertCount=0;
    {
        const size_t previous_size=previousDenseBuffer.size();
        unsigned int index=0;
        while(index<dense_size)
        {
            const DensePlayerState &dense=tempDenseBuffer[index];
            //slot is within the previous broadcast -> can compare with it
            if(index<previous_size)
            {
                const DensePlayerState &sent=previousDenseBuffer[index];
                if(dense.isEqual(sent))
                {
                }//no change, nothing to send: one compare covers
                 //x+y+direction+db-id AND the empty==empty case
                 //(setEmpty() is canonical)
                else
                {
                    //slot emptied since the last broadcast -> 0x69 remove entry.
                    //(dense!=sent here, so the recipients still have a player in
                    //this slot; the already-removed-last-tick case is swallowed
                    //by the equality fast path above.)
                    if(dense.isEmpty())
                    {
                        MapVisibilityAlgorithm::tempBigBufferForRemove[1+4+1+removeCount]=static_cast<char>(index);
                        removeCount++;
                    }
                    //slot was empty at the last broadcast, or a DIFFERENT
                    //character moved in -> full re-insert. The empty checks MUST
                    //run before isSameCharacter(): see DensePlayerState.hpp.
                    else if(sent.isEmpty() || !dense.isSameCharacter(sent))
                    {
                        #ifdef CATCHCHALLENGER_TESTING
                        assertXYInRange(dense.getX(),dense.getY(),"min_balanced_path2_replaced");
                        #endif
                        MapVisibilityAlgorithm::tempInsertSlots[insertCount]=static_cast<uint8_t>(index);
                        insertCount++;
                    }
                    //same character, position or direction changed -> 0x66 change entry
                    else
                    {
                        #ifdef CATCHCHALLENGER_TESTING
                        assertXYInRange(dense.getX(),dense.getY(),"min_balanced_path2_change");
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
            //slot is beyond the previous broadcast -> new slot appeared (map grew)
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

    //Pre-compose the size+count of the shared 0x69 / 0x66 packets for the
    //FULL entry set, once. Recipients that have no entry of their own (the
    //common case) then copy the finished packet with a single memcpy; only
    //the ones cutting their own entry out patch these two fields.
    if(removeCount>0)
    {
        {const uint32_t _tmp_le=(htole32(1+removeCount));memcpy(MapVisibilityAlgorithm::tempBigBufferForRemove+1,&_tmp_le,sizeof(_tmp_le));}//dynamic size = count_byte + indices
        MapVisibilityAlgorithm::tempBigBufferForRemove[1+4]=static_cast<char>(removeCount);//player count
    }
    if(changesCount>0)
    {
        {const uint32_t _tmp_le=(htole32(1+changesCount*(1+1+1+1)));memcpy(MapVisibilityAlgorithm::tempBigBufferForChanges+1,&_tmp_le,sizeof(_tmp_le));}//dynamic size = count_byte + count * 4 bytes per entry
        MapVisibilityAlgorithm::tempBigBufferForChanges[1+4]=static_cast<char>(changesCount);//player count
    }

    //Cursors into the three shared entry arrays. Recipients are visited in
    //ascending slot order and the arrays were built in ascending slot order,
    //so "does the shared diff carry an entry for MY slot?" costs one
    //monotonic advance amortised over the whole map -- no per-recipient
    //search, and no lookup table to clear every tick.
    unsigned int cursorChange=0;
    unsigned int cursorRemove=0;
    unsigned int cursorInsert=0;
    //Set only when some recipient actually needs the catch-up pass below.
    //Without it every tick paid a second walk of map_clients_id with a
    //virtual rwWithMap() per live slot to discover there was nothing to do,
    //which is pure loss on a small map where the per-tick constant is the
    //whole cost.
    bool haveCatchUp=false;
    //Nothing moved anywhere on this map: every in-sync recipient is already
    //up to date, so skip its cursor walk and composition outright. This is
    //the usual state of a real server's maps (most are quiet most ticks),
    //and it is also what keeps the per-tick constant small on a map with
    //only two or three players, where there is no quadratic term to win
    //back. PATH 1 clients still have to be served, so the recipient loop
    //itself cannot be skipped.
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
                        #ifdef CATCHCHALLENGER_BENCHMARK
                        if(false)//benchmark: skip the ping entirely (see min_CPU above)
                        #else
                        if(clientWithMap.pingCountInProgress()<=0)
                        #endif
                        {
                            posOutput+=clientWithMap.sendPing(ProtocolParsingBase::tempBigBufferForOutput+posOutput);
                        }
                        clientWithMap.sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                        //This client has just been sent the whole map, so it is AT
                        //the dense snapshot -- exactly what previousDenseBuffer is
                        //set to at the end of this tick. Drop any private baseline:
                        //it rejoins the shared snapshot.
                        clientWithMap.sendedStatus.clear();
                    }
                    /// PATH 2: same map as last tick -> emit the shared diff with THIS
                    /// recipient's own entry cut out, so the bytes on the wire are
                    /// exactly what the old per-recipient diff produced.
                    /// Buffer layout when inserts exist:
                    ///   [0x6B insert_header+data][0x69 removes?][0x66 changes?][0xE3 ping]
                    /// Buffer layout when no inserts:
                    ///   [0x69 removes?][0x66 changes?][0xE3 ping]
                    /// Recipient has not ACKed the previous broadcast: hold. Keep
                    /// (or take) a private baseline of what it last really got and
                    /// send nothing, so a slow link is not handed bytes it cannot
                    /// drain. It is served in the catch-up pass once it ACKs.
                    else if(clientWithMap.pingCountInProgress()>0)
                    {
                        if(clientWithMap.sendedStatus.empty())
                        {
                            //First tick held back. previousDenseBuffer still holds
                            //LAST tick's state here (it is refreshed at the end of
                            //this function), which is precisely what this client
                            //received before it went quiet.
                            const size_t baseline_size=previousDenseBuffer.size();
                            if(baseline_size>0)
                            {
                                clientWithMap.sendedStatus.resize(baseline_size);
                                memcpy(clientWithMap.sendedStatus.data(),previousDenseBuffer.data(),
                                       baseline_size*sizeof(DensePlayerState));
                            }
                        }
                    }
                    /// Recipient owns a private baseline but has ACKed: it is served
                    /// by the catch-up pass after this loop, because composing its
                    /// delta reuses the shared entry buffers that the loop is still
                    /// reading.
                    else if(!clientWithMap.sendedStatus.empty())
                    {
                        haveCatchUp=true;
                    }
                    else if(!sharedEmpty)
                    {
                        //advance the cursors to this recipient's slot
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
                        //A recipient is never told about itself, so its own entry is
                        //dropped here. Each recipient owns exactly ONE slot, so an
                        //effective count is at most 254 and the clamp below can never
                        //disagree with the number of entries actually written.
                        const uint8_t changesEff=selfChange?static_cast<uint8_t>(changesCount-1):changesCount;
                        const uint8_t removeEff=selfRemove?static_cast<uint8_t>(removeCount-1):removeCount;
                        const uint8_t insertEff=selfInsert?static_cast<uint8_t>(insertCount-1):insertCount;

                        if(changesEff>0 || removeEff>0 || insertEff>0)
                        {
                            uint32_t posOutput=0;
                            posOutput+=1+4+1+2+1;//reserve [0..8] for 0x6B header (used only if insertEff>0)
                            if(insertEff>0)
                            {
                                //Only the SLOT list is shared: an insert entry is
                                //variable length, so the bytes are still written per
                                //recipient exactly as before -- which skips this
                                //recipient's own slot for free.
                                unsigned int k=0;
                                while(k<insertCount)
                                {
                                    const uint8_t insertSlot=MapVisibilityAlgorithm::tempInsertSlots[k];
                                    if(insertSlot!=index_client)
                                    {
                                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<char>(insertSlot);//local slot
                                        posOutput+=1;
                                        posOutput+=playerToFullInsert(ClientList::list->at(map_clients_id[insertSlot]),ProtocolParsingBase::tempBigBufferForOutput+posOutput);
                                    }
                                    k++;
                                }
                                //fill the reserved 0x6B header at [0..8] now that we know insert data size
                                ProtocolParsingBase::tempBigBufferForOutput[0x00]=0x6B;//full Insert player on map
                                {const uint32_t _tmp_le=(htole32(posOutput-1-4));memcpy(ProtocolParsingBase::tempBigBufferForOutput+1,&_tmp_le,sizeof(_tmp_le));}//set the dynamic size (data bytes after code+size)
                                ProtocolParsingBase::tempBigBufferForOutput[1+4]=0x01;//map list count
                                {const uint16_t _tmp_le=(htole16(mapIndex));memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+4+1,&_tmp_le,sizeof(_tmp_le));}//map id
                                if(insertEff<254)
                                {
                                    ProtocolParsingBase::tempBigBufferForOutput[1+4+1+2]=static_cast<uint8_t>(insertEff);//player count
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
                            //Fill the shared packet's size+count for the FULL entry
                            //set once; the 0x69 code byte was set in the constructor
                            //and is never rewritten.
                            if(removeEff>0)
                            {
                                char * const removeOut=ProtocolParsingBase::tempBigBufferForOutput+posOutput;
                                if(selfRemove)
                                {
                                    //this recipient's own entry is cut out, so its
                                    //count and size differ: copy the shared header
                                    //(carrying the constant 0x69) then patch the two
                                    //fields that vary, then the entries either side.
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
                                    //nothing to cut: one memcpy of the whole
                                    //pre-composed packet, exactly as before
                                    memcpy(removeOut,MapVisibilityAlgorithm::tempBigBufferForRemove,1+4+1+removeCount);
                                }
                                posOutput+=1+4+1+removeEff;
                            }
                            //append 0x66 changes packet (if any) after removes
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
                            //only append ping if none pending, to avoid exhausting query numbers
                            #ifdef CATCHCHALLENGER_BENCHMARK
                            if(false)//benchmark: skip the ping entirely (see min_CPU above)
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

    //====== catch-up pass ======
    //Recipients that were held back and have now ACKed get ONE delta from
    //their private baseline, covering every tick they missed. Done in a
    //second pass because composing it reuses tempBigBufferForChanges /
    //tempBigBufferForRemove, which the loop above is still reading. Normally
    //this walk is skipped entirely (haveCatchUp false).
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
                //benchmark: the pending-ping term is the ACK-based throttle
                //itself -- it withholds the delta until the client replies. Drop
                //only that term; the sendedStatus/sendedMap conditions are
                //correctness, not flow control, and stay.
                if(!clientWithMap.sendedStatus.empty()
                   && clientWithMap.sendedMap==clientWithMap.mapIndex)
                #else
                if(!clientWithMap.sendedStatus.empty() && clientWithMap.pingCountInProgress()<=0
                   && clientWithMap.sendedMap==clientWithMap.mapIndex)
                #endif
                {
                    sendCoalescedDelta(clientWithMap,mapIndex,index_client,dense_size);
                    //back in step with everybody else: drop the private baseline
                    //and rejoin the shared snapshot
                    clientWithMap.sendedStatus.clear();
                }
            }
        }
        index_client++;
    }

    //Refresh the map's broadcast state ONCE for the whole tick -- this used
    //to be one memcpy of the same bytes per recipient. Flat memcpy: the
    //dense snapshot IS the new state (see PATH 1 note), internally
    //vectorised by libc. resize() reaches the map's slot high-water mark
    //and then stops allocating, because map_clients_id never shrinks.
    //resize() only when the slot count actually grew: map_clients_id never
    //shrinks, so after the first ticks this is a no-op call we can skip.
    if(previousDenseBuffer.size()!=dense_size)
    {
        previousDenseBuffer.resize(dense_size);
    }
    if(dense_size>0)
    {
        memcpy(previousDenseBuffer.data(),tempDenseBuffer,dense_size*sizeof(DensePlayerState));
    }
}

/// Offset to ADD to a coordinate of the map on `side` of `map` to express it
/// in `map` OWN frame. Mirrors the crossing formulas of MoveOnTheMap.hpp,
/// with the offsets already resolved by Map_loader (border.left.y_offset is
/// the delta to ADD when crossing to the left map, and the left map carries
/// the opposite one):
///   top:    (x,0)            -> other(x+map.border.top.x_offset,other.height-1)
///   bottom: (x,map.height-1) -> other(x+map.border.bottom.x_offset,0)
///   left:   (0,y)            -> other(other.width-1,y+map.border.left.y_offset)
///   right:  (map.width-1,y)  -> other(0,y+map.border.right.y_offset)
/// so going back is minus that offset on the crossed axis, plus/minus a map
/// size on the other one.
/// side: 0 top, 1 bottom, 2 left, 3 right. false when there is no map there.
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

/// Add otherIndex to the neighbour list of map, when not already there and
/// when its rect TOUCHES the map rect -- the same rule as the client
/// MapVisualiser::rectTouch(), which decides the maps it loads and displays
/// around the current one. A map that does not touch would never be loaded
/// client side and its inserts would pile up in the client delayedActions.
static void addNeighbour(MapVisibilityAlgorithm &map,const CATCHCHALLENGER_TYPE_MAPID &selfIndex,
                         const CATCHCHALLENGER_TYPE_MAPID &otherIndex,const int16_t &offset_x,const int16_t &offset_y)
{
    if(otherIndex==selfIndex)
        return;
    unsigned int index=0;
    while(index<map.neighbours.size())
    {
        if(map.neighbours.at(index).mapIndex==otherIndex)
            return;//already reached by a shorter path, keep the first one
        index++;
    }
    const MapVisibilityAlgorithm &other=MapVisibilityAlgorithm::flat_map_list.at(otherIndex);
    //touch, edges included: [offset,offset+size] against [0,size]
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

/// Precompute, for every map, the border maps a player standing on it can
/// see into (min_network() candidates). Done ONCE at load: the map list and the
/// border offsets never change afterwards, so the tick does no adjacency
/// work at all -- and nothing is done when a player moves either.
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
                /* Second hop: this is how the DIAGONAL maps are reached, a
                 * border only links the 4 sides. Two maps away in the SAME
                 * direction no more touches this rect, addNeighbour() drops
                 * it, so the list stays the client displayed set. */
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

/// Half extents of the view rectangle, from the client window and the DATAPACK
/// zoom -- see the long comment in MapVisibilityAlgorithm.hpp. Same factor
/// rule as MapControllerMP::scaleFactor(), the one both clients draw with:
/// ceil(min(w,h)*zoom/512). The max of the two axes covers a portrait window,
/// +1 tile of margin.
void MapVisibilityAlgorithm::resolveViewRange(const uint8_t &datapackZoom)
{
    uint32_t zoom=datapackZoom;
    if(zoom<1)
        zoom=CATCHCHALLENGER_SERVER_MAP_VIEW_ZOOM_DEFAULT;//datapack without map/layers.xml
    uint32_t screenMin=CATCHCHALLENGER_SERVER_MAP_VIEW_SCREEN_WIDTH;
    if(CATCHCHALLENGER_SERVER_MAP_VIEW_SCREEN_HEIGHT<screenMin)
        screenMin=CATCHCHALLENGER_SERVER_MAP_VIEW_SCREEN_HEIGHT;
    //ceil(screenMin*zoom/512), never below 1
    uint32_t factor=(screenMin*zoom+511)/512;
    if(factor<1)
        factor=1;
    const uint32_t tileScreen=CATCHCHALLENGER_SERVER_MAP_VIEW_TILE_PIXEL*factor;
    //tile count rounded UP: a half drawn tile is still a drawn tile
    uint32_t tiles=(CATCHCHALLENGER_SERVER_MAP_VIEW_SCREEN_WIDTH+tileScreen-1)/tileScreen;
    const uint32_t tilesHeight=(CATCHCHALLENGER_SERVER_MAP_VIEW_SCREEN_HEIGHT+tileScreen-1)/tileScreen;
    if(tilesHeight>tiles)
        tiles=tilesHeight;
    uint32_t resolvedX=tiles/2+1;
    uint32_t resolvedY=tiles/2+1;
    //a map is never bigger than 127x127, a wider view can not see more
    if(resolvedX>127)
        resolvedX=127;
    if(resolvedY>127)
        resolvedY=127;
    view_x=static_cast<uint8_t>(resolvedX);
    view_y=static_cast<uint8_t>(resolvedY);
}

/// Fill the 0x6B header of a group started at groupStart, once its player
/// entries are written. One group by SOURCE map: the packet carries a single
/// map id for its whole player list.
/// Layout: [0x6B][size:4][map count:1][map id:2][player count:1][entries...]
static void closeInsertGroup(char * const buffer,const uint32_t &groupStart,const uint32_t &posOutput,
                             const CATCHCHALLENGER_TYPE_MAPID &mapId,const uint8_t &count)
{
    buffer[groupStart]=0x6B;//full Insert player on map
    {const uint32_t _tmp_le=(htole32(posOutput-groupStart-1-4));memcpy(buffer+groupStart+1,&_tmp_le,sizeof(_tmp_le));}
    buffer[groupStart+1+4]=0x01;//map list count
    {const uint16_t _tmp_le=(htole16(mapId));memcpy(buffer+groupStart+1+4+1,&_tmp_le,sizeof(_tmp_le));}
    buffer[groupStart+1+4+1+2]=static_cast<char>(count);
}

/// Same rectangle as min_network(), asked about ONE pair instead of broadcast:
/// used by /trade and /battle (Client::otherPlayerIsInRange) so a player can
/// only interact with somebody it actually sees, border maps included.
bool MapVisibilityAlgorithm::inViewRange(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const COORD_TYPE &x,const COORD_TYPE &y,
                                         const CATCHCHALLENGER_TYPE_MAPID &otherMapIndex,const COORD_TYPE &otherX,const COORD_TYPE &otherY)
{
    if(mapIndex>=flat_map_list.size() || otherMapIndex>=flat_map_list.size())
        return false;
    int16_t offset_x=0;
    int16_t offset_y=0;
    if(otherMapIndex!=mapIndex)
    {
        //the other map has to be one this player can see into at all
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

/// One server tick starts: every map snapshot taken during the previous one
/// is stale. Called by the timer that drives the broadcast, NOT by the
/// algorithm, because a map's snapshot is read by its neighbours before its
/// own turn comes.
void MapVisibilityAlgorithm::beginTick()
{
    visibilityTick++;
}

/// Rebuild this map's candidate snapshot, once per tick whoever asks first.
/// Nothing inserts or removes on a map during the broadcast, so a snapshot
/// taken at the first read stays valid for the whole tick.
void MapVisibilityAlgorithm::refreshCandidates()
{
    if(candidatesTick==visibilityTick)
        return;
    candidatesTick=visibilityTick;
    const size_t slotCount=map_clients_id.size();
    /* COMPACTED: only the live slots land here. map_clients_id is sparse
     * (removeOnMap leaves a hole, insertOnMap refills it LIFO) and the wire
     * slot a recipient uses comes from its OWN table, never from this one, so
     * a hole carries nothing -- dropping it takes the "is this slot empty"
     * test out of the innermost loop and shortens the walk by the hole count.
     * capacity() keeps the map's high-water mark, so this allocates once. */
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

/// min_network (view range visibility): a recipient is told ONLY about the
/// players inside its view rectangle, on its own map OR on a border map --
/// see doc/algo/visibility/MapVisibilityAlgorithm-WithBorderAndRange.png.
///
/// Nothing is done when a player moves: the tick reads x/y and the border
/// map list is precomputed at load (resolveNeighbours()), so a move is still
/// only the two coordinates written by MapBasicMove.
///
/// The visible set differs from one recipient to the next, so unlike
/// min_balanced() there is no shared snapshot/diff: each recipient owns its
/// slot table (ClientWithMap::visibleSlots), which is BOTH the wire slot
/// allocation and the diff baseline. Cost by recipient is
/// O(candidates + visible slots), with the candidates limited to the maps
/// whose rect intersects the view.
///
/// That per-recipient cost is the DEAL, not a defect: this strategy exists to
/// minimise the INTERNET usage (and to keep the view nice across the map
/// seams). A server that wants its cpu back runs "balanced" or "cpu", which
/// share one snapshot for the whole map -- and send the whole map.
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
                /* Same ACK flow control as min_balanced(): a recipient that has
                 * not answered the previous 0xE3 is handed NOTHING, its link
                 * can not drain it. It costs no extra code here: visibleSlots
                 * IS the baseline and does not move while it is held back, so
                 * the tick it answers it gets ONE delta covering everything
                 * that happened meanwhile. */
                #ifdef CATCHCHALLENGER_BENCHMARK
                //benchmark: drop the throttle, see min_CPU() above
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

/// The min_network() work for ONE recipient: rebuild what it must see, diff it
/// against what it already displays, emit the delta.
/// Buffer layout: [0x65 drop all?][0x69 removes][0x6B inserts by map][0x66 changes][0xE3 ping]
/// The removes come FIRST so a slot freed this tick can be reused by an
/// insert of the same tick without the client applying them in the wrong
/// order.
void MapVisibilityAlgorithm::sendViewDelta(ClientWithMap &recipient,const PLAYER_INDEX_FOR_CONNECTED &recipientIndex,
                                           const CATCHCHALLENGER_TYPE_MAPID &mapIndex)
{
    uint32_t posOutput=0;
    /* PATH 1: first tick on this map, or a teleport. The client keeps its
     * other players in a list indexed by slot while it drops+reloads the maps
     * around it, so the whole view has to be dropped and rebuilt -- same as
     * min_balanced() PATH 1. A simple border crossing pays it too: telling
     * both apart would need the teleport path to say so, which is more
     * existing code changed than what it saves. */
    if(recipient.sendedMap!=mapIndex)
    {
        recipient.sendedMap=mapIndex;
        recipient.visibleSlots.clear();
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x65;//drop all player on map
        posOutput+=1;
    }
    std::vector<ClientWithMap::VisibleSlot> &slots=recipient.visibleSlots;

    //====== 1) index what this recipient displays right now ======
    //slot lookup by connected player in O(1), and the seen flags reset in the
    //same walk so no memset by recipient.
    uint8_t liveCount=0;
    unsigned int slotIndex=0;
    /* Hoist every vector size out of the walks below. The candidate loop calls
     * ClientList::at(), which is virtual, so the compiler must assume it can
     * touch these vectors and reloads _M_finish-_M_start on EVERY iteration:
     * callgrind put 19% of the whole algorithm on that single subtraction. */
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

    //====== 2) walk the candidates: this map, then its border maps ======
    //mapVisibility Max caps what ONE RECIPIENT sees here, not what the map
    //holds: 254 is the last slot expressible on the 8 bits wire index.
    uint8_t visibleMax=254;
    if(GlobalServerData::serverSettings.mapVisibility.simple.max<254)
        visibleMax=static_cast<uint8_t>(GlobalServerData::serverSettings.mapVisibility.simple.max);
    const int16_t recipientX=static_cast<int16_t>(recipient.getX());
    const int16_t recipientY=static_cast<int16_t>(recipient.getY());
    const int16_t viewX=static_cast<int16_t>(view_x);
    const int16_t viewY=static_cast<int16_t>(view_y);
    //keep range = view + the CONFIGURED hysteresis (mapVisibility/ViewMargin):
    //somebody walking on the view edge then costs a 4 bytes move instead of a
    //full insert + a remove every tick
    const int16_t margin=static_cast<int16_t>(GlobalServerData::serverSettings.mapVisibility.viewMargin);
    const int16_t keepX=static_cast<int16_t>(viewX+margin);
    const int16_t keepY=static_cast<int16_t>(viewY+margin);
    uint8_t changesCount=0;
    uint8_t removeCount=0;
    uint8_t insertCount=0;
    //the sparse index is grown in phase 1 and only READ from here on
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
        //whole source map out of the keep rectangle: skip its client list
        //without reading a single Client. A player standing away from every
        //border of its map skips ALL its border maps with 4 compares each.
        if((offset_x+static_cast<int16_t>(sourceMap->width)-1)>=(recipientX-keepX) &&
           (recipientX+keepX)>=offset_x &&
           (offset_y+static_cast<int16_t>(sourceMap->height)-1)>=(recipientY-keepY) &&
           (recipientY+keepY)>=offset_y)
        {
            //ONE snapshot per map per tick, then a linear walk of it: the
            //Client objects are only touched for somebody entering the view
            sourceMap->refreshCandidates();
            unsigned int sourceSlot=0;
            const unsigned int sourceSlotCount=sourceMap->candidatesCount;
            const CandidateState * const sourceCandidates=sourceMap->candidates.data();
            while(sourceSlot<sourceSlotCount)
            {
                const CandidateState &entry=sourceCandidates[sourceSlot];
                const PLAYER_INDEX_FOR_CONNECTED candidateIndex=entry.player;
                //a recipient is never told about itself
                if(candidateIndex!=PLAYER_INDEX_FOR_CONNECTED_MAX && candidateIndex!=recipientIndex)
                {
                        //integer only: the candidate coordinates translated into
                        //this map frame, then |dx| and |dy| against the range
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
                            //already displayed: hysteresis, only dropped past the margin
                            if(dx<=keepX && dy<=keepY)
                            {
                                const uint8_t slot=static_cast<uint8_t>(known-1);
                                tempSeenSlot[slot]=0x01;
                                ClientWithMap::VisibleSlot &visibleSlot=slots[slot];//bounded by phase 1
                                const DensePlayerState &current=entry.state;
                                if(visibleSlot.map!=sourceMapIndex || !current.isSameCharacter(visibleSlot.state))
                                {
                                    //changed map (0x66 carries no map id), or the connected
                                    //slot was recycled by another character: full insert
                                    //again on the SAME wire slot, the client drops its
                                    //previous entry for that slot by itself
                                    tempInsertPlayers[insertCount]=candidateIndex;
                                    tempInsertSlots[insertCount]=slot;
                                    insertCount++;
                                    visibleSlot.map=sourceMapIndex;
                                    visibleSlot.state=current;
                                }
                                else if(!current.isEqual(visibleSlot.state))
                                {
                                    //moved: slot + x + y + direction, composed in a
                                    //register and flushed with one 32 bits store
                                    char * const changeEntry=MapVisibilityAlgorithm::tempBigBufferForChanges+(1+4+1)+changesCount*(1+1+1+1);
                                    {const uint32_t _tmp_le=(htole32(current.wireChangeWord(slot)));memcpy(changeEntry,&_tmp_le,sizeof(_tmp_le));}
                                    changesCount++;
                                    visibleSlot.state=current;
                                }
                            }
                        }
                        else if(dx<=viewX && dy<=viewY)
                        {
                            //entered the view: the wire slot is allocated after the
                            //remove pass, so a slot freed this tick is reused
                            if(liveCount<visibleMax && insertCount<254)
                            {
                                tempInsertPlayers[insertCount]=candidateIndex;
                                tempInsertSlots[insertCount]=0xff;//allocated below
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

    //====== 3) what is no more visible, and drop the sparse index ======
    //one walk does both: the index MUST be clean when leaving, it is shared
    //by every recipient of every map.
    slotIndex=0;
    while(slotIndex<slotCount)//phase 4 grows slots, this walk does not
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
        return;//nothing changed in this player view

    //====== 4) allocate the wire slot of the players that just appeared ======
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
                //no hole left: grow. Bounded by visibleMax (<=254), and the
                //vector reaches this player high-water mark then stops
                //allocating, like map_clients_id does by map.
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

    //====== 5) compose the block ======
    if(removeCount>0)
    {
        char * const removeOut=ProtocolParsingBase::tempBigBufferForOutput+posOutput;
        //the 0x69 code byte comes from the constructor-seeded shared buffer
        memcpy(removeOut,MapVisibilityAlgorithm::tempBigBufferForRemove,1+4+1);
        {const uint32_t _tmp_le=(htole32(1+removeCount));memcpy(removeOut+1,&_tmp_le,sizeof(_tmp_le));}
        removeOut[1+4]=static_cast<char>(removeCount);
        memcpy(removeOut+(1+4+1),MapVisibilityAlgorithm::tempBigBufferForRemove+(1+4+1),removeCount);
        posOutput+=1+4+1+removeCount;
    }
    insertIndex=0;
    while(insertIndex<insertCount)
    {
        /* One 0x6B by source map. The candidate walk goes map after map so
         * the entries are ALREADY grouped: close the group as soon as the map
         * changes. */
        const CATCHCHALLENGER_TYPE_MAPID groupMap=
                ClientList::list->at(MapVisibilityAlgorithm::tempInsertPlayers[insertIndex]).mapIndex;
        const uint32_t groupStart=posOutput;
        posOutput+=1+4+1+2+1;//header filled by closeInsertGroup() below
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
        //the 0x66 code byte comes from the constructor-seeded shared buffer
        memcpy(changeOut,MapVisibilityAlgorithm::tempBigBufferForChanges,1+4+1);
        {const uint32_t _tmp_le=(htole32(1+changesCount*(1+1+1+1)));memcpy(changeOut+1,&_tmp_le,sizeof(_tmp_le));}
        changeOut[1+4]=static_cast<char>(changesCount);
        memcpy(changeOut+(1+4+1),MapVisibilityAlgorithm::tempBigBufferForChanges+(1+4+1),changesCount*(1+1+1+1));
        posOutput+=1+4+1+changesCount*(1+1+1+1);
    }
    //only append ping if none pending, to avoid exhausting query numbers
    #ifdef CATCHCHALLENGER_BENCHMARK
    if(false)//benchmark: skip the ping entirely (see min_CPU above)
    #else
    if(recipient.pingCountInProgress()<=0)
    #endif
    {
        posOutput+=recipient.sendPing(ProtocolParsingBase::tempBigBufferForOutput+posOutput);
    }
    recipient.sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}
