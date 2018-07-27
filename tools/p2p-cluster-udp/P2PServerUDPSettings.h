#ifndef P2PSERVERUDPSETTINGS_H
#define P2PSERVERUDPSETTINGS_H

#include "../../server/base/TinyXMLSettings.h"
#include "P2PServerUDP.h"
#include <unordered_set>

class P2PServerUDPSettings : public P2PServerUDP
{
public:
    P2PServerUDPSettings(uint8_t *privatekey/*ED25519_KEY_SIZE*/, uint8_t *ca_publickey/*ED25519_KEY_SIZE*/, uint8_t *ca_signature/*ED25519_SIGNATURE_SIZE*/);
    static void genPrivateKey(uint8_t *privatekey);

    TinyXMLSettings settings;
    std::vector<P2PServerUDP::HostToConnect> hostToConnectTemp;
    uint16_t port;
protected:
    bool newPeer(const sockaddr_in &si_other);
private:
    std::unordered_set<std::string> knownPeers;
};

#endif // P2PSERVERUDPSETTINGS_H
