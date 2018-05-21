#include "Api_protocol.h"
#include "../../general/base/ProtocolVersion.h"

using namespace CatchChallenger;

const unsigned char protocolHeaderToMatchLogin[] = PROTOCOL_HEADER_LOGIN;
const unsigned char protocolHeaderToMatchGameServer[] = PROTOCOL_HEADER_GAMESERVER;

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#endif
#include <iostream>

#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/CommonSettingsServer.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/GeneralType.h"
#ifndef BOTTESTCONNECT
#include "SslCert.h"
#include "LanguagesSelect.h"
#endif

#include <QCoreApplication>
#include <QStandardPaths>
#include <QSslKey>
#include <QDataStream>

#ifdef BENCHMARKMUTIPLECLIENT
char Api_protocol::hurgeBufferForBenchmark[4096];
bool Api_protocol::precomputeDone=false;
char Api_protocol::hurgeBufferMove[4];

#include <iostream>
#include <fstream>
#include <unistd.h>
#endif

//need host + port here to have datapack base

std::unordered_set<std::string> Api_protocol::extensionAllowed;

bool Api_protocol::internalVersionDisplayed=false;

std::string Api_protocol::text_balise_root_start="<root>";
std::string Api_protocol::text_balise_root_stop="</root>";
std::string Api_protocol::text_name="name";
std::string Api_protocol::text_description="description";
std::string Api_protocol::text_en="en";
std::string Api_protocol::text_lang="lang";
std::string Api_protocol::text_slash="/";

Api_protocol::Api_protocol(ConnectedSocket *socket,bool tolerantMode) :
    ProtocolParsingInputOutput(socket,PacketModeTransmission_Client),
    tolerantMode(tolerantMode)
{
    datapackStatus=DatapackStatus::Base;

    #ifdef BENCHMARKMUTIPLECLIENT
    if(!Api_protocol::precomputeDone)
    {
        Api_protocol::precomputeDone=true;
        hurgeBufferMove[0]=0x40;
    }
    #endif
    if(extensionAllowed.empty())
    {
        const std::vector<std::string> &v=stringsplit(std::string(CATCHCHALLENGER_EXTENSION_ALLOWED),';');
        extensionAllowed=std::unordered_set<std::string>(v.cbegin(),v.cend());
    }

    player_informations.recipes=NULL;
    player_informations.encyclopedia_monster=NULL;
    player_informations.encyclopedia_item=NULL;
    player_informations.bot_already_beaten=NULL;
    stageConnexion=StageConnexion::Stage1;
    resetAll();

    QObject::connect(socket,&ConnectedSocket::destroyed,this,&Api_protocol::QtsocketDestroyed);
    if(socket->sslSocket!=NULL)
    {
        if(!QObject::connect(socket,&ConnectedSocket::readyRead,this,&Api_protocol::readForFirstHeader))
            abort();
        if(socket->bytesAvailable())
            readForFirstHeader();
    }
    else
    {
        if(socket->fakeSocket!=NULL)
            haveFirstHeader=true;
        QObject::connect(socket,&ConnectedSocket::readyRead,this,&Api_protocol::parseIncommingData,Qt::QueuedConnection);//put queued to don't have circular loop Client -> Server -> Client
        if(socket->bytesAvailable())
            parseIncommingData();
    }

    if(!Api_protocol::internalVersionDisplayed)
    {
        Api_protocol::internalVersionDisplayed=true;
        std::cout << "Qt version: " << qVersion() << std::endl;
    }

    {
        lastQueryNumber.reserve(16);
        uint8_t index=1;
        while(index<16)
        {
            lastQueryNumber.push_back(index);
            index++;
        }
    }
}

Api_protocol::~Api_protocol()
{
    qDebug() << "Api_protocol::~Api_protocol()";
    if(player_informations.encyclopedia_monster!=NULL)
    {
        delete player_informations.encyclopedia_monster;
        player_informations.encyclopedia_monster=NULL;
    }
    if(player_informations.encyclopedia_item!=NULL)
    {
        delete player_informations.encyclopedia_item;
        player_informations.encyclopedia_item=NULL;
    }
}

bool Api_protocol::disconnectClient()
{
    if(socket!=NULL)
        socket->disconnect();
    is_logged=false;
    character_selected=false;
    if(player_informations.encyclopedia_monster!=NULL)
    {
        delete player_informations.encyclopedia_monster;
        player_informations.encyclopedia_monster=NULL;
    }
    if(player_informations.encyclopedia_item!=NULL)
    {
        delete player_informations.encyclopedia_item;
        player_informations.encyclopedia_item=NULL;
    }
    return true;
}

void Api_protocol::QtsocketDestroyed()
{
    socketDestroyed();
}

void Api_protocol::socketDestroyed()
{
    socket=NULL;
}

std::map<uint8_t,uint64_t> Api_protocol::getQuerySendTimeList() const
{
    return querySendTime;
}

void Api_protocol::parseIncommingData()
{
    ProtocolParsingInputOutput::parseIncommingData();
}

void Api_protocol::errorParsingLayer(const std::string &error)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    abort();
    #endif
    newError(("Internal error, file: "+std::string(__FILE__)+":"+std::to_string(__LINE__)),error);
}

void Api_protocol::messageParsingLayer(const std::string &message) const
{
    qDebug() << QString::fromStdString(message);
}

void Api_protocol::parseError(const std::string &userMessage,const std::string &errorString)
{
    if(tolerantMode)
        std::cerr << "packet ignored due to: " << errorString << std::endl;
    else
    {
        std::cerr << userMessage << " " << errorString << std::endl;
        newError(userMessage,errorString);
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    abort();
    #endif
}

Player_private_and_public_informations &Api_protocol::get_player_informations()
{
    if(!getCaracterSelected())
    {
        std::cerr << "Api_protocol::get_player_informations(): !getCaracterSelected() (internal error)" << std::endl;
        return player_informations;
    }
    return player_informations;
}

const Player_private_and_public_informations &Api_protocol::get_player_informations_ro() const
{
    if(!getCaracterSelected())
    {
        std::cerr << "Api_protocol::get_player_informations_ro(): !getCaracterSelected() (internal error)" << std::endl;
        return player_informations;
    }
    return player_informations;
}

std::string Api_protocol::getPseudo()
{
    if(!getCaracterSelected())
    {
        std::cerr << "Api_protocol::getPseudo(): !getCaracterSelected() (internal error)" << std::endl;
        return std::string();
    }
    return player_informations.public_informations.pseudo;
}

uint16_t Api_protocol::getId()
{
    if(!getCaracterSelected())
    {
        std::cerr << "Api_protocol::getId(): !getCaracterSelected() (internal error)" << std::endl;
        return 0;
    }
    return player_informations.public_informations.simplifiedId;
}

uint8_t Api_protocol::queryNumber()
{
    if(lastQueryNumber.empty())
    {
        std::cerr << "Api_protocol::queryNumber(): no more lastQueryNumber" << std::endl;
        abort();
    }
    const uint8_t lastQueryNumberTemp=this->lastQueryNumber.back();
    const std::time_t result = std::time(nullptr);
    querySendTime[lastQueryNumberTemp]=result;
    this->lastQueryNumber.pop_back();
    return lastQueryNumberTemp;
}

bool Api_protocol::sendProtocol()
{
    if(have_send_protocol)
    {
        newError("Internal problem","Api_protocol::sendProtocol() Have already send the protocol");
        return false;
    }
    if(!haveFirstHeader)
    {
        newError("Internal problem","Api_protocol::sendProtocol() !haveFirstHeader");
        return false;
    }

    have_send_protocol=true;
    if(stageConnexion==StageConnexion::Stage1)
        packOutcommingQuery(0xA0,queryNumber(),reinterpret_cast<const char *>(protocolHeaderToMatchLogin),sizeof(protocolHeaderToMatchLogin));
    else if(stageConnexion==StageConnexion::Stage3)
    {
        stageConnexion=CatchChallenger::Api_protocol::StageConnexion::Stage4;
        packOutcommingQuery(0xA0,queryNumber(),reinterpret_cast<const char *>(protocolHeaderToMatchGameServer),sizeof(protocolHeaderToMatchGameServer));
    }
    else
        newError("Internal problem","stageConnexion!=StageConnexion::Stage1/3");
    return true;
}

std::string Api_protocol::socketDisconnectedForReconnect()
{
    if(stageConnexion!=StageConnexion::Stage2)
    {
        if(stageConnexion!=StageConnexion::Stage3)
        {
            newError("Internal problem","Api_protocol::socketDisconnectedForReconnect(): "+std::to_string((int)stageConnexion));
            return std::string();
        }
        else
        {
            std::cerr << "socketDisconnectedForReconnect() double call detected, just drop it" << std::endl;
            return std::string();
        }
    }
    if(selectedServerIndex==-1)
    {
        parseError("Internal error, file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),std::string("selectedServerIndex==-1 with Api_protocol::socketDisconnectedForReconnect()"));
        return std::string();
    }
    const ServerFromPoolForDisplay &serverFromPoolForDisplay=*serverOrdenedList.at(selectedServerIndex);
    if(serverFromPoolForDisplay.host.empty())
    {
        parseError("Internal error, file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),std::string("serverFromPoolForDisplay.host.isEmpty() with Api_protocol::socketDisconnectedForReconnect()"));
        return std::string();
    }
    if(socket==NULL)
    {
        parseError("Internal error, file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),std::string("socket==NULL with Api_protocol::socketDisconnectedForReconnect()"));
        return serverFromPoolForDisplay.host+":"+std::to_string(serverFromPoolForDisplay.port);
    }
    message("stageConnexion=CatchChallenger::Api_protocol::StageConnexion::Stage3 set at "+std::string(__FILE__)+":"+std::to_string(__LINE__));
    stageConnexion=CatchChallenger::Api_protocol::StageConnexion::Stage3;//prevent loop in stage2
    haveFirstHeader=false;
    std::cout << "Api_protocol::socketDisconnectedForReconnect(), Try connect to: " << serverFromPoolForDisplay.host << ":" << serverFromPoolForDisplay.port << std::endl;
    socket->connectToHost(QString::fromStdString(serverFromPoolForDisplay.host),serverFromPoolForDisplay.port);
    return serverFromPoolForDisplay.host+":"+std::to_string(serverFromPoolForDisplay.port);
}

bool Api_protocol::protocolWrong() const
{
    return have_send_protocol && !have_receive_protocol;
}

bool Api_protocol::tryLogin(const std::string &login, const std::string &pass)
{
    if(!have_send_protocol)
    {
        newError(std::string("Internal problem"),std::string("Have not send the protocol"));
        return false;
    }
    if(is_logged)
    {
        newError(std::string("Internal problem"),std::string("Is already logged"));
        return false;
    }
    if(token.empty())
    {
        newError(std::string("Internal problem"),std::string("Token is empty"));
        return false;
    }
    std::string outputData;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    std::string tempDoubleHash;
    #endif
    {
        QCryptographicHash hashLogin(QCryptographicHash::Sha224);
        hashLogin.addData((QString::fromStdString(login)+/*salt*/"RtR3bm9Z1DFMfAC3").toUtf8());
        loginHash=std::string(hashLogin.result().constData(),hashLogin.result().size());
        outputData+=loginHash;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            QCryptographicHash hashLogin2(QCryptographicHash::Sha224);
            hashLogin2.addData(QByteArray(loginHash.data(),loginHash.size()));
            tempDoubleHash=std::string(hashLogin2.result().data(),hashLogin2.result().size());
        }
        #endif
    }
    QCryptographicHash hashAndToken(QCryptographicHash::Sha224);
    {
        QCryptographicHash hashPass(QCryptographicHash::Sha224);
        hashPass.addData((QString::fromStdString(pass)+/*salt*/"AwjDvPIzfJPTTgHs"+QString::fromStdString(login)/*add unique salt*/).toUtf8());
        passHash=std::string(hashPass.result().data(),hashPass.result().size());

        hashAndToken.addData(QByteArray(passHash.data(),passHash.size()));
        hashAndToken.addData(QByteArray(token.data(),token.size()));
        outputData+=std::string(hashAndToken.result().data(),hashAndToken.result().size());
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    std::cout << "Try auth: password " << binarytoHexa(passHash.data(),passHash.size())
              << ", token: " << binarytoHexa(token.data(),token.size())
              << ", password+token " << binarytoHexa(hashAndToken.result().data(),hashAndToken.result().size())
              << " (" << binarytoHexa(tempDoubleHash.data(),tempDoubleHash.size())
              << ") for the login: "
              << binarytoHexa(passHash.data(),passHash.size());
    #endif
    std::string peerName=socket->peerName().toStdString();
    if(peerName.size()>255)
    {
        newError(QObject::tr("Hostname too big").toStdString(),std::string("Hostname too big"));
        return false;
    }

    packOutcommingQuery(0xA8,queryNumber(),outputData.data(),outputData.size());
    return true;
}

bool Api_protocol::tryCreateAccount()
{
    if(!have_send_protocol)
    {
        newError(std::string("Internal problem"),std::string("Have not send the protocol"));
        return false;
    }
    if(is_logged)
    {
        newError(std::string("Internal problem"),std::string("Is already logged"));
        return false;
    }
    /*double hashing on client part
     * '''Prevent login leak in case of MiM attack re-ask the password''' (Trafic modification, replace the server return code OK by ACCOUNT CREATION)
     * Do some DDOS protection because it offload the hashing */
    std::string outputData;
    {
        QCryptographicHash hashLogin(QCryptographicHash::Sha224);
        hashLogin.addData(QByteArray(loginHash.data(),loginHash.size()));
        outputData+=std::string(hashLogin.result().data(),hashLogin.result().size());
    }
    //pass
    outputData+=passHash;

    packOutcommingQuery(0xA9,queryNumber(),outputData.data(),outputData.size());
    std::cout << "Try create account: login: " << binarytoHexa(loginHash.data(),loginHash.size())
              << " and pass: " << binarytoHexa(passHash.data(),passHash.size()) << std::endl;
    return true;
}

void Api_protocol::send_player_move(const uint8_t &moved_unit,const Direction &direction)
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
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    uint8_t directionInt=static_cast<uint8_t>(direction);
    if(directionInt<1 || directionInt>8)
    {
        std::cerr << "direction given wrong: " << directionInt << std::endl;
        abort();
    }
    if(last_direction_is_set==false)
        abort();
    //correct integration with MoveOnTheMap::newDirection()
    if(last_direction!=direction)
    {
        send_player_move_internal(last_step,last_direction);
        send_player_move_internal(moved_unit,direction);
    }
    else
    {
        bool isAMove=false;
        switch(direction)
        {
            case Direction_move_at_top:
            case Direction_move_at_right:
            case Direction_move_at_bottom:
            case Direction_move_at_left:
                isAMove=true;
            return;
            break;
            default:
            break;
        }
        if(isAMove)
        {
            last_step+=moved_unit;
            return;
        }
        else // if look
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(moved_unit>0)
                abort();
            if(last_step>0)
                abort();
            #endif
            last_step=0;
            return;//2x time look on smae direction, drop
        }
    }
    last_step=0;
    last_direction=direction;
}

void Api_protocol::send_player_move_internal(const uint8_t &moved_unit,const CatchChallenger::Direction &direction)
{
    uint8_t directionInt=static_cast<uint8_t>(direction);
    if(directionInt<1 || directionInt>8)
    {
        std::cerr << "direction given wrong: " << directionInt << std::endl;
        abort();
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << moved_unit;
    out << directionInt;
    is_logged=character_selected=packOutcommingData(0x02,outputData.constData(),outputData.size());
}

void Api_protocol::send_player_direction(const Direction &the_direction)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    newDirection(the_direction);
}

void Api_protocol::sendChatText(const Chat_type &chatType, const std::string &text)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(chatType!=Chat_type_local && chatType!=Chat_type_all && chatType!=Chat_type_clan && chatType!=Chat_type_aliance && chatType!=Chat_type_system && chatType!=Chat_type_system_important)
    {
        std::cerr << "chatType wrong: " << chatType << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)chatType;
    {
        const std::string &tempText=text;
        if(tempText.size()>255)
        {
            std::cerr << "text in Utf8 too big, line: " << __FILE__ << ": " << __LINE__ << std::endl;
            return;
        }
        out << (uint8_t)tempText.size();
        outputData+=QByteArray(tempText.data(),tempText.size());
        out.device()->seek(out.device()->pos()+tempText.size());
    }
    is_logged=character_selected=packOutcommingData(0x03,outputData.constData(),outputData.size());
}

void Api_protocol::sendPM(const std::string &text,const std::string &pseudo)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(this->player_informations.public_informations.pseudo==pseudo)
        return;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)Chat_type_pm;
    {
        const std::string &tempText=text;
        if(tempText.size()>255)
        {
            std::cerr << "text in Utf8 too big, line: " << __FILE__ << ": " << __LINE__ << std::endl;
            return;
        }
        out << (uint8_t)tempText.size();
        outputData+=QByteArray(tempText.data(),tempText.size());
        out.device()->seek(out.device()->pos()+tempText.size());
    }
    {
        const std::string &tempText=pseudo;
        if(tempText.size()>255)
        {
            std::cerr << "text in Utf8 too big, line: " << __FILE__ << ": " << __LINE__ << std::endl;
            return;
        }
        out << (uint8_t)tempText.size();
        outputData+=QByteArray(tempText.data(),tempText.size());
        out.device()->seek(out.device()->pos()+tempText.size());
    }
    is_logged=character_selected=packOutcommingData(0x03,outputData.constData(),outputData.size());
}

bool Api_protocol::teleportDone()
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return false;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return false;
    }
    const TeleportQueryInformation &teleportQueryInformation=teleportList.front();
    if(!last_direction_is_set)
    {
        parseError("Procotol wrong or corrupted",
                   "Api_protocol::teleportDone() !last_direction_is_set value, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    last_direction=teleportQueryInformation.direction;
    last_step=0;
    is_logged=character_selected=postReplyData(teleportQueryInformation.queryId,NULL,0);
    teleportList.erase(teleportList.cbegin());
    return true;
}

bool Api_protocol::addCharacter(const uint8_t &charactersGroupIndex,const uint8_t &profileIndex, const std::string &pseudo, const uint8_t &monsterGroupId, const uint8_t &skinId)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return false;
    }
    if(skinId>=CommonDatapack::commonDatapack.skins.size())
    {
        newError(std::string("Internal problem"),"skin provided: "+std::to_string(skinId)+" is not into skin listed");
        return false;
    }
    const Profile &profile=CommonDatapack::commonDatapack.profileList.at(profileIndex);
    if(!profile.forcedskin.empty() && !vectorcontainsAtLeastOne(profile.forcedskin,skinId))
    {
        newError(std::string("Internal problem"),"skin provided: "+std::to_string(skinId)+" is not into profile "+std::to_string(profileIndex)+" forced skin list");
        return false;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)charactersGroupIndex;
    out << (uint8_t)profileIndex;
    {
        const std::string &rawPseudo=toUTF8WithHeader(pseudo);
        if(rawPseudo.size()>255 || rawPseudo.empty())
        {
            std::cerr << "rawPseudo too big or not compatible with utf8" << std::endl;
            return false;
        }
        outputData+=QByteArray(rawPseudo.data(),rawPseudo.size());
        out.device()->seek(out.device()->size());
    }
    out << (uint8_t)monsterGroupId;
    out << (uint8_t)skinId;
    is_logged=packOutcommingQuery(0xAA,queryNumber(),outputData.constData(),outputData.size());
    return true;
}

bool Api_protocol::removeCharacter(const uint8_t &charactersGroupIndex,const uint32_t &characterId)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return false;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)charactersGroupIndex;
    out << characterId;
    is_logged=packOutcommingQuery(0xAB,queryNumber(),outputData.constData(),outputData.size());
    return true;
}

bool Api_protocol::selectCharacter(const uint8_t &charactersGroupIndex, const uint32_t &serverUniqueKey, const uint32_t &characterId)
{
    if(characterId==0)
    {
        std::cerr << "Api_protocol::selectCharacter() can't have characterId==0" << std::endl;
        abort();
    }
    unsigned int index=0;
    while(index<serverOrdenedList.size())
    {
        const ServerFromPoolForDisplay * const server=serverOrdenedList.at(index);
        if(server->uniqueKey==serverUniqueKey && server->charactersGroupIndex==charactersGroupIndex)
            return selectCharacter(charactersGroupIndex,serverUniqueKey,characterId,index);
        index++;
    }
    std::cerr << "index of server not found, charactersGroupIndex: " << (uint32_t)charactersGroupIndex << ", serverUniqueKey: " << serverUniqueKey << ", line: " << __FILE__ << ": " << __LINE__ << std::endl;
    return false;
}

bool Api_protocol::selectCharacter(const uint8_t &charactersGroupIndex, const uint32_t &serverUniqueKey, const uint32_t &characterId,const uint32_t &serverIndex)
{
    if(characterId==0)
    {
        std::cerr << "Api_protocol::selectCharacter() with server index can't have characterId==0" << std::endl;
        abort();
    }
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return false;
    }
    if(character_selected)
    {
        std::cerr << "character already selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return false;
    }
    if(character_select_send)
    {
        std::cerr << "character select already send, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return false;
    }

    character_select_send=true;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)charactersGroupIndex;
    out << (uint32_t)serverUniqueKey;
    out << characterId;
    is_logged=packOutcommingQuery(0xAC,queryNumber(),outputData.constData(),outputData.size());
    this->selectedServerIndex=serverIndex;
    std::cerr << "this: " << this << ", socket: " << socket << ", select char: " << characterId << ", charactersGroupIndex: " << (uint32_t)charactersGroupIndex << ", serverUniqueKey: " << serverUniqueKey << ", line: " << __FILE__ << ": " << __LINE__ << std::endl;
    return true;
}

bool Api_protocol::character_select_is_send()
{
    return character_select_send;
}

void Api_protocol::useSeed(const uint8_t &plant_id)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    outputData[0]=plant_id;
    if(CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer==false)
        is_logged=character_selected=packOutcommingQuery(0x83,queryNumber(),outputData.constData(),outputData.size());
    else
        is_logged=character_selected=packOutcommingData(0x19,outputData.constData(),outputData.size());
}

void Api_protocol::monsterMoveUp(const uint8_t &number)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x01;
    out << number;
    is_logged=character_selected=packOutcommingData(0x0D,outputData.constData(),outputData.size());
}

void Api_protocol::confirmEvolutionByPosition(const uint8_t &monterPosition)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)monterPosition;
    is_logged=character_selected=packOutcommingData(0x0F,outputData.constData(),outputData.size());
}

void Api_protocol::monsterMoveDown(const uint8_t &number)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    std::cerr << "confirm evolution of monster position: " << std::to_string(number) << ", line: " << __FILE__ << ": " << __LINE__ << std::endl;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x02;
    out << number;
    is_logged=character_selected=packOutcommingData(0x0D,outputData.constData(),outputData.size());
}

//inventory
void Api_protocol::destroyObject(const uint16_t &object, const uint32_t &quantity)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << object;
    out << quantity;
    is_logged=character_selected=packOutcommingData(0x13,outputData.constData(),outputData.size());
}

bool Api_protocol::useObject(const uint16_t &object)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return false;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return false;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << object;
    is_logged=character_selected=packOutcommingQuery(0x86,queryNumber(),outputData.constData(),outputData.size());
    lastObjectUsed.push_back(object);
    return true;
}

bool Api_protocol::useObjectOnMonsterByPosition(const uint16_t &object,const uint8_t &monsterPosition)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return false;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return false;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << object;
    out << monsterPosition;
    is_logged=character_selected=packOutcommingData(0x10,outputData.constData(),outputData.size());
    return true;
}


void Api_protocol::wareHouseStore(const int64_t &cash, const std::vector<std::pair<uint16_t,int32_t> > &items,
                                  const std::vector<uint32_t> &withdrawMonsters, const std::vector<uint32_t> &depositeMonsters)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (quint64)cash;

    out << (uint16_t)items.size();
    unsigned int index=0;
    while(index<items.size())
    {
        out << (uint16_t)items.at(index).first;
        out << (int32_t)items.at(index).second;
        index++;
    }

    out << (uint32_t)withdrawMonsters.size();
    index=0;
    while(index<withdrawMonsters.size())
    {
        out << (uint32_t)withdrawMonsters.at(index);
        index++;
    }
    out << (uint32_t)depositeMonsters.size();
    index=0;
    while(index<depositeMonsters.size())
    {
        out << (uint32_t)depositeMonsters.at(index);
        index++;
    }

    is_logged=character_selected=packOutcommingData(0x17,outputData.constData(),outputData.size());
}

void Api_protocol::takeAnObjectOnMap()
{
    //std::cout << "Api_protocol::takeAnObjectOnMap(): " << player_informations.public_informations.pseudo << std::endl;
    packOutcommingData(0x18,NULL,0);
}

void Api_protocol::getShopList(const uint16_t &shopId)/// \see CommonMap, std::unordered_map<std::pair<uint8_t,uint8_t>,std::vector<uint16_t>, pairhash> shops;
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint16_t)shopId;
    is_logged=character_selected=packOutcommingQuery(0x87,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::buyObject(const uint16_t &shopId, const uint16_t &objectId, const uint32_t &quantity, const uint32_t &price)/// \see CommonMap, std::unordered_map<std::pair<uint8_t,uint8_t>,std::vector<uint16_t>, pairhash> shops;
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint16_t)shopId;
    out << (uint16_t)objectId;
    out << (uint32_t)quantity;
    out << (uint32_t)price;
    is_logged=character_selected=packOutcommingQuery(0x88,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::sellObject(const uint16_t &shopId,const uint16_t &objectId,const uint32_t &quantity,const uint32_t &price)/// \see CommonMap, std::unordered_map<std::pair<uint8_t,uint8_t>,std::vector<uint16_t>, pairhash> shops;
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint16_t)shopId;
    out << (uint16_t)objectId;
    out << (uint32_t)quantity;
    out << (uint32_t)price;
    is_logged=character_selected=packOutcommingQuery(0x89,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::getFactoryList(const uint16_t &factoryId)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint16_t)factoryId;
    is_logged=character_selected=packOutcommingQuery(0x8A,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::buyFactoryProduct(const uint16_t &factoryId,const uint16_t &objectId,const uint32_t &quantity,const uint32_t &price)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint16_t)factoryId;
    out << (uint16_t)objectId;
    out << (uint32_t)quantity;
    out << (uint32_t)price;
    is_logged=character_selected=packOutcommingQuery(0x8B,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::sellFactoryResource(const uint16_t &factoryId,const uint16_t &objectId,const uint32_t &quantity,const uint32_t &price)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint16_t)factoryId;
    out << (uint16_t)objectId;
    out << (uint32_t)quantity;
    out << (uint32_t)price;
    is_logged=character_selected=packOutcommingQuery(0x8C,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::tryEscape()
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    is_logged=character_selected=packOutcommingData(0x07,NULL,0);
}

void Api_protocol::heal()
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    is_logged=character_selected=packOutcommingData(0x0B,NULL,0);
}

void Api_protocol::requestFight(const uint16_t &fightId)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(player_informations.bot_already_beaten==NULL)
    {
        std::cerr << "player_informations.bot_already_beaten==NULL, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        abort();
    }
    if(player_informations.bot_already_beaten[fightId/8] & (1<<(7-fightId%8)))
    {
        std::cerr << "player_informations.bot_already_beaten["+std::to_string(fightId)+"], line: " << __FILE__ << ": " << __LINE__ << std::endl;
        abort();
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint16_t)fightId;
    is_logged=character_selected=packOutcommingData(0x0C,outputData.constData(),outputData.size());
}

void Api_protocol::changeOfMonsterInFightByPosition(const uint8_t &monsterPosition)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)monsterPosition;
    is_logged=character_selected=packOutcommingData(0x0E,outputData.constData(),outputData.size());
}

void Api_protocol::useSkill(const uint16_t &skill)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint16_t)skill;
    is_logged=character_selected=packOutcommingData(0x11,outputData.constData(),outputData.size());
}

void Api_protocol::learnSkillByPosition(const uint8_t &monsterPosition,const uint16_t &skill)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)monsterPosition;
    out << (uint16_t)skill;
    is_logged=character_selected=packOutcommingData(0x09,outputData.constData(),outputData.size());
}

void Api_protocol::startQuest(const uint16_t &questId)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint16_t)questId;
    is_logged=character_selected=packOutcommingData(0x1B,outputData.constData(),outputData.size());
}

void Api_protocol::finishQuest(const uint16_t &questId)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint16_t)questId;
    is_logged=character_selected=packOutcommingData(0x1C,outputData.constData(),outputData.size());
}

void Api_protocol::cancelQuest(const uint16_t &questId)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint16_t)questId;
    is_logged=character_selected=packOutcommingData(0x1D,outputData.constData(),outputData.size());
}

void Api_protocol::nextQuestStep(const uint16_t &questId)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint16_t)questId;
    is_logged=character_selected=packOutcommingData(0x1E,outputData.constData(),outputData.size());
}

void Api_protocol::createClan(const std::string &name)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x01;
    {
        const std::string &rawText=toUTF8WithHeader(name);
        if(rawText.size()>255 || rawText.empty())
        {
            std::cerr << "rawText too big or not compatible with utf8" << std::endl;
            return;
        }
        outputData+=QByteArray(rawText.data(),rawText.size());
        out.device()->seek(out.device()->size());
    }
    is_logged=character_selected=packOutcommingQuery(0x92,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::leaveClan()
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x02;
    is_logged=character_selected=packOutcommingQuery(0x92,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::dissolveClan()
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x03;
    is_logged=character_selected=packOutcommingQuery(0x92,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::inviteClan(const std::string &pseudo)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x04;
    {
        const std::string &rawText=toUTF8WithHeader(pseudo);
        if(rawText.size()>255 || rawText.empty())
        {
            std::cerr << "rawText too big or not compatible with utf8" << std::endl;
            return;
        }
        outputData+=QByteArray(rawText.data(),rawText.size());
        out.device()->seek(out.device()->size());
    }
    is_logged=character_selected=packOutcommingQuery(0x92,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::ejectClan(const std::string &pseudo)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x05;
    {
        const std::string &rawText=toUTF8WithHeader(pseudo);
        if(rawText.size()>255 || rawText.empty())
        {
            std::cerr << "rawText too big or not compatible with utf8" << std::endl;
            return;
        }
        outputData+=QByteArray(rawText.data(),rawText.size());
        out.device()->seek(out.device()->size());
    }
    is_logged=character_selected=packOutcommingQuery(0x92,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::inviteAccept(const bool &accept)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    if(accept)
        out << (uint8_t)0x01;
    else
        out << (uint8_t)0x02;
    is_logged=character_selected=packOutcommingData(0x04,outputData.constData(),outputData.size());
}

void Api_protocol::waitingForCityCapture(const bool &cancel)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    if(!cancel)
        out << (uint8_t)0x00;
    else
        out << (uint8_t)0x01;
    is_logged=character_selected=packOutcommingData(0x1F,outputData.constData(),outputData.size());
}

//market
void Api_protocol::getMarketList()
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    is_logged=character_selected=packOutcommingQuery(0x8D,queryNumber(),NULL,0);
}

void Api_protocol::buyMarketObject(const uint32_t &marketObjectId, const uint32_t &quantity)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x01;
    out << marketObjectId;
    out << quantity;
    is_logged=character_selected=packOutcommingQuery(0x8E,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::buyMarketMonster(const uint32_t &monsterMarketId)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x02;
    out << monsterMarketId;
    is_logged=character_selected=packOutcommingQuery(0x8E,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::putMarketObject(const uint16_t &objectId, const uint32_t &quantity, const uint64_t &price)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x01;
    out << (quint16)objectId;
    out << (quint32)quantity;
    out << (quint64)price;
    is_logged=character_selected=packOutcommingQuery(0x8F,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::putMarketMonsterByPosition(const uint8_t &monsterPosition,const uint64_t &price)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x02;
    out << (uint8_t)monsterPosition;
    out << (quint64)price;
    is_logged=character_selected=packOutcommingQuery(0x8F,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::recoverMarketCash()
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    is_logged=character_selected=packOutcommingQuery(0x90,queryNumber(),NULL,0);
}

void Api_protocol::withdrawMarketObject(const uint16_t &objectPosition,const uint32_t &quantity)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x01;
    out << objectPosition;
    out << quantity;
    is_logged=character_selected=packOutcommingQuery(0x91,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::withdrawMarketMonster(const uint32_t &monsterMarketId)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x02;
    out << monsterMarketId;
    is_logged=character_selected=packOutcommingQuery(0x91,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::collectMaturePlant()
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer==false)
        is_logged=character_selected=packOutcommingQuery(0x84,queryNumber(),NULL,0);
    else
        is_logged=character_selected=packOutcommingData(0x1A,NULL,0);
}

//crafting
void Api_protocol::useRecipe(const uint16_t &recipeId)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint16_t)recipeId;
    is_logged=character_selected=packOutcommingQuery(0x85,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::addRecipe(const uint16_t &recipeId)
{
    if(player_informations.recipes==NULL)
    {
        std::cerr << "player_informations.recipes NULL, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    player_informations.recipes[recipeId/8]|=(1<<(7-recipeId%8));
}

void Api_protocol::battleRefused()
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(battleRequestId.empty())
    {
        newError(std::string("Internal problem"),std::string("no battle request to refuse"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x02;
    is_logged=character_selected=postReplyData(battleRequestId.front(),outputData.data(),outputData.size());
    battleRequestId.erase(battleRequestId.cbegin());
}

void Api_protocol::battleAccepted()
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(battleRequestId.empty())
    {
        newError(std::string("Internal problem"),std::string("no battle request to accept"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x01;
    is_logged=character_selected=postReplyData(battleRequestId.front(),outputData.data(),outputData.size());
    battleRequestId.erase(battleRequestId.cbegin());
}

//trade
void Api_protocol::tradeRefused()
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(tradeRequestId.empty())
    {
        newError(std::string("Internal problem"),std::string("no trade request to refuse"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x02;
    is_logged=character_selected=postReplyData(tradeRequestId.front(),outputData.data(),outputData.size());
    tradeRequestId.erase(tradeRequestId.cbegin());
}

void Api_protocol::tradeAccepted()
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(tradeRequestId.empty())
    {
        newError(std::string("Internal problem"),std::string("no trade request to accept"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x01;
    is_logged=character_selected=postReplyData(tradeRequestId.front(),outputData.constData(),outputData.size());
    tradeRequestId.erase(tradeRequestId.cbegin());
    isInTrade=true;
}

void Api_protocol::tradeCanceled()
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!isInTrade)
    {
        newError(std::string("Internal problem"),std::string("in not in trade"));
        return;
    }
    isInTrade=false;
    is_logged=character_selected=packOutcommingData(0x16,NULL,0);
}

void Api_protocol::tradeFinish()
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!isInTrade)
    {
        newError(std::string("Internal problem"),std::string("in not in trade"));
        return;
    }
    is_logged=character_selected=packOutcommingData(0x15,NULL,0);
}

void Api_protocol::addTradeCash(const uint64_t &cash)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(cash==0)
    {
        newError(std::string("Internal problem"),std::string("can't send 0 for the cash"));
        return;
    }
    if(!isInTrade)
    {
        newError(std::string("Internal problem"),std::string("no in trade to send cash"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x01;
    out << (quint64)cash;
    is_logged=character_selected=packOutcommingData(0x14,outputData.data(),outputData.size());
}

void Api_protocol::addObject(const uint16_t &item, const uint32_t &quantity)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(quantity==0)
    {
        newError(std::string("Internal problem"),std::string("can't send a quantity of 0"));
        return;
    }
    if(!isInTrade)
    {
        newError(std::string("Internal problem"),std::string("no in trade to send object"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x02;
    out << item;
    out << quantity;
    is_logged=character_selected=packOutcommingData(0x14,outputData.constData(),outputData.size());
}

void Api_protocol::addMonsterByPosition(const uint8_t &monsterPosition)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!character_selected)
    {
        std::cerr << "character not selected, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(!isInTrade)
    {
        newError(std::string("Internal problem"),std::string("no in trade to send monster"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
    out << (uint8_t)0x03;
    out << monsterPosition;
    is_logged=character_selected=packOutcommingData(0x14,outputData.constData(),outputData.size());
}

Api_protocol::StageConnexion Api_protocol::stage() const
{
    return stageConnexion;
}

//to reset all
void Api_protocol::resetAll()
{
    if(stageConnexion==StageConnexion::Stage2)
        qDebug() << "Api_protocol::resetAll() Suspect internal bug";
    //status for the query
    token.clear();
    message("Api_protocol::resetAll(): stageConnexion=CatchChallenger::Api_protocol::StageConnexion::Stage1 set at "+std::string(__FILE__)+":"+std::to_string(__LINE__));
    stageConnexion=StageConnexion::Stage1;
    if(socket==NULL || socket->fakeSocket==NULL)
        haveFirstHeader=false;
    else
        haveFirstHeader=true;
    character_select_send=false;
    delayedLogin.data.clear();
    delayedLogin.packetCode=0;
    delayedLogin.queryNumber=0;
    number_of_map=0;
    delayedMessages.clear();
    haveTheServerList=false;
    haveTheLogicalGroupList=false;
    is_logged=false;
    character_selected=false;
    have_send_protocol=false;
    have_receive_protocol=false;
    max_players=65535;
    max_players_real=65535;
    selectedServerIndex=-1;
    player_informations.allow.clear();
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
    player_informations.playerMonster.clear();
    player_informations.items.clear();
    player_informations.reputation.clear();
    player_informations.quests.clear();
    player_informations.itemOnMap.clear();
    player_informations.plantOnMap.clear();
    tokenForGameServer.clear();
    //to move by unit
    last_step=255;
    last_direction=Direction_look_at_bottom;
    last_direction_is_set=false;

    unloadSelection();
    isInTrade=false;
    tradeRequestId.clear();
    isInBattle=false;
    battleRequestId.clear();
    mDatapackBase=QCoreApplication::applicationDirPath().toStdString()+"/datapack/";
    mDatapackMain=mDatapackBase+"map/main/[main]/";
    mDatapackSub=mDatapackMain+"sub/[sub]/";
    CommonSettingsServer::commonSettingsServer.mainDatapackCode="[main]";
    CommonSettingsServer::commonSettingsServer.subDatapackCode="[sub]";
    if(player_informations.recipes!=NULL)
    {
        delete player_informations.recipes;
        player_informations.recipes=NULL;
    }
    if(player_informations.encyclopedia_monster!=NULL)
    {
        delete player_informations.encyclopedia_monster;
        player_informations.encyclopedia_monster=NULL;
    }
    if(player_informations.encyclopedia_item!=NULL)
    {
        delete player_informations.encyclopedia_item;
        player_informations.encyclopedia_item=NULL;
    }
    if(player_informations.bot_already_beaten!=NULL)
    {
        delete player_informations.bot_already_beaten;
        player_informations.bot_already_beaten=NULL;
    }

    ProtocolParsingInputOutput::reset();
    flags|=0x08;
}

void Api_protocol::unloadSelection()
{
    selectedServerIndex=-1;
    logicialGroup.logicialGroupList.clear();
    serverOrdenedList.clear();
    characterListForSelection.clear();
    logicialGroup.name.clear();
    logicialGroup.servers.clear();
}

ServerFromPoolForDisplay Api_protocol::getCurrentServer(const int &index)
{
    ServerFromPoolForDisplay tempVar=*serverOrdenedList.at(index);
    /*selectedServerIndex=-1;
    serverOrdenedList.clear();*/
    return tempVar;
}

std::string Api_protocol::datapackPathBase() const
{
    return mDatapackBase;
}

std::string Api_protocol::datapackPathMain() const
{
    return mDatapackMain;
}

std::string Api_protocol::datapackPathSub() const
{
    return mDatapackSub;
}

std::string Api_protocol::mainDatapackCode() const
{
    return CommonSettingsServer::commonSettingsServer.mainDatapackCode;
}

std::string Api_protocol::subDatapackCode() const
{
    return CommonSettingsServer::commonSettingsServer.subDatapackCode;
}

void Api_protocol::setDatapackPath(const std::string &datapack_path)
{
    if(stringEndsWith(datapack_path,'/'))
        mDatapackBase=datapack_path;
    else
        mDatapackBase=datapack_path+"/";
    mDatapackMain=mDatapackBase+"map/main/[main]/";
    mDatapackSub=mDatapackMain+"sub/[sub]/";
    CommonSettingsServer::commonSettingsServer.mainDatapackCode="[main]";
    CommonSettingsServer::commonSettingsServer.subDatapackCode="[sub]";
}

bool Api_protocol::getIsLogged() const
{
    return is_logged;
}

bool Api_protocol::getCaracterSelected() const
{
    return character_selected;
}

LogicialGroup * Api_protocol::addLogicalGroup(const std::string &path,const std::string &xml,const std::string &language)
{
    std::string nameString;

    tinyxml2::XMLDocument domDocument;
    const auto loadOkay = domDocument.Parse((Api_protocol::text_balise_root_start+xml+Api_protocol::text_balise_root_stop).c_str());
    if(!CATCHCHALLENGER_XMLDOCUMENTRETURNISLOADED(loadOkay))
    {
        std::cerr << "Api_protocol::addLogicalGroup(): "+CATCHCHALLENGER_XMLDOCUMENTERROR(&domDocument) << std::endl;
        return NULL;
    }
    else
    {
        const tinyxml2::XMLElement *root = domDocument.RootElement();
        //load the name
        {
            bool name_found=false;
            const tinyxml2::XMLElement * name = root->FirstChildElement("name");
            if(!language.empty() && language!="en")
                while(name!=NULL)
                {
                    if(name->Attribute("lang")!=NULL && name->Attribute("lang")==language)
                    {
                        nameString=name->GetText();
                        name_found=true;
                        break;
                    }
                    name = name->NextSiblingElement("name");
                }
            if(!name_found)
            {
                name = root->FirstChildElement("name");
                while(name!=NULL)
                {
                    if(name->Attribute("lang")==NULL || strcmp(name->Attribute("lang"),"en")==0)
                    {
                        nameString=name->GetText();
                        name_found=true;
                        break;
                    }
                    name = name->NextSiblingElement("name");
                }
            }
            if(!name_found)
            {
                //normal case, the group can be without any name
            }
        }
    }
    LogicialGroup *logicialGroupCursor=&this->logicialGroup;
    std::vector<std::string> pathSplited=stringsplit(path,'/');
    while(!pathSplited.empty())
    {
        const std::string &node=pathSplited.front();
        if(logicialGroupCursor->logicialGroupList.find(node)==logicialGroupCursor->logicialGroupList.cend())
            logicialGroupCursor->logicialGroupList[node]=new LogicialGroup;
        logicialGroupCursor=logicialGroupCursor->logicialGroupList[node];
        pathSplited.erase(pathSplited.cbegin());
    }
    if(!nameString.empty())
        logicialGroupCursor->name=nameString;
    return logicialGroupCursor;
}

ServerFromPoolForDisplay * Api_protocol::addLogicalServer(const ServerFromPoolForDisplayTemp &server, const std::string &language)
{
    std::string nameString;
    std::string descriptionString;

    tinyxml2::XMLDocument domDocument;
    const auto loadOkay = domDocument.Parse((Api_protocol::text_balise_root_start+server.xml+Api_protocol::text_balise_root_stop).c_str());
    if(!CATCHCHALLENGER_XMLDOCUMENTRETURNISLOADED(loadOkay))
    {
        std::cerr << "Api_protocol::addLogicalServer(): "+CATCHCHALLENGER_XMLDOCUMENTERROR(&domDocument) << std::endl;
        return NULL;
    }
    else
    {
        const tinyxml2::XMLElement *root = domDocument.RootElement();

        //load the name
        {
            bool name_found=false;
            const tinyxml2::XMLElement *name = root->FirstChildElement("name");
            if(!language.empty() && language!="en")
                while(name!=NULL)
                {
                    if(name->Attribute("lang")!=NULL && name->Attribute("lang")==language)
                    {
                        nameString=name->GetText();
                        name_found=true;
                        break;
                    }
                    name = name->NextSiblingElement("name");
                }
            if(!name_found)
            {
                name = root->FirstChildElement("name");
                while(name!=NULL)
                {
                    if(name->Attribute("lang")==NULL || strcmp(name->Attribute("lang"),"en")==0)
                    {
                        nameString=name->GetText();
                        name_found=true;
                        break;
                    }
                    name = name->NextSiblingElement("name");
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
            const tinyxml2::XMLElement *description = root->FirstChildElement("description");
            if(!language.empty() && language!="en")
                while(description!=NULL)
                {
                    if(description->Attribute("lang")!=NULL || description->Attribute("lang")==language)
                    {
                        descriptionString=description->GetText();
                        description_found=true;
                        break;
                    }
                    description = description->NextSiblingElement("description");
                }
            if(!description_found)
            {
                description = root->FirstChildElement("description");
                while(description!=NULL)
                {
                    if(description->Attribute("lang")!=NULL || strcmp(description->Attribute("lang"),"en")==0)
                    {
                        descriptionString=description->GetText();
                        description_found=true;
                        break;
                    }
                    description = description->NextSiblingElement("description");
                }
            }
            if(!description_found)
            {
                //normal case, the group can be without any description
            }
        }
    }

    LogicialGroup * logicialGroupCursor;
    if(server.logicalGroupIndex>=logicialGroupIndexList.size())
    {
        qDebug() << (QString("out of range for addLogicalGroup: %1, server.logicalGroupIndex %2 <= logicialGroupIndexList.size() %3 (defaulting to root folder)")
                     .arg(QString::fromStdString(server.xml))
                     .arg(server.logicalGroupIndex)
                     .arg(logicialGroupIndexList.size())
                     );
        logicialGroupCursor=&logicialGroup;
    }
    else
        logicialGroupCursor=logicialGroupIndexList.at(server.logicalGroupIndex);

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

    logicialGroupCursor->servers.push_back(newServer);
    return &logicialGroupCursor->servers.back();
}

LogicialGroup Api_protocol::getLogicialGroup() const
{
    return logicialGroup;
}

void Api_protocol::readForFirstHeader()
{
    if(haveFirstHeader)
        return;
    if(socket->sslSocket==NULL)
    {
        newError(std::string("Internal problem"),std::string("Api_protocol::readForFirstHeader() socket->sslSocket==NULL"));
        return;
    }
    if(stageConnexion!=StageConnexion::Stage1 && stageConnexion!=StageConnexion::Stage2 && stageConnexion!=StageConnexion::Stage3)
    {
        newError(std::string("Internal problem"),std::string("Api_protocol::readForFirstHeader() stageConnexion!=StageConnexion::Stage1 && stageConnexion!=StageConnexion::Stage2"));
        return;
    }
    if(stageConnexion==StageConnexion::Stage2)
    {
        message("stageConnexion=CatchChallenger::Api_protocol::StageConnexion::Stage3 set at "+std::string(__FILE__)+":"+std::to_string(__LINE__));
        stageConnexion=StageConnexion::Stage3;
    }
    {
        if(socket->sslSocket->mode()!=QSslSocket::UnencryptedMode)
        {
            newError(std::string("Internal problem"),std::string("socket->sslSocket->mode()!=QSslSocket::UnencryptedMode into Api_protocol::readForFirstHeader()"));
            return;
        }
        uint8_t value;
        if(socket->sslSocket->read((char*)&value,sizeof(value))==sizeof(value))
        {
            haveFirstHeader=true;
            if(value==0x01)
            {
                socket->sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
                socket->sslSocket->ignoreSslErrors();
                socket->sslSocket->startClientEncryption();
                QObject::connect(socket->sslSocket,&QSslSocket::encrypted,this,&Api_protocol::sslHandcheckIsFinished);
            }
            else
                connectTheExternalSocketInternal();
        }
    }
}

void Api_protocol::sslHandcheckIsFinished()
{
    connectTheExternalSocketInternal();
}

void Api_protocol::connectTheExternalSocketInternal()
{
    if(socket->sslSocket==NULL)
    {
        newError(std::string("Internal problem"),std::string("Api_protocol::connectTheExternalSocket() socket->sslSocket==NULL"));
        return;
    }
    if(socket->peerName().isEmpty() || socket->sslSocket->state()!=QSslSocket::SocketState::ConnectedState)
    {
        newError(std::string("Internal problem"),std::string("Api_protocol::connectTheExternalSocket() socket->sslSocket->peerAddress()==QHostAddress::Null: ")+
                 socket->peerName().toStdString()+"-"+std::to_string(socket->peerPort())+
                 ", state: "+std::to_string(socket->sslSocket->state())
                 );
        return;
    }
    //check the certificat
    {
        QDir datapackCert(QString("%1/cert/").arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation)));
        datapackCert.mkpath(datapackCert.absolutePath());
        QFile certFile;
        if(stageConnexion==StageConnexion::Stage1)
            certFile.setFileName(datapackCert.absolutePath()+"/"+socket->peerName()+"-"+QString::number(socket->peerPort()));
        else if(stageConnexion==StageConnexion::Stage3 || stageConnexion==StageConnexion::Stage4)
        {
            if(selectedServerIndex==-1)
            {
                parseError("Internal error, file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),std::string("Api_protocol::connectTheExternalSocket() selectedServerIndex==-1"));
                return;
            }
            const ServerFromPoolForDisplay &serverFromPoolForDisplay=*serverOrdenedList.at(selectedServerIndex);
            certFile.setFileName(
                        datapackCert.absolutePath()+QString("/")+
                                 QString::fromStdString(serverFromPoolForDisplay.host)+QString("-")+
                        QString::number(serverFromPoolForDisplay.port)
                        );
        }
        else
        {
            parseError("Internal error, file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),std::string("Api_protocol::connectTheExternalSocket() stageConnexion!=StageConnexion::Stage1/3"));
            return;
        }
        if(certFile.exists())
        {
            if(socket->sslSocket->mode()==QSslSocket::UnencryptedMode)
            {
                #if (!defined(CATCHCHALLENGER_VERSION_SOLO) || defined(CATCHCHALLENGER_MULTI)) && ! defined(BOTTESTCONNECT)
                SslCert sslCert(NULL);
                sslCert.exec();
                if(sslCert.validated())
                    saveCert(certFile.fileName().toStdString());
                else
                {
                    socket->sslSocket->disconnectFromHost();
                    return;
                }
                #endif
            }
            else if(certFile.open(QIODevice::ReadOnly))
            {
                if(socket->sslSocket->peerCertificate().publicKey().toPem()!=certFile.readAll())
                {
                    #if (!defined(CATCHCHALLENGER_VERSION_SOLO) || defined(CATCHCHALLENGER_MULTI)) && ! defined(BOTTESTCONNECT)
                    SslCert sslCert(NULL);
                    sslCert.exec();
                    if(sslCert.validated())
                        saveCert(certFile.fileName().toStdString());
                    else
                    {
                        socket->sslSocket->disconnectFromHost();
                        return;
                    }
                    #endif
                }
                certFile.close();
            }
        }
        else
        {
            if(socket->sslSocket->mode()!=QSslSocket::UnencryptedMode)
                saveCert(certFile.fileName().toStdString());

        }
    }
    //continue the normal procedure
    if(stageConnexion==StageConnexion::Stage1)
        connectedOnLoginServer();
    if(stageConnexion==StageConnexion::Stage2 || stageConnexion==StageConnexion::Stage3)
    {
        message("stageConnexion=CatchChallenger::Api_protocol::StageConnexion::Stage3 set at "+std::string(__FILE__)+":"+std::to_string(__LINE__));
        stageConnexion=StageConnexion::Stage3;
        connectedOnGameServer();
    }

    if(stageConnexion==StageConnexion::Stage1)
        QObject::connect(socket,&ConnectedSocket::readyRead,this,&Api_protocol::parseIncommingData,Qt::QueuedConnection);//put queued to don't have circular loop Client -> Server -> Client
    //need wait the sslHandcheck
    sendProtocol();
    if(socket->bytesAvailable())
        parseIncommingData();
}

void Api_protocol::saveCert(const std::string &file)
{
    if(socket->sslSocket==NULL)
        return;
    QFile certFile(QString::fromStdString(file));
    if(socket->sslSocket->mode()==QSslSocket::UnencryptedMode)
        certFile.remove();
    else
    {
        if(certFile.open(QIODevice::WriteOnly))
        {
            qDebug() << "Register the certificate into" << certFile.fileName();
            qDebug() << socket->sslSocket->peerCertificate().issuerInfo(QSslCertificate::Organization);
            qDebug() << socket->sslSocket->peerCertificate().issuerInfo(QSslCertificate::CommonName);
            qDebug() << socket->sslSocket->peerCertificate().issuerInfo(QSslCertificate::LocalityName);
            qDebug() << socket->sslSocket->peerCertificate().issuerInfo(QSslCertificate::OrganizationalUnitName);
            qDebug() << socket->sslSocket->peerCertificate().issuerInfo(QSslCertificate::CountryName);
            qDebug() << socket->sslSocket->peerCertificate().issuerInfo(QSslCertificate::StateOrProvinceName);
            qDebug() << socket->sslSocket->peerCertificate().issuerInfo(QSslCertificate::EmailAddress);
            certFile.write(socket->sslSocket->peerCertificate().publicKey().toPem());
            certFile.close();
        }
    }
}

bool Api_protocol::postReplyData(const uint8_t &queryNumber, const char * const data,const int &size)
{
    const quint8 packetCode=inputQueryNumberToPacketCode[queryNumber];
    removeFromQueryReceived(queryNumber);
    const uint8_t &fixedSize=ProtocolParsingBase::packetFixedSize[packetCode+128];
    if(fixedSize!=0xFE)
    {
        if(fixedSize!=size)
        {
            std::cout << "postReplyData() Sended packet size: " << size << ": " << binarytoHexa(data,size)
                      << ", but the fixed packet size defined at: " << std::to_string((int)fixedSize) << std::endl;
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            abort();
            #endif
            return false;
        }
        //fixed size
        //send the network message
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
        posOutput+=1;

        memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1,data,size);
        posOutput+=size;

        return internalSendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
    else
    {
        //dynamic size
        //send the network message
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
        posOutput+=1+4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(size);//set the dynamic size

        memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1+4,data,size);
        posOutput+=size;

        return internalSendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
}

bool Api_protocol::packOutcommingData(const uint8_t &packetCode,const char * const data,const int &size)
{
    const uint8_t &fixedSize=ProtocolParsingBase::packetFixedSize[packetCode];
    if(fixedSize!=0xFE)
    {
        if(fixedSize!=size)
        {
            std::cout << "packOutcommingData(): Sended packet size: " << size << ": " << binarytoHexa(data,size)
                      << ", but the fixed packet size defined at: " << std::to_string((int)fixedSize) << std::endl;
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            abort();
            #endif
            return false;
        }
        //fixed size
        //send the network message
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=packetCode;
        posOutput+=1;

        memcpy(ProtocolParsingBase::tempBigBufferForOutput+1,data,size);
        posOutput+=size;

        return internalSendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
    else
    {
        //dynamic size
        //send the network message
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=packetCode;
        posOutput+=1+4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(size);//set the dynamic size

        memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+4,data,size);
        posOutput+=size;

        return internalSendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
}


bool Api_protocol::packOutcommingQuery(const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const int &size)
{
    registerOutputQuery(queryNumber,packetCode);
    const uint8_t &fixedSize=ProtocolParsingBase::packetFixedSize[packetCode];
    if(fixedSize!=0xFE)
    {
        if(fixedSize!=size)
        {
            std::cout << "packOutcommingQuery(): Sended packet size: " << size << ": " << binarytoHexa(data,size)
                      << ", but the fixed packet size defined at: " << std::to_string((int)fixedSize) << std::endl;
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            abort();
            #endif
            return false;
        }
        //fixed size
        //send the network message
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=packetCode;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
        posOutput+=1;

        memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1,data,size);
        posOutput+=size;

        return internalSendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
    else
    {
        //dynamic size
        //send the network message
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=packetCode;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
        posOutput+=1+4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(size);//set the dynamic size

        memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1+4,data,size);
        posOutput+=size;

        return internalSendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
    }
}

std::string Api_protocol::toUTF8WithHeader(const std::string &text)
{
    if(text.empty())
        return std::string();
    if(text.size()>254)
        return std::string();
    std::string data;
    data.resize(1);
    data+=text;
    data[0]=static_cast<uint8_t>(text.size());
    return data;
}

void Api_protocol::have_main_and_sub_datapack_loaded()//can now load player_informations
{
    if(!delayedLogin.data.empty())
    {
        bool returnCode=parseCharacterBlockCharacter(delayedLogin.packetCode,delayedLogin.queryNumber,delayedLogin.data);
        delayedLogin.data.clear();
        delayedLogin.packetCode=0;
        delayedLogin.queryNumber=0;
        if(!returnCode)
            return;
    }
    unsigned int index=0;
    while(index<delayedMessages.size())
    {
        const DelayedMessage &delayedMessageTemp=delayedMessages.at(index);
        if(!parseMessage(delayedMessageTemp.packetCode,delayedMessageTemp.data))
        {
            delayedMessages.clear();
            return;
        }
        index++;
    }
    delayedMessages.clear();
}

bool Api_protocol::dataToPlayerMonster(QDataStream &in,PlayerMonster &monster)
{
    quint32 sub_index;
    PlayerBuff buff;
    PlayerMonster::PlayerSkill skill;
    /*if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster id bd, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> monster.id;*/
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster id, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> monster.monster;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster level, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> monster.level;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster remaining_xp, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> monster.remaining_xp;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster hp, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> monster.hp;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster sp, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> monster.sp;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster captured_with, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> monster.catched_with;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster captured_with, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    quint8 gender;
    in >> gender;
    switch(gender)
    {
        case 0x01:
        case 0x02:
        case 0x03:
            monster.gender=(Gender)gender;
        break;
        default:
            parseError("Procotol wrong or corrupted",std::string("gender code wrong: ")+std::to_string(gender)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        break;
    }
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster egg_step, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> monster.egg_step;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster character_origin, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    {
        quint8 character_origin;
        in >> character_origin;
        monster.character_origin=character_origin;
    }

    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster size of list of the buff monsters, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    uint8_t sub_size8;
    in >> sub_size8;
    sub_index=0;
    while(sub_index<sub_size8)
    {
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster buff, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        in >> buff.buff;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster buff level, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        in >> buff.level;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster buff level, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        in >> buff.remainingNumberOfTurn;
        monster.buffs.push_back(buff);
        sub_index++;
    }

    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster size of list of the skill monsters, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    uint16_t sub_size16;
    in >> sub_size16;
    sub_index=0;
    while(sub_index<sub_size16)
    {
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster skill, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        in >> skill.skill;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster skill level, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        in >> skill.level;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster skill level, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        in >> skill.endurance;
        monster.skills.push_back(skill);
        sub_index++;
    }
    return true;
}

bool Api_protocol::setMapNumber(const unsigned int number_of_map)
{
    if(number_of_map==0)
    {
        std::cerr << "to reset this number use resetAll()" << std::endl;
        return false;
    }
    std::cout << "number of map: " << std::to_string(number_of_map) << std::endl;
    this->number_of_map=number_of_map;
    return true;
}
