#ifndef CATCHCHALLENGER_FACILITYLIB_GATEWAY_H
#define CATCHCHALLENGER_FACILITYLIB_GATEWAY_H

#include <string>
#include <vector>

namespace CatchChallenger {
class FacilityLibGateway
{
public:
    static bool mkpath(const std::string &dir);
    static bool dolocalfolder(const std::string &dir);
};
}

#endif
