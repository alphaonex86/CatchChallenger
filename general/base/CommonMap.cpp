#include "CommonMap.hpp"
#include "Map_loader.hpp"
#include "FacilityLibGeneral.hpp"
#include <iostream>
#include <chrono>

using namespace CatchChallenger;

void * CommonMap::flat_map_list=nullptr;
CATCHCHALLENGER_TYPE_MAPID CommonMap::flat_map_list_count=0;
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
    if(flat_map_list_count<=0)
    {
        std::cerr << "CommonMap::indexToMap() flat_map_list_size<=0" << std::endl;
        abort();
    }
    if(index>=flat_map_list_count)
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
    if(flat_map_list_count<=0)
    {
        std::cerr << "CommonMap::indexToMap() flat_map_list_size<=0" << std::endl;
        abort();
    }
    if(index>=flat_map_list_count)
    {
        std::cerr << "CommonMap::indexToMap() index>=flat_map_list_size the check have to be done at index creation" << std::endl;
        abort();
    }
#endif
    return ((char *)flat_map_list)+index*flat_map_object_size;
}

bool CommonMap::parseUnknownMoving(std::string type,uint32_t object_x,uint32_t object_y,std::unordered_map<std::string,std::string> property_text)
{
    (void)type;
    (void)object_x;
    (void)object_y;
    (void)property_text;
    return false;
}

bool CommonMap::parseUnknownObject(std::string type,uint32_t object_x,uint32_t object_y,std::unordered_map<std::string,std::string> property_text)
{
    if(type=="rescue")
        return true;
    (void)type;
    (void)object_x;
    (void)object_y;
    (void)property_text;
    return false;
}

bool CommonMap::parseUnknownBotStep(uint32_t object_x,uint32_t object_y,const tinyxml2::XMLElement *step)
{
    if(strcmp(step->Attribute("type"),"heal")==0)
        return true;
    if(strcmp(step->Attribute("type"),"zonecapture")==0)
        return true;
    (void)object_x;
    (void)object_y;
    (void)step;
    return false;
}
