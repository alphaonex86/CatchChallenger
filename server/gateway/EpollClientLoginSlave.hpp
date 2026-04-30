#ifndef EPOLLCLIENTLOGINMASTER_H
#define EPOLLCLIENTLOGINMASTER_H

#include "../epoll/EpollClient.hpp"
#include "../epoll/EpollSslClient.hpp"
#include "../../general/base/ProtocolParsing.hpp"
#include "../base/VariableServer.hpp"
#include "../epoll/db/EpollPostgresql.hpp"
#include "LinkToGameServer.hpp"
#include "../base/DdosBuffer.hpp"

#include <string>
#include <unordered_set>
#include <vector>
#include <regex>

#define BASE_PROTOCOL_MAGIC_SIZE 8

#ifdef CATCHCHALLENGER_SERVER
    #if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_SERVER_DATAPACK_MIN_FILEPURGE_KB*1024
    #error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_SERVER_DATAPACK_MIN_FILEPURGE_KB
    #endif
    #if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_SERVER_DATAPACK_MAX_FILEPURGE_KB*1024
    #error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_SERVER_DATAPACK_MAX_FILEPURGE_KB
    #endif
    #if CATCHCHALLENGER_BIGBUFFERSIZE < CATCHCHALLENGER_SERVER_DATAPACK_ZLIB_COMPRESSEDFILEPURGE_KB*1024
    #error CATCHCHALLENGER_BIGBUFFERSIZE can t be lower than CATCHCHALLENGER_SERVER_DATAPACK_ZLIB_COMPRESSEDFILEPURGE_KB
    #endif
    // Same rationale as server/base/Client.hpp: BIGBUFFERSIZE only needs
    // to fit one wire packet (CATCHCHALLENGER_MAX_PACKET_SIZE). The XZ /
    // DONT_COMPRESS guards were over-strict; with streaming compression
    // and sendfile() the runtime never holds a full file in this buffer.
#endif

#define CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE 8
#define CATCHCHALLENGER_DDOS_COMPUTERINTERVAL 5//in seconds
#define CATCHCHALLENGER_DDOS_KICKLIMITMOVE 140
#define CATCHCHALLENGER_DDOS_KICKLIMITCHAT 15
#define CATCHCHALLENGER_DDOS_KICKLIMITOTHER 45

namespace CatchChallenger {

struct FileToSend
{
    //not QFile * to prevent too many file open
    std::string file;
};
bool operator<(const FileToSend &fileToSend1,const FileToSend &fileToSend2);

class EpollClientLoginSlave : public EpollClient, public ProtocolParsingInputOutput
{
public:
    EpollClientLoginSlave(
        #ifdef CATCHCHALLENGER_SERVER_SSL
            const int &infd, SSL_CTX *ctx
        #else
            const int &infd
        #endif
        );
    ~EpollClientLoginSlave();
    bool disconnectClient();
    enum EpollClientLoginStat : uint8_t
    {
        None,
        ProtocolGood,
        GameServerConnecting,
        GameServerConnected,
    };
    EpollClientLoginStat stat;
    enum DatapackStatus : uint8_t
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
    void breakNeedMoreData();

    void parseNetworkReadError(const std::string &errorString);
    bool sendRawBlock(const char * const data,const int &size);
    bool removeFromQueryReceived(const uint8_t &queryNumber);
    bool sendDatapackProgression(const uint8_t progression);
    void allowDynamicSize();

    ssize_t readFromSocket(char * data, const size_t &size);
    ssize_t writeToSocket(const char * const data, const size_t &size);
    void closeSocket();

    bool fastForward;
    LinkToGameServer *linkToGameServer;
    char *socketString;
    int socketStringSize;

    struct DatapackCacheFile
    {
        //uint32_t mtime;
        uint32_t partialHash;
    };
    struct DatapackData
    {
        //uint32_t mtime;
        std::unordered_map<std::string,DatapackCacheFile> datapack_file_hash_cache;
    };
    //static std::unordered_map<std::string,uint32_t> datapack_file_list_cache_base,datapack_file_list_cache_main,datapack_file_list_cache_sub;//same than above
    static DatapackData datapack_file_base;
    static std::unordered_set<std::string> compressedExtension;
    static std::unordered_map<std::string,DatapackData> datapack_file_main;
    static std::unordered_map<std::string,std::unordered_map<std::string,DatapackData> > datapack_file_sub;
    static std::unordered_map<std::string,DatapackCacheFile> datapack_file_list(const std::string &path,const bool withHash=true);
private:
    #ifdef CATCHCHALLENGER_HARDENED
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
    bool parseMessage(const uint8_t &mainCodeType,const char * const data,const unsigned int &size);
    //have query with reply
    bool parseQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size);
    //send reply
    bool parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size);

    bool parseInputBeforeLogin(const uint8_t &mainCodeType, const uint8_t &queryNumber, const char * const data, const unsigned int &size);

    bool sendFile(const std::string &datapackPath, const std::string &fileName);
    void datapackList(const uint8_t &query_id, const std::vector<std::string> &files, const std::vector<uint32_t> &partialHashList);

    void addDatapackListReply(const bool &fileRemove);
    void purgeDatapackListReply(const uint8_t &query_id);
    void sendFileContent();
    #ifndef CATCHCHALLENGER_SERVER_NO_COMPRESSION
    void sendCompressedFileContent();
    #endif
public:
    bool sendRawBlock(const char * const data, const unsigned int &size);
private:
    static std::vector<EpollClientLoginSlave *> client_list;

    DdosBuffer<uint8_t,3> movePacketKick;
    DdosBuffer<uint8_t,3> chatPacketKick;
    DdosBuffer<uint8_t,3> otherPacketKick;

    static uint8_t tempDatapackListReplySize;
    static std::vector<char> tempDatapackListReplyArray;
    static uint8_t tempDatapackListReply;
    static unsigned int tempDatapackListReplyTestCount;
    static std::vector<char> rawFilesBuffer,compressedFilesBuffer;
    static unsigned int rawFilesBufferCount,compressedFilesBufferCount;

    static std::regex fileNameStartStringRegex;
    static std::regex datapack_rightFileName;
};
}

#endif // EPOLLCLIENTLOGINMASTER_H
