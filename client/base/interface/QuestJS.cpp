#include "QuestJS.h"
#include "BaseWindow.h"
#include "../../../general/base/CommonDatapack.h"
#include "../../../general/base/CommonDatapackServerSpec.h"
#include "../../../general/base/GeneralStructures.h"

Quest::Quest(const uint32_t &quest) :
    quest(quest)
{
}

int Quest::currentQuestStep() const
{
    std::unordered_map<uint16_t, CatchChallenger::PlayerQuest> quests=CatchChallenger::BaseWindow::baseWindow->getQuests();
    if(quests.find(quest)==quests.cend())
        return 0;
    else
        return quests.at(quest).step;
}

int Quest::currentBot() const
{
    std::unordered_map<uint16_t, CatchChallenger::PlayerQuest> quests=CatchChallenger::BaseWindow::baseWindow->getQuests();
    if(quests.find(quest)==quests.cend())
        return 0;
    else
        return CatchChallenger::BaseWindow::baseWindow->getActualBotId();
}

bool Quest::finishOneTime() const
{
    std::unordered_map<uint16_t, CatchChallenger::PlayerQuest> quests=CatchChallenger::BaseWindow::baseWindow->getQuests();
    if(quests.find(quest)==quests.cend())
        return false;
    else
        return quests.at(quest).finish_one_time;
}

bool Quest::haveQuestStepRequirements() const
{
    std::unordered_map<uint16_t, CatchChallenger::PlayerQuest> quests=CatchChallenger::BaseWindow::baseWindow->getQuests();
    if(quests.find(quest)==quests.cend())
        return false;
    else
    {
        if(quests.at(quest).step<=0)
            return false;
        if(CatchChallenger::BaseWindow::baseWindow->haveNextStepQuestRequirements(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.quests.at(quest)))
            return true;
        return false;
    }
}
