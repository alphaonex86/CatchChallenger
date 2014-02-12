#include "CommonMap.h"
#include "FacilityLib.h"

using namespace CatchChallenger;

void CommonMap::removeParsedLayer(const ParsedLayer &parsed_layer)
{
    if(parsed_layer.dirt!=NULL)
        delete parsed_layer.dirt;
    if(parsed_layer.monstersCollisionMap!=NULL)
        delete parsed_layer.monstersCollisionMap;
    if(parsed_layer.ledges!=NULL)
        delete parsed_layer.ledges;
    if(parsed_layer.walkable!=NULL)
        delete parsed_layer.walkable;
}
