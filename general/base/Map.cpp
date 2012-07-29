#include "Map.h"
#include "ProtocolParsing.h"

Map::Map()
{
}

bool Map::loadInternalVariables()
{
	rawMapFile=ProtocolParsing::toUTF8(map_file);
	if(map_file.size()==0)
		return false;

	return true;
}
