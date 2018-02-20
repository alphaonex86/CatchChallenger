#include "LinkToLogin.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/cpp11addition.h"
#include <iostream>

using namespace CatchChallenger;

bool LinkToLogin::parseInputBeforeLogin(const uint8_t &mainCodeType, const uint8_t &, const char * const , const unsigned int &)
{
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("wrong data before login with mainIdent: "+std::to_string(mainCodeType));
        return false;
    }
}

bool LinkToLogin::parseMessage(const uint8_t &mainCodeType,const char * const data,const unsigned int &size)
{
    if(stat!=Stat::Logged)
    {
        parseNetworkReadError("parseFullMessage() not logged to send: "+std::to_string(mainCodeType));
        return false;
    }
    (void)data;
    (void)size;
    switch(mainCodeType)
    {
        case 0x44:
        {
            unsigned int pos=0;
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                return false;
            }
            uint8_t logicalGroupSize;
            logicalGroupSize=data[pos];
            pos+=1;

            logicalGroups.clear();
            unsigned int index=0;
            while(index<logicalGroupSize)
            {
                LogicalGroup logicalGroup;
                //folder/path
                {
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint8_t stringSize;
                    stringSize=data[pos];
                    pos+=1;
                    if(stringSize>0)
                    {
                        if((unsigned int)(size-pos)<(unsigned int)stringSize)
                        {
                            errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                            return false;
                        }
                        logicalGroup.path=std::string(data+pos,stringSize);
                        pos+=stringSize;
                    }
                }

                //Translation xml
                {
                    if((size-pos)<(int)sizeof(uint16_t))
                    {
                        errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint16_t stringSize;
                    stringSize=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+pos)));
                    pos+=2;
                    if(stringSize>0)
                    {
                        if((unsigned int)(size-pos)<(unsigned int)stringSize)
                        {
                            errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                            return false;
                        }
                        logicalGroup.xml=std::string(data+pos,stringSize);
                        pos+=stringSize;
                    }
                }
                logicalGroups.push_back(logicalGroup);

                index++;
            }
            updateJsonFile(LinkToLogin::withIndentation);

            return true;
        }
        case 0x40:
        {
            unsigned int pos=0;
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                return false;
            }
            uint8_t serverMode;
            serverMode=data[pos];
            pos+=1;
            if(serverMode<1 || serverMode>2)
            {
                errorParsingLayer(std::string("wrong server mode: ")+__FILE__+":"+std::to_string(__LINE__));
                return false;
            }
            proxyMode=ProxyMode(serverMode);
            /*switch(proxyMode)
            {
                case ProxyMode::Proxy:
                    std::cout << "Proxy mode: Proxy" << std::endl;
                break;
                case ProxyMode::Reconnect:
                    std::cout << "Proxy mode: Reconnect" << std::endl;
                break;
                default:
                    std::cout << "Proxy mode: ???" << std::endl;
                break;
            }*/
            std::unordered_map<uint8_t/*charactersgroup index*/,std::unordered_set<uint32_t/*unique key*/> > duplicateDetect;
            uint8_t serverListSize=0;
            uint8_t serverListIndex=0;
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                return false;
            }
            serverListSize=data[pos];
            pos+=1;
            serverList.clear();
            while(serverListIndex<serverListSize)
            {
                ServerFromPoolForDisplay server;
                server.currentPlayer=0;
                server.maxPlayer=0;
                server.uniqueKey=0;
                //group index
                {
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                        return false;
                    }
                    server.groupIndex=data[pos];
                    pos+=1;
                }
                //uniquekey
                {
                    if((size-pos)<(int)sizeof(uint32_t))
                    {
                        errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                        return false;
                    }
                    server.uniqueKey=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                    pos+=4;
                }
                //add more control
                {
                    if(duplicateDetect.find(server.groupIndex)==duplicateDetect.cend())
                        duplicateDetect[server.groupIndex]=std::unordered_set<uint32_t/*unique key*/>();
                    std::unordered_set<uint32_t/*unique key*/> &duplicateDetectEntry=duplicateDetect[server.groupIndex];
                    if(duplicateDetectEntry.find(server.uniqueKey)!=duplicateDetectEntry.cend())//exists, bug
                    {
                        std::cerr << "Duplicate unique key for packet 45 found: " << std::to_string(server.uniqueKey) << std::endl;
                        abort();
                    }
                    else
                        duplicateDetectEntry.insert(server.uniqueKey);
                }
                if(proxyMode==ProxyMode::Reconnect)
                {
                    //host
                    {
                        if((size-pos)<(int)sizeof(uint8_t))
                        {
                            errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                            return false;
                        }
                        uint8_t stringSize;
                        stringSize=data[pos];
                        pos+=1;
                        if(stringSize>0)
                        {
                            if((unsigned int)(size-pos)<(unsigned int)stringSize)
                            {
                                errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                                return false;
                            }
                            pos+=stringSize;
                        }
                    }
                    //port
                    {
                        if((size-pos)<(int)sizeof(uint16_t))
                        {
                            errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                            return false;
                        }
                        pos+=2;
                    }
                }
                //xml (name, description, ...)
                {
                    if((size-pos)<(int)sizeof(uint16_t))
                    {
                        errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint16_t stringSize;
                    stringSize=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+pos)));
                    pos+=2;
                    if(stringSize>0)
                    {
                        if((unsigned int)(size-pos)<(unsigned int)stringSize)
                        {
                            errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                            return false;
                        }
                        server.xml=std::string(data+pos,stringSize);
                        pos+=stringSize;
                    }
                }
                //logical to contruct the tree
                {
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                        return false;
                    }
                    server.logicalGroupIndex=data[pos];
                    pos+=1;
                }
                //max player
                {
                    if((size-pos)<(int)sizeof(uint16_t))
                    {
                        errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                        return false;
                    }
                    server.maxPlayer=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+pos)));
                    pos+=2;
                }
                serverList.push_back(server);
                serverListIndex++;
            }
            if((size-pos)!=((int)sizeof(uint16_t)*serverList.size()))
            {
                errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                return false;
            }
            serverListIndex=0;
            while(serverListIndex<serverList.size())
            {
                serverList[serverListIndex].currentPlayer=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+pos)));
                pos+=2;
                serverListIndex++;
            }
            updateJsonFile(LinkToLogin::withIndentation);
        }
        break;
        //Update the game server current player number on the game server
        case 0x47:
        {
            if(size!=((int)sizeof(uint16_t)*serverList.size()))
            {
                errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                return false;
            }
            unsigned int pos=0;
            uint8_t serverListIndex=0;
            while(serverListIndex<serverList.size())
            {
                serverList[serverListIndex].currentPlayer=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+pos)));
                pos+=2;
                serverListIndex++;
            }
            updateJsonFile(LinkToLogin::withIndentation);
        }
        break;
        case 0x48:
        {
            if(size==2 && data[0x00]==0 && data[0x01]==0)
                return true;
            unsigned int pos=0;
            uint8_t serverListIndex=0;
            const uint8_t &deleteSize=data[pos];
            pos+=1;
            uint8_t serverBlockListSizeBeforeAdd=0;

            //performance boost and null size problem covered with this
            if(deleteSize>=serverList.size())//do without control, should be true
            {
                if(deleteSize>serverList.size())
                {
                    std::cerr << "dump data: " << binarytoHexa(data,pos) << " " << binarytoHexa(data+pos,size-pos) << std::endl;
                    parseNetworkReadError("deleteSize>serverListCount main ident: "+std::to_string(mainCodeType));
                    return false;
                }
                serverList.clear();
                pos+=deleteSize;
            }
            else
            {
                //do the delete
                if(deleteSize>0)
                {
                    size_t index=0;
                    if((size-pos)<(deleteSize*sizeof(uint8_t)))
                    {
                        std::cerr << "dump data: " << binarytoHexa(data,pos) << " " << binarytoHexa(data+pos,size-pos) << std::endl;
                        parseNetworkReadError("for main ident: "+std::to_string(mainCodeType)+", (size-cursor)<(deleteSize*sizeof(uint16_t)), file:"+__FILE__+":"+std::to_string(__LINE__));
                        return false;
                    }
                    while(index<deleteSize)
                    {
                        const uint8_t &deleteIndex=data[pos];
                        pos+=1;
                        if(deleteIndex<serverList.size())
                            serverList.erase(serverList.cbegin()+deleteIndex);
                        else
                        {
                            parseNetworkReadError("for main ident: "+std::to_string(mainCodeType)+", delete entry is out of range, file:"+__FILE__+":"+std::to_string(__LINE__));
                            return false;
                        }
                        index++;
                    }
                }
                serverBlockListSizeBeforeAdd=serverList.size();
            }

            const uint8_t &serverListSize=data[pos];
            pos+=1;
            //do the insert the first part
            while(serverListIndex<serverListSize)
            {
                ServerFromPoolForDisplay server;
                server.currentPlayer=0;
                server.maxPlayer=0;
                server.uniqueKey=0;
                //group index
                {
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        std::cerr << "dump data: " << binarytoHexa(data,pos) << " " << binarytoHexa(data+pos,size-pos) << ", proxyMode: " << std::to_string(proxyMode) << std::endl;
                        errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                        return false;
                    }
                    server.groupIndex=data[pos];
                    pos+=1;
                }
                //uniquekey
                {
                    if((size-pos)<(int)sizeof(uint32_t))
                    {
                        std::cerr << "dump data: " << binarytoHexa(data,pos) << " " << binarytoHexa(data+pos,size-pos) << ", proxyMode: " << std::to_string(proxyMode) << std::endl;
                        errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                        return false;
                    }
                    server.uniqueKey=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                    pos+=4;
                }
                if(proxyMode==ProxyMode::Reconnect)
                {
                    //host
                    {
                        if((size-pos)<(int)sizeof(uint8_t))
                        {
                            std::cerr << "dump data: " << binarytoHexa(data,pos) << " " << binarytoHexa(data+pos,size-pos) << ", proxyMode: " << std::to_string(proxyMode) << std::endl;
                            errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                            return false;
                        }
                        uint8_t stringSize;
                        stringSize=data[pos];
                        pos+=1;
                        if(stringSize>0)
                        {
                            if((unsigned int)(size-pos)<(unsigned int)stringSize)
                            {
                                std::cerr << "dump data: " << binarytoHexa(data,pos) << " " << binarytoHexa(data+pos,size-pos) << ", proxyMode: " << std::to_string(proxyMode) << std::endl;
                                errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                                return false;
                            }
                            pos+=stringSize;
                        }
                    }
                    //port
                    {
                        if((size-pos)<(int)sizeof(uint16_t))
                        {
                            std::cerr << "dump data: " << binarytoHexa(data,pos) << " " << binarytoHexa(data+pos,size-pos) << ", proxyMode: " << std::to_string(proxyMode) << std::endl;
                            errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                            return false;
                        }
                        pos+=2;
                    }
                }
                //xml (name, description, ...)
                {
                    if((size-pos)<(int)sizeof(uint16_t))
                    {
                        std::cerr << "dump data: " << binarytoHexa(data,pos) << " " << binarytoHexa(data+pos,size-pos) << ", proxyMode: " << std::to_string(proxyMode) << std::endl;
                        errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint16_t stringSize;
                    stringSize=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+pos)));
                    pos+=2;
                    if(stringSize>0)
                    {
                        if((unsigned int)(size-pos)<(unsigned int)stringSize)
                        {
                            std::cerr << "dump data: " << binarytoHexa(data,pos) << " " << binarytoHexa(data+pos,size-pos) << ", proxyMode: " << std::to_string(proxyMode) << std::endl;
                            errorParsingLayer(std::string("wrong size: ")+std::to_string(stringSize)+" "+__FILE__+":"+std::to_string(__LINE__));
                            return false;
                        }
                        server.xml=std::string(data+pos,stringSize);
                        pos+=stringSize;
                    }
                }
                //logical to contruct the tree
                {
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        std::cerr << "dump data: " << binarytoHexa(data,pos) << " " << binarytoHexa(data+pos,size-pos) << ", proxyMode: " << std::to_string(proxyMode) << std::endl;
                        errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                        return false;
                    }
                    server.logicalGroupIndex=data[pos];
                    pos+=1;
                }
                //max player
                {
                    if((size-pos)<(int)sizeof(uint16_t))
                    {
                        std::cerr << "dump data: " << binarytoHexa(data,pos) << " " << binarytoHexa(data+pos,size-pos) << ", proxyMode: " << std::to_string(proxyMode) << std::endl;
                        errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                        return false;
                    }
                    server.maxPlayer=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+pos)));
                    pos+=2;
                }
                serverList.push_back(server);
                serverListIndex++;
            }
            if(static_cast<ssize_t>(size-pos)!=((int)sizeof(uint16_t)*serverListSize))
            {
                std::cerr << "dump data: " << binarytoHexa(data,pos) << " " << binarytoHexa(data+pos,size-pos) << ", proxyMode: " << std::to_string(proxyMode) << std::endl;
                errorParsingLayer(std::string("wrong size: ")+__FILE__+":"+std::to_string(__LINE__));
                return false;
            }
            serverListIndex=0;
            while(serverListIndex<serverListSize)
            {
                serverList[serverBlockListSizeBeforeAdd+serverListIndex].currentPlayer=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+pos)));
                pos+=2;
                serverListIndex++;
            }
            updateJsonFile(LinkToLogin::withIndentation);
        }
        return true;
        default:
            parseNetworkReadError("unknown main ident: "+std::to_string(mainCodeType));
            return false;
        break;
    }
    return true;
}

//have query with reply
bool LinkToLogin::parseQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    if(stat!=Stat::Logged)
    {
        return parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
    }
    //do the work here
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("unknown main ident: "+std::to_string(mainCodeType));
            return false;
        break;
    }
    return true;
}

//send reply
bool LinkToLogin::parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    queryNumberList.push_back(queryNumber);
    //do the work here
    switch(mainCodeType)
    {
        //Protocol initialization and auth for login
        case 0xA0:
        {
            if(stat!=Stat::ProtocolSent)
            {
                std::cerr << "ERROR 0xA0 reply: stat!=Stat::ProtocolSent (abort)" << std::endl;
                abort();
            }
            if(size<1)
            {
                std::cerr << "Need more size for protocol header " << std::endl;
                abort();
            }
            //Protocol initialization
            const uint8_t &returnCode=data[0x00];
            if(returnCode==0x08)
            {
                if(size!=(1+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT))
                {
                    std::cerr << "wrong size for protocol header " << returnCode << std::endl;
                    abort();
                }
                switch(returnCode)
                {
                    case 0x08:
                    break;
                    default:
                        std::cerr << "compression type wrong with main ident: 1 and queryNumber: %2, type: query_type_protocol" << queryNumber << std::endl;
                        abort();
                    return false;
                }
                stat=Stat::ProtocolGood;
                registerStatsClient(data+1);
                return true;
            }
            else
            {
                if(returnCode==0x02)
                    std::cerr << "Protocol not supported" << std::endl;
                else
                    std::cerr << "Unknown error " << returnCode << std::endl;
                abort();
                return false;
            }
        }
        return true;
        //Stat client
        case 0xAD:
        {
            if(size<1)
            {
                std::cerr << "reply to 07 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                abort();
            }
            switch(data[0x00])
            {
                case 0x01:
                stat=Stat::Logged;
                return true;
                case 0x02:
                default:
                    std::cerr << "reply return code error (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    abort();
                break;
            }
        }
        return true;
        default:
            parseNetworkReadError("unknown main ident: "+std::to_string(mainCodeType));
            return false;
        break;
    }
    parseNetworkReadError("The server for now not ask anything: "+std::to_string(mainCodeType)+" "+std::to_string(queryNumber));
    return false;
}

void LinkToLogin::parseNetworkReadError(const std::string &errorString)
{
    errorParsingLayer(errorString);
}
