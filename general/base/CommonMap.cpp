#include "CommonMap.hpp"
#include "FacilityLib.hpp"
#include "Map_loader.hpp"

using namespace CatchChallenger;

void CommonMap::removeParsedLayer(const ParsedLayer &parsed_layer)
{
    Map_loader::removeMapLayer(parsed_layer);
}
