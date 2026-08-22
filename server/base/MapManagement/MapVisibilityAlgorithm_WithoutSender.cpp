#include "MapVisibilityAlgorithm_WithoutSender.hpp"
#include "MapVisibilityAlgorithm.hpp"
#include "../GlobalServerData.hpp"
#include "../../general/base/ProtocolParsing.hpp"

using namespace CatchChallenger;

MapVisibilityAlgorithm_WithoutSender MapVisibilityAlgorithm_WithoutSender::mapVisibilityAlgorithm_WithoutSender;

MapVisibilityAlgorithm_WithoutSender::MapVisibilityAlgorithm_WithoutSender()
{
}

MapVisibilityAlgorithm_WithoutSender::~MapVisibilityAlgorithm_WithoutSender()
{
}

void MapVisibilityAlgorithm_WithoutSender::generalPurgeBuffer()
{
    if(!GlobalServerData::serverSettings.mapVisibility.enable)
        return;
    //if display 0 or 1 player mean just display them self
    if(GlobalServerData::serverSettings.mapVisibility.simple.max<2)
        return;

    unsigned int index=0;
    switch(GlobalServerData::serverSettings.mapVisibility.minimize)
    {
    case GameServerSettings::MapVisibility::Minimize_CPU:
       while(index<MapVisibilityAlgorithm::flat_map_list.size())//put loop into condition to have best performance
       {
           MapVisibilityAlgorithm::flat_map_list.at(index).min_CPU(static_cast<CATCHCHALLENGER_TYPE_MAPID>(index));
           index++;
       }
       break;
    //"balanced": whole map, but only what changed. It WAS the network
    //minimising algorithm, named "network", before the view range one below
    //existed.
    case GameServerSettings::MapVisibility::Minimize_Balanced:
       ProtocolParsingBase::tempBigBufferForOutput[0x00]=0x6B;
       ProtocolParsingBase::tempBigBufferForOutput[1+4]=0x01;//map list count
       while(index<MapVisibilityAlgorithm::flat_map_list.size())//put loop into condition to have best performance
       {
           MapVisibilityAlgorithm::flat_map_list.at(index).min_balanced(static_cast<CATCHCHALLENGER_TYPE_MAPID>(index));
           index++;
       }
       break;
    //"network": only what is into the view range of each player, border maps
    //included. min_network() composes each 0x6B header itself (one by source
    //map), so nothing is pre-seeded into the shared buffer here.
    case GameServerSettings::MapVisibility::Minimize_Network:
       /* A new tick: min_network()'s per-map candidate snapshots are stale.
        * Done HERE and not inside min_network() because a map's snapshot is
        * read by its border maps before its own turn in the loop below comes.
        * Inside the case and not above the switch: the two other algorithms
        * never read a snapshot, so they must not pay for it. */
       MapVisibilityAlgorithm::beginTick();
       while(index<MapVisibilityAlgorithm::flat_map_list.size())//put loop into condition to have best performance
       {
           MapVisibilityAlgorithm::flat_map_list.at(index).min_network(static_cast<CATCHCHALLENGER_TYPE_MAPID>(index));
           index++;
       }
       break;
    }
}
