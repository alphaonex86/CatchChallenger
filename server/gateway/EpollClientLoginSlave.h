#ifndef EPOLLCLIENTLOGINMASTER_H
#define EPOLLCLIENTLOGINMASTER_H

#include "../epoll/EpollClient.h"
#include "../epoll/EpollSslClient.h"
#include "../../general/base/ProtocolParsing.h"
#include "../VariableServer.h"
#include "../epoll/db/EpollPostgresql.h"
#include "LinkToGameServer.h"

#include <string>
#include <unordered_set>
#include <vector>
#include <regex>

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

struct FileToSend
{
    //not QFile * to prevent too many file open
    std::string file;
};
bool operator<(const FileToSend &fileToSend1,const FileToSend &fileToSend2);

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

    void parseIncommingData();
    void doDDOSCompute();
    static void doDDOSComputeAll();

    void parseNetworkReadError(const std::string &errorString);
    bool sendRawSmallPacket(const char * const data,const int &size);
    bool removeFromQueryReceived(const uint8_t &queryNumber);

    bool fastForward;
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
        std::unordered_map<std::string,DatapackCacheFile> datapack_file_hash_cache;
    };
    //static std::unordered_map<std::string,quint32> datapack_file_list_cache_base,datapack_file_list_cache_main,datapack_file_list_cache_sub;//same than above
    static DatapackData datapack_file_base;
    static std::unordered_set<std::string> compressedExtension;
    static std::unordered_map<std::string,DatapackData> datapack_file_main;
    static std::unordered_map<std::string,std::unordered_map<std::string,DatapackData> > datapack_file_sub;
    static std::unordered_map<std::string,DatapackCacheFile> datapack_file_list(const std::string &path,const bool withHash=true);
private:
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    std::vector<std::string> paramToPassToCallBackType;
    #endif

    static const unsigned char protocolHeaderToMatch[5];
    BaseClassSwitch::EpollObjectType getType() const;
private:
    void errorParsingLayer(const std::string &error);
    void messageParsingLayer(const std::string &message) const;
    void errorParsingLayer(const char * const error);
    void messageParsingLayer(const char * const message) const;
    //have message without reply
    bool parseMessage(const quint8 &mainCodeType,const char * const data,const unsigned int &size);
    //have query with reply
    bool parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size);
    //send reply
    bool parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size);

    bool parseInputBeforeLogin(const quint8 &mainCodeType, const quint8 &queryNumber, const char * const data, const unsigned int &size);
    void disconnectClient();

    bool sendFile(const std::string &datapackPath, const std::string &fileName);
    void datapackList(const quint8 &query_id, const std::vector<std::string> &files, const std::vector<quint32> &partialHashList);

    void addDatapackListReply(const bool &fileRemove);
    void purgeDatapackListReply(const quint8 &query_id);
    void sendFileContent();
    void sendCompressedFileContent();
public:
    bool sendRawSmallPacket(const char * const data, const unsigned int &size);
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

    static std::regex fileNameStartStringRegex;
    static std::regex datapack_rightFileName;
};
}

#endif // EPOLLCLIENTLOGINMASTER_H
