#include "TestUnitReputation.h"

using namespace CatchChallenger;

void TestUnitReputation::reputationLevelDown()
{
    {
        Reputation reputation;
        reputation.reputation_positive={0,150,10000};
        reputation.reputation_negative={-1,-150,-10000};
        PlayerReputation playerReputation;
        playerReputation.level=2;
        playerReputation.point=0;
        FacilityLib::appendReputationPoint(&playerReputation,-50,reputation);
        if(playerReputation.level==1 && playerReputation.point==(10000-50))
            qDebug() << "FacilityLib::appendReputationPoint level down from 2: Ok";
        else
        {
            qDebug() << "FacilityLib::appendReputationPoint level down from 2: Failed";
            finalResult=false;
        }
    }
    {
        Reputation reputation;
        reputation.reputation_positive={0,150,10000};
        reputation.reputation_negative={-1,-150,-10000};
        PlayerReputation playerReputation;
        playerReputation.level=1;
        playerReputation.point=9950;
        FacilityLib::appendReputationPoint(&playerReputation,-10000,reputation);
        if(playerReputation.level==0 && playerReputation.point==100)
            qDebug() << "FacilityLib::appendReputationPoint level down from -1: Ok";
        else
        {
            qDebug() << "FacilityLib::appendReputationPoint level down from -1: Failed";
            finalResult=false;
        }
    }
    {
        Reputation reputation;
        reputation.reputation_positive={0,150,10000};
        reputation.reputation_negative={-1};
        PlayerReputation playerReputation;
        playerReputation.level=0;
        playerReputation.point=100;
        FacilityLib::appendReputationPoint(&playerReputation,-200,reputation);
        if(playerReputation.level==0 && playerReputation.point==0)
            qDebug() << "FacilityLib::appendReputationPoint level down from -1 blocking 1: Ok";
        else
        {
            qDebug() << "FacilityLib::appendReputationPoint level down from -1 blocking 1: Failed";
            finalResult=false;
        }
    }
    {
        Reputation reputation;
        reputation.reputation_positive={0,150,10000};
        PlayerReputation playerReputation;
        playerReputation.level=0;
        playerReputation.point=100;
        FacilityLib::appendReputationPoint(&playerReputation,-200,reputation);
        if(playerReputation.level==0 && playerReputation.point==0)
            qDebug() << "FacilityLib::appendReputationPoint level down from -1 blocking 2: Ok";
        else
        {
            qDebug() << "FacilityLib::appendReputationPoint level down from -1 blocking 2: Failed";
            finalResult=false;
        }
    }
    {
        Reputation reputation;
        reputation.reputation_positive={0,150,10000};
        reputation.reputation_negative={-1,-150,-10000};
        PlayerReputation playerReputation;
        playerReputation.level=0;
        playerReputation.point=100;
        FacilityLib::appendReputationPoint(&playerReputation,-200,reputation);
        if(playerReputation.level==0 && playerReputation.point==-100)
            qDebug() << "FacilityLib::appendReputationPoint level down from -1: Ok";
        else
        {
            qDebug() << "FacilityLib::appendReputationPoint level down from -1: Failed";
            finalResult=false;
        }
    }
    {
        Reputation reputation;
        reputation.reputation_positive={0,150,10000};
        reputation.reputation_negative={-1,-150,-10000};
        PlayerReputation playerReputation;
        playerReputation.level=0;
        playerReputation.point=-100;
        FacilityLib::appendReputationPoint(&playerReputation,-100,reputation);
        if(playerReputation.level==-1 && playerReputation.point==-50)
            qDebug() << "FacilityLib::appendReputationPoint level down from 0: Ok";
        else
        {
            qDebug() << "FacilityLib::appendReputationPoint level down from 0: Failed";
            finalResult=false;
        }
    }
    {
        Reputation reputation;
        reputation.reputation_positive={0,150,10000};
        reputation.reputation_negative={-1,-150,-10000};
        PlayerReputation playerReputation;
        playerReputation.level=-1;
        playerReputation.point=-9950;
        FacilityLib::appendReputationPoint(&playerReputation,-200,reputation);
        if(playerReputation.level==-2 && playerReputation.point==0)
            qDebug() << "FacilityLib::appendReputationPoint level down from 1a: Ok";
        else
        {
            qDebug() << "FacilityLib::appendReputationPoint level down from 1a: Failed";
            finalResult=false;
        }
    }
    {
        Reputation reputation;
        reputation.reputation_positive={0,150,10000};
        reputation.reputation_negative={-1,-150,-10000};
        PlayerReputation playerReputation;
        playerReputation.level=-1;
        playerReputation.point=-9950;
        FacilityLib::appendReputationPoint(&playerReputation,-50,reputation);
        if(playerReputation.level==-2 && playerReputation.point==0)
            qDebug() << "FacilityLib::appendReputationPoint level down from 1b: Ok";
        else
        {
            qDebug() << "FacilityLib::appendReputationPoint level down from 1b: Failed";
            finalResult=false;
        }
    }
}
