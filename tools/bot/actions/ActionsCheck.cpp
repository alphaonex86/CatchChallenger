#include "ActionsAction.h"
#include "../../general/base/CommonDatapack.h"
#include <iostream>

void ActionsAction::showTip(const QString &text)
{
    std::cout << text.toStdString() << std::endl;
}

void ActionsAction::appendReputationRewards(CatchChallenger::Api_protocol_Qt *api,const std::vector<CatchChallenger::ReputationRewards> &reputationList)
{
    QList<CatchChallenger::ReputationRewards> reputationListTemp;
    unsigned int index=0;
    while(index<reputationList.size())
    {
        reputationListTemp << reputationList.at(index);
        index++;
    }
    appendReputationRewards(api,reputationListTemp);
}

void ActionsAction::appendReputationRewards(CatchChallenger::Api_protocol_Qt *api,const QList<CatchChallenger::ReputationRewards> &reputationList)
{
    int index=0;
    while(index<reputationList.size())
    {
        const CatchChallenger::ReputationRewards &reputationRewards=reputationList.at(index);
        appendReputationPoint(api,CatchChallenger::CommonDatapack::commonDatapack.reputation.at(reputationRewards.reputationId).name,
                reputationRewards.point);
        index++;
    }
    //show_reputation();
}

bool ActionsAction::haveReputationRequirements(const CatchChallenger::Api_protocol_Qt *api,const std::vector<CatchChallenger::ReputationRequirements> &reputationList)
{
    QList<CatchChallenger::ReputationRequirements> reputationListTemp;
    unsigned int index=0;
    while(index<reputationList.size())
    {
        reputationListTemp << reputationList.at(index);
        index++;
    }
    return haveReputationRequirements(api,reputationListTemp);
}

bool ActionsAction::haveReputationRequirements(const CatchChallenger::Api_protocol_Qt *api, const QList<CatchChallenger::ReputationRequirements> &reputationList)
{
    int index=0;
    while(index<reputationList.size())
    {
        const CatchChallenger::ReputationRequirements &reputation=reputationList.at(index);
        if(api->player_informations.reputation.find(reputation.reputationId)!=api->player_informations.reputation.cend())
        {
            const CatchChallenger::PlayerReputation &playerReputation=api->player_informations.reputation.at(reputation.reputationId);
            if(!reputation.positif)
            {
                if(-reputation.level<playerReputation.level)
                {
                    //emit message(QStringLiteral("reputation.level(%1)<playerReputation.level(%2)").arg(reputation.level).arg(playerReputation.level));
                    return false;
                }
            }
            else
            {
                if(reputation.level>playerReputation.level || playerReputation.point<0)
                {
                    //emit message(QStringLiteral("reputation.level(%1)>playerReputation.level(%2) || playerReputation.point(%3)<0").arg(reputation.level).arg(playerReputation.level).arg(playerReputation.point));
                    return false;
                }
            }
        }
        else
            if(!reputation.positif)//default level is 0, but required level is negative
            {
                /*emit message(QStringLiteral("reputation.level(%1)<0 and no reputation.type=%2").arg(reputation.level).arg(
                                 QString::fromStdString(CatchChallenger::CommonDatapack::commonDatapack.reputation.at(reputation.reputationId).name)
                                 ));*/
                return false;
            }
        index++;
    }
    return true;
}
