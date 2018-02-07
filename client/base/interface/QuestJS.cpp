#include "QuestJS.h"
#include "BaseWindow.h"
#include "../../../general/base/CommonDatapack.h"
#include "../../../general/base/CommonDatapackServerSpec.h"
#include "../../../general/base/GeneralStructures.h"

Quest::Quest(const uint16_t &quest, void *baseWindow) :
    quest(quest),
    baseWindow(static_cast<CatchChallenger::BaseWindow *>(baseWindow))
{
}

int Quest::currentQuestStep() const
{
    CatchChallenger::BaseWindow *baseWindow=static_cast<CatchChallenger::BaseWindow *>(this->baseWindow);
    std::unordered_map<uint16_t, CatchChallenger::PlayerQuest> quests=baseWindow->getQuests();
    if(quests.find(quest)==quests.cend())
        return 0;
    else
        return quests.at(quest).step;
}

int Quest::currentBot() const
{
    CatchChallenger::BaseWindow *baseWindow=static_cast<CatchChallenger::BaseWindow *>(this->baseWindow);
    std::unordered_map<uint16_t, CatchChallenger::PlayerQuest> quests=baseWindow->getQuests();
    if(quests.find(quest)==quests.cend())
        return 0;
    else
        return baseWindow->getActualBotId();
}

bool Quest::finishOneTime() const
{
    CatchChallenger::BaseWindow *baseWindow=static_cast<CatchChallenger::BaseWindow *>(this->baseWindow);
    std::unordered_map<uint16_t, CatchChallenger::PlayerQuest> quests=baseWindow->getQuests();
    if(quests.find(quest)==quests.cend())
        return false;
    else
        return quests.at(quest).finish_one_time;
}

bool Quest::haveQuestStepRequirements() const
{
    CatchChallenger::BaseWindow *baseWindow=static_cast<CatchChallenger::BaseWindow *>(this->baseWindow);
    std::unordered_map<uint16_t, CatchChallenger::PlayerQuest> quests=baseWindow->getQuests();
    if(quests.find(quest)==quests.cend())
        return false;
    else
    {
        if(quests.at(quest).step<=0)
            return false;
        if(baseWindow->haveNextStepQuestRequirements(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(quest)))
            return true;
        return false;
    }
}
