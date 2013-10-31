#include "CommonMap.h"
#include "FacilityLib.h"

using namespace CatchChallenger;

void Map::removeParsedLayer(const ParsedLayer &parsed_layer)
{
    if(parsed_layer.dirt!=NULL)
        delete parsed_layer.dirt;
    if(parsed_layer.grass!=NULL)
        delete parsed_layer.grass;
    if(parsed_layer.ledges!=NULL)
        delete parsed_layer.ledges;
    if(parsed_layer.walkable!=NULL)
        delete parsed_layer.walkable;
    if(parsed_layer.water!=NULL)
        delete parsed_layer.water;
}
