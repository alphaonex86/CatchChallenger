#ifndef EPOLLCLIENTLOGINMASTER_H
#define EPOLLCLIENTLOGINMASTER_H

#include "../epoll/EpollClient.h"
#include "../epoll/EpollSslClient.h"
#include "../../general/base/ProtocolParsing.h"
#include "../epoll/db/EpollPostgresql.h"
#include "LoginLinkToMaster.h"

#include <QString>

#define BASE_PROTOCOL_MAGIC_SIZE 8
#define TOKEN_SIZE 64
#define CATCHCHALLENGER_SERVER_MAXNOTLOGGEDCONNECTION 50 //smaller = more performance, but less simmulaneous connexion try, this number * (CATCHCHALLENGER_TOKENSIZE+pointer)
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
    struct TokenLink
    {
        void * client;
        char value[CATCHCHALLENGER_TOKENSIZE];
    };
    void parseIncommingData();

    char *socketString;
    int socketStringSize;

    static LoginLinkToMaster *linkToMaster;
    static char private_token[TOKEN_SIZE];
    static QList<unsigned int> maxAccountIdList;
    static QList<unsigned int> maxCharacterIdList;
    static QList<unsigned int> maxClanIdList;
    static bool maxAccountIdRequested;
    static bool maxCharacterIdRequested;
    static bool maxMonsterIdRequested;
    static char maxAccountIdRequest[4];
    static char maxCharacterIdRequest[4];
    static char maxMonsterIdRequest[4];
    static char replyToRegisterLoginServer[1024];
    static int replyToRegisterLoginServerBaseOffset;
private:
    QList<DatabaseBase::CallBack *> callbackRegistred;
    QList<void *> paramToPassToCallBack;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    QStringList paramToPassToCallBackType;
    #endif

    quint8 accountCharatersCount;
    bool have_send_protocol;
    bool is_logging_in_progess;
    unsigned int account_id;

    static unsigned char protocolReplyProtocolNotSupported[4];
    static unsigned char protocolReplyServerFull[4];
    static unsigned char protocolReplyCompressionNone[4+CATCHCHALLENGER_TOKENSIZE];
    static unsigned char protocolReplyCompresssionZlib[4+CATCHCHALLENGER_TOKENSIZE];
    static unsigned char protocolReplyCompressionXz[4+CATCHCHALLENGER_TOKENSIZE];

    static unsigned char loginInProgressBuffer[4];
    static unsigned char loginIsWrongBuffer[4];

    static const unsigned char protocolHeaderToMatch[5];
    BaseClassSwitch::Type getType() const;
private:
    void parseNetworkReadError(const QString &errorString);

    void errorParsingLayer(const QString &error);
    void messageParsingLayer(const QString &message) const;
    void errorParsingLayer(const char * const error);
    void messageParsingLayer(const char * const message) const;
    //have message without reply
    void parseMessage(const quint8 &mainCodeType,const char *data,const int &size);
    void parseFullMessage(const quint8 &mainCodeType,const quint16 &subCodeType,const char *data,const int &size);
    //have query with reply
    void parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size);
    void parseFullQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char *data,const int &size);
    //send reply
    void parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size);
    void parseFullReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char *data,const int &size);

    void parseInputBeforeLogin(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size);
    void disconnectClient();

    void askLogin(const quint8 &query_id,const char *rawdata);
    static void askLogin_static(void *object);
    void askLogin_object();
    void askLogin_return(AskLoginParam *askLoginParam);
    void createAccount(const quint8 &query_id, const char *rawdata);
    static void createAccount_static(void *object);
    void createAccount_object();
    void createAccount_return(AskLoginParam *askLoginParam);
    static void character_list_static(void *object);
    void character_list_object();
    void character_list_return(const quint8 &query_id);
    void deleteCharacterNow(const quint32 &characterId);
    static void deleteCharacterNow_static(void *object);
    void deleteCharacterNow_object();
    void deleteCharacterNow_return(const quint32 &characterId);
    void addCharacter(const quint8 &query_id, const quint8 &profileIndex, const QString &pseudo, const quint8 &skinId);
    static void addCharacter_static(void *object);
    void addCharacter_object();
    void addCharacter_return(const quint8 &query_id,const quint8 &profileIndex,const QString &pseudo,const quint8 &skinId);
    void removeCharacter(const quint8 &query_id, const quint32 &characterId);
    static void removeCharacter_static(void *object);
    void removeCharacter_object();
    void removeCharacter_return(const quint8 &query_id,const quint32 &characterId);
    void dbQueryWriteLogin(const char * const queryText);
    void dbQueryWriteCommon(const char * const queryText);

    void sendFullPacket(const quint8 &mainIdent,const quint16 &subIdent,const char *data=NULL,const int &size=0);
    void sendPacket(const quint8 &mainIdent,const char *data=NULL,const int &size=0);
    void sendRawSmallPacket(const char *data,const int &size);
    void sendQuery(const quint8 &mainIdent,const quint16 &subIdent,const quint8 &queryNumber,const char *data=NULL,const int &size=0);
    void postReply(const quint8 &queryNumber,const char *data=NULL,const int &size=0);

    void loginIsWrong(const quint8 &query_id,const quint8 &returnCode,const QString &debugMessage);
private:
    //connection
    static FILE *fpRandomFile;
    static TokenLink tokenForAuth[CATCHCHALLENGER_SERVER_MAXNOTLOGGEDCONNECTION];
    static quint32 tokenForAuthSize;
public:
    static bool automatic_account_creation;
    static unsigned int character_delete_time;
    static QString httpDatapackMirror;
    static unsigned int min_character;
    static unsigned int max_character;
    static unsigned int max_pseudo_size;
    static EpollPostgresql databaseBaseLogin;
    static EpollPostgresql databaseBaseCommon;
};
}

#endif // EPOLLCLIENTLOGINMASTER_H
