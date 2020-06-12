#ifndef CATCHCHALLENGER_FACILITYLIBClient_H
#define CATCHCHALLENGER_FACILITYLIBClient_H

#include <QRect>
#include "../../general/base/GeneralStructures.hpp"

namespace CatchChallenger {
class FacilityLibClient
{
public:
    static std::string timeToString(const uint32_t &sec);
    static bool rectTouch(QRect r1,QRect r2);
    static std::string reputationRequirementsToText(const CatchChallenger::ReputationRequirements &reputationRequirements);
};

}

#endif
