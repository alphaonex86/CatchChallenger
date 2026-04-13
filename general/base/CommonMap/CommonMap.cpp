#include "CommonMap.hpp"
#include "../Map_loader.hpp"
#include "../FacilityLibGeneral.hpp"
#include <iostream>
#include <chrono>

using namespace CatchChallenger;

bool CommonMap::parseUnknownMoving(std::string type,uint32_t object_x,uint32_t object_y,std::unordered_map<std::string,std::string> property_text)
{
    unknownMovingBuffer.push_back({std::move(type),object_x,object_y,std::move(property_text)});
    return true;
}

bool CommonMap::parseUnknownObject(std::string type,uint32_t object_x,uint32_t object_y,std::unordered_map<std::string,std::string> property_text)
{
    unknownObjectBuffer.push_back({std::move(type),object_x,object_y,std::move(property_text)});
    return true;
}

#ifndef CATCHCHALLENGER_NOXML
bool CommonMap::parseUnknownBotStep(uint32_t object_x,uint32_t object_y,const tinyxml2::XMLElement *step)
{
    unknownBotStepBuffer.push_back({object_x,object_y,step});
    return true;
}
#endif
