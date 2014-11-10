#include <QCoreApplication>
#include <QDebug>
#include "reputation/TestUnitReputation.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    TestUnitReputation testUnitReputation;
    //test FacilityLib::appendReputationPoint
    {
        testUnitReputation.finalResult=true;
        testUnitReputation.reputationJustAppend();
        testUnitReputation.reputationJustAppendAtLevel1();
        testUnitReputation.reputationJustAppendAtLevelMax();
        testUnitReputation.reputationLevelUp();
        testUnitReputation.reputationLevelDown();
    }
    if(testUnitReputation.finalResult)
    {
        qDebug() << "Final result: Ok";
        return EXIT_SUCCESS;
    }
    else
    {
        qDebug() << "Final result: Failed";
        return EXIT_FAILURE;
    }
}
