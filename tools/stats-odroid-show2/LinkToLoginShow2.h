#ifndef LINKTOLOGINSHOW2_H
#define LINKTOLOGINSHOW2_H

#include "../stats/LinkToLogin.h"

class LinkToLoginShow2 : public CatchChallenger::LinkToLogin
{
public:
    explicit LinkToLoginShow2(
        #ifdef SERVERSSL
            const int &infd, SSL_CTX *ctx
        #else
            const int &infd
        #endif
            );
    void updateJsonFile();
    void tryReconnect();
    static LinkToLoginShow2 *linkToLogin;
    unsigned int displayedServerNumber;
    unsigned int displayedPlayer;
    unsigned int displayedMaxPlayer;
    unsigned int displayedMaxServer;
};

#endif // LINKTOLOGINSHOW2_H
