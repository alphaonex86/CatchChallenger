#ifndef EPOLL_SERVER_LOGIN_MASTER_H
#define EPOLL_SERVER_LOGIN_MASTER_H

#ifndef SERVERSSL

#include "../epoll/EpollGenericServer.h"
#include "../base/BaseServerLogin.h"
#include "../base/TinyXMLSettings.h"
#include "EpollClientLoginSlave.h"

#include <string>
#include <vector>

namespace CatchChallenger {
class EpollServerLoginSlave : public EpollGenericServer, public BaseServerLogin
{
public:
    EpollServerLoginSlave();
    ~EpollServerLoginSlave();
    bool tryListen();
    void close();
    void setSkinPair(const uint8_t &internalId,const uint16_t &databaseId);
    void setProfilePair(const uint8_t &internalId,const uint16_t &databaseId);
    void compose04Reply();
public:
    struct LoginProfile
    {
        struct Reputation
        {
            uint16_t reputationDatabaseId;//datapack order, can can need the dicionary to db resolv
            int8_t level;
            int32_t point;
        };
        struct Monster
        {
            struct Skill
            {
                uint16_t id;
                uint8_t level;
                uint8_t endurance;
            };
            uint16_t id;
            uint8_t level;
            uint16_t captured_with;
            uint32_t hp;
            int8_t ratio_gender;
            std::vector<Skill> skills;
        };
        struct Item
        {
            CATCHCHALLENGER_TYPE_ITEM id;
            uint32_t quantity;
        };
        uint16_t databaseId;
        std::vector<uint8_t> forcedskin;
        uint64_t cash;
        std::vector<Monster> monsters;
        std::vector<Reputation> reputation;
        std::vector<Item> items;
        char * preparedQueryChar;
        uint8_t preparedQueryPos[6];
        uint8_t preparedQuerySize[6];
    };
    std::vector<LoginProfile> loginProfileList;

    static EpollServerLoginSlave *epollServerLoginSlave;
public:
    bool tcpNodelay,tcpCork;
    bool serverReady;
private:
    std::string server_ip;
    std::string server_port;
private:
    void preload_profile();
    void generateToken(TinyXMLSettings &settings);
    void generateTokenStatClient(TinyXMLSettings &settings);
    void SQL_common_load_finish();
};
}

#endif

#endif // EPOLL_SERVER_H
