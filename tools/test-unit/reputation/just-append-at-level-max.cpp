#include "TestUnitReputation.h"

using namespace CatchChallenger;

void TestUnitReputation::reputationJustAppendAtLevelMax()
{
    {
        Reputation reputation;
        reputation.reputation_positive << 0 << 150 << 10000;
        reputation.reputation_negative << -1 << -150 << -10000;
        PlayerReputation playerReputation;
        playerReputation.level=2;
        playerReputation.point=0;
        FacilityLib::appendReputationPoint(&playerReputation,50,reputation);
        if(playerReputation.level==2 && playerReputation.point==0)
            qDebug() << "FacilityLib::appendReputationPoint just append +50 at level Max: Ok";
        else
        {
            qDebug() << "FacilityLib::appendReputationPoint just append +50 at level Max: Failed";
            finalResult=false;
        }
    }
    {
        Reputation reputation;
        reputation.reputation_positive << 0 << 150 << 10000;
        reputation.reputation_negative << -1 << -150 << -10000;
        PlayerReputation playerReputation;
        playerReputation.level=2;
        playerReputation.point=50;
        FacilityLib::appendReputationPoint(&playerReputation,-30,reputation);
        if(playerReputation.level==2 && playerReputation.point==0)
            qDebug() << "FacilityLib::appendReputationPoint just append -30 at level Max: Ok";
        else
        {
            qDebug() << "FacilityLib::appendReputationPoint just append -30 at level Max: Failed";
            finalResult=false;
        }
    }
    {
        Reputation reputation;
        reputation.reputation_positive << 0 << 150 << 10000;
        reputation.reputation_negative << -1 << -150 << -10000;
        PlayerReputation playerReputation;
        playerReputation.level=-2;
        playerReputation.point=-50;
        FacilityLib::appendReputationPoint(&playerReputation,-10,reputation);
        if(playerReputation.level==-2 && playerReputation.point==0)
            qDebug() << "FacilityLib::appendReputationPoint just append negative -50 at level -Max: Ok";
        else
        {
            qDebug() << "FacilityLib::appendReputationPoint just append negative -50 at level -Max: Failed";
            finalResult=false;
        }
    }
    {
        Reputation reputation;
        reputation.reputation_positive << 0 << 150 << 10000;
        reputation.reputation_negative << -1 << -150 << -10000;
        PlayerReputation playerReputation;
        playerReputation.level=-2;
        playerReputation.point=-50;
        FacilityLib::appendReputationPoint(&playerReputation,30,reputation);
        if(playerReputation.level==-2 && playerReputation.point==0)
            qDebug() << "FacilityLib::appendReputationPoint just append negative +30 at level -Max: Ok";
        else
        {
            qDebug() << "FacilityLib::appendReputationPoint just append negative +30 at level -Max: Failed";
            finalResult=false;
        }
    }
} 
