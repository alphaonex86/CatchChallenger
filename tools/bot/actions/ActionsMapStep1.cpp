#include "ActionsAction.h"

#include "../../general/base/CommonSettingsServer.h"
#include "../../general/base/Map_loader.h"
#include "../../general/base/FacilityLibGeneral.h"

#include <regex>
#include <string>
#include <vector>

bool ActionsAction::preload_the_map_step1()
{
    unsigned int index=0;
    while(index<map_list.size())
    {
        MapServerMini * const map=static_cast<MapServerMini *>(flat_map_list[index]);
        map->preload_step1();
        index++;
    }
    return true;
}

bool ActionsAction::preload_the_map_step2()
{
    unsigned int index=0;
    while(index<map_list.size())
    {
        MapServerMini * const map=static_cast<MapServerMini *>(flat_map_list[index]);
        map->preload_step2();
        index++;
    }
    return true;
}
