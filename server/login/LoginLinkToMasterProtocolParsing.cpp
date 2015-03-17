#include "LoginLinkToMaster.h"

using namespace CatchChallenger;

#include <iostream>

using namespace CatchChallenger;

void LoginLinkToMaster::parseInputBeforeLogin(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    Q_UNUSED(size);
    switch(mainCodeType)
    {
        case 0x03:
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            removeFromQueryReceived(queryNumber);
            #endif
            //if lot of un logged connection, remove the first
            if(LoginLinkToMaster::tokenForAuthSize>=CATCHCHALLENGER_SERVER_MAXNOTLOGGEDCONNECTION)
            {
                EpollClientLoginSlave *client=static_cast<EpollClientLoginSlave *>(LoginLinkToMaster::tokenForAuth[0].client);
                client->disconnectClient();
                delete client;
                LoginLinkToMaster::tokenForAuthSize--;
                if(LoginLinkToMaster::tokenForAuthSize>0)
                {
                    quint32 index=0;
                    while(index<LoginLinkToMaster::tokenForAuthSize)
                    {
                        LoginLinkToMaster::tokenForAuth[index]=LoginLinkToMaster::tokenForAuth[index+1];
                        index++;
                    }
                    //don't work:memmove(LoginLinkToMaster::tokenForAuth,LoginLinkToMaster::tokenForAuth+sizeof(TokenLink),LoginLinkToMaster::tokenForAuthSize*sizeof(TokenLink));
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(LoginLinkToMaster::tokenForAuth[0].client==NULL)
                        abort();
                    #endif
                }
                return;
            }
            if(memcmp(data,LoginLinkToMaster::protocolHeaderToMatch,sizeof(LoginLinkToMaster::protocolHeaderToMatch))==0)
            {
                TokenLink *token=&LoginLinkToMaster::tokenForAuth[LoginLinkToMaster::tokenForAuthSize];
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
                        *(LoginLinkToMaster::protocolReplyCompressionNone+1)=queryNumber;
                        memcpy(LoginLinkToMaster::protocolReplyCompressionNone+4,token->value,CATCHCHALLENGER_TOKENSIZE);
                        internalSendRawSmallPacket(reinterpret_cast<char *>(LoginLinkToMaster::protocolReplyCompressionNone),sizeof(LoginLinkToMaster::protocolReplyCompressionNone));
                    break;
                    case CompressionType_Zlib:
                        *(LoginLinkToMaster::protocolReplyCompresssionZlib+1)=queryNumber;
                        memcpy(LoginLinkToMaster::protocolReplyCompresssionZlib+4,token->value,CATCHCHALLENGER_TOKENSIZE);
                        internalSendRawSmallPacket(reinterpret_cast<char *>(LoginLinkToMaster::protocolReplyCompresssionZlib),sizeof(LoginLinkToMaster::protocolReplyCompresssionZlib));
                    break;
                    case CompressionType_Xz:
                        *(LoginLinkToMaster::protocolReplyCompressionXz+1)=queryNumber;
                        memcpy(LoginLinkToMaster::protocolReplyCompressionXz+4,token->value,CATCHCHALLENGER_TOKENSIZE);
                        internalSendRawSmallPacket(reinterpret_cast<char *>(LoginLinkToMaster::protocolReplyCompressionXz),sizeof(LoginLinkToMaster::protocolReplyCompressionXz));
                    break;
                    default:
                        parseNetworkReadError("Compression selected wrong");
                    return;
                }
                #else
                *(LoginLinkToMaster::protocolReplyCompressionNone+1)=queryNumber;
                memcpy(LoginLinkToMaster::protocolReplyCompressionNone+4,token->value,CATCHCHALLENGER_TOKENSIZE);
                internalSendRawSmallPacket(reinterpret_cast<char *>(LoginLinkToMaster::protocolReplyCompressionNone),sizeof(LoginLinkToMaster::protocolReplyCompressionNone));
                #endif
                LoginLinkToMaster::tokenForAuthSize++;
                have_send_protocol=true;
                #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
                normalOutput(QStringLiteral("Protocol sended and replied"));
                #endif
            }
            else
            {
                *(LoginLinkToMaster::protocolReplyProtocolNotSupported+1)=queryNumber;
                internalSendRawSmallPacket(reinterpret_cast<char *>(LoginLinkToMaster::protocolReplyProtocolNotSupported),sizeof(LoginLinkToMaster::protocolReplyProtocolNotSupported));
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
                *(LoginLinkToMaster::loginInProgressBuffer+1)=queryNumber;
                internalSendRawSmallPacket(reinterpret_cast<char *>(LoginLinkToMaster::loginInProgressBuffer),sizeof(LoginLinkToMaster::loginInProgressBuffer));
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
                *(LoginLinkToMaster::loginInProgressBuffer+1)=queryNumber;
                internalSendRawSmallPacket(reinterpret_cast<char *>(LoginLinkToMaster::loginInProgressBuffer),sizeof(LoginLinkToMaster::loginInProgressBuffer));
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
                    *(LoginLinkToMaster::loginInProgressBuffer+1)=queryNumber;
                    internalSendRawSmallPacket(reinterpret_cast<char *>(LoginLinkToMaster::loginInProgressBuffer),sizeof(LoginLinkToMaster::loginInProgressBuffer));
                    parseNetworkReadError("Account creation not premited");
                }
            }
        break;
        default:
            parseNetworkReadError("wrong data before login with mainIdent: "+QString::number(mainCodeType));
        break;
    }
}

void LoginLinkToMaster::parseMessage(const quint8 &mainCodeType,const char *data,const int &size)
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

void LoginLinkToMaster::parseFullMessage(const quint8 &mainCodeType,const quint16 &subCodeType,const char *rawData,const int &size)
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
void LoginLinkToMaster::parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size)
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

void LoginLinkToMaster::parseFullQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char *rawData,const int &size)
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
            case 0x0003:
            {
                QByteArray data(rawData,size);
                QDataStream in(data);
                in.setVersion(QDataStream::Qt_4_4);
                quint8 profileIndex;
                QString pseudo;
                quint8 skinId;
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                in >> profileIndex;
                if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                {
                    parseNetworkReadError(QStringLiteral("error to get pseudo with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                in >> pseudo;
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseNetworkReadError(QStringLiteral("error to get skin with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(data.toHex())));
                    return;
                }
                in >> skinId;
                addCharacter(queryNumber,profileIndex,pseudo,skinId);
                if((in.device()->size()-in.device()->pos())!=0)
                {
                    parseNetworkReadError(QStringLiteral("remaining data: parseQuery(%1,%2,%3): %4 %5")
                               .arg(mainCodeType)
                               .arg(subCodeType)
                               .arg(queryNumber)
                               .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                               .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                               );
                    return;
                }
                return;
            }
            break;
            //Remove character
            case 0x0004:
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(size!=(int)sizeof(quint32))
                {
                    parseNetworkReadError(QStringLiteral("wrong size with the main ident: %1, data: %2").arg(mainCodeType).arg(QString(QByteArray(rawData,size).toHex())));
                    return;
                }
                #endif
                const quint32 &characterId=be32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData)));
                removeCharacter(queryNumber,characterId);
            }
            break;
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
void LoginLinkToMaster::parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    //queryNumberList << queryNumber;
    Q_UNUSED(data);
    Q_UNUSED(size);
    parseNetworkReadError(QStringLiteral("The server for now not ask anything: %1, %2").arg(mainCodeType).arg(queryNumber));
    return;
}

void LoginLinkToMaster::parseFullReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    (void)data;
    (void)size;
    //queryNumberList << queryNumber;
    Q_UNUSED(data);
    parseNetworkReadError(QStringLiteral("The server for now not ask anything: %1 %2, %3").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
}

void LoginLinkToMaster::parseNetworkReadError(const QString &errorString)
{
    errorParsingLayer(errorString);
}
