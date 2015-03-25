#ifndef EPOLL_SERVER_LOGIN_MASTER_H
#define EPOLL_SERVER_LOGIN_MASTER_H

#ifndef SERVERSSL

#include "../epoll/EpollGenericServer.h"
#include "EpollClientLoginMaster.h"
#include "CharactersGroup.h"
#include "../epoll/db/EpollPostgresql.h"
#include "../base/BaseServerCommon.h"

#include <QSettings>

namespace CatchChallenger {
class EpollServerLoginMaster : public EpollGenericServer, public BaseServerCommon
{
public:
    EpollServerLoginMaster();
    ~EpollServerLoginMaster();
    bool tryListen();
private:
    char * server_ip;
    char * server_port;
    char * rawServerListForC20011;
    int rawServerListForC20011Size;

    EpollPostgresql *databaseBaseLogin;

    quint32 character_delete_time;
    quint8 min_character;
    quint8 max_character;
    quint8 max_pseudo_size;

    QByteArray datapackHash;
private:
    void generateToken(QSettings &settings);

    void load_account_max_id();
    static void load_account_max_id_static(void *object);
    void load_account_max_id_return();

    void loadTheDatapack();
    void loadTheDatapackFileList();
    QHash<QString,quint32> datapack_file_list();
    void loadLoginSettings(QSettings &settings);
    void loadDBLoginSettings(QSettings &settings);
    QStringList loadCharactersGroup(QSettings &settings);
    void charactersGroupListReply(QStringList &charactersGroupList);
    void doTheLogicalGroup(QSettings &settings);
    void doTheServerList();
    void doTheReplyCache();
    void loadTheProfile();
    void SQL_common_load_finish();
};
}

#endif

#endif // EPOLL_SERVER_H
