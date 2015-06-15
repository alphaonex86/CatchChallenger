#include "PurgeTheLockedAccount.h"
#include "CharactersGroup.h"

using namespace CatchChallenger;

PurgeTheLockedAccount::PurgeTheLockedAccount()
{
    setInterval(15*60*1000);
}

void PurgeTheLockedAccount::exec()
{
    int index=0;
    while(index<CharactersGroup::list.size())
    {
        CharactersGroup::list.at(index)->purgeTheLockedAccount();
        index++;
    }
}
