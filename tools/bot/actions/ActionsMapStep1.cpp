#include "ActionsAction.h"

#include "../../general/base/CommonSettingsServer.h"
#include "../../general/base/Map_loader.h"
#include "../../general/base/FacilityLibGeneral.h"

#include <regex>
#include <string>
#include <vector>

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
    return true;
}

void ActionsAction::loadFinishedReemitTheDelayedFunction()
{
    allMapIsLoaded=true;
    QHashIterator<CatchChallenger::Api_protocol *,Player> i(clientList);
    while (i.hasNext()) {
        i.next();
        CatchChallenger::Api_protocol *api=i.key();
        const Player &player=i.value();

        unsigned int index=0;
        while(index<player.delayedMapPlayerChange.size())
        {
            const DelayedMapPlayerChange &delayedMapPlayerChange=player.delayedMapPlayerChange.at(index);
            switch(delayedMapPlayerChange.type)
            {
                case DelayedMapPlayerChangeType_Insert:
                    insert_player(api,delayedMapPlayerChange.player,delayedMapPlayerChange.mapId,delayedMapPlayerChange.x,delayedMapPlayerChange.y,delayedMapPlayerChange.direction);
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
