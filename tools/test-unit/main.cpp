#include <QCoreApplication>
#include <QDebug>
#include "reputation/TestUnitReputation.h"
#include "TestUnitCpp.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    //test FacilityLib::appendReputationPoint
    {
        TestUnitReputation testUnitReputation;

        testUnitReputation.finalResult=true;
        testUnitReputation.reputationJustAppend();
        testUnitReputation.reputationJustAppendAtLevel1();
        testUnitReputation.reputationJustAppendAtLevelMax();
        testUnitReputation.reputationLevelUp();
        testUnitReputation.reputationLevelDown();

        if(!testUnitReputation.finalResult)
        {
            qDebug() << "Final result: Failed: " << __LINE__;
            return EXIT_FAILURE;
        }
    }


    {
        TestUnitCpp testUnitCpp;

        testUnitCpp.finalResult=true;
        testUnitCpp.testHexaToBinary();

        if(!testUnitCpp.finalResult)
        {
            qDebug() << "Final result: Failed: " << __LINE__;
            return EXIT_FAILURE;
        }
    }

    {
        qDebug() << "Final result: Ok";
        return EXIT_SUCCESS;
    }
}
