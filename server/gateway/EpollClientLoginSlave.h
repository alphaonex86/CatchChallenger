#ifndef EPOLLCLIENTLOGINMASTER_H
#define EPOLLCLIENTLOGINMASTER_H

#include "../epoll/EpollClient.h"
#include "../epoll/EpollSslClient.h"
#include "../../general/base/ProtocolParsing.h"
#include "../VariableServer.h"
#include "../epoll/db/EpollPostgresql.h"
#include "LinkToGameServer.h"

#include <QString>
#include <vector>
#include <QRegularExpression>

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
    enum EpollClientLoginStat
    {
        None,
        ProtocolGood,
        GameServerConnecting,
        GameServerConnected,
    };
    EpollClientLoginStat stat;
    enum DatapackStatus
    {
        Base=0x01,
        Main=0x02,
        Sub=0x03,
        Finished=0x04
    };
    DatapackStatus datapackStatus;
    struct FileToSend
    {
        //not QFile * to prevent too many file open
        QString file;
    };

    void parseIncommingData();
    void doDDOSCompute();
    static void doDDOSComputeAll();

    void parseNetworkReadError(const QString &errorString);

    LinkToGameServer *linkToGameServer;
    char *socketString;
    int socketStringSize;

    struct DatapackCacheFile
    {
        //quint32 mtime;
        quint32 partialHash;
    };
    struct DatapackData
    {
        //quint32 mtime;
        QHash<QString,DatapackCacheFile> datapack_file_hash_cache;
    };
    //static QHash<QString,quint32> datapack_file_list_cache_base,datapack_file_list_cache_main,datapack_file_list_cache_sub;//same than above
    static DatapackData datapack_file_base;
    static QHash<QString,DatapackData> datapack_file_main;
    static QHash<QString,QHash<QString,DatapackData> > datapack_file_sub;
    static QHash<QString,DatapackCacheFile> datapack_file_list(const QString &path,const bool withHash=true);
private:
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    QStringList paramToPassToCallBackType;
    #endif

    static const unsigned char protocolHeaderToMatch[5];
    BaseClassSwitch::EpollObjectType getType() const;
private:
    void errorParsingLayer(const QString &error);
    void messageParsingLayer(const QString &message) const;
    void errorParsingLayer(const char * const error);
    void messageParsingLayer(const char * const message) const;
    //have message without reply
    void parseMessage(const quint8 &mainCodeType,const char * const data,const unsigned int &size);
    void parseFullMessage(const quint8 &mainCodeType,const quint8 &subCodeType,const char * const data,const unsigned int &size);
    //have query with reply
    void parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size);
    void parseFullQuery(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size);
    //send reply
    void parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size);
    void parseFullReplyData(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size);

    void parseInputBeforeLogin(const quint8 &mainCodeType, const quint8 &queryNumber, const char * const data, const unsigned int &size);
    void disconnectClient();

    bool sendFile(const QString &fileName);
    void datapackList(const quint8 &query_id, const QStringList &files, const QList<quint32> &partialHashList);

    void addDatapackListReply(const bool &fileRemove);
    void purgeDatapackListReply(const quint8 &query_id);
    void sendFileContent();
    void sendCompressedFileContent();
public:
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

    static quint8 tempDatapackListReplySize;
    static QByteArray tempDatapackListReplyArray;
    static quint8 tempDatapackListReply;
    static int tempDatapackListReplyTestCount;
    static QByteArray rawFilesBuffer,compressedFilesBuffer;
    static int rawFilesBufferCount,compressedFilesBufferCount;

    static QString text_dotslash;
    static QString text_antislash;
    static QString text_double_slash;
    static QString text_slash;

    static QRegularExpression fileNameStartStringRegex;
    static QRegularExpression datapack_rightFileName;
};
}

#endif // EPOLLCLIENTLOGINMASTER_H
