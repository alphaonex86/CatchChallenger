#include "DatabaseBase.h"

using namespace CatchChallenger;

DatabaseBase::DatabaseBase() :
    tryInterval(5),
    considerDownAfterNumberOfTry(3)
{
}
