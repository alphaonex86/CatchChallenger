#include "BaseServer.hpp"
#include "DictionaryServer.hpp"
#include "GlobalServerData.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"

using namespace CatchChallenger;

void BaseServer::preload_15_async_map_semi_after_db_id()
{
    if(DictionaryServer::dictionary_map_database_to_internal.size()==0)
    {
        std::cerr << "Need be called after preload_dictionary_map()" << std::endl;
        abort();
    }

    semi_loaded_map.clear();

    for(const std::pair<CATCHCHALLENGER_TYPE_QUEST,Quest>& n : CommonDatapackServerSpec::commonDatapackServerSpec.get_quests())
    {
        const Quest &src=n.second;
        QuestServer dest;
        dest.id=src.id;
        dest.repeatable=src.repeatable;
        dest.requirements.quests=src.requirements.quests;
        dest.requirements.reputation=src.requirements.reputation;
        unsigned int index=0;
        while(index<src.steps.size())
        {
            const Quest::Step &sSrc=src.steps.at(index);
            QuestServer::StepServer sDst;
            sDst.botToTalk.fightBot=sSrc.botToTalk.fightBot;
            sDst.botToTalk.map=sSrc.botToTalk.map;//search into map list to resolv
            unsigned int subindex=0;
            while(subindex<sSrc.itemsMonster.size())
            {
                const Quest::ItemMonster &ssSrc=sSrc.itemsMonster.at(index);
                QuestServer::ItemMonsterServer ssDst;
                ssDst.item=ssSrc.item;
                ssDst.monsters=ssSrc.monsters;
                ssDst.rate=ssSrc.rate;
                sDst.itemsMonster.push_back(ssDst);
                subindex++;
            }
            subindex=0;
            while(subindex<sSrc.requirements.fights.size())
            {
                const BotMap &ssSrc=sSrc.requirements.fights.at(index);
                BotMapServer ssDst;
                ssDst.fightBot=ssSrc.fightBot;
                ssDst.map=ssSrc.map;//search into map list to resolv
                sDst.requirements.fights.push_back(ssDst);
                subindex++;
            }
            subindex=0;
            while(subindex<sSrc.requirements.items.size())
            {
                const Quest::Item &ssSrc=sSrc.requirements.items.at(index);
                QuestServer::ItemServer ssDst;
                ssDst.item=ssSrc.item;
                ssDst.quantity=ssSrc.quantity;
                sDst.requirements.items.push_back(ssDst);
                subindex++;
            }
            dest.steps.push_back(sDst);
            index++;
        }
        dest.rewards.allow=src.rewards.allow;
        index=0;
        while(index<src.rewards.items.size())
        {
            const Quest::Item &sSrc=src.rewards.items.at(index);
            QuestServer::ItemServer sDst;
            sDst.item=sSrc.item;
            sDst.quantity=sSrc.quantity;
            dest.rewards.items.push_back(sDst);
            index++;
        }
        dest.rewards.reputation=src.rewards.reputation;
        quests[n.first]=dest;
    }
    CommonDatapackServerSpec::commonDatapackServerSpec.unload();

    preload_16_async_zone_sql();
}
