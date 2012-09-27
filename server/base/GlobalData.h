#ifndef POKECRAFT_GLOBALDATA_H
#define POKECRAFT_GLOBALDATA_H

#include "ServerStructures.h"

namespace Pokecraft {
class GlobalData
{
public:
	//shared into all the program
	static ServerSettings serverSettings;
	static ServerPrivateVariables serverPrivateVariables;
};
}

#endif
