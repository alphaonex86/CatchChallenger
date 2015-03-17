#ifndef LOGINLINKTOMASTER_H
#define LOGINLINKTOMASTER_H

#include "../../general/base/ProtocolParsing.h"

namespace CatchChallenger {
class LoginLinkToMaster : public BaseClassSwitch, public ProtocolParsingInputOutput
{
public:
    LoginLinkToMaster();
};

#endif // LOGINLINKTOMASTER_H
