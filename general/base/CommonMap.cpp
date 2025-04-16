#include "CommonMap.hpp"
#include "Map_loader.hpp"
#include "FacilityLibGeneral.hpp"
#include <iostream>
#include <chrono>

using namespace CatchChallenger;

void * CommonMap::flat_map_list=nullptr;
CATCHCHALLENGER_TYPE_MAPID CommonMap::flat_map_list_size=0;
size_t CommonMap::flat_map_object_size=0;//store in full length to easy multiply by index (16Bits) and have full size pointer

CommonMap::Teleporter* CommonMap::flat_teleporter=nullptr;
CATCHCHALLENGER_TYPE_TELEPORTERID CommonMap::flat_teleporter_list_size=0;//temp, used as size when finish

uint8_t *CommonMap::flat_simplified_map=nullptr;
CATCHCHALLENGER_TYPE_MAPID CommonMap::flat_simplified_map_list_size=0;//temp, used as size when finish

const void * CommonMap::indexToMap(const CATCHCHALLENGER_TYPE_MAPID &index)
{
#ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(flat_map_list==nullptr)
    {
        std::cerr << "CommonMap::indexToMap() flat_map_list==nullptr" << std::endl;
        abort();
    }
    if(flat_map_object_size<=sizeof(CommonMap))
    {
        std::cerr << "CommonMap::indexToMap() map_object_size<=sizeof(CommonMap) " << sizeof(CommonMap) << std::endl;
        abort();
    }
    if(flat_map_list_size<=0)
    {
        std::cerr << "CommonMap::indexToMap() flat_map_list_size<=0" << std::endl;
        abort();
    }
    if(index>=flat_map_list_size)
    {
        std::cerr << "CommonMap::indexToMap() index>=flat_map_list_size the check have to be done at index creation" << std::endl;
        abort();
    }
#endif
    return ((char *)flat_map_list)+index*flat_map_object_size;
}

void * CommonMap::indexToMapWritable(const CATCHCHALLENGER_TYPE_MAPID &index)
{
#ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(flat_map_list==nullptr)
    {
        std::cerr << "CommonMap::indexToMap() flat_map_list==nullptr" << std::endl;
        abort();
    }
    if(flat_map_object_size<=sizeof(CommonMap))
    {
        std::cerr << "CommonMap::indexToMap() map_object_size<=sizeof(CommonMap) " << sizeof(CommonMap) << std::endl;
        abort();
    }
    if(flat_map_list_size<=0)
    {
        std::cerr << "CommonMap::indexToMap() flat_map_list_size<=0" << std::endl;
        abort();
    }
    if(index>=flat_map_list_size)
    {
        std::cerr << "CommonMap::indexToMap() index>=flat_map_list_size the check have to be done at index creation" << std::endl;
        abort();
    }
#endif
    return ((char *)flat_map_list)+index*flat_map_object_size;
}
