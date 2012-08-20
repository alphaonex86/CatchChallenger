#include "Api_client_virtual.h"

using namespace Pokecraft;

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#endif

//need host + port here to have datapack base

Api_client_virtual::Api_client_virtual(QAbstractSocket *socket) :
	Api_protocol(socket)
{
}

Api_client_virtual::~Api_client_virtual()
{
}
