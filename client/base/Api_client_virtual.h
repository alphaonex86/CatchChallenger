#ifndef CATCHCHALLENGER_API_CLIENT_VIRTUAL_H
#define CATCHCHALLENGER_API_CLIENT_VIRTUAL_H

#include <string>

#include "../../general/base/ConnectedSocket.h"
#include "Api_protocol.h"

namespace CatchChallenger {
class Api_client_virtual : public Api_protocol
{
public:
    explicit Api_client_virtual(ConnectedSocket *socket);
    ~Api_client_virtual();
    void sendDatapackContentBase(const std::string &hashBase=std::string());
    void sendDatapackContentMainSub(const std::string &hashMain=std::string(),const std::string &hashSub=std::string());
    void tryDisconnect();
protected:
    //general data
    void defineMaxPlayers(const uint16_t &);
};
}

#endif // Protocol_and_virtual_data_H
