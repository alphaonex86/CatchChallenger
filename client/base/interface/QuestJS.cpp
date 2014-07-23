#include "QuestJS.h"
#include "BaseWindow.h"
#include "../../../general/base/CommonDatapack.h"
#include "../../../general/base/GeneralStructures.h"

Quest::Quest(const quint32 &quest) :
    quest(quest)
{
}

int Quest::currentQuestStep() const
{
    QHash<quint16, CatchChallenger::PlayerQuest> quests=CatchChallenger::BaseWindow::baseWindow->getQuests();
    if(!quests.contains(quest))
        return 0;
    else
        return quests.value(quest).step;
}

int Quest::currentBot() const
{
    QHash<quint16, CatchChallenger::PlayerQuest> quests=CatchChallenger::BaseWindow::baseWindow->getQuests();
    if(!quests.contains(quest))
        return 0;
    else
        return CatchChallenger::BaseWindow::baseWindow->getActualBotId();
}

bool Quest::finishOneTime() const
{
    QHash<quint16, CatchChallenger::PlayerQuest> quests=CatchChallenger::BaseWindow::baseWindow->getQuests();
    if(!quests.contains(quest))
        return false;
    else
        return quests.value(quest).finish_one_time;
}

bool Quest::haveQuestStepRequirements() const
{
    QHash<quint16, CatchChallenger::PlayerQuest> quests=CatchChallenger::BaseWindow::baseWindow->getQuests();
    if(!quests.contains(quest))
        return false;
    else
    {
        if(quests.value(quest).step<=0)
            return false;
        if(CatchChallenger::BaseWindow::baseWindow->haveNextStepQuestRequirements(CatchChallenger::CommonDatapack::commonDatapack.quests.value(quest)))
            return true;
        return false;
    }
}
