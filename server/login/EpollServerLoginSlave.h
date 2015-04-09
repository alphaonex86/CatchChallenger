#ifndef EPOLL_SERVER_LOGIN_MASTER_H
#define EPOLL_SERVER_LOGIN_MASTER_H

#ifndef SERVERSSL

#include "../epoll/EpollGenericServer.h"
#include "../base/BaseServerLogin.h"
#include "EpollClientLoginSlave.h"

#include <QSettings>
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
    void setSkinPair(const quint8 &internalId,const quint16 &databaseId);
    void setProfilePair(const quint8 &internalId,const quint16 &databaseId);
    void compose04Reply();
public:
    struct LoginProfile
    {
        struct Reputation
        {
            quint16 reputationDatabaseId;//datapack order, can can need the dicionary to db resolv
            qint8 level;
            qint32 point;
        };
        struct Monster
        {
            struct Skill
            {
                quint16 id;
                quint8 level;
                quint8 endurance;
            };
            quint16 id;
            quint8 level;
            quint16 captured_with;
            quint32 hp;
            qint8 ratio_gender;
            std::vector<Skill> skills;
        };
        struct Item
        {
            CATCHCHALLENGER_TYPE_ITEM id;
            quint32 quantity;
        };
        quint16 databaseId;
        std::vector<quint8> forcedskin;
        quint64 cash;
        std::vector<Monster> monsters;
        std::vector<Reputation> reputation;
        std::vector<Item> items;
        char * preparedQueryChar;
        quint8 preparedQueryPos[6];
        quint8 preparedQuerySize[6];
    };
    std::vector<LoginProfile> loginProfileList;

    static EpollServerLoginSlave *epollServerLoginSlave;
public:
    bool tcpNodelay,tcpCork;
    bool serverReady;
private:
    char * server_ip;
    char * server_port;
private:
    void preload_profile();
    void generateToken(QSettings &settings);
    void SQL_common_load_finish();
};
}

#endif

#endif // EPOLL_SERVER_H
