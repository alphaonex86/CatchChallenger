#ifndef EPOLLCLIENTLOGINMASTER_H
#define EPOLLCLIENTLOGINMASTER_H

#include "../epoll/EpollClient.h"
#include "../epoll/EpollSslClient.h"
#include "../../general/base/ProtocolParsing.h"
#include "../VariableServer.h"
#include "../epoll/db/EpollPostgresql.h"
#include "LinkToMaster.h"
#include "LinkToGameServer.h"

#include <QString>
#include <vector>

#define BASE_PROTOCOL_MAGIC_SIZE 8

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

#define CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE 8
#define CATCHCHALLENGER_DDOS_COMPUTERINTERVAL 5
#define CATCHCHALLENGER_DDOS_KICKLIMITMOVE 60
#define CATCHCHALLENGER_DDOS_KICKLIMITCHAT 5
#define CATCHCHALLENGER_DDOS_KICKLIMITOTHER 30

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
    enum EpollClientLoginStat
    {
        None,
        ProtocolGood,
        LoginInProgress,
        Logged,
        CharacterSelecting,
        CharacterSelected,
        GameServerConnecting,
        GameServerConnected,
    };
    EpollClientLoginStat stat;

    void parseIncommingData();
    void doDDOSCompute();
    static void doDDOSComputeAll();

    void selectCharacter_ReturnToken(const quint8 &query_id,const char * const token);
    void selectCharacter_ReturnFailed(const quint8 &query_id,const quint8 &errorCode);
    void addCharacter_ReturnOk(const quint8 &query_id,const quint32 &characterId);
    void addCharacter_ReturnFailed(const quint8 &query_id,const quint8 &errorCode);
    void removeCharacter_ReturnOk(const quint8 &query_id);
    void removeCharacter_ReturnFailed(const quint8 &query_id,const quint8 &errorCode,const QString &errorString=QString());
    void parseNetworkReadError(const QString &errorString);

    LinkToGameServer *linkToGameServer;
    quint8 charactersGroupIndex;
    quint32 serverUniqueKey;
    char *socketString;
    int socketStringSize;
    unsigned int account_id;
    /** \warning Need be checked in real time because can be opened on multiple login server
     * quint8 accountCharatersCount; */

    static QList<unsigned int> maxAccountIdList;
    static QList<unsigned int> maxCharacterIdList;
    static QList<unsigned int> maxClanIdList;
    static bool maxAccountIdRequested;
    static bool maxCharacterIdRequested;
    static bool maxMonsterIdRequested;
    static char maxAccountIdRequest[3];
    static char maxCharacterIdRequest[3];
    static char maxMonsterIdRequest[3];
    static char selectCharaterRequestOnMaster[3/*header*/+1+4+4+4];
    static char replyToRegisterLoginServerCharactersGroup[1024];
    static unsigned int replyToRegisterLoginServerCharactersGroupSize;
    static char baseDatapackSum[28];
    static char loginGood[256*1024];
    static unsigned int loginGoodSize;

    static char serverServerList[256*1024];
    static unsigned int serverServerListSize;
    static unsigned int serverServerListCurrentPlayerSize;
    static char serverServerListComputedMessage[256*1024];
    static unsigned int serverServerListComputedMessageSize;
    static char serverLogicalGroupList[256*1024];
    static unsigned int serverLogicalGroupListSize;
    static char serverLogicalGroupAndServerList[512*1024];
    static unsigned int serverLogicalGroupAndServerListSize;

    static unsigned char loginIsWrongBufferReply[4];

    static char characterSelectionIsWrongBufferCharacterNotFound[64];
    static char characterSelectionIsWrongBufferCharacterAlreadyConnectedOnline[64];
    static char characterSelectionIsWrongBufferServerInternalProblem[64];
    static char characterSelectionIsWrongBufferServerNotFound[64];
    static quint8 characterSelectionIsWrongBufferSize;
private:
    QList<DatabaseBase::CallBack *> callbackRegistred;
    QList<void *> paramToPassToCallBack;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    QStringList paramToPassToCallBackType;
    #endif

    static unsigned char protocolReplyProtocolNotSupported[4];
    static unsigned char protocolReplyServerFull[4];
    static unsigned char protocolReplyCompressionNone[4+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
    static unsigned char protocolReplyCompresssionZlib[4+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];
    static unsigned char protocolReplyCompressionXz[4+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT];

    static unsigned char loginInProgressBuffer[4];
    static unsigned char addCharacterIsWrongBuffer[8];
    static char loginCharacterList[1024];
    static unsigned char addCharacterReply[3+1+4];
    static unsigned char removeCharacterReply[3+1];

    static const unsigned char protocolHeaderToMatch[5];
    BaseClassSwitch::EpollObjectType getType() const;
private:
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

    void sendFullPacket(const quint8 &mainIdent,const quint8 &subIdent,const char * const data, const unsigned int &size);
    void sendFullPacket(const quint8 &mainIdent,const quint8 &subIdent);
    void sendPacket(const quint8 &mainIdent, const char * const data, const unsigned int &size);
    void sendPacket(const quint8 &mainIdent);
    void sendRawSmallPacket(const char * const data, const unsigned int &size);
    void sendQuery(const quint8 &mainIdent,const quint8 &subIdent,const quint8 &queryNumber,const char * const data, const unsigned int &size);
    void sendQuery(const quint8 &mainIdent,const quint8 &subIdent,const quint8 &queryNumber);
    void postReply(const quint8 &queryNumber,const char * const data, const unsigned int &size);
    void postReply(const quint8 &queryNumber);
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
    //to be ordered
    QMap<quint8,CharacterListForReply> characterTempListForReply;
    quint8 characterListForReplyInSuspend;
    //by client because is where is merged all servers time reply from all catchchallenger_common*
    char *serverListForReplyRawData;
    unsigned int serverListForReplyRawDataSize;

    quint8 serverListForReplyInSuspend;
    static std::vector<EpollClientLoginSlave *> client_list;

    quint8 movePacketKick[CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE];
    quint8 movePacketKickSize;
    quint8 movePacketKickTotalCache;
    quint8 movePacketKickNewValue;
    quint8 chatPacketKick[CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE];
    quint8 chatPacketKickSize;
    quint8 chatPacketKickTotalCache;
    quint8 chatPacketKickNewValue;
    quint8 otherPacketKick[CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE];
    quint8 otherPacketKickSize;
    quint8 otherPacketKickTotalCache;
    quint8 otherPacketKickNewValue;
public:
    static EpollPostgresql databaseBaseLogin;
    static EpollPostgresql databaseBaseCommon;
};
}

#endif // EPOLLCLIENTLOGINMASTER_H
