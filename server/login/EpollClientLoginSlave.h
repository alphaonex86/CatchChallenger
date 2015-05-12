#ifndef EPOLLCLIENTLOGINMASTER_H
#define EPOLLCLIENTLOGINMASTER_H

#include "../epoll/EpollClient.h"
#include "../epoll/EpollSslClient.h"
#include "../../general/base/ProtocolParsing.h"
#include "../VariableServer.h"
#include "../epoll/db/EpollPostgresql.h"
#include "LoginLinkToMaster.h"

#include <QString>

#define BASE_PROTOCOL_MAGIC_SIZE 8
#define TOKEN_SIZE 64
#define CATCHCHALLENGER_SERVER_MINIDBLOCK 20
#define CATCHCHALLENGER_SERVER_MAXIDBLOCK 50

#ifdef EPOLLCATCHCHALLENGERSERVER
    #if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_SERVER_DATAPACK_MIN_FILEPURGE_KB*1024
    #error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_SERVER_DATAPACK_MIN_FILEPURGE_KB
    #endif
    #if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_SERVER_DATAPACK_MAX_FILEPURGE_KB*1024
    #error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_SERVER_DATAPACK_MAX_FILEPURGE_KB
    #endif
    #if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_SERVER_DATAPACK_ZLIB_COMPRESSEDFILEPURGE_KB*1024
    #error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_SERVER_DATAPACK_ZLIB_COMPRESSEDFILEPURGE_KB
    #endif
    #if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_SERVER_DATAPACK_XZ_COMPRESSEDFILEPURGE_KB*1024
    #error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_SERVER_DATAPACK_XZ_COMPRESSEDFILEPURGE_KB
    #endif
    #if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_SERVER_DATAPACK_DONT_COMPRESS_GREATER_THAN_KB*1024
    #error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_SERVER_DATAPACK_DONT_COMPRESS_GREATER_THAN_KB
    #endif
#endif

namespace CatchChallenger {
class EpollClientLoginSlave : public BaseClassSwitch, public ProtocolParsingInputOutput
{
public:
    EpollClientLoginSlave(
        #ifdef SERVERSSL
            const int &infd, SSL_CTX *ctx
        #else
            const int &infd
        #endif
        );
    ~EpollClientLoginSlave();
    struct AddCharacterParam
    {
        quint8 query_id;
        quint8 profileIndex;
        QString pseudo;
        quint8 skinId;
    };
    struct RemoveCharacterParam
    {
        quint8 query_id;
        quint32 characterId;
    };
    struct DeleteCharacterNow
    {
        quint32 characterId;
    };
    struct AskLoginParam
    {
        quint8 query_id;
        QByteArray login;
        QByteArray pass;
    };
    struct SelectCharacterParam
    {
        quint8 query_id;
        quint32 characterId;
    };
    struct SelectIndexParam
    {
        quint32 index;
    };
    enum ProxyMode
    {
        Reconnect=0x01,
        Proxy=0x02
    };
    static ProxyMode proxyMode;

    void parseIncommingData();

    void selectCharacter_ReturnToken(const quint8 &query_id,const char * const token);
    void selectCharacter_ReturnFailed(const quint8 &query_id,const quint8 &errorCode);
    void addCharacter_ReturnOk(const quint8 &query_id,const quint32 &characterId);
    void addCharacter_ReturnFailed(const quint8 &query_id,const quint8 &errorCode);
    void removeCharacter_ReturnOk(const quint8 &query_id);
    void removeCharacter_ReturnFailed(const quint8 &query_id,const quint8 &errorCode);

    char *socketString;
    int socketStringSize;
    unsigned int account_id;
    quint8 accountCharatersCount;

    static LoginLinkToMaster *linkToMaster;
    static char private_token[TOKEN_SIZE];
    static QList<unsigned int> maxAccountIdList;
    static QList<unsigned int> maxCharacterIdList;
    static QList<unsigned int> maxClanIdList;
    static bool maxAccountIdRequested;
    static bool maxCharacterIdRequested;
    static bool maxMonsterIdRequested;
    static char maxAccountIdRequest[3];
    static char maxCharacterIdRequest[3];
    static char maxMonsterIdRequest[3];
    static char selectCharaterRequest[3+4+1+4];
    static char replyToRegisterLoginServerCharactersGroup[1024];
    static int replyToRegisterLoginServerCharactersGroupSize;
    static char baseDatapackSum[28];
    static char loginGood[256*1024];
    static int loginGoodSize;

    static char serverPartialServerList[256*1024];
    static int serverPartialServerListSize;
    static char serverServerList[256*1024];
    static int serverServerListSize;
    static char serverLogicalGroupList[256*1024];
    static int serverLogicalGroupListSize;
    static char serverLogicalGroupAndServerList[512*1024];
    static int serverLogicalGroupAndServerListSize;
private:
    QList<DatabaseBase::CallBack *> callbackRegistred;
    QList<void *> paramToPassToCallBack;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    QStringList paramToPassToCallBackType;
    #endif

    bool have_send_protocol;
    bool is_logging_in_progess;

    static unsigned char protocolReplyProtocolNotSupported[4];
    static unsigned char protocolReplyServerFull[4];
    static unsigned char protocolReplyCompressionNone[4+CATCHCHALLENGER_TOKENSIZE];
    static unsigned char protocolReplyCompresssionZlib[4+CATCHCHALLENGER_TOKENSIZE];
    static unsigned char protocolReplyCompressionXz[4+CATCHCHALLENGER_TOKENSIZE];

    static unsigned char loginInProgressBuffer[4];
    static unsigned char loginIsWrongBuffer[4];
    static unsigned char addCharacterIsWrongBuffer[8];
    static char loginCharacterList[1024];
    static unsigned char addCharacterReply[3+1+4];
    static unsigned char removeCharacterReply[3+1];

    static const unsigned char protocolHeaderToMatch[5];
    BaseClassSwitch::Type getType() const;
private:
    void parseNetworkReadError(const QString &errorString);

    void errorParsingLayer(const QString &error);
    void messageParsingLayer(const QString &message) const;
    void errorParsingLayer(const char * const error);
    void messageParsingLayer(const char * const message) const;
    //have message without reply
    void parseMessage(const quint8 &mainCodeType,const char *data,const unsigned int &size);
    void parseFullMessage(const quint8 &mainCodeType,const quint8 &subCodeType,const char *data,const unsigned int &size);
    //have query with reply
    void parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const unsigned int &size);
    void parseFullQuery(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char *data,const unsigned int &size);
    //send reply
    void parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const unsigned int &size);
    void parseFullReplyData(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char *data,const unsigned int &size);

    void parseInputBeforeLogin(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const unsigned int &size);
    void disconnectClient();

public:
    void askLogin_cancel();
    void askLogin(const quint8 &query_id,const char *rawdata);
    static void askLogin_static(void *object);
    void askLogin_object();
    void askLogin_return(AskLoginParam *askLoginParam);
    void createAccount(const quint8 &query_id, const char *rawdata);
    static void createAccount_static(void *object);
    void createAccount_object();
    void createAccount_return(AskLoginParam *askLoginParam);

    void character_list_return(const quint8 &characterGroupIndex,char * const tempRawData,const int &tempRawDataSize);
    void server_list_return(const quint8 &serverCount,char * const tempRawData,const int &tempRawDataSize);

    void sendFullPacket(const quint8 &mainIdent,const quint16 &subIdent,const char *data=NULL,const int &size=0);
    void sendPacket(const quint8 &mainIdent,const char *data=NULL,const int &size=0);
    void sendRawSmallPacket(const char *data,const int &size);
    void sendQuery(const quint8 &mainIdent,const quint16 &subIdent,const quint8 &queryNumber,const char *data=NULL,const int &size=0);
    void postReply(const quint8 &queryNumber,const char *data=NULL,const int &size=0);
    void characterSelectionIsWrong(const quint8 &query_id,const quint8 &returnCode,const QString &debugMessage);
private:
    void deleteCharacterNow(const quint32 &characterId);
    void addCharacter(const quint8 &query_id, const quint8 &characterGroupIndex, const quint8 &profileIndex, const QString &pseudo, const quint8 &skinId);
    void removeCharacter(const quint8 &query_id, const quint8 &characterGroupIndex, const quint32 &characterId);
    void dbQueryWriteLogin(const char * const queryText);

    void loginIsWrong(const quint8 &query_id,const quint8 &returnCode,const QString &debugMessage);
    void selectCharacter(const quint8 &query_id,const quint32 &serverUniqueKey,const quint8 &charactersGroupIndex,const quint32 &characterId);
private:
    struct CharacterListForReply
    {
        char *rawData;
        int rawDataSize;
    };
    QMap<quint8,CharacterListForReply> characterTempListForReply;
    quint8 characterListForReplyInSuspend;

    char *serverListForReplyRawData;
    int serverListForReplyRawDataSize;
    quint8 serverListForReplyInSuspend;
    quint8 serverPlayedTimeCount;
    AskLoginParam *askLoginParam;
public:
    static bool automatic_account_creation;
    static unsigned int character_delete_time;
    static QString httpDatapackMirror;
    static unsigned int min_character;
    static unsigned int max_character;
    static unsigned int max_pseudo_size;
    static quint8 max_player_monsters;// \warning Never put number greater than 10 here
    static quint16 max_warehouse_player_monsters;
    static quint8 max_player_items;
    static quint16 max_warehouse_player_items;
    static EpollPostgresql databaseBaseLogin;
    static EpollPostgresql databaseBaseCommon;
};
}

#endif // EPOLLCLIENTLOGINMASTER_H
