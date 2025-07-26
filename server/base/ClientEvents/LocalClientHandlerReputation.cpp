#include "../Client.hpp"

#include "../../general/base/FacilityLib.hpp"
#include "../../general/base/CommonDatapack.hpp"

using namespace CatchChallenger;

void Client::appendReputationRewards(const std::vector<ReputationRewards> &reputationList)
{
    unsigned int index=0;
    while(index<reputationList.size())
    {
        const ReputationRewards &reputationRewards=reputationList.at(index);
        appendReputationPoint(reputationRewards.reputationId,reputationRewards.point);
        index++;
    }
    //syncDatabaseReputation();//do previously, optimise that's
}

//reputation
void Client::appendReputationPoint(const uint8_t &reputationId, const int32_t &point)
{
    if(point==0)
        return;
    const Reputation &reputation=CommonDatapack::commonDatapack.get_reputation().at(reputationId);
    //bool isNewReputation=false;
    PlayerReputation *playerReputation=NULL;
    //search
    {
        if(public_and_private_informations.reputation.find(reputationId)==public_and_private_informations.reputation.cend())
        {
            PlayerReputation temp;
            temp.point=0;
            temp.level=0;
            //isNewReputation=true;
            public_and_private_informations.reputation[reputationId]=temp;
        }
    }
    playerReputation=&public_and_private_informations.reputation[reputationId];

    #ifdef DEBUG_MESSAGE_CLIENT_REPUTATION
    normalOutput("Reputation "+std::to_string(reputationId)+" at level: "+std::to_string(playerReputation->level)+" with point: "+std::to_string(playerReputation->point));
    #endif
    FacilityLib::appendReputationPoint(playerReputation,point,reputation);
    /*if(isNewReputation)
    {
        std::string queryText=PreparedDBQueryCommon::db_query_insert_reputation;
        stringreplaceOne(queryText,"%1",std::to_string(character_id));
        stringreplaceOne(queryText,"%2",std::to_string(reputation.reverse_database_id));
        stringreplaceOne(queryText,"%3",std::to_string(playerReputation->point));
        stringreplaceOne(queryText,"%4",std::to_string(playerReputation->level));
        dbQueryWriteCommon(queryText);
    }
    else
    {
        std::string queryText=PreparedDBQueryCommon::db_query_update_reputation;
        stringreplaceOne(queryText,"%1",std::to_string(character_id));
        stringreplaceOne(queryText,"%2",std::to_string(reputation.reverse_database_id));
        stringreplaceOne(queryText,"%3",std::to_string(playerReputation->point));
        stringreplaceOne(queryText,"%4",std::to_string(playerReputation->level));
        dbQueryWriteCommon(queryText);
    }*/
    syncDatabaseReputation();
}

bool Client::haveReputationRequirements(const std::vector<ReputationRequirements> &reputationList) const
{
    unsigned int index=0;
    while(index<reputationList.size())
    {
        const CatchChallenger::ReputationRequirements &reputationRequierement=reputationList.at(index);
        if(public_and_private_informations.reputation.find(reputationRequierement.reputationId)!=public_and_private_informations.reputation.cend())
        {
            const PlayerReputation &playerReputation=public_and_private_informations.reputation.at(reputationRequierement.reputationId);
            if(!reputationRequierement.positif)
            {
                if(-reputationRequierement.level<playerReputation.level)
                {
                    normalOutput("reputation.level("+std::to_string(reputationRequierement.level)+")<playerReputation.level("+std::to_string(playerReputation.level)+")");
                    return false;
                }
            }
            else
            {
                if(reputationRequierement.level>playerReputation.level || playerReputation.point<0)
                {
                    normalOutput("reputation.level("+std::to_string(reputationRequierement.level)+
                                 ")>playerReputation.level("+std::to_string(playerReputation.level)+
                                 ") || playerReputation.point("+std::to_string(playerReputation.point)+")<0");
                    return false;
                }
            }
        }
        else
            if(!reputationRequierement.positif)//default level is 0, but required level is negative
            {
                normalOutput("reputation.level("+std::to_string(reputationRequierement.level)+
                             ")<0 and no reputation.type="+CommonDatapack::commonDatapack.get_reputation().at(reputationRequierement.reputationId).name);
                return false;
            }
        index++;
    }
    return true;
}
