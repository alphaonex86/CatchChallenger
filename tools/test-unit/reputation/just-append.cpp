#include "TestUnitReputation.h"

using namespace CatchChallenger;

void TestUnitReputation::reputationJustAppend()
{
    {
        Reputation reputation;
        reputation.reputation_positive={0,150,10000};
        reputation.reputation_negative={-1,-150,-10000};
        PlayerReputation playerReputation;
        playerReputation.level=0;
        playerReputation.point=0;
        FacilityLib::appendReputationPoint(&playerReputation,50,reputation);
        if(playerReputation.level==0 && playerReputation.point==50)
            qDebug() << "FacilityLib::appendReputationPoint just append +50: Ok";
        else
        {
            qDebug() << "FacilityLib::appendReputationPoint just append +50: Failed";
            finalResult=false;
        }
    }
    {
        Reputation reputation;
        reputation.reputation_positive={0,150,10000};
        reputation.reputation_negative={-1,-150,-10000};
        PlayerReputation playerReputation;
        playerReputation.level=0;
        playerReputation.point=50;
        FacilityLib::appendReputationPoint(&playerReputation,-30,reputation);
        if(playerReputation.level==0 && playerReputation.point==20)
            qDebug() << "FacilityLib::appendReputationPoint just append -30: Ok";
        else
        {
            qDebug() << "FacilityLib::appendReputationPoint just append -30: Failed";
            finalResult=false;
        }
    }
    {
        Reputation reputation;
        reputation.reputation_positive={0,150,10000};
        reputation.reputation_negative={-1,-150,-10000};
        PlayerReputation playerReputation;
        playerReputation.level=0;
        playerReputation.point=-50;
        FacilityLib::appendReputationPoint(&playerReputation,-10,reputation);
        if(playerReputation.level==0 && playerReputation.point==-60)
            qDebug() << "FacilityLib::appendReputationPoint just append negative -50: Ok";
        else
        {
            qDebug() << "FacilityLib::appendReputationPoint just append negative -50: Failed";
            finalResult=false;
        }
    }
    {
        Reputation reputation;
        reputation.reputation_positive={0,150,10000};
        reputation.reputation_negative={-1,-150,-10000};
        PlayerReputation playerReputation;
        playerReputation.level=0;
        playerReputation.point=-50;
        FacilityLib::appendReputationPoint(&playerReputation,30,reputation);
        if(playerReputation.level==0 && playerReputation.point==-20)
            qDebug() << "FacilityLib::appendReputationPoint just append negative +30: Ok";
        else
        {
            qDebug() << "FacilityLib::appendReputationPoint just append negative +30: Failed";
            finalResult=false;
        }
    }
}
