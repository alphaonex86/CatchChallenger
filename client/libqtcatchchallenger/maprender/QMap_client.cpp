#include "QMap_client.hpp"

std::unordered_map<CATCHCHALLENGER_TYPE_MAPID,CatchChallenger::QMap_client *> CatchChallenger::QMap_client::all_map;
std::unordered_map<CATCHCHALLENGER_TYPE_MAPID,CatchChallenger::QMap_client *> CatchChallenger::QMap_client::old_all_map;
std::unordered_map<CATCHCHALLENGER_TYPE_MAPID,uint8_t> CatchChallenger::QMap_client::old_all_map_time_count;

CatchChallenger::QMap_client::QMap_client()
{
    tiledMap=NULL;
    tiledRender=NULL;
    objectGroup=NULL;
    objectGroupIndex=0;
    relative_x=0;
    relative_y=0;
    relative_x_pixel=0;
    relative_y_pixel=0;
    displayed=false;
}
