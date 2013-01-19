#ifndef POKECRAFT_GLOBALSERVERDATA_H
#define POKECRAFT_GLOBALSERVERDATA_H

#include "ServerStructures.h"

namespace Pokecraft {
class GlobalServerData
{
public:
	//shared into all the program
	static ServerSettings serverSettings;
	static ServerPrivateVariables serverPrivateVariables;
};
}

#endif
