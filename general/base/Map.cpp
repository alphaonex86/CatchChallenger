#include "Map.h"
#include "ProtocolParsing.h"

Map::Map()
{
}

bool Map::loadInternalVariables()
{
	QString tempFile=map_file;
	tempFile.remove(".tmx");
	rawMapFile=ProtocolParsing::toUTF8(map_file);
	if(map_file.size()==0)
		return false;

	return true;
}
