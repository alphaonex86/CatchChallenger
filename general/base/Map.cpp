#include "Map.h"
#include "FacilityLib.h"

using namespace Pokecraft;

Map::Map()
{
}

bool Map::loadInternalVariables()
{
	QString tempFile=map_file;
	tempFile.remove(".tmx");
	rawMapFile=FacilityLib::toUTF8(map_file);
	if(map_file.size()==0)
		return false;

	return true;
}
