#include "DatabaseBase.h"

using namespace CatchChallenger;

DatabaseBase::DatabaseBase() :
    tryInterval(1),
    considerDownAfterNumberOfTry(30)
{
}

void DatabaseBase::clear()
{
}
