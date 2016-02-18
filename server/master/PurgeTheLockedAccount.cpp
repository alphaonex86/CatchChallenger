#include "PurgeTheLockedAccount.h"
#include "CharactersGroup.h"

using namespace CatchChallenger;

PurgeTheLockedAccount::PurgeTheLockedAccount(unsigned int intervalInSeconds)
{
    start(intervalInSeconds*1000);
}

void PurgeTheLockedAccount::exec()
{
    unsigned int index=0;
    while(index<CharactersGroup::list.size())
    {
        CharactersGroup::list.at(index)->purgeTheLockedAccount();
        index++;
    }
}
