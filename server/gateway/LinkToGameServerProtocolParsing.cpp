#include "LinkToGameServer.h"
#include "EpollClientLoginSlave.h"
#include <iostream>
#include "EpollServerLoginSlave.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/CommonSettingsServer.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "DatapackDownloaderBase.h"
#include "DatapackDownloaderMainSub.h"
#include "../epoll/Epoll.h"

using namespace CatchChallenger;

bool LinkToGameServer::parseInputBeforeLogin(const uint8_t &mainCodeType, const uint8_t &queryNumber, const char * const data, const unsigned int &size)
{
    (void)queryNumber;
    (void)size;
    (void)data;
    switch(mainCodeType)
    {
        //Protocol initialization for client
        case 0xA0:
        {
            if(stat==Stat::Reconnecting)
            {
                //Protocol initialization
                if(size!=(sizeof(uint8_t)))
                {
                    parseNetworkReadError("compression type wrong size (stage 3) with main ident: "+std::to_string(mainCodeType)+
                                          " and queryNumber: "+std::to_string(queryNumber)+", type: query_type_protocol");
                    return false;
                }
                const uint8_t &returnCode=data[0x00];
                if(returnCode>=0x04 && returnCode<=0x07)
                {
                    if(!LinkToGameServer::compressionSet)
                    {
                        switch(returnCode)
                        {
                            case 0x04:
                                ProtocolParsing::compressionTypeClient=ProtocolParsing::CompressionType::None;
                            break;
                            case 0x08:
                                ProtocolParsing::compressionTypeClient=ProtocolParsing::CompressionType::Zstandard;
                            break;
                            default:
                                parseNetworkReadError("compression type wrong with main ident: "+std::to_string(mainCodeType)+
                                                      " and queryNumber: "+std::to_string(queryNumber)+
                                                      ", type: query_type_protocol");
                            return false;
                        }
                    }
                    else
                    {
                        ProtocolParsing::CompressionType tempCompression;
                        switch(returnCode)
                        {
                            case 0x04:
                                tempCompression=ProtocolParsing::CompressionType::None;
                            break;
                            case 0x08:
                                tempCompression=ProtocolParsing::CompressionType::Zstandard;
                            break;
                            default:
                                parseNetworkReadError("compression type wrong with main ident: "+std::to_string(mainCodeType)+
                                                      " and queryNumber: "+std::to_string(queryNumber)+", type: query_type_protocol");
                            return false;
                        }
                        if(tempCompression!=ProtocolParsing::compressionTypeClient)
                        {
                            parseNetworkReadError("compression change main ident: "+std::to_string(mainCodeType)+
                                                  " and queryNumber: "+std::to_string(queryNumber)+", type: query_type_protocol");
                            return false;
                        }
                    }

                    //send the network query
                    registerOutputQuery(queryIdToReconnect,0x93);
                    uint32_t posOutput=0;
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x93;
                    posOutput+=1;
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryIdToReconnect;
                    posOutput+=1+4;
                    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(sizeof(tokenForGameServer));//set the dynamic size

                    memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1+4,tokenForGameServer,sizeof(tokenForGameServer));
                    posOutput+=sizeof(tokenForGameServer);

                    sendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                    return true;
                }
                else
                {
                    if(client!=NULL)
                    {
                        stat=Stat::ProtocolGood;
                        //send the network reply
                        client->removeFromQueryReceived(queryIdToReconnect);
                        uint32_t posOutput=0;
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
                        posOutput+=1;
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryIdToReconnect;
                        posOutput+=1+4;
                        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(size);//set the dynamic size

                        memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1+4,data,size);
                        posOutput+=size;

                        client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                    }
                    if(returnCode==0x03)
                        parseNetworkReadError("Server full");
                    else
                        parseNetworkReadError("Unknown error "+std::to_string(returnCode));
                    return false;
                }
            }
            else
            {
                //Protocol initialization
                if(size==(sizeof(uint8_t)))
                {
                    if(client!=NULL)
                    {
                        //send the network reply
                        client->removeFromQueryReceived(queryNumber);
                        uint32_t posOutput=0;
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
                        posOutput+=1;
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
                        posOutput+=1+4;
                        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(size);//set the dynamic size

                        memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1+4,data,size);
                        posOutput+=size;

                        client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

                        if(data[0x00]==0x03)
                            parseNetworkReadError("Server full");
                        else
                            parseNetworkReadError("Unknown error "+std::to_string(data[0x00]));
                    }
                    else
                        parseNetworkReadError("size==(sizeof(uint8_t)) and client null with main ident: "+std::to_string(mainCodeType)+
                                              " and queryNumber: "+std::to_string(queryNumber)+", type: query_type_protocol");
                    return false;
                }
                if(size!=(sizeof(uint8_t)+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT))
                {
                    parseNetworkReadError("compression type wrong size (stage 3) with main ident: "+std::to_string(mainCodeType)+
                                          " and queryNumber: "+std::to_string(queryNumber)+", type: query_type_protocol");
                    return false;
                }
                const uint8_t &returnCode=data[0x00];
                if(returnCode>=0x04 && returnCode<=0x07)
                {
                    if(!LinkToGameServer::compressionSet)
                    {
                        switch(returnCode)
                        {
                            case 0x04:
                                ProtocolParsing::compressionTypeClient=ProtocolParsing::CompressionType::None;
                            break;
                            case 0x08:
                                ProtocolParsing::compressionTypeClient=ProtocolParsing::CompressionType::Zstandard;
                            break;
                            default:
                                parseNetworkReadError("compression type wrong with main ident: "+std::to_string(mainCodeType)+
                                                      " and queryNumber: "+std::to_string(queryNumber)+", type: query_type_protocol");
                            return false;
                        }
                    }
                    else
                    {
                        ProtocolParsing::CompressionType tempCompression;
                        switch(returnCode)
                        {
                            case 0x04:
                                tempCompression=ProtocolParsing::CompressionType::None;
                            break;
                            case 0x08:
                                tempCompression=ProtocolParsing::CompressionType::Zstandard;
                            break;
                            default:
                                parseNetworkReadError("compression type wrong with main ident: "+std::to_string(mainCodeType)+
                                                      " and queryNumber: "+std::to_string(queryNumber)+", type: query_type_protocol");
                            return false;
                        }
                        if(tempCompression!=ProtocolParsing::compressionTypeClient)
                        {
                            parseNetworkReadError("compression change main ident: "+std::to_string(mainCodeType)+
                                                  " and queryNumber: "+std::to_string(queryNumber)+", type: query_type_protocol");
                            return false;
                        }
                    }

                    //send token to game server
                    if(client!=NULL)
                    {
                        stat=Stat::ProtocolGood;
                        //send the network reply
                        client->removeFromQueryReceived(queryNumber);
                        uint32_t posOutput=0;
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
                        posOutput+=1;
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
                        posOutput+=1+4;
                        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);//set the dynamic size

                        //the gateway can different compression than server connected
                        switch(ProtocolParsing::compressionTypeServer)
                        {
                            case ProtocolParsing::CompressionType::None:
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x04;
                            break;
                            case ProtocolParsing::CompressionType::Zstandard:
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x08;
                            break;
                            default:
                                parseNetworkReadError("compression type wrong with main ident: "+std::to_string(mainCodeType)+" and queryNumber: "+std::to_string(queryNumber)+", type: query_type_protocol");
                            return false;
                        }
                        posOutput+=1;
                        //std::cout << "Transmit the token: " << binarytoHexa(data+1,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT) << std::endl;
                        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,data+1,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        posOutput+=TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT;

                        return client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                    }
                    else
                    {
                        parseNetworkReadError("Protocol initialised but client already disconnected");
                        return false;
                    }
                    stat=ProtocolGood;
                    return true;
                }
                else
                {
                    parseNetworkReadError("Unknown error "+std::to_string(returnCode));
                    return false;
                }
            }
        }
        break;
        default:
            parseNetworkReadError("unknown sort ident reply code: "+std::to_string(mainCodeType)+", line: "+__FILE__+":"+std::to_string(__LINE__));
            return false;
        break;
    }
}

bool LinkToGameServer::parseMessage(const uint8_t &mainCodeType,const char * const data,const unsigned int &size)
{
    if(client==NULL)
    {
        parseNetworkReadError("client not connected");
        return false;
    }
    if(mainCodeType==0x75)
        return true;//drop, need be recreate by the gateway in case of no mirror use
    if(stat!=Stat::ProtocolGood || mainCodeType==0x40)
    {
        if(mainCodeType==0x44)//send Logical group
        {}
        else if(mainCodeType==0x40)//send Send server list to real player
        {
            if(size>0)
            {
                switch(data[0x00])
                {
                    case 0x01:
                    break;
                    case 0x02:
                    {
                        gameServerMode=GameServerMode::Proxy;
                        //all right but need reemit
                        //send the network message
                        uint32_t posOutput=0;
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=mainCodeType;
                        posOutput+=1+4;
                        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(size);//set the dynamic size

                        memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+4,data,size);
                        posOutput+=size;

                        return client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                    }
                    break;
                    default:
                        parseNetworkReadError("parseFullMessage() wrong server list mode: "+std::to_string(mainCodeType));
                    return false;
                }
                //send the network message
                uint32_t posOutput=0;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=mainCodeType;
                posOutput+=1+4;

                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;//in gateway it's in proxy mode only
                posOutput+=1;

                unsigned int inputPos=1;
                if((size-inputPos)<1)
                {
                    parseNetworkReadError("parseFullMessage() missing data for server list size: "+std::to_string(mainCodeType));
                    return false;
                }
                const uint8_t &serverListSize=data[inputPos];
                inputPos+=1;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=serverListSize;
                posOutput+=1;
                uint8_t serverListIndex=0;
                while(serverListIndex<serverListSize)
                {
                    if((size-inputPos)<(1+4+1))
                    {
                        parseNetworkReadError("parseFullMessage() missing data for server unique key size: "+std::to_string(mainCodeType));
                        return false;
                    }
                    //charactersgroup
                    const uint8_t &charactersGroupIndex=data[inputPos];
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=data[inputPos];
                    posOutput+=1;
                    inputPos+=1;
                    //unique key
                    const uint32_t &uniqueKey=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+inputPos)));
                    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,data+inputPos,4);
                    posOutput+=4;
                    inputPos+=4;
                    ServerReconnect serverReconnect;
                    //host, skip because in gateway it's in proxy mode only
                    {
                        const uint8_t &stringSize=data[inputPos];
                        inputPos+=1;
                        if((size-inputPos)<stringSize)
                        {
                            parseNetworkReadError("parseFullMessage() missing data for server host string size: "+std::to_string(mainCodeType));
                            return false;
                        }
                        if(stringSize>0)
                            serverReconnect.host=std::string(data+inputPos,stringSize);
                        if(serverReconnect.host.empty())
                        {
                            parseNetworkReadError("parseFullMessage() server list, host can't be empty: "+std::to_string(mainCodeType));
                            return false;
                        }
                        inputPos+=stringSize;
                    }
                    if((size-inputPos)<(2+2))
                    {
                        parseNetworkReadError("parseFullMessage() missing data for server port start description size: "+std::to_string(mainCodeType));
                        return false;
                    }
                    //port, skip because in gateway it's in proxy mode only
                    serverReconnect.port=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+inputPos)));
                    inputPos+=2;
                    //description
                    {
                        const uint16_t &stringSize=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+inputPos)));
                        inputPos+=2;
                        if((size-inputPos)<stringSize)
                        {
                            parseNetworkReadError("parseFullMessage() missing data for server host string size: "+std::to_string(mainCodeType));
                            return false;
                        }
                        if(stringSize>0)
                        {
                            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,data+inputPos-2,2+stringSize);
                            //posOutput+=stringSize;do after
                            inputPos+=stringSize;
                        }
                        else
                            memset(ProtocolParsingBase::tempBigBufferForOutput+posOutput,0x00,2);
                        posOutput+=2+stringSize;
                    }
                    //Logical group and max player
                    if((size-inputPos)<(1+2))
                    {
                        parseNetworkReadError("parseFullMessage() missing data for server port start description size: "+std::to_string(mainCodeType));
                        return false;
                    }
                    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,data+inputPos,1+2);
                    posOutput+=1+2;

                    inputPos+=1+2;

                    serverReconnectList[charactersGroupIndex][uniqueKey]=serverReconnect;
                    serverListIndex++;
                }

                //Second list part with same size
                memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,data+inputPos,2*serverListSize);
                posOutput+=2*serverListSize;

                gameServerMode=GameServerMode::Reconnect;

                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size

                client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                return true;
            }
        }
        else
        {
            parseNetworkReadError("parseFullMessage() not logged to send: "+std::to_string(mainCodeType)+", stat: "+std::to_string(stat));
            return false;
        }
    }

    switch(mainCodeType)
    {
        //file as input
        case 0x76://raw
        case 0x77://compressed
        {
            unsigned int pos=0;
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseNetworkReadError("wrong size with main ident: "+std::to_string(mainCodeType)+", file: "+__FILE__+":"+std::to_string(__LINE__));
                return false;
            }
            uint8_t fileListSize=data[pos];
            pos++;
            if(fileListSize==0)
            {
                std::cerr << "fileListSize==0 with main ident: "+std::to_string(mainCodeType) << std::endl;
                return false;
            }

            uint32_t sub_size32=size-pos;
            uint32_t decompressedSize=0;
            if(ProtocolParsingBase::compressionTypeClient==CompressionType::None || mainCodeType==0x76)
            {
                decompressedSize=sub_size32;
                memcpy(ProtocolParsingBase::tempBigBufferForUncompressedInput,data+pos,sub_size32);
            }
            else
                decompressedSize=computeDecompression(data+pos,ProtocolParsingBase::tempBigBufferForUncompressedInput,sub_size32,sizeof(ProtocolParsingBase::tempBigBufferForUncompressedInput),ProtocolParsingBase::compressionTypeClient);

            unsigned int decompressedDataPos=0;
            uint8_t index=0;
            while(index<fileListSize)
            {
                if((decompressedSize-decompressedDataPos)<(int)(sizeof(uint8_t)))
                {
                    parseNetworkReadError("wrong size with main ident: "+std::to_string(mainCodeType)+", file: "+__FILE__+":"+std::to_string(__LINE__));
                    return false;
                }
                std::string fileName;
                uint8_t fileNameSize=ProtocolParsingBase::tempBigBufferForUncompressedInput[decompressedDataPos];
                decompressedDataPos+=1;
                if(fileNameSize>0)
                {
                    if((decompressedSize-decompressedDataPos)<fileNameSize)
                    {
                        parseNetworkReadError("wrong size with main ident: "+std::to_string(mainCodeType)+", file: "+__FILE__+":"+std::to_string(__LINE__));
                        return false;
                    }
                    fileName=std::string(ProtocolParsingBase::tempBigBufferForUncompressedInput+decompressedDataPos,fileNameSize);
                    decompressedDataPos+=fileNameSize;
                }
                if(DatapackDownloaderBase::extensionAllowed.find(CatchChallenger::FacilityLibGeneral::getSuffix(fileName))==DatapackDownloaderBase::extensionAllowed.cend())
                {
                    std::cerr << "datadump: " << binarytoHexa(ProtocolParsingBase::tempBigBufferForUncompressedInput,decompressedDataPos) << " " << binarytoHexa(ProtocolParsingBase::tempBigBufferForUncompressedInput+decompressedDataPos,decompressedSize-decompressedDataPos) << std::endl;
                    parseNetworkReadError("main code: "+std::to_string(mainCodeType)+" extension not allowed: \""+CatchChallenger::FacilityLibGeneral::getSuffix(fileName)+"\" for file \""+fileName+"\" with main ident: "+std::to_string(mainCodeType)+", file: "+__FILE__+":"+std::to_string(__LINE__));
                    return false;
                }
                if((decompressedSize-decompressedDataPos)<(int)(sizeof(uint32_t)))
                {
                    parseNetworkReadError("wrong size with main ident: "+std::to_string(mainCodeType)+", file: "+__FILE__+":"+std::to_string(__LINE__));
                    return false;
                }
                const uint32_t &fileSize=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(ProtocolParsingBase::tempBigBufferForUncompressedInput+decompressedDataPos)));
                decompressedDataPos+=4;
                if((decompressedSize-decompressedDataPos)<fileSize)
                {
                    parseNetworkReadError("wrong file data size with main ident: "+std::to_string(mainCodeType)+", file: "+__FILE__+":"+std::to_string(__LINE__));
                    return false;
                }
                std::vector<char> dataFile;
                dataFile.resize(fileSize);
                memcpy(dataFile.data(),ProtocolParsingBase::tempBigBufferForUncompressedInput+decompressedDataPos,fileSize);
                decompressedDataPos+=fileSize;
                if(mainCodeType==0x76)
                    std::cout << "Raw file to create: " << fileName << std::endl;
                else
                    std::cout << "Compressed file to create: " << fileName << std::endl;

                if(replySelectListInWait!=NULL)
                    DatapackDownloaderBase::datapackDownloaderBase->writeNewFileBase(fileName,dataFile);
                else if(replySelectCharInWait!=NULL)
                {
                    if(DatapackDownloaderMainSub::datapackDownloaderMainSub.find(main)!=DatapackDownloaderMainSub::datapackDownloaderMainSub.cend())
                    {
                        if(DatapackDownloaderMainSub::datapackDownloaderMainSub.at(main).find(sub)!=DatapackDownloaderMainSub::datapackDownloaderMainSub.at(main).cend())
                            DatapackDownloaderMainSub::datapackDownloaderMainSub.at(main).at(sub)->writeNewFileToRoute(fileName,dataFile);
                        else
                        {
                            parseNetworkReadError("unable to route mainCodeType==0xC2 && subCodeType==0x03 return, sub datapack code is not found: "+sub);
                            return false;
                        }
                    }
                    else
                    {
                        parseNetworkReadError("unable to route mainCodeType==0xC2 && subCodeType==0x03 return, main datapack code is not found: "+main);
                        return false;
                    }
                }
                else
                {
                    parseNetworkReadError("unable to route mainCodeType==0xC2 && subCodeType==0x03 return");
                    return false;
                }

                index++;
            }
            return true;//no remaining data, because all remaing is used as file data
        }
        break;
        case 0x78://Gateway Cache updating
        {
            unsigned int pos=0;
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseNetworkReadError("wrong size with main ident: "+std::to_string(mainCodeType)+", file: "+__FILE__+":"+std::to_string(__LINE__));
                return false;
            }
            uint8_t previousGatewayNumber=data[pos];
            pos++;
            if(previousGatewayNumber<255 && previousGatewayNumber>0)
                if(EpollServerLoginSlave::epollServerLoginSlave->gatewayNumber<(previousGatewayNumber+1))
                {
                    EpollServerLoginSlave::epollServerLoginSlave->gatewayNumber=previousGatewayNumber+1;
                    if(client!=NULL)
                    {
                        client->sendDatapackProgression(0);
                        if(previousGatewayNumber!=1)
                            return true;
                    }
                }
        }
        break;
        default:
        break;
    }

    uint8_t fixedSize=ProtocolParsingBase::packetFixedSize[mainCodeType];
    if(fixedSize!=0xFE)
    {
        //fixed size
        //send the network message
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=mainCodeType;
        posOutput+=1;

        memcpy(ProtocolParsingBase::tempBigBufferForOutput+1,data,size);
        posOutput+=size;

        return client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
    else
    {
        //dynamic size
        //send the network message
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=mainCodeType;
        posOutput+=1+4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(size);//set the dynamic size

        memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+4,data,size);
        posOutput+=size;

        return client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
}

//have query with reply
bool LinkToGameServer::parseQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    if(client==NULL)
    {
        parseNetworkReadError("client not connected");
        return false;
    }
    if(stat!=Stat::ProtocolGood)
        return parseInputBeforeLogin(mainCodeType,queryNumber,data,size);

    client->registerOutputQuery(queryNumber,mainCodeType);
    uint8_t fixedSize=ProtocolParsingBase::packetFixedSize[mainCodeType];
    if(fixedSize!=0xFE)
    {
        //fixed size
        //send the network message
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=mainCodeType;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
        posOutput+=1;

        memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1,data,size);
        posOutput+=size;

        return client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
    else
    {
        //dynamic size
        //send the network message
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=mainCodeType;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
        posOutput+=1+4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(size);//set the dynamic size

        memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1+4,data,size);
        posOutput+=size;

        return client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
}

//send reply
bool LinkToGameServer::parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    if(client==NULL)
    {
        parseNetworkReadError("client not connected");
        return false;
    }
    if(mainCodeType==0xA0 && stat==Stat::Connected)
        return parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
    //do the work here

    /* intercept part here */
    if(mainCodeType==0xA8)//Get first data and send the login
    {
        if(size>(40+CATCHCHALLENGER_SHA224HASH_SIZE) && data[0x00]==0x01)//all is good, change the reply
        {
            unsigned int pos=14;
            if(replySelectListInWait!=NULL)
            {
                parseNetworkReadError("another reply04inWait in suspend");
                return false;
            }
            {
                if(DatapackDownloaderBase::datapackDownloaderBase==NULL)
                    DatapackDownloaderBase::datapackDownloaderBase=new DatapackDownloaderBase(LinkToGameServer::mDatapackBase);
            }
            {
                DatapackDownloaderBase::datapackDownloaderBase->sendedHashBase.resize(CATCHCHALLENGER_SHA224HASH_SIZE);
                memcpy(DatapackDownloaderBase::datapackDownloaderBase->sendedHashBase.data(),data+pos,CATCHCHALLENGER_SHA224HASH_SIZE);
            }
            pos+=CATCHCHALLENGER_SHA224HASH_SIZE;
            if((size-pos)<1)
            {
                parseNetworkReadError("need more size: "+std::string(__FILE__)+":"+std::to_string(__LINE__)+", data:"+binarytoHexa(data,size));
                return false;
            }
            const uint16_t startString=pos;
            const uint8_t &stringSize=data[pos];
            pos+=1;
            if((size-pos)<stringSize)
            {
                parseNetworkReadError("need more size: "+std::string(__FILE__)+":"+std::to_string(__LINE__)+", data:"+binarytoHexa(data,size));
                return false;
            }
            if(stringSize>0)
            {
                const std::string httpDatapackMirrorBase(data+pos,stringSize);
                pos+=stringSize;
                if(DatapackDownloaderBase::httpDatapackMirrorBaseList.empty())//can't change for performance and current download in progression corruption
                {
                    DatapackDownloaderBase::httpDatapackMirrorBaseList=stringsplit(httpDatapackMirrorBase,';');
                    vectorRemoveEmpty(DatapackDownloaderBase::httpDatapackMirrorBaseList);
                    {
                        unsigned int index=0;
                        while(index<DatapackDownloaderBase::httpDatapackMirrorBaseList.size())
                        {
                            const std::string &currentMirror=DatapackDownloaderBase::httpDatapackMirrorBaseList.at(index);

                            const std::string::size_type &pos=currentMirror.find(LinkToGameServer::protocolString);
                            if(pos==std::string::npos)
                            {
                                parseNetworkReadError("no \"://\" found: "+currentMirror+", data dump: "+binarytoHexa(data,size));
                                return false;
                            }
                            const std::string &protocol=currentMirror.substr(0,pos);
                            if(protocol!=LinkToGameServer::protocolHttp && protocol!=LinkToGameServer::protocolHttps)
                            {
                                parseNetworkReadError("protocol not supported: "+protocol+" into "+currentMirror+", data dump: "+binarytoHexa(data,size));
                                return false;
                            }

                            if(!stringEndsWith(currentMirror,'/'))
                                DatapackDownloaderBase::httpDatapackMirrorBaseList[index]+='/';
                            index++;
                        }
                    }
                }
            }
            if(size<pos)
            {
                parseNetworkReadError("size"+std::to_string(size)+"<pos"+std::to_string(pos)+" to final packet making: "+std::string(__FILE__)+":"+std::to_string(__LINE__)+", data:"+binarytoHexa(data,size));
                return false;
            }
            unsigned int remainingSize=size-pos;
            if(remainingSize>8*1024*1024)
            {
                parseNetworkReadError("remainingSize>8M: size"+std::to_string(size)+"<pos"+std::to_string(pos)+" to final packet making: "+std::string(__FILE__)+":"+std::to_string(__LINE__)+", data:"+binarytoHexa(data,size));
                return false;
            }
            if(remainingSize==0)
            {
                parseNetworkReadError("remainingSize==0: size"+std::to_string(size)+"<pos"+std::to_string(pos)+" to final packet making: "+std::string(__FILE__)+":"+std::to_string(__LINE__)+", data:"+binarytoHexa(data,size));
                return false;
            }
            replySelectListInWaitSize=startString+LinkToGameServer::httpDatapackMirrorRewriteBase.size()+remainingSize;
            replySelectListInWait=new char[replySelectListInWaitSize];
            memcpy(replySelectListInWait+0,data,startString);
            memcpy(replySelectListInWait+startString,LinkToGameServer::httpDatapackMirrorRewriteBase.data(),LinkToGameServer::httpDatapackMirrorRewriteBase.size());
            memcpy(replySelectListInWait+startString+LinkToGameServer::httpDatapackMirrorRewriteBase.size(),data+pos,remainingSize);
            client->allowDynamicSize();
            flags|=0x08;

            DatapackDownloaderBase::datapackDownloaderBase->sendDatapackProgressionBase(client);
            if(DatapackDownloaderBase::datapackDownloaderBase->hashBase.empty())//checksum never done
            {
                replySelectListInWaitQueryNumber=queryNumber;
                DatapackDownloaderBase::datapackDownloaderBase->clientInSuspend.push_back(this);
                if(DatapackDownloaderBase::datapackDownloaderBase->clientInSuspend.size()==1)
                    DatapackDownloaderBase::datapackDownloaderBase->sendDatapackContentBase();
            }
            else if(DatapackDownloaderBase::datapackDownloaderBase->hashBase!=DatapackDownloaderBase::datapackDownloaderBase->sendedHashBase)//need download the datapack content
            {
                replySelectListInWaitQueryNumber=queryNumber;
                DatapackDownloaderBase::datapackDownloaderBase->clientInSuspend.push_back(this);
                if(DatapackDownloaderBase::datapackDownloaderBase->clientInSuspend.size()==1)
                    DatapackDownloaderBase::datapackDownloaderBase->sendDatapackContentBase();
            }
            else
            {
                //send the network reply
                client->removeFromQueryReceived(queryNumber);
                uint32_t posOutput=0;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
                posOutput+=1;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
                posOutput+=1+4;
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(replySelectListInWaitSize);//set the dynamic size

                memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,replySelectListInWait,replySelectListInWaitSize);
                posOutput+=replySelectListInWaitSize;

                client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

                delete replySelectListInWait;
                replySelectListInWait=NULL;
                replySelectListInWaitSize=0;
            }
            return true;
        }
    }
    else if(mainCodeType==0x93 || mainCodeType==0xAC)//Select character
    {
        if(gameServerMode==GameServerMode::Reconnect)
        {
            if(selectedServer.host.empty())
            {
                parseNetworkReadError("Reply of 0205 with empty host");
                return false;
            }
            if(size!=CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER)
            {
                parseNetworkReadError("size!=CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER");
                return false;
            }

            const int &socketFd=LinkToGameServer::tryConnect(selectedServer.host.c_str(),selectedServer.port,5,1);
            if(Q_LIKELY(socketFd>=0))
            {
                epollSocket.reopen(socketFd);
                this->socketFd=socketFd;

                epoll_event event;
                event.data.ptr = this;
                event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;//EPOLLET | EPOLLOUT
                {
                    const int &s = Epoll::epoll.ctl(EPOLL_CTL_ADD, socketFd, &event);
                    if(s == -1)
                    {
                        std::cerr << "epoll_ctl on socket error" << std::endl;
                        disconnectClient();
                        return false;
                    }
                }

                queryIdToReconnect=queryNumber;
                stat=Stat::Reconnecting;
                memcpy(tokenForGameServer,data,CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER);
                //send the protocol
                //wait readTheFirstSslHeader() to sendProtocolHeader();
                haveTheFirstSslHeader=false;
                setConnexionSettings();
                parseIncommingData();
            }
            else
            {
                /*static_cast<EpollClientLoginSlave * const>(client)
                ->parseNetworkReadError("not able to connect on the game server as proxy, parseReplyData("+std::to_string(mainCodeType)+","+std::to_string(queryNumber)+")");*/
                //message done by LinkToGameServer::tryConnect()
                static_cast<EpollClientLoginSlave * const>(client)->disconnectClient();
            }
            selectedServer.host.clear();
        }
        else
        {
            switch(gameServerMode)
            {
                case GameServerMode::Proxy:
                case GameServerMode::Reconnect:
                break;
                default:
                    std::cerr << "Unknown game server mode or not set: " << std::to_string(gameServerMode) << std::endl;
                abort();
            }
            if(size<=40)
            {
                if(size==1)
                {
                    if(client!=NULL)
                    {
                        //send the network reply
                        client->removeFromQueryReceived(queryNumber);
                        uint32_t posOutput=0;
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
                        posOutput+=1;
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
                        posOutput+=1+4;
                        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint32_t)*data;
                        posOutput+=1;

                        client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                    }
                    parseNetworkReadError("error code at charater selection: "+std::to_string((uint32_t)*data));
                }
                else
                {
                    parseNetworkReadError("need more size (hole), data dump: "+binarytoHexa(data,size)+
                                          " at "+std::string(__FILE__)+":"+std::to_string(__LINE__)+
                                          " with main code: "+std::to_string(mainCodeType)+
                                          " for gameServerMode: "+std::to_string(gameServerMode)
                                          );
                }
                return false;
            }
            else
            {
                unsigned int pos=40;

                {
                    if((size-pos)<1)
                    {
                        parseNetworkReadError("need more size");
                        return false;
                    }
                    const uint8_t &stringSize=data[pos];
                    pos+=1;
                    if(stringSize>0)
                    {
                        if((size-pos)<stringSize)
                        {
                            parseNetworkReadError("need more size: "+std::string(__FILE__)+":"+std::to_string(__LINE__)+", data:"+binarytoHexa(data,size));
                            return false;
                        }
                        main=std::string(data+pos,stringSize);
                        pos+=stringSize;
                        if(!regex_search(main,LinkToGameServer::mainDatapackCodeFilter))
                        {
                            parseNetworkReadError("main datapack code wrong: "+main+", data dump: "+binarytoHexa(data,size)+" at "+std::string(__FILE__)+":"+std::to_string(__LINE__)+" with main code: "+std::to_string(mainCodeType));
                            return false;
                        }
                    }
                    else
                    {
                        parseNetworkReadError("main datapack code can't be empty, data dump: "+binarytoHexa(data,size)+" at "+std::string(__FILE__)+":"+std::to_string(__LINE__)+" with main code: "+std::to_string(mainCodeType));
                        return false;
                    }
                }
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                messageParsingLayer("main datapack code: "+main);
                #endif
                {
                    if((size-pos)<1)
                    {
                        parseNetworkReadError("need more size: "+std::string(__FILE__)+":"+std::to_string(__LINE__)+", data:"+binarytoHexa(data,size));
                        return false;
                    }
                    const uint8_t &stringSize=data[pos];
                    pos+=1;
                    if(stringSize>0)
                    {
                        if((size-pos)<stringSize)
                        {
                            parseNetworkReadError("need more size: "+std::string(__FILE__)+":"+std::to_string(__LINE__)+", data:"+binarytoHexa(data,size));
                            return false;
                        }
                        sub=std::string(data+pos,stringSize);
                        pos+=stringSize;
                        if(!regex_search(sub,LinkToGameServer::subDatapackCodeFilter))
                        {
                            parseNetworkReadError("sub datapack code wrong: "+main+", data dump: "+binarytoHexa(data,size)+" at "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                    }
                    else
                        sub.clear();
                }
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                messageParsingLayer("sub datapack code: "+sub);
                #endif

                DatapackDownloaderMainSub *downloader=NULL;
                {
                    if(DatapackDownloaderMainSub::datapackDownloaderMainSub.find(main)==DatapackDownloaderMainSub::datapackDownloaderMainSub.cend())
                        DatapackDownloaderMainSub::datapackDownloaderMainSub[main][sub]=new DatapackDownloaderMainSub(LinkToGameServer::mDatapackBase,main,sub);
                    else if(DatapackDownloaderMainSub::datapackDownloaderMainSub.at(main).find(sub)==DatapackDownloaderMainSub::datapackDownloaderMainSub.at(main).cend())
                        DatapackDownloaderMainSub::datapackDownloaderMainSub[main][sub]=new DatapackDownloaderMainSub(LinkToGameServer::mDatapackBase,main,sub);
                    downloader=DatapackDownloaderMainSub::datapackDownloaderMainSub.at(main).at(sub);
                }
                if((size-pos)<CATCHCHALLENGER_SHA224HASH_SIZE)
                {
                    parseNetworkReadError("need more size: "+std::string(__FILE__)+":"+std::to_string(__LINE__)+", data:"+binarytoHexa(data,size));
                    return false;
                }
                downloader->sendedHashMain.resize(CATCHCHALLENGER_SHA224HASH_SIZE);
                memcpy(downloader->sendedHashMain.data(),data+pos,CATCHCHALLENGER_SHA224HASH_SIZE);
                pos+=CATCHCHALLENGER_SHA224HASH_SIZE;
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                messageParsingLayer("sendedHashMain: "+binarytoHexa(downloader->sendedHashMain));
                #endif
                if(!sub.empty())
                {
                    if((size-pos)<CATCHCHALLENGER_SHA224HASH_SIZE)
                    {
                        parseNetworkReadError("need more size: "+std::string(__FILE__)+":"+std::to_string(__LINE__)+", data:"+binarytoHexa(data,size));
                        return false;
                    }
                    downloader->sendedHashSub.resize(CATCHCHALLENGER_SHA224HASH_SIZE);
                    memcpy(downloader->sendedHashSub.data(),data+pos,CATCHCHALLENGER_SHA224HASH_SIZE);
                    pos+=CATCHCHALLENGER_SHA224HASH_SIZE;
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    messageParsingLayer("sendedHashSub: "+binarytoHexa(downloader->sendedHashSub));
                    #endif
                }
                if((size-pos)<1)
                {
                    parseNetworkReadError("need more size: "+std::string(__FILE__)+":"+std::to_string(__LINE__)+", data:"+binarytoHexa(data,size));
                    return false;
                }
                const uint16_t startString=pos;
                const uint8_t &stringSize=data[pos];
                pos+=1;
                if((size-pos)<stringSize)
                {
                    parseNetworkReadError("need more size: "+std::string(__FILE__)+":"+std::to_string(__LINE__)+", data:"+binarytoHexa(data,size));
                    return false;
                }
                if(stringSize>0)
                {
                    const std::string httpDatapackMirrorServer(data+pos,stringSize);
                    pos+=stringSize;
                    if(DatapackDownloaderMainSub::httpDatapackMirrorServerList.empty())//can't change for performance and current download in progression corruption
                    {
                        DatapackDownloaderMainSub::httpDatapackMirrorServerList=stringsplit(httpDatapackMirrorServer,';');
                        vectorRemoveEmpty(DatapackDownloaderMainSub::httpDatapackMirrorServerList);
                        {
                            unsigned int index=0;
                            while(index<DatapackDownloaderMainSub::httpDatapackMirrorServerList.size())
                            {
                                const std::string &currentMirror=DatapackDownloaderMainSub::httpDatapackMirrorServerList.at(index);

                                const std::string::size_type &pos=currentMirror.find(LinkToGameServer::protocolString);
                                if(pos==std::string::npos)
                                {
                                    parseNetworkReadError("no \"://\" found: "+currentMirror+", data dump: "+binarytoHexa(data,size));
                                    return false;
                                }
                                const std::string &protocol=currentMirror.substr(0,pos);
                                if(protocol!=LinkToGameServer::protocolHttp && protocol!=LinkToGameServer::protocolHttps)
                                {
                                    parseNetworkReadError("protocol not supported: "+protocol+" into "+currentMirror+", data dump: "+binarytoHexa(data,size));
                                    return false;
                                }

                                if(!stringEndsWith(currentMirror,'/'))
                                    DatapackDownloaderMainSub::httpDatapackMirrorServerList[index]+='/';
                                index++;
                            }
                        }
                    }
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    messageParsingLayer("httpDatapackMirrorServer: "+httpDatapackMirrorServer);
                    #endif
                }
                if(size<pos)
                {
                    parseNetworkReadError("size"+std::to_string(size)+"<pos"+std::to_string(pos)+" to final packet making: "+std::string(__FILE__)+":"+std::to_string(__LINE__)+", data:"+binarytoHexa(data,size));
                    return false;
                }
                unsigned int remainingSize=size-pos;
                if(remainingSize>8*1024*1024)
                {
                    parseNetworkReadError("remainingSize>8M: size"+std::to_string(size)+"<pos"+std::to_string(pos)+" to final packet making: "+std::string(__FILE__)+":"+std::to_string(__LINE__)+", data:"+binarytoHexa(data,size));
                    return false;
                }
                if(remainingSize==0)
                {
                    parseNetworkReadError("remainingSize==0: size"+std::to_string(size)+"<pos"+std::to_string(pos)+" to final packet making: "+std::string(__FILE__)+":"+std::to_string(__LINE__)+", data:"+binarytoHexa(data,size));
                    return false;
                }
                replySelectCharInWaitSize=startString+LinkToGameServer::httpDatapackMirrorRewriteBase.size()+remainingSize;
                replySelectCharInWait=new char[replySelectCharInWaitSize];
                memcpy(replySelectCharInWait+0,data,startString);
                memcpy(replySelectCharInWait+startString,LinkToGameServer::httpDatapackMirrorRewriteBase.data(),LinkToGameServer::httpDatapackMirrorRewriteBase.size());
                memcpy(replySelectCharInWait+startString+LinkToGameServer::httpDatapackMirrorRewriteBase.size(),data+pos,remainingSize);

                downloader->sendDatapackProgressionMainSub(client);
                if(downloader->hashMain.empty() || (!sub.empty() && downloader->hashSub.empty()))//checksum never done
                {
                    replySelectCharInWaitQueryNumber=queryNumber;
                    downloader->clientInSuspend.push_back(this);
                    if(downloader->clientInSuspend.size()==1)
                        downloader->sendDatapackContentMainSub();
                }
                else if(downloader->hashMain!=downloader->sendedHashMain || (!sub.empty() && downloader->hashSub!=downloader->sendedHashSub))//need download the datapack content
                {
                    replySelectCharInWaitQueryNumber=queryNumber;
                    downloader->clientInSuspend.push_back(this);
                    if(downloader->clientInSuspend.size()==1)
                        downloader->sendDatapackContentMainSub();
                }
                else
                {
                    //send the network reply
                    if(!client->removeFromQueryReceived(queryNumber))
                    {
                        errorParsingLayer("!client->removeFromQueryReceived("+std::to_string(queryNumber)+"): already replied?");
                        return false;
                    }
                    uint32_t posOutput=0;
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
                    posOutput+=1;
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
                    posOutput+=1+4;
                    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(replySelectCharInWaitSize);//set the dynamic size

                    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,replySelectCharInWait,replySelectCharInWaitSize);
                    posOutput+=replySelectCharInWaitSize;

                    client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

                    delete replySelectCharInWait;
                    replySelectCharInWait=NULL;
                    replySelectCharInWaitSize=0;
                }
                return true;
            }
        }
    }
    //intercept the file sending
    else if(mainCodeType==0xA1)//reply of datapack file list
    {
        if(replySelectListInWait!=NULL)
        {
            DatapackDownloaderBase::datapackDownloaderBase->datapackFileList(data,size);
            return true;
        }
        else if(replySelectCharInWait!=NULL)
        {
            switch(client->datapackStatus)
            {
                case EpollClientLoginSlave::DatapackStatus::Base:
                {
                    DatapackDownloaderBase *downloader=DatapackDownloaderBase::datapackDownloaderBase;
                    downloader->datapackFileList(data,size);
                }
                break;
                case EpollClientLoginSlave::DatapackStatus::Main:
                {
                    DatapackDownloaderMainSub *downloader=NULL;
                    if(DatapackDownloaderMainSub::datapackDownloaderMainSub.find(main)==DatapackDownloaderMainSub::datapackDownloaderMainSub.cend())
                        DatapackDownloaderMainSub::datapackDownloaderMainSub[main][sub]=new DatapackDownloaderMainSub(LinkToGameServer::mDatapackBase,main,sub);
                    else if(DatapackDownloaderMainSub::datapackDownloaderMainSub.at(main).find(sub)==DatapackDownloaderMainSub::datapackDownloaderMainSub.at(main).cend())
                        DatapackDownloaderMainSub::datapackDownloaderMainSub[main][sub]=new DatapackDownloaderMainSub(LinkToGameServer::mDatapackBase,main,sub);
                    downloader=DatapackDownloaderMainSub::datapackDownloaderMainSub.at(main).at(sub);
                    downloader->datapackFileList(data,size);
                }
                break;
                case EpollClientLoginSlave::DatapackStatus::Sub:
                {
                    DatapackDownloaderMainSub *downloader=NULL;
                    if(DatapackDownloaderMainSub::datapackDownloaderMainSub.find(main)==DatapackDownloaderMainSub::datapackDownloaderMainSub.cend())
                        DatapackDownloaderMainSub::datapackDownloaderMainSub[main][sub]=new DatapackDownloaderMainSub(LinkToGameServer::mDatapackBase,main,sub);
                    else if(DatapackDownloaderMainSub::datapackDownloaderMainSub.at(main).find(sub)==DatapackDownloaderMainSub::datapackDownloaderMainSub.at(main).cend())
                        DatapackDownloaderMainSub::datapackDownloaderMainSub[main][sub]=new DatapackDownloaderMainSub(LinkToGameServer::mDatapackBase,main,sub);
                    downloader=DatapackDownloaderMainSub::datapackDownloaderMainSub.at(main).at(sub);
                    downloader->datapackFileList(data,size);
                }
                break;
                default:
                return false;
            }

            //block copied from above
            //send the network reply
            if(!client->removeFromQueryReceived(replySelectCharInWaitQueryNumber))
            {
                errorParsingLayer("!client->removeFromQueryReceived("+std::to_string(replySelectCharInWaitQueryNumber)+"): already replied?");
                return false;
            }
            uint32_t posOutput=0;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=replySelectCharInWaitQueryNumber;
            posOutput+=1+4;
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(replySelectCharInWaitSize);//set the dynamic size

            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,replySelectCharInWait,replySelectCharInWaitSize);
            posOutput+=replySelectCharInWaitSize;

            client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

            delete replySelectCharInWait;
            replySelectCharInWait=NULL;
            replySelectCharInWaitSize=0;

            return true;
        }
        else
        {
            parseNetworkReadError("unable to route mainCodeType==0x02 && subCodeType==0x0C return");
            return false;
        }
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(client==NULL)
    {
        std::cerr << "client==NULL can't be at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    #endif

    if(mainCodeType==0x93 /*Select character on game server*/ || mainCodeType==0xAC/*Select character*/)
        client->fastForward=true;

    std::cout << "remove for query: " << std::to_string(queryNumber) << ", main code: " << std::to_string(mainCodeType) << std::endl;
    //send the network reply
    if(!client->removeFromQueryReceived(queryNumber))
    {
        errorParsingLayer("!client->removeFromQueryReceived("+std::to_string(queryNumber)+"): already replied?");
        return false;
    }
    const uint8_t &fixedSize=ProtocolParsingBase::packetFixedSize[mainCodeType+128];
    if(fixedSize!=0xFE)
    {
        //fixed size
        //send the network message
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
        posOutput+=1;

        memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1,data,size);
        posOutput+=size;

        return client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
    else
    {
        //dynamic size
        //send the network message
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
        posOutput+=1+4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(size);//set the dynamic size

        memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1+4,data,size);
        posOutput+=size;

        return client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }

    return true;
}

void LinkToGameServer::parseNetworkReadError(const std::string &errorString)
{
    errorParsingLayer(errorString);
}
