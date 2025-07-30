#include "CommonMap.hpp"
#include "Map_loader.hpp"
#include "FacilityLibGeneral.hpp"
#include <iostream>
#include <chrono>

using namespace CatchChallenger;

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
