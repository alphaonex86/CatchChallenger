#include "../base/LocalClientHandler.h"
#include "../../general/base/ProtocolParsing.h"

using namespace Pokecraft;

void LocalClientHandler::getRandomNumberIfNeeded()
{
    if(randomSeeds.size()<=POKECRAFT_SERVER_MIN_RANDOM_LIST_SIZE)
        emit askRandomNumber();
}

void LocalClientHandler::newRandomNumber(const QByteArray &randomData)
{
    randomSeeds+=randomData;
}
