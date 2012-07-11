#include "Map_server.h"
#include "../../general/base/ProtocolParsing.h"

Map_server::Map_server()
{
}

bool Map_server::loadInternalVariables()
{
	rawMapFile=ProtocolParsing::toUTF8(map_file);
	if(map_file.size()==0)
		return false;

	return true;
}
