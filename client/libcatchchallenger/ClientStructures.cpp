#include "ClientStructures.hpp"

using namespace CatchChallenger;

bool ServerFromPoolForDisplay::operator<(const ServerFromPoolForDisplay &serverFromPoolForDisplay) const
{
    if(serverFromPoolForDisplay.uniqueKey<this->uniqueKey)
        return true;
    else
        return false;
}
