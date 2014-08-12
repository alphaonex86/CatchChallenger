#include "CommonMap.h"
#include "FacilityLib.h"
#include "Map_loader.h"

using namespace CatchChallenger;

void CommonMap::removeParsedLayer(const ParsedLayer &parsed_layer)
{
    Map_loader::removeMapLayer(parsed_layer);
}
