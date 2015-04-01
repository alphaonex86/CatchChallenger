#include "EpollClientLoginSlave.h"

#include <iostream>

using namespace CatchChallenger;

void EpollClientLoginSlave::parseInputBeforeLogin(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    Q_UNUSED(size);
    switch(mainCodeType)
    {
        case 0x03:
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            removeFromQueryReceived(queryNumber);
            #endif
            //if lot of un logged connection, remove the first
            if(EpollClientLoginSlave::tokenForAuthSize>=CATCHCHALLENGER_SERVER_MAXNOTLOGGEDCONNECTION)
            {
                EpollClientLoginSlave *client=static_cast<EpollClientLoginSlave *>(EpollClientLoginSlave::tokenForAuth[0].client);
                client->disconnectClient();
                delete client;
                EpollClientLoginSlave::tokenForAuthSize--;
                if(EpollClientLoginSlave::tokenForAuthSize>0)
                {
                    quint32 index=0;
                    while(index<EpollClientLoginSlave::tokenForAuthSize)
                    {
                        EpollClientLoginSlave::tokenForAuth[index]=EpollClientLoginSlave::tokenForAuth[index+1];
                        index++;
                    }
                    //don't work:memmove(EpollClientLoginSlave::tokenForAuth,EpollClientLoginSlave::tokenForAuth+sizeof(TokenLink),EpollClientLoginSlave::tokenForAuthSize*sizeof(TokenLink));
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(EpollClientLoginSlave::tokenForAuth[0].client==NULL)
                        abort();
                    #endif
                }
                return;
            }
            if(memcmp(data,EpollClientLoginSlave::protocolHeaderToMatch,sizeof(EpollClientLoginSlave::protocolHeaderToMatch))==0)
            {
                TokenLink *token=&EpollClientLoginSlave::tokenForAuth[EpollClientLoginSlave::tokenForAuthSize];
                {
                    token->client=this;
                    if(fpRandomFile==NULL)
                    {
                        int index=0;
                        while(index<CATCHCHALLENGER_TOKENSIZE)
                        {
                            token->value[index]=rand()%256;
                            index++;
                        }
                    }
                    else
                    {
                        if(fread(token->value,CATCHCHALLENGER_TOKENSIZE,1,fpRandomFile)!=CATCHCHALLENGER_TOKENSIZE)
                        {
                            parseNetworkReadError("Not correct number of byte to generate the token");
                            return;
                        }
                    }
                }
                #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                switch(ProtocolParsing::compressionType)
                {
                    case CompressionType_None:
                        *(EpollClientLoginSlave::protocolReplyCompressionNone+1)=queryNumber;
                        memcpy(EpollClientLoginSlave::protocolReplyCompressionNone+4,token->value,CATCHCHALLENGER_TOKENSIZE);
                        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::protocolReplyCompressionNone),sizeof(EpollClientLoginSlave::protocolReplyCompressionNone));
                    break;
                    case CompressionType_Zlib:
                        *(EpollClientLoginSlave::protocolReplyCompresssionZlib+1)=queryNumber;
                        memcpy(EpollClientLoginSlave::protocolReplyCompresssionZlib+4,token->value,CATCHCHALLENGER_TOKENSIZE);
                        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::protocolReplyCompresssionZlib),sizeof(EpollClientLoginSlave::protocolReplyCompresssionZlib));
                    break;
                    case CompressionType_Xz:
                        *(EpollClientLoginSlave::protocolReplyCompressionXz+1)=queryNumber;
                        memcpy(EpollClientLoginSlave::protocolReplyCompressionXz+4,token->value,CATCHCHALLENGER_TOKENSIZE);
                        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::protocolReplyCompressionXz),sizeof(EpollClientLoginSlave::protocolReplyCompressionXz));
                    break;
                    default:
                        parseNetworkReadError("Compression selected wrong");
                    return;
                }
                #else
                *(EpollClientLoginSlave::protocolReplyCompressionNone+1)=queryNumber;
                memcpy(EpollClientLoginSlave::protocolReplyCompressionNone+4,token->value,CATCHCHALLENGER_TOKENSIZE);
                internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::protocolReplyCompressionNone),sizeof(EpollClientLoginSlave::protocolReplyCompressionNone));
                #endif
                EpollClientLoginSlave::tokenForAuthSize++;
                have_send_protocol=true;
                #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
                normalOutput(QStringLiteral("Protocol sended and replied"));
                #endif
            }
            else
            {
                *(EpollClientLoginSlave::protocolReplyProtocolNotSupported+1)=queryNumber;
                internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::protocolReplyProtocolNotSupported),sizeof(EpollClientLoginSlave::protocolReplyProtocolNotSupported));
                parseNetworkReadError("Wrong protocol");
                return;
            }
        break;
        case 0x04:
            if(!have_send_protocol)
            {
                parseNetworkReadError("send login before the protocol");
                return;
            }
            if(is_logging_in_progess)
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                removeFromQueryReceived(queryNumber);
                #endif
                *(EpollClientLoginSlave::loginInProgressBuffer+1)=queryNumber;
                internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::loginInProgressBuffer),sizeof(EpollClientLoginSlave::loginInProgressBuffer));
                parseNetworkReadError("Loggin already in progress");
            }
            else
            {
                is_logging_in_progess=true;
                askLogin(queryNumber,data);
                return;
            }
        break;
        case 0x05:
            if(!have_send_protocol)
            {
                parseNetworkReadError("send login before the protocol");
                return;
            }
            if(is_logging_in_progess)
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                removeFromQueryReceived(queryNumber);
                #endif
                *(EpollClientLoginSlave::loginInProgressBuffer+1)=queryNumber;
                internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::loginInProgressBuffer),sizeof(EpollClientLoginSlave::loginInProgressBuffer));
                parseNetworkReadError("Loggin already in progress");
            }
            else
            {
                if(automatic_account_creation)
                {
                    is_logging_in_progess=true;
                    createAccount(queryNumber,data);
                    return;
                }
                else
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    removeFromQueryReceived(queryNumber);
                    #endif
                    *(EpollClientLoginSlave::loginInProgressBuffer+1)=queryNumber;
                    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginSlave::loginInProgressBuffer),sizeof(EpollClientLoginSlave::loginInProgressBuffer));
                    parseNetworkReadError("Account creation not premited");
                }
            }
        break;
        default:
            parseNetworkReadError("wrong data before login with mainIdent: "+QString::number(mainCodeType));
        break;
    }
}

void EpollClientLoginSlave::parseMessage(const quint8 &mainCodeType,const char *data,const int &size)
{
    (void)data;
    (void)size;
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
}

void EpollClientLoginSlave::parseFullMessage(const quint8 &mainCodeType,const quint16 &subCodeType,const char *rawData,const int &size)
{
    (void)rawData;
    (void)size;
    (void)subCodeType;
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
}

//have query with reply
void EpollClientLoginSlave::parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    Q_UNUSED(data);
    if(!have_send_protocol)
    {
        parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
        return;
    }
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
}

void EpollClientLoginSlave::parseFullQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char *rawData,const int &size)
{
    (void)subCodeType;
    (void)queryNumber;
    (void)rawData;
    (void)size;
    if(account_id==0)
    {
        parseNetworkReadError(QStringLiteral("is not logged, parseQuery(%1,%2)").arg(mainCodeType).arg(queryNumber));
        return;
    }
    //do the work here
    switch(mainCodeType)
    {
        case 0x02:
        switch(subCodeType)
        {
            //Add character
            case 0x03:
            {
                int cursor=0;
                quint8 charactersGroupIndex;
                quint8 profileIndex;
                QString pseudo;
                quint8 skinId;
                quint8 pseudoSize;
                if((size-cursor)<(int)sizeof(quint8))
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                charactersGroupIndex=rawData[cursor];
                cursor+=1;
                if((size-cursor)<(int)sizeof(quint8))
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                profileIndex=rawData[cursor];
                cursor+=1;
                if((size-cursor)<(int)sizeof(quint8))
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                pseudoSize=rawData[cursor];
                cursor+=1;
                if((size-cursor)<(int)pseudoSize)
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                pseudo=QString::fromUtf8(rawData+cursor,pseudoSize);
                if((size-cursor)<(int)sizeof(quint8))
                {
                    parseNetworkReadError(QStringLiteral("error to get skin with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                skinId=rawData[cursor];
                cursor+=1;
                addCharacter(queryNumber,charactersGroupIndex,profileIndex,pseudo,skinId);
                if((size-cursor)!=0)
                {
                    parseNetworkReadError(QStringLiteral("remaining data: parseQuery(%1,%2,%3): %4 %5")
                               .arg(mainCodeType)
                               .arg(subCodeType)
                               .arg(queryNumber)
                               .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                               .arg(QString(data.mid(in.device()->pos(),(size-cursor)).toHex()))
                               );
                    return;
                }
                return;
            }
            break;
            //Remove character
            case 0x004:
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=((int)sizeof(quint32)+(int)sizeof(quint8)))
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                const quint8 &charactersGroupIndex=rawData[0];
                const quint32 &characterId=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+1)));
                removeCharacter(queryNumber,charactersGroupIndex,characterId);
            }
            break;
            //Select character
            case 0x05:
            {
                const quint32 &serverUniqueKey=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+0)));
                const quint8 &charactersGroupIndex=rawData[4];
                const quint32 &characterId=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+5)));
                selectCharacter(queryNumber,serverUniqueKey,charactersGroupIndex,characterId);
            }
            default:
                parseNetworkReadError(QStringLiteral("ident: %1, unknown sub ident: %2").arg(mainCodeType).arg(subCodeType));
                return;
            break;
        }
        break;
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
}

//send reply
void EpollClientLoginSlave::parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    //queryNumberList << queryNumber;
    Q_UNUSED(data);
    Q_UNUSED(size);
    parseNetworkReadError(QStringLiteral("The server for now not ask anything: %1, %2").arg(mainCodeType).arg(queryNumber));
    return;
}

void EpollClientLoginSlave::parseFullReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    (void)data;
    (void)size;
    //queryNumberList << queryNumber;
    Q_UNUSED(data);
    parseNetworkReadError(QStringLiteral("The server for now not ask anything: %1 %2, %3").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
}

void EpollClientLoginSlave::parseNetworkReadError(const QString &errorString)
{
    errorParsingLayer(errorString);
}
