#include "ActionsAction.h"

#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/Map_loader.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/CommonDatapack.hpp"

#include <regex>
#include <string>
#include <vector>
#include <QCoreApplication>

bool ActionsAction::preload_other_pre()
{
    unsigned int index=0;
    while(index<map_list.size())
    {
        static_cast<MapServerMini *>(flat_map_list[index])->preload_other_pre();
        QCoreApplication::processEvents();
        index++;
        loaded++;
    }
    return true;
}

bool ActionsAction::preload_the_map_step1()
{
    unsigned int index=0;
    while(index<map_list.size())
    {
        MapServerMini * const map=static_cast<MapServerMini *>(flat_map_list[index]);
        map->preload_step1();
        index++;
        loaded++;
    }
    return true;
}

bool ActionsAction::preload_the_map_step2()
{
    unsigned int index=0;
    while(index<map_list.size())
    {
        static_cast<MapServerMini *>(flat_map_list[index])->preload_step2();
        QCoreApplication::processEvents();
        index++;
        loaded++;
    }
    index=0;
    while(index<map_list.size())
    {
        static_cast<MapServerMini *>(flat_map_list[index])->preload_step2b();
        QCoreApplication::processEvents();
        index++;
        loaded++;
    }
    index=0;
    while(index<map_list.size())
    {
        static_cast<MapServerMini *>(flat_map_list[index])->preload_step2c();
        QCoreApplication::processEvents();
        index++;
        loaded++;
    }
    return true;
}

bool ActionsAction::preload_post_subdatapack()
{
    if(map_list.empty())
        abort();
    unsigned int index=0;
    while(index<map_list.size())
    {
        static_cast<MapServerMini *>(flat_map_list[index])->preload_post_subdatapack();
        //QCoreApplication::processEvents(); do a bug, try parse GUI
        index++;
        loaded++;
    }
    minitemprice=0;
    maxitemprice=1;
    for( const auto& n : CatchChallenger::CommonDatapack::commonDatapack.items.item ) {
        const CatchChallenger::Item &item=n.second;
        if(maxitemprice==0 || maxitemprice<item.price)
            if(item.price>0)
                maxitemprice=item.price;
        if(minitemprice==0 || minitemprice>item.price)
            if(item.price>0)
                minitemprice=item.price;
    }
    return true;
}

void ActionsAction::loadFinishedReemitTheDelayedFunction()
{
    allMapIsLoaded=true;
    for(const auto& n:delayedMessage) {
        CatchChallenger::Api_protocol_Qt  *api=n.first;
        const std::vector<DelayedMapPlayerChange> &delayedMapPlayerChangeList=n.second;

        unsigned int index=0;
        while(index<delayedMapPlayerChangeList.size())
        {
            const DelayedMapPlayerChange &delayedMapPlayerChange=delayedMapPlayerChangeList.at(index);
            switch(delayedMapPlayerChange.type)
            {
            case DelayedMapPlayerChangeType_Insert:
                insert_player(api,delayedMapPlayerChange.player,delayedMapPlayerChange.mapId,delayedMapPlayerChange.x,delayedMapPlayerChange.y,delayedMapPlayerChange.direction);
            break;
            case DelayedMapPlayerChangeType_InsertAll:
                insert_player_all(api,delayedMapPlayerChange.player,delayedMapPlayerChange.mapId,delayedMapPlayerChange.x,delayedMapPlayerChange.y,delayedMapPlayerChange.direction);
            break;
            case DelayedMapPlayerChangeType_Delete:
                remove_player(api,delayedMapPlayerChange.player.simplifiedId);
            break;
            default:
                abort();
            }

            index++;
        }
    }
}

uint64_t ActionsAction::elementToLoad() const
{
    return map_list.size()*(1/*server load*/+1/*step1*/+3/*step2*/);
}

uint64_t ActionsAction::elementLoaded() const
{
    return loaded;
}
