#include "Api_protocol.h"

using namespace CatchChallenger;

const unsigned char protocolHeaderToMatch[] = PROTOCOL_HEADER;

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#endif

#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/CommonSettingsServer.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/GeneralType.h"
#include "LanguagesSelect.h"

#include <QCoreApplication>

#ifdef BENCHMARKMUTIPLECLIENT
char Api_protocol::hurgeBufferForBenchmark[4096];
bool Api_protocol::precomputeDone=false;
char Api_protocol::hurgeBufferMove[4];

#include <iostream>
#include <fstream>
#include <unistd.h>
#endif

//need host + port here to have datapack base

QSet<QString> Api_protocol::extensionAllowed;

Api_protocol* Api_protocol::client=NULL;
bool Api_protocol::internalVersionDisplayed=false;

QString Api_protocol::text_balise_root_start="<root>";
QString Api_protocol::text_balise_root_stop="</root>";
QString Api_protocol::text_name="name";
QString Api_protocol::text_description="description";
QString Api_protocol::text_en="en";
QString Api_protocol::text_lang="lang";
QString Api_protocol::text_slash="/";

Api_protocol::Api_protocol(ConnectedSocket *socket,bool tolerantMode) :
    ProtocolParsingInputOutput(socket,PacketModeTransmission_Client),
    tolerantMode(tolerantMode)
{
    #ifdef BENCHMARKMUTIPLECLIENT
    if(!Api_protocol::precomputeDone)
    {
        Api_protocol::precomputeDone=true;
        hurgeBufferMove[0]=0x40;
    }
    #endif
    if(extensionAllowed.isEmpty())
        extensionAllowed=QString(CATCHCHALLENGER_EXTENSION_ALLOWED).split(";").toSet();

    connect(socket,&ConnectedSocket::destroyed,this,&Api_protocol::socketDestroyed,Qt::DirectConnection);
    //connect(socket,&ConnectedSocket::readyRead,this,&Api_protocol::parseIncommingData,Qt::DirectConnection);-> why direct?
    connect(socket,&ConnectedSocket::readyRead,this,&Api_protocol::parseIncommingData,Qt::QueuedConnection);//put queued to don't have circular loop Client -> Server -> Client

    resetAll();

    if(!Api_protocol::internalVersionDisplayed)
    {
        Api_protocol::internalVersionDisplayed=true;
        #if defined(Q_CC_GNU)
            qDebug() << QStringLiteral("GCC %1.%2.%3 build: ").arg(__GNUC__).arg(__GNUC_MINOR__).arg(__GNUC_PATCHLEVEL__)+__DATE__+" "+__TIME__;
        #else
            #if defined(__DATE__) && defined(__TIME__)
                qDebug() << QStringLiteral("Unknown compiler: ")+__DATE__+" "+__TIME__;
            #else
                qDebug() << QStringLiteral("Unknown compiler");
            #endif
        #endif
        qDebug() << QStringLiteral("Qt version: %1 (%2)").arg(qVersion()).arg(QT_VERSION);
    }
}

Api_protocol::~Api_protocol()
{
}

void Api_protocol::disconnectClient()
{
    if(socket!=NULL)
        socket->disconnect();
    is_logged=false;
    character_selected=false;
}

void Api_protocol::socketDestroyed()
{
    socket=NULL;
}

QMap<quint8,QTime> Api_protocol::getQuerySendTimeList() const
{
    return querySendTime;
}

void Api_protocol::parseIncommingData()
{
    ProtocolParsingInputOutput::parseIncommingData();
}

void Api_protocol::errorParsingLayer(const QString &error)
{
    emit newError(tr("Internal error"),error);
}

void Api_protocol::messageParsingLayer(const QString &message) const
{
    qDebug() << message;
}

void Api_protocol::parseError(const QString &userMessage,const QString &errorString)
{
    if(tolerantMode)
        DebugClass::debugConsole(QStringLiteral("packet ignored due to: %1").arg(errorString));
    else
        newError(userMessage,errorString);
}

Player_private_and_public_informations Api_protocol::get_player_informations()
{
    return player_informations;
}

QString Api_protocol::getPseudo()
{
    return player_informations.public_informations.pseudo;
}

quint16 Api_protocol::getId()
{
    return player_informations.public_informations.simplifiedId;
}

quint8 Api_protocol::queryNumber()
{
    if(lastQueryNumber>=254)
        lastQueryNumber=1;
    querySendTime[lastQueryNumber].start();
    return lastQueryNumber++;
}

bool Api_protocol::sendProtocol()
{
    if(have_send_protocol)
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("Have already send the protocol"));
        return false;
    }
    have_send_protocol=true;
    QByteArray outputData(reinterpret_cast<char *>(const_cast<unsigned char *>(protocolHeaderToMatch)),sizeof(protocolHeaderToMatch));
    packOutcommingQuery(0x03,queryNumber(),outputData.constData(),outputData.size());
    return true;
}

bool Api_protocol::tryLogin(const QString &login, const QString &pass)
{
    if(!have_send_protocol)
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("Have not send the protocol"));
        return false;
    }
    if(is_logged)
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("Is already logged"));
        return false;
    }
    if(token.isEmpty())
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("Token is empty"));
        return false;
    }
    QByteArray outputData;
    {
        QCryptographicHash hashLogin(QCryptographicHash::Sha224);
        hashLogin.addData((login+/*salt*/"RtR3bm9Z1DFMfAC3").toLatin1());
        loginHash=hashLogin.result();
        outputData+=loginHash;
    }
    {
        QCryptographicHash hashPass(QCryptographicHash::Sha224);
        hashPass.addData((pass+/*salt*/"AwjDvPIzfJPTTgHs").toLatin1());
        passHash=hashPass.result();

        QCryptographicHash hashAndToken(QCryptographicHash::Sha224);
        hashAndToken.addData(passHash);
        hashAndToken.addData(token);
        outputData+=hashAndToken.result();
    }
    const quint8 &query_number=queryNumber();
    packOutcommingQuery(0x04,query_number,outputData.constData(),outputData.size());
    return true;
}

bool Api_protocol::tryCreate()
{
    if(!have_send_protocol)
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("Have not send the protocol"));
        return false;
    }
    if(is_logged)
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("Is already logged"));
        return false;
    }
    QByteArray outputData;
    outputData+=loginHash;
    outputData+=passHash;
    const quint8 &query_number=queryNumber();
    packOutcommingQuery(0x05,query_number,outputData.constData(),outputData.size());
    return true;
}

void Api_protocol::send_player_move(const quint8 &moved_unit,const Direction &direction)
{
    #ifdef BENCHMARKMUTIPLECLIENT
    hurgeBufferMove[1]=moved_unit;
    hurgeBufferMove[2]=direction;
    const int &infd=socket->sslSocket->socketDescriptor();
    if(infd!=-1)
        ::write(infd,hurgeBufferMove,3);
    else
        internalSendRawSmallPacket(hurgeBufferMove,3);
    return;
    #endif
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    quint8 directionInt=static_cast<quint8>(direction);
    if(directionInt<1 || directionInt>8)
    {
        DebugClass::debugConsole(QStringLiteral("direction given wrong: %1").arg(directionInt));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << moved_unit;
    out << directionInt;
    is_logged=character_selected=packOutcommingData(0x40,outputData.constData(),outputData.size());
}

void Api_protocol::send_player_direction(const Direction &the_direction)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    newDirection(the_direction);
}

void Api_protocol::sendChatText(const Chat_type &chatType, const QString &text)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    if(chatType!=Chat_type_local && chatType!=Chat_type_all && chatType!=Chat_type_clan && chatType!=Chat_type_aliance && chatType!=Chat_type_system && chatType!=Chat_type_system_important)
    {
        DebugClass::debugConsole("chatType wrong: "+QString::number(chatType));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)chatType;
    {
        const QByteArray &tempText=text.toUtf8();
        if(tempText.size()>255)
        {
            DebugClass::debugConsole(QStringLiteral("text in Utf8 too big, line: %1").arg(__LINE__));
            return;
        }
        out << (quint8)tempText.size();
        outputData+=tempText;
        out.device()->seek(out.device()->pos()+tempText.size());
    }
    is_logged=character_selected=packOutcommingData(0x43,outputData.constData(),outputData.size());
}

void Api_protocol::sendPM(const QString &text,const QString &pseudo)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    if(this->player_informations.public_informations.pseudo==pseudo)
        return;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)Chat_type_pm;
    {
        const QByteArray &tempText=text.toUtf8();
        if(tempText.size()>255)
        {
            DebugClass::debugConsole(QStringLiteral("text in Utf8 too big, line: %1").arg(__LINE__));
            return;
        }
        out << (quint8)tempText.size();
        outputData+=tempText;
        out.device()->seek(out.device()->pos()+tempText.size());
    }
    {
        const QByteArray &tempText=pseudo.toUtf8();
        if(tempText.size()>255)
        {
            DebugClass::debugConsole(QStringLiteral("text in Utf8 too big, line: %1").arg(__LINE__));
            return;
        }
        out << (quint8)tempText.size();
        outputData+=tempText;
        out.device()->seek(out.device()->pos()+tempText.size());
    }
    is_logged=character_selected=packOutcommingData(0x43,outputData.constData(),outputData.size());
}

void Api_protocol::teleportDone()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    is_logged=character_selected=postReplyData(teleportList.first(),NULL,0);
    teleportList.removeFirst();
}

bool Api_protocol::addCharacter(const quint8 &charactersGroupIndex,const quint8 &profileIndex, const QString &pseudo, const quint8 &skinId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return false;
    }
    if(skinId>=CommonDatapack::commonDatapack.skins.size())
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("skin provided: %1 is not into skin listed").arg(skinId));
        return false;
    }
    const Profile &profile=CommonDatapack::commonDatapack.profileList.at(profileIndex);
    if(!profile.forcedskin.isEmpty() && !profile.forcedskin.contains(skinId))
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("skin provided: %1 is not into profile %2 forced skin list").arg(skinId).arg(profileIndex));
        return false;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)charactersGroupIndex;
    out << (quint8)profileIndex;
    {
        const QByteArray &rawPseudo=FacilityLibGeneral::toUTF8WithHeader(pseudo);
        if(rawPseudo.size()>255 || rawPseudo.isEmpty())
        {
            DebugClass::debugConsole(QStringLiteral("rawPseudo too big or not compatible with utf8"));
            return false;
        }
        outputData+=rawPseudo;
        out.device()->seek(out.device()->size());
    }
    out << (quint8)skinId;
    is_logged=packFullOutcommingQuery(0x02,0x03,queryNumber(),outputData.constData(),outputData.size());
    return true;
}

bool Api_protocol::removeCharacter(const quint8 &charactersGroupIndex,const quint32 &characterId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return false;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)charactersGroupIndex;
    out << characterId;
    is_logged=packFullOutcommingQuery(0x02,0x04,queryNumber(),outputData.constData(),outputData.size());
    return true;
}

bool Api_protocol::selectCharacter(const quint8 &charactersGroupIndex,const quint32 &serverUniqueKey,const quint32 &characterId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return false;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)charactersGroupIndex;
    out << (quint32)serverUniqueKey;
    out << characterId;
    is_logged=packFullOutcommingQuery(0x02,0x05,queryNumber(),outputData.constData(),outputData.size());
    unloadSelection();
    return true;
}

void Api_protocol::useSeed(const quint8 &plant_id)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    outputData[0]=plant_id;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x06,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::monsterMoveUp(const quint8 &number)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)0x01;
    out << number;
    is_logged=character_selected=packFullOutcommingData(0x60,0x08,outputData.constData(),outputData.size());
}

void Api_protocol::confirmEvolution(const quint32 &monterId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint32)monterId;
    is_logged=character_selected=packFullOutcommingData(0x60,0x000A,outputData.constData(),outputData.size());
}

void Api_protocol::monsterMoveDown(const quint8 &number)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)0x02;
    out << number;
    is_logged=character_selected=packFullOutcommingData(0x60,0x08,outputData.constData(),outputData.size());
}

//inventory
void Api_protocol::destroyObject(const quint16 &object, const quint32 &quantity)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << object;
    out << quantity;
    is_logged=character_selected=packFullOutcommingData(0x50,0x02,outputData.constData(),outputData.size());
}

void Api_protocol::useObject(const quint16 &object)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << object;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x09,queryNumber(),outputData.constData(),outputData.size());
    lastObjectUsed << object;
}

void Api_protocol::useObjectOnMonster(const quint16 &object,const quint32 &monster)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << object;
    out << monster;
    is_logged=character_selected=packFullOutcommingData(0x60,0x000B,outputData.constData(),outputData.size());
}


void Api_protocol::wareHouseStore(const qint64 &cash, const QList<QPair<quint16,qint32> > &items, const QList<quint32> &withdrawMonsters, const QList<quint32> &depositeMonsters)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << cash;

    out << (quint16)items.size();
    int index=0;
    while(index<items.size())
    {
        out << (quint16)items.at(index).first;
        out << (qint32)items.at(index).second;
        index++;
    }

    out << (quint32)withdrawMonsters.size();
    index=0;
    while(index<withdrawMonsters.size())
    {
        out << (quint32)withdrawMonsters.at(index);
        index++;
    }
    out << (quint32)depositeMonsters.size();
    index=0;
    while(index<depositeMonsters.size())
    {
        out << (quint32)depositeMonsters.at(index);
        index++;
    }

    is_logged=character_selected=packFullOutcommingData(0x50,0x06,outputData.constData(),outputData.size());
}

void Api_protocol::takeAnObjectOnMap()
{
    packFullOutcommingData(0x50,0x07,NULL,0);
}

void Api_protocol::getShopList(const quint32 &shopId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint16)shopId;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x000A,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::buyObject(const quint32 &shopId,const quint32 &objectId,const quint32 &quantity,const quint32 &price)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint16)shopId;
    out << (quint16)objectId;
    out << (quint32)quantity;
    out << (quint32)price;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x000B,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::sellObject(const quint32 &shopId,const quint32 &objectId,const quint32 &quantity,const quint32 &price)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint16)shopId;
    out << (quint16)objectId;
    out << (quint32)quantity;
    out << (quint32)price;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x000C,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::getFactoryList(const quint16 &factoryId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint16)factoryId;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x000D,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::buyFactoryProduct(const quint16 &factoryId,const quint16 &objectId,const quint32 &quantity,const quint32 &price)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint16)factoryId;
    out << (quint16)objectId;
    out << (quint32)quantity;
    out << (quint32)price;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x000E,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::sellFactoryResource(const quint16 &factoryId,const quint16 &objectId,const quint32 &quantity,const quint32 &price)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint16)factoryId;
    out << (quint16)objectId;
    out << (quint32)quantity;
    out << (quint32)price;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x000F,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::tryEscape()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    is_logged=character_selected=packFullOutcommingData(0x60,0x02,NULL,0);
}

void Api_protocol::heal()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    is_logged=character_selected=packFullOutcommingData(0x60,0x06,NULL,0);
}

void Api_protocol::requestFight(const quint32 &fightId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint16)fightId;
    is_logged=character_selected=packFullOutcommingData(0x60,0x07,outputData.constData(),outputData.size());
}

void Api_protocol::changeOfMonsterInFight(const quint32 &monsterId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint32)monsterId;
    is_logged=character_selected=packFullOutcommingData(0x60,0x09,outputData.constData(),outputData.size());
}

void Api_protocol::useSkill(const quint16 &skill)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint16)skill;
    is_logged=character_selected=packOutcommingData(0x61,outputData.constData(),outputData.size());
}

void Api_protocol::learnSkill(const quint32 &monsterId,const quint16 &skill)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint32)monsterId;
    out << (quint16)skill;
    is_logged=character_selected=packFullOutcommingData(0x60,0x04,outputData.constData(),outputData.size());
}

void Api_protocol::startQuest(const quint16 &questId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint16)questId;
    is_logged=character_selected=packFullOutcommingData(0x6a,0x01,outputData.constData(),outputData.size());
}

void Api_protocol::finishQuest(const quint16 &questId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint16)questId;
    is_logged=character_selected=packFullOutcommingData(0x6a,0x02,outputData.constData(),outputData.size());
}

void Api_protocol::cancelQuest(const quint16 &questId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint16)questId;
    is_logged=character_selected=packFullOutcommingData(0x6a,0x03,outputData.constData(),outputData.size());
}

void Api_protocol::nextQuestStep(const quint16 &questId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint16)questId;
    is_logged=character_selected=packFullOutcommingData(0x6a,0x04,outputData.constData(),outputData.size());
}

void Api_protocol::createClan(const QString &name)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)0x01;
    out << name;
    is_logged=character_selected=packFullOutcommingQuery(0x02,0x000D,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::leaveClan()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)0x02;
    is_logged=character_selected=packFullOutcommingQuery(0x02,0x000D,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::dissolveClan()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)0x03;
    is_logged=character_selected=packFullOutcommingQuery(0x02,0x000D,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::inviteClan(const QString &pseudo)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)0x04;
    out << pseudo;
    is_logged=character_selected=packFullOutcommingQuery(0x02,0x000D,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::ejectClan(const QString &pseudo)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)0x05;
    out << pseudo;
    is_logged=character_selected=packFullOutcommingQuery(0x02,0x000D,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::inviteAccept(const bool &accept)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    if(accept)
        out << (quint8)0x01;
    else
        out << (quint8)0x02;
    is_logged=character_selected=packFullOutcommingData(0x42,0x04,outputData.constData(),outputData.size());
}

void Api_protocol::waitingForCityCapture(const bool &cancel)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    if(!cancel)
        out << (quint8)0x00;
    else
        out << (quint8)0x01;
    is_logged=character_selected=packFullOutcommingData(0x6a,0x05,outputData.constData(),outputData.size());
}

//market
void Api_protocol::getMarketList()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x10,queryNumber(),NULL,0);
}

void Api_protocol::buyMarketObject(const quint32 &marketObjectId, const quint32 &quantity)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)0x01;
    out << marketObjectId;
    out << quantity;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x11,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::buyMarketMonster(const quint32 &monsterId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)0x02;
    out << monsterId;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x11,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::putMarketObject(const quint32 &objectId,const quint32 &quantity,const quint32 &price)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)0x01;
    out << objectId;
    out << quantity;
    out << price;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x12,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::putMarketMonster(const quint32 &monsterId,const quint32 &price)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)0x02;
    out << monsterId;
    out << price;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x12,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::recoverMarketCash()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x13,queryNumber(),NULL,0);
}

void Api_protocol::withdrawMarketObject(const quint32 &objectId,const quint32 &quantity)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)0x01;
    out << objectId;
    out << quantity;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x14,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::withdrawMarketMonster(const quint32 &monsterId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)0x02;
    out << monsterId;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x14,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::collectMaturePlant()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x07,queryNumber(),NULL,0);
}

//crafting
void Api_protocol::useRecipe(const quint16 &recipeId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint16)recipeId;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x08,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::addRecipe(const quint16 &recipeId)
{
    player_informations.recipes << recipeId;
}

void Api_protocol::battleRefused()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    if(battleRequestId.isEmpty())
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("no battle request to refuse"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)0x02;
    is_logged=character_selected=postReplyData(battleRequestId.first(),outputData.constData(),outputData.size());
    battleRequestId.removeFirst();
}

void Api_protocol::battleAccepted()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    if(battleRequestId.isEmpty())
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("no battle request to accept"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)0x01;
    is_logged=character_selected=postReplyData(battleRequestId.first(),outputData.constData(),outputData.size());
    battleRequestId.removeFirst();
}

//trade
void Api_protocol::tradeRefused()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    if(tradeRequestId.isEmpty())
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("no trade request to refuse"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)0x02;
    is_logged=character_selected=postReplyData(tradeRequestId.first(),outputData.constData(),outputData.size());
    tradeRequestId.removeFirst();
}

void Api_protocol::tradeAccepted()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    if(tradeRequestId.isEmpty())
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("no trade request to accept"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)0x01;
    is_logged=character_selected=postReplyData(tradeRequestId.first(),outputData.constData(),outputData.size());
    tradeRequestId.removeFirst();
    isInTrade=true;
}

void Api_protocol::tradeCanceled()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    if(!isInTrade)
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("in not in trade"));
        return;
    }
    isInTrade=false;
    is_logged=character_selected=packFullOutcommingData(0x50,0x05,NULL,0);
}

void Api_protocol::tradeFinish()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    if(!isInTrade)
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("in not in trade"));
        return;
    }
    is_logged=character_selected=packFullOutcommingData(0x50,0x04,NULL,0);
}

void Api_protocol::addTradeCash(const quint64 &cash)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    if(cash==0)
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("can't send 0 for the cash"));
        return;
    }
    if(!isInTrade)
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("no in trade to send cash"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)0x01;
    out << cash;
    is_logged=character_selected=packFullOutcommingData(0x50,0x03,outputData.constData(),outputData.size());
}

void Api_protocol::addObject(const quint16 &item, const quint32 &quantity)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    if(quantity==0)
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("can't send a quantity of 0"));
        return;
    }
    if(!isInTrade)
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("no in trade to send object"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)0x02;
    out << item;
    out << quantity;
    is_logged=character_selected=packFullOutcommingData(0x50,0x03,outputData.constData(),outputData.size());
}

void Api_protocol::addMonster(const quint32 &monsterId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    if(!isInTrade)
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("no in trade to send monster"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint8)0x03;
    out << monsterId;
    is_logged=character_selected=packFullOutcommingData(0x50,0x03,outputData.constData(),outputData.size());
}

//to reset all
void Api_protocol::resetAll()
{
    //status for the query
    token.clear();
    is_logged=false;
    character_selected=false;
    have_send_protocol=false;
    have_receive_protocol=false;
    max_players=65535;
    number_of_map=0;
    player_informations.allow.clear();
    player_informations.bot_already_beaten.clear();
    player_informations.cash=0;
    player_informations.clan=0;
    player_informations.clan_leader=false;
    player_informations.warehouse_cash=0;
    player_informations.warehouse_items.clear();
    player_informations.warehouse_playerMonster.clear();
    player_informations.public_informations.pseudo.clear();
    player_informations.public_informations.simplifiedId=0;
    player_informations.public_informations.skinId=0;
    player_informations.public_informations.speed=0;
    player_informations.public_informations.type=Player_type_normal;
    player_informations.repel_step=0;
    player_informations.recipes.clear();
    player_informations.playerMonster.clear();
    player_informations.items.clear();
    player_informations.reputation.clear();
    player_informations.quests.clear();
    player_informations.itemOnMap.clear();
    unloadSelection();
    isInTrade=false;
    tradeRequestId.clear();
    isInBattle=false;
    battleRequestId.clear();
    mDatapackBase=QStringLiteral("%1/datapack/").arg(QCoreApplication::applicationDirPath());
    mDatapackMain=mDatapackBase+"map/main/main/";
    mDatapackSub=mDatapackMain+"sub/sub/";

    //to send trame
    lastQueryNumber=1;

    ProtocolParsingInputOutput::reset();
}

void Api_protocol::unloadSelection()
{
    logicialGroup.logicialGroupList.clear();
    serverOrdenedList.clear();
    characterListForSelection.clear();
    logicialGroup.name.clear();
    logicialGroup.servers.clear();
}

ServerFromPoolForDisplay Api_protocol::getCurrentServer(const int &index)
{
    ServerFromPoolForDisplay tempVar=*serverOrdenedList.at(index);
    serverOrdenedList.clear();
    return tempVar;
}

QString Api_protocol::datapackPathBase() const
{
    return mDatapackBase;
}

QString Api_protocol::datapackPathMain() const
{
    return mDatapackMain;
}

QString Api_protocol::datapackPathSub() const
{
    return mDatapackSub;
}

QString Api_protocol::mainDatapackCode() const
{
    return QString();
}

QString Api_protocol::subDatapackCode() const
{
    return QString();
}

void Api_protocol::setDatapackPath(const QString &datapack_path)
{
    if(datapack_path.endsWith(QLatin1Literal("/")))
        mDatapackBase=datapack_path;
    else
        mDatapackBase=datapack_path+QLatin1Literal("/");
    mDatapackMain=mDatapackBase+"map/main/main/";
    mDatapackSub=mDatapackMain+"sub/sub/";
}

bool Api_protocol::getIsLogged() const
{
    return is_logged;
}

bool Api_protocol::getCaracterSelected() const
{
    return character_selected;
}

LogicialGroup * Api_protocol::addLogicalGroup(const QString &path,const QString &xml,const QString &language)
{
    QString nameString;

    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(Api_protocol::text_balise_root_start+xml+Api_protocol::text_balise_root_stop, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << (QStringLiteral("Xml not correct for addLogicalGroup: %1, Parse error at line %2, column %3: %4").arg(xml).arg(errorLine).arg(errorColumn).arg(errorStr));
    }
    else
    {
        const QDomElement &root = domDocument.documentElement();
        //load the name
        {
            bool name_found=false;
            QDomElement name = root.firstChildElement(Api_protocol::text_name);
            if(!language.isEmpty() && language!=Api_protocol::text_en)
                while(!name.isNull())
                {
                    if(name.isElement())
                    {
                        if(name.hasAttribute(Api_protocol::text_lang) && name.attribute(Api_protocol::text_lang)==language)
                        {
                            nameString=name.text();
                            name_found=true;
                            break;
                        }
                    }
                    name = name.nextSiblingElement(Api_protocol::text_name);
                }
            if(!name_found)
            {
                name = root.firstChildElement(Api_protocol::text_name);
                while(!name.isNull())
                {
                    if(name.isElement())
                    {
                        if(!name.hasAttribute(Api_protocol::text_lang) || name.attribute(Api_protocol::text_lang)==Api_protocol::text_en)
                        {
                            nameString=name.text();
                            name_found=true;
                            break;
                        }
                    }
                    name = name.nextSiblingElement(Api_protocol::text_name);
                }
            }
            if(!name_found)
            {
                //normal case, the group can be without any name
            }
        }
    }
    LogicialGroup *logicialGroupCursor=&this->logicialGroup;
    QStringList pathSplited=path.split(Api_protocol::text_slash,QString::SkipEmptyParts);
    while(!pathSplited.isEmpty())
    {
        const QString &node=pathSplited.first();
        if(!logicialGroupCursor->logicialGroupList.contains(node))
        {
            LogicialGroup tempGroup;
            logicialGroupCursor->logicialGroupList[node]=tempGroup;
        }
        logicialGroupCursor=&logicialGroupCursor->logicialGroupList[node];
        pathSplited.removeFirst();
    }
    if(!nameString.isEmpty())
        logicialGroupCursor->name=nameString;
    return logicialGroupCursor;
}

ServerFromPoolForDisplay * Api_protocol::addLogicalServer(const ServerFromPoolForDisplayTemp &server, const QString &language)
{
    QString nameString;
    QString descriptionString;

    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(Api_protocol::text_balise_root_start+server.xml+Api_protocol::text_balise_root_stop, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << (QStringLiteral("Xml not correct for addLogicalGroup: %1, Parse error at line %2, column %3: %4").arg(server.xml).arg(errorLine).arg(errorColumn).arg(errorStr));
    }
    else
    {
        const QDomElement &root = domDocument.documentElement();

        //load the name
        {
            bool name_found=false;
            QDomElement name = root.firstChildElement(Api_protocol::text_name);
            if(!language.isEmpty() && language!=Api_protocol::text_en)
                while(!name.isNull())
                {
                    if(name.isElement())
                    {
                        if(name.hasAttribute(Api_protocol::text_lang) && name.attribute(Api_protocol::text_lang)==language)
                        {
                            nameString=name.text();
                            name_found=true;
                            break;
                        }
                    }
                    name = name.nextSiblingElement(Api_protocol::text_name);
                }
            if(!name_found)
            {
                name = root.firstChildElement(Api_protocol::text_name);
                while(!name.isNull())
                {
                    if(name.isElement())
                    {
                        if(!name.hasAttribute(Api_protocol::text_lang) || name.attribute(Api_protocol::text_lang)==Api_protocol::text_en)
                        {
                            nameString=name.text();
                            name_found=true;
                            break;
                        }
                    }
                    name = name.nextSiblingElement(Api_protocol::text_name);
                }
            }
            if(!name_found)
            {
                //normal case, the group can be without any name
            }
        }

        //load the description
        {
            bool description_found=false;
            QDomElement description = root.firstChildElement(Api_protocol::text_description);
            if(!language.isEmpty() && language!=Api_protocol::text_en)
                while(!description.isNull())
                {
                    if(description.isElement())
                    {
                        if(description.hasAttribute(Api_protocol::text_lang) && description.attribute(Api_protocol::text_lang)==language)
                        {
                            descriptionString=description.text();
                            description_found=true;
                            break;
                        }
                    }
                    description = description.nextSiblingElement(Api_protocol::text_description);
                }
            if(!description_found)
            {
                description = root.firstChildElement(Api_protocol::text_description);
                while(!description.isNull())
                {
                    if(description.isElement())
                    {
                        if(!description.hasAttribute(Api_protocol::text_lang) || description.attribute(Api_protocol::text_lang)==Api_protocol::text_en)
                        {
                            descriptionString=description.text();
                            description_found=true;
                            break;
                        }
                    }
                    description = description.nextSiblingElement(Api_protocol::text_description);
                }
            }
            if(!description_found)
            {
                //normal case, the group can be without any description
            }
        }
    }

    if(server.logicalGroupIndex>=logicialGroupIndexList.size())
    {
        qDebug() << (QStringLiteral("out of range for addLogicalGroup: %1, server.logicalGroupIndex %2 <= logicialGroupIndexList.size() %3")
                     .arg(server.xml)
                     .arg(server.logicalGroupIndex)
                     .arg(logicialGroupIndexList.size())
                     );
        return NULL;
    }
    LogicialGroup * const logicialGroupCursor=logicialGroupIndexList.at(server.logicalGroupIndex);

    ServerFromPoolForDisplay newServer;
    newServer.charactersGroupIndex=server.charactersGroupIndex;
    newServer.currentPlayer=server.currentPlayer;
    newServer.description=descriptionString;
    newServer.host=server.host;
    newServer.maxPlayer=server.maxPlayer;
    newServer.name=nameString;
    newServer.port=server.port;
    newServer.uniqueKey=server.uniqueKey;
    newServer.lastConnect=0;
    newServer.playedTime=0;

    logicialGroupCursor->servers << newServer;
    return &logicialGroupCursor->servers.last();
}

LogicialGroup Api_protocol::getLogicialGroup() const
{
    return logicialGroup;
}
