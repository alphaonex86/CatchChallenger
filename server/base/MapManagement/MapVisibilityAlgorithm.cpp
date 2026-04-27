#include "MapVisibilityAlgorithm.hpp"
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

//to prevent allocate memory
char MapVisibilityAlgorithm::tempBigBufferForChanges[];
char MapVisibilityAlgorithm::tempBigBufferForRemove[];
std::vector<MapVisibilityAlgorithm> MapVisibilityAlgorithm::flat_map_list;

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
        return 0;
    uint32_t posOutput=0;
    output[posOutput]=0x6B;
    posOutput+=1+4;//skip code(1) + size(4), size filled at end
    //////////////////////////// insert //////////////////////////
    // can be only this map with this algo, then 1 map
    output[posOutput]=0x01;//map list count
    posOutput+=1;
    *reinterpret_cast<uint16_t *>(output+posOutput)=htole16(mapIndex);//map id
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
            posOutput+=playerToFullInsert(c,output+posOutput);
            count++;
        }
        index++;
    }
    *reinterpret_cast<uint32_t *>(output+1)=htole32(posOutput-1-4);//set the dynamic size (data bytes after code+size)
    if(count<254)
        output[1+4+1+2]=static_cast<uint8_t>(count);//player count
    else
        output[1+4+1+2]=static_cast<uint8_t>(254);//player count
    std::cerr << "send_reinsertAll() map=" << mapIndex << " players=" << count << " bytes=" << posOutput << std::endl;
    return posOutput;
}

/// Same as send_reinsertAll but skips the player at local slot skipped_id (the recipient).
unsigned int MapVisibilityAlgorithm::send_reinsertAllWithFilter(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,char *output,const size_t &clients_size,const size_t &skipped_id)
{
    if(clients_size<=1)
        return 0;
    if(skipped_id>=255)
        return send_reinsertAll(mapIndex,output,clients_size);
    uint32_t posOutput=0;
    output[posOutput]=0x6B;
    posOutput+=1+4;
    //////////////////////////// insert //////////////////////////
    // can be only this map with this algo, then 1 map
    output[posOutput]=0x01;//map list count
    posOutput+=1;
    *reinterpret_cast<uint16_t *>(output+posOutput)=htole16(mapIndex);//map id
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
            posOutput+=playerToFullInsert(c,output+posOutput);
            count++;
        }
        index++;
    }
    *reinterpret_cast<uint32_t *>(output+1)=htole32(posOutput-1-4);//set the dynamic size
    if(count<254)
        output[1+4+1+2]=static_cast<uint8_t>(count);//player count
    else
        output[1+4+1+2]=static_cast<uint8_t>(254);//player count
    std::cerr << "send_reinsertAllWithFilter() map=" << mapIndex << " players=" << count << " skipped_slot=" << skipped_id << " bytes=" << posOutput << std::endl;
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
        clients_size=254;
    clients_size-=map_removed_index.size();
    if(clients_size>=GlobalServerData::serverSettings.mapVisibility.simple.max)
    {
        std::cerr << "min_CPU() map=" << mapIndex << " SKIP clients_size=" << clients_size << " >= max=" << GlobalServerData::serverSettings.mapVisibility.simple.max << std::endl;
        return;
    }
    if(clients_size<=1)
        return;

    std::cerr << "min_CPU() map=" << mapIndex << " clients_size=" << clients_size << " map_clients_id.size()=" << map_clients_id.size() << std::endl;

    unsigned int index_client=0;
    while(index_client<map_clients_id.size())
    {
        const PLAYER_INDEX_FOR_CONNECTED &map_c_idP=map_clients_id.at(index_client);
        if(map_c_idP!=PLAYER_INDEX_FOR_CONNECTED_MAX)
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(!ClientList::list->isNull(map_c_idP))
            #endif
            {
                Client &client=ClientList::list->rw(map_c_idP);
                ClientWithMap &clientWithMap=ClientList::list->rwWithMap(map_c_idP);
                {
                    //first time on this map: prepend 0x6C header with self slot index
                    if(clientWithMap.sendedMap!=client.mapIndex)//async multiple map change to more performance
                    {
                        std::cerr << "min_CPU() slot=" << index_client << " globalId=" << map_c_idP << " player=" << client.getPseudo() << " NEW_MAP sendedMap=" << clientWithMap.sendedMap << " mapIndex=" << client.mapIndex << " -> send 0x6C+0x65+0x6B" << std::endl;
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
                        std::cerr << "min_CPU() slot=" << index_client << " globalId=" << map_c_idP << " player=" << client.getPseudo() << " SAME_MAP -> send 0x65+0x6B" << std::endl;
                        //same map as last tick: skip the 0x6C header (positions [0..1])
                        posOutput=2;
                        baseOutput=2;
                    }
                    //build the 0x65+0x6B block once, reuse for all clients on this map
                    if(cached==false)
                    {
                        cached=true;
                        std::cerr << "min_CPU() slot=" << index_client << " building cached 0x65+0x6B block" << std::endl;

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
                        std::cerr << "min_CPU() slot=" << index_client << " player=" << client.getPseudo() << " +ping" << std::endl;
                    }
                    else
                        std::cerr << "min_CPU() slot=" << index_client << " player=" << client.getPseudo() << " skip_ping pingInProgress=" << std::to_string(client.pingCountInProgress()) << std::endl;
                    std::cerr << "min_CPU() slot=" << index_client << " player=" << client.getPseudo() << " SEND bytes=" << (posOutput-baseOutput) << std::endl;
                    client.sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput+baseOutput,posOutput-baseOutput);
                }
            }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
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
        clients_size=254;
    clients_size-=map_removed_index.size();
    if(clients_size>=GlobalServerData::serverSettings.mapVisibility.simple.max)
    {
        std::cerr << "min_network() map=" << mapIndex << " SKIP clients_size=" << clients_size << " >= max=" << GlobalServerData::serverSettings.mapVisibility.simple.max << std::endl;
        return;
    }
    //Same guard as min_CPU(): nothing to broadcast with 0 or 1 client on the
    //map, and send_reinsertAllWithFilter() would return 0 leaving the buffer
    //half-composed.
    if(clients_size<=1)
        return;

    unsigned int index_client=0;
    while(index_client<map_clients_id.size())
    {
        if(index_client==0)
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+4+1)=htole16(mapIndex);//map id (pre-set for path 2 insert header)
        const PLAYER_INDEX_FOR_CONNECTED &map_c_idP=map_clients_id.at(index_client);
        if(map_c_idP!=PLAYER_INDEX_FOR_CONNECTED_MAX)
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
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
                        std::cerr << "min_network() PATH1 slot=" << index_client << " globalId=" << map_c_idP << " player=" << clientWithMap.getPseudo()
                                  << " NEW_MAP sendedMap=" << clientWithMap.sendedMap << " mapIndex=" << clientWithMap.mapIndex << " -> send 0x65+0x6B" << std::endl;
                        clientWithMap.sendedMap=clientWithMap.mapIndex;
                        uint32_t posOutput=0;
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x65;//drop all player on map
                        posOutput+=1;//drop all
                        posOutput+=send_reinsertAllWithFilter(mapIndex,ProtocolParsingBase::tempBigBufferForOutput+posOutput,clients_size,index_client);
                        //only append ping if none pending, to avoid exhausting query numbers
                        if(clientWithMap.pingCountInProgress()<=0)
                        {
                            posOutput+=clientWithMap.sendPing(ProtocolParsingBase::tempBigBufferForOutput+posOutput);
                            std::cerr << "min_network() PATH1 slot=" << index_client << " player=" << clientWithMap.getPseudo() << " +ping" << std::endl;
                        }
                        else
                            std::cerr << "min_network() PATH1 slot=" << index_client << " player=" << clientWithMap.getPseudo() << " skip_ping pingInProgress=" << std::to_string(clientWithMap.pingCountInProgress()) << std::endl;
                        std::cerr << "min_network() PATH1 slot=" << index_client << " player=" << clientWithMap.getPseudo() << " SEND bytes=" << posOutput << std::endl;
                        clientWithMap.sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                        //populate sendedStatus with current state of all visible players
                        clientWithMap.sendedStatus.resize(std::min(map_clients_id.size(),static_cast<size_t>(255)));
                        for(unsigned int ss=0;ss<clientWithMap.sendedStatus.size();ss++)
                        {
                            const PLAYER_INDEX_FOR_CONNECTED &oid=map_clients_id.at(ss);
                            if(oid!=PLAYER_INDEX_FOR_CONNECTED_MAX)
                            {
                                const Client &c=ClientList::list->at(oid);
                                clientWithMap.sendedStatus[ss].characterId_db=c.getPlayerId();
                                clientWithMap.sendedStatus[ss].x=c.getX();
                                clientWithMap.sendedStatus[ss].y=c.getY();
                                clientWithMap.sendedStatus[ss].direction=c.getLastDirection();
                            }
                            else
                                clientWithMap.sendedStatus[ss].characterId_db=0xffffffff;
                        }
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

                        //scan all slots, compare with sendedStatus to detect differences
                        unsigned int index=0;
                        while(index<map_clients_id.size() && index<255)
                        {
                            if(index_client==index)//skip self
                            {
                                index++;
                                continue;
                            }
                            //slot is within sendedStatus range -> can compare with last-sent state
                            if(index<clientWithMap.sendedStatus.size())
                            {
                                ClientWithMap::SendedStatus &c_stat=clientWithMap.sendedStatus[index];
                                const PLAYER_INDEX_FOR_CONNECTED &other_client_id=map_clients_id.at(index);
                                if(other_client_id!=PLAYER_INDEX_FOR_CONNECTED_MAX)
                                {
                                    const Client &other_client=ClientList::list->at(other_client_id);
                                    //same character in this slot as last tick
                                    if(other_client.getPlayerId()==c_stat.characterId_db)
                                    {
                                        if(c_stat.direction==other_client.getLastDirection() && c_stat.x==other_client.getX() && c_stat.y==other_client.getY())
                                        {}//no change, nothing to send
                                        else
                                        {
                                            std::cerr << "min_network() PATH2 slot=" << index_client << " scan other_slot=" << index << " other=" << other_client.getPseudo()
                                                      << " CHANGE old_x=" << std::to_string(c_stat.x) << " old_y=" << std::to_string(c_stat.y) << " old_dir=" << std::to_string(c_stat.direction)
                                                      << " new_x=" << std::to_string(other_client.getX()) << " new_y=" << std::to_string(other_client.getY()) << " new_dir=" << std::to_string(other_client.getLastDirection()) << std::endl;
                                            //position or direction changed -> send 0x66 change entry
                                            {
                                                //only send partial changes: slot + x + y + direction (4 bytes per entry)
                                                MapVisibilityAlgorithm::tempBigBufferForChanges[1+4+1+changesCount*(1+1+1+1)]=static_cast<uint8_t>(index);
                                                MapVisibilityAlgorithm::tempBigBufferForChanges[1+4+1+changesCount*(1+1+1+1)+1]=static_cast<uint8_t>(other_client.getX());
                                                MapVisibilityAlgorithm::tempBigBufferForChanges[1+4+1+changesCount*(1+1+1+1)+1+1]=static_cast<uint8_t>(other_client.getY());
                                                MapVisibilityAlgorithm::tempBigBufferForChanges[1+4+1+changesCount*(1+1+1+1)+1+1+1]=static_cast<uint8_t>(other_client.getLastDirection());
                                                changesCount++;
                                            }
                                        }
                                    }
                                    //different character now occupies this slot -> re-insert
                                    else
                                    {
                                        std::cerr << "min_network() PATH2 slot=" << index_client << " scan other_slot=" << index << " other=" << other_client.getPseudo()
                                                  << " REPLACED old_charId=" << c_stat.characterId_db << " new_charId=" << other_client.getPlayerId() << " -> insert" << std::endl;
                                        //full insert other player on map (another player replaced the old one)
                                        {
                                            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=index;//local slot
                                            posOutput+=1;
                                            posOutput+=playerToFullInsert(ClientList::list->at(other_client_id),ProtocolParsingBase::tempBigBufferForOutput+posOutput);
                                            insertCount++;
                                        }
                                    }
                                }
                                //slot is now empty but was occupied -> remove
                                else
                                {
                                    if(c_stat.characterId_db==0xffffffff)
                                    {
                                        std::cerr << "min_network() PATH2 slot=" << index_client << " scan other_slot=" << index << " EMPTY_ALREADY" << std::endl;
                                    }//donothing, already deleted in previous tick
                                    else
                                    {
                                        std::cerr << "min_network() PATH2 slot=" << index_client << " scan other_slot=" << index << " REMOVE old_charId=" << c_stat.characterId_db << std::endl;
                                        //player left this slot -> send 0x69 remove entry
                                        {
                                            MapVisibilityAlgorithm::tempBigBufferForRemove[1+4+1+removeCount]=static_cast<uint8_t>(index);
                                            removeCount++;
                                        }
                                    }
                                }
                            }
                            //slot is beyond sendedStatus range -> new slot appeared (map grew)
                            else
                            {
                                const PLAYER_INDEX_FOR_CONNECTED &other_client_id=map_clients_id.at(index);
                                if(other_client_id!=PLAYER_INDEX_FOR_CONNECTED_MAX)
                                {
                                    std::cerr << "min_network() PATH2 slot=" << index_client << " scan other_slot=" << index << " NEW_SLOT globalId=" << other_client_id
                                              << " other=" << ClientList::list->at(other_client_id).getPseudo() << " -> insert" << std::endl;
                                    //new player in a slot we haven't seen before -> full insert
                                    {
                                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=index;//local slot
                                        posOutput+=1;
                                        posOutput+=playerToFullInsert(ClientList::list->at(other_client_id),ProtocolParsingBase::tempBigBufferForOutput+posOutput);
                                        insertCount++;
                                    }
                                }
                                else
                                    std::cerr << "min_network() PATH2 slot=" << index_client << " scan other_slot=" << index << " NEW_SLOT_EMPTY" << std::endl;
                            }
                            index++;
                        }

                        //flush the buffer: compose and send all detected differences
                        if(changesCount>0 || removeCount>0 || insertCount>0)
                        {
                            std::cerr << "min_network() PATH2 slot=" << index_client << " player=" << clientWithMap.getPseudo()
                                      << " SENDING inserts=" << std::to_string(insertCount) << " removes=" << std::to_string(removeCount) << " changes=" << std::to_string(changesCount) << std::endl;
                            if(insertCount>0)
                            {
                                std::cerr << "min_network() PATH2 slot=" << index_client << " +0x6B insert_bytes=" << posOutput << std::endl;
                                //fill the reserved 0x6B header at [0..8] now that we know insert data size
                                ProtocolParsingBase::tempBigBufferForOutput[0x00]=0x6B;//full Insert player on map
                                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size (data bytes after code+size)
                                ProtocolParsingBase::tempBigBufferForOutput[1+4]=0x01;//map list count
                                *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+4+1)=htole16(mapIndex);//map id
                                if(insertCount<254)
                                    ProtocolParsingBase::tempBigBufferForOutput[1+4+1+2]=static_cast<uint8_t>(insertCount);//player count
                                else
                                    ProtocolParsingBase::tempBigBufferForOutput[1+4+1+2]=static_cast<uint8_t>(254);//player count
                            }
                            else
                            {
                                std::cerr << "min_network() PATH2 slot=" << index_client << " no_inserts reset posOutput=0" << std::endl;
                                posOutput=0;//no inserts: reset to 0, don't send the unused reserved 0x6B header space
                            }

                            //append 0x69 remove packet (if any) after the insert data (or at position 0 if no inserts)
                            if(removeCount>0)
                            {
                                std::cerr << "min_network() PATH2 slot=" << index_client << " +0x69 remove count=" << std::to_string(removeCount) << " at_pos=" << posOutput << std::endl;
                                MapVisibilityAlgorithm::tempBigBufferForRemove[1+4]=static_cast<uint8_t>(removeCount);//player count
                                *reinterpret_cast<uint32_t *>(MapVisibilityAlgorithm::tempBigBufferForRemove+1)=htole32(1+removeCount);//dynamic size = count_byte + indices
                                //copy the pre-built remove buffer [0x69][size4][count1][indices...] into main output
                                memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,MapVisibilityAlgorithm::tempBigBufferForRemove,1+4+1+removeCount);
                                posOutput+=1+4+1+removeCount;
                            }
                            //append 0x66 changes packet (if any) after removes
                            if(changesCount>0)
                            {
                                std::cerr << "min_network() PATH2 slot=" << index_client << " +0x66 changes count=" << std::to_string(changesCount) << " at_pos=" << posOutput << std::endl;
                                MapVisibilityAlgorithm::tempBigBufferForChanges[1+4]=static_cast<uint8_t>(changesCount);//player count
                                *reinterpret_cast<uint32_t *>(MapVisibilityAlgorithm::tempBigBufferForChanges+1)=htole32(1+changesCount*(1+1+1+1));//dynamic size = count_byte + count * 4 bytes per entry
                                //copy the pre-built changes buffer [0x66][size4][count1][entries...] into main output
                                memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,MapVisibilityAlgorithm::tempBigBufferForChanges,1+4+1+changesCount*(1+1+1+1));
                                posOutput+=1+4+1+changesCount*(1+1+1+1);
                            }
                            //only append ping if none pending, to avoid exhausting query numbers
                            if(clientWithMap.pingCountInProgress()<=0)
                            {
                                posOutput+=clientWithMap.sendPing(ProtocolParsingBase::tempBigBufferForOutput+posOutput);
                                std::cerr << "min_network() PATH2 slot=" << index_client << " player=" << clientWithMap.getPseudo() << " +ping" << std::endl;
                            }
                            else
                                std::cerr << "min_network() PATH2 slot=" << index_client << " player=" << clientWithMap.getPseudo() << " skip_ping pingInProgress=" << std::to_string(clientWithMap.pingCountInProgress()) << std::endl;
                            std::cerr << "min_network() PATH2 slot=" << index_client << " player=" << clientWithMap.getPseudo() << " SEND total_bytes=" << posOutput
                                      << " first_byte=0x" << std::hex << std::to_string((uint8_t)ProtocolParsingBase::tempBigBufferForOutput[0]) << std::dec << std::endl;
                            clientWithMap.sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                            //update sendedStatus with current state after send so next tick can detect differences
                            clientWithMap.sendedStatus.resize(std::min(map_clients_id.size(),static_cast<size_t>(255)));
                            for(unsigned int ss=0;ss<clientWithMap.sendedStatus.size();ss++)
                            {
                                const PLAYER_INDEX_FOR_CONNECTED &oid=map_clients_id.at(ss);
                                if(oid!=PLAYER_INDEX_FOR_CONNECTED_MAX)
                                {
                                    const Client &c=ClientList::list->at(oid);
                                    clientWithMap.sendedStatus[ss].characterId_db=c.getPlayerId();
                                    clientWithMap.sendedStatus[ss].x=c.getX();
                                    clientWithMap.sendedStatus[ss].y=c.getY();
                                    clientWithMap.sendedStatus[ss].direction=c.getLastDirection();
                                }
                                else
                                    clientWithMap.sendedStatus[ss].characterId_db=0xffffffff;
                            }
                        }
                        else
                        {
                            if(clientWithMap.pingCountInProgress()>0)
                                std::cerr << "min_network() PATH2 slot=" << index_client << " player=" << clientWithMap.getPseudo() << " NO_DIFF but pingCountInProgress=" << std::to_string(clientWithMap.pingCountInProgress()) << std::endl;
                        }
                    }
                }
            }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            else
                std::cerr << "MapVisibilityAlgorithm::min_network() ClientList::list.empty(): " << map_c_idP << std::endl;
            #endif
        }
        index_client++;
    }
}
