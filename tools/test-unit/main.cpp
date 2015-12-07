#include <QCoreApplication>
#include <QDebug>
#include "reputation/TestUnitReputation.h"
#include "TestUnitCpp.h"
#include "TestUnitMessageParsing.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    //test FacilityLib::appendReputationPoint
    {
        TestUnitReputation testUnitReputation;

        testUnitReputation.finalResult=true;
        testUnitReputation.reputationJustAppend();
        if(testUnitReputation.finalResult)
            testUnitReputation.reputationJustAppendAtLevel1();
        if(testUnitReputation.finalResult)
            testUnitReputation.reputationJustAppendAtLevelMax();
        if(testUnitReputation.finalResult)
            testUnitReputation.reputationLevelUp();
        if(testUnitReputation.finalResult)
            testUnitReputation.reputationLevelDown();

        if(!testUnitReputation.finalResult)
        {
            qDebug() << "Final result: Failed: " << __FILE__ << ":" <<  __LINE__;
            return EXIT_FAILURE;
        }
    }


    {
        TestUnitCpp testUnitCpp;

        testUnitCpp.finalResult=true;
        testUnitCpp.testHexaToBinary();
        if(testUnitCpp.finalResult)
            testUnitCpp.testStringSplit();
        if(testUnitCpp.finalResult)
            testUnitCpp.testFSabsoluteFilePath();

        if(!testUnitCpp.finalResult)
        {
            qDebug() << "Final result: Failed: " << __FILE__ << ":" <<  __LINE__;
            return EXIT_FAILURE;
        }
    }

    {
        TestUnitMessageParsing testUnitParsing;

        testUnitParsing.finalResult=true;
        testUnitParsing.testDropAllPlayer();

        if(!testUnitParsing.finalResult)
        {
            qDebug() << "Final result: Failed: " << __FILE__ << ":" <<  __LINE__;
            return EXIT_FAILURE;
        }
    }


    {
        qDebug() << "Final result: Ok";
        return EXIT_SUCCESS;
    }
}
