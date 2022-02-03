#ifndef TESTUNITREPUTATION_H
#define TESTUNITREPUTATION_H

#include <QDebug>

#include "../../../general/base/FacilityLib.h"

class TestUnitReputation
{
public:
    bool finalResult;

    void reputationJustAppend();
    void reputationJustAppendAtLevel1();
    void reputationJustAppendAtLevelMax();
    void reputationLevelUp();
    void reputationLevelDown();
};

#endif // TESTUNITREPUTATION_H
