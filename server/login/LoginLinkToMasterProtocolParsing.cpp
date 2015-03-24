#include "LoginLinkToMaster.h"
#include <iostream>

using namespace CatchChallenger;

void LoginLinkToMaster::parseInputBeforeLogin(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    Q_UNUSED(size);
    switch(mainCodeType)
    {
        case 0x03:
        break;
        case 0x04:
        break;
        case 0x05:
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
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
}

//send reply
void LoginLinkToMaster::parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    queryNumberList.push_back(queryNumber);
    Q_UNUSED(data);
    Q_UNUSED(size);
    //do the work here
    switch(mainCodeType)
    {
        case 0x08:
        Account creation: 01
        Character delete time: 00000001
        Character max: 01
        Character min: 01
        Pseudo size max: 10
        Characters group list size: 01
            text: "hello" 8bits headers + utf8
        50 maxAccountIdList: 00000001
            50 maxCharacterIdList: 00000001
            50 maxMonsterIdList: 00000001
        break;
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
    parseNetworkReadError(QStringLiteral("The server for now not ask anything: %1, %2").arg(mainCodeType).arg(queryNumber));
    return;
}

void LoginLinkToMaster::parseFullReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    if(!have_send_protocol)
    {
        std::cerr << "parseFullReplyData() reply to unknown query: mainCodeType: " << mainCodeType << ", subCodeType: " << subCodeType << ", queryNumber: " << queryNumber << std::endl;
        abort();
    }
    (void)data;
    (void)size;
    queryNumberList.push_back(queryNumber);
    Q_UNUSED(data);
    parseNetworkReadError(QStringLiteral("The server for now not ask anything: %1 %2, %3").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
}

void LoginLinkToMaster::parseNetworkReadError(const QString &errorString)
{
    errorParsingLayer(errorString);
}
