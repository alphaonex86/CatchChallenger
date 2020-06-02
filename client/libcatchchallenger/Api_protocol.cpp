#include "Api_protocol.hpp"
#include "../../general/base/ProtocolVersion.hpp"
#include "../../general/base/tinyXML2/tinyxml2.hpp"
#include "../../general/base/tinyXML2/customtinyxml2.hpp"

using namespace CatchChallenger;

const unsigned char protocolHeaderToMatchLogin[] = PROTOCOL_HEADER_LOGIN;
const unsigned char protocolHeaderToMatchGameServer[] = PROTOCOL_HEADER_GAMESERVER;

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#endif
#include <iostream>

#include "../../general/base/GeneralStructures.hpp"
#include "../../general/base/GeneralVariable.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/FacilityLib.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/GeneralType.hpp"

//need host + port here to have datapack base

std::unordered_set<std::string> Api_protocol::extensionAllowed;

/*std::string "<root>"="<root>";
std::string "</root>"="</root>";
std::string Api_protocol::text_name="name";
std::string Api_protocol::text_description="description";
std::string Api_protocol::text_en="en";
std::string Api_protocol::text_lang="lang";
std::string Api_protocol::text_slash="/";*/

Api_protocol::Api_protocol() :
    ProtocolParsingInputOutput(PacketModeTransmission_Client),
    tolerantMode(false)
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
    //qDebug() << "Api_protocol::~Api_protocol()";
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

void Api_protocol::socketDestroyed()
{
}

std::map<uint8_t,uint64_t> Api_protocol::getQuerySendTimeList() const
{
    return querySendTime;
}

std::vector<uint8_t> Api_protocol::getEvents()
{
    return events;
}

void Api_protocol::parseIncommingData()
{
    ProtocolParsingInputOutput::parseIncommingData();
}

void Api_protocol::errorParsingLayer(const std::string &error)
{
    newError(("Internal error, file: "+std::string(__FILE__)+":"+std::to_string(__LINE__)),error);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    //abort();-> same behaviour on all platform
    #endif
}

void Api_protocol::messageParsingLayer(const std::string &message) const
{
    std::cout << message << std::endl;
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
        std::cerr << "Api_protocol::get_player_informations(): !getCharacterSelected() (internal error)" << std::endl;
        return player_informations;
    }
    return player_informations;
}

const Player_private_and_public_informations &Api_protocol::get_player_informations_ro() const
{
    if(!getCaracterSelected())
    {
        std::cerr << "Api_protocol::get_player_informations_ro(): !getCharacterSelected() (internal error)" << std::endl;
        return player_informations;
    }
    return player_informations;
}

std::string Api_protocol::getPseudo() const
{
    if(!getCaracterSelected())
    {
        std::cerr << "Api_protocol::getPseudo(): !getCaracterSelected() (internal error)" << std::endl;
        return std::string();
    }
    return player_informations.public_informations.pseudo;
}

uint16_t Api_protocol::getId() const
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
    const ServerFromPoolForDisplay &serverFromPoolForDisplay=serverOrdenedList.at(selectedServerIndex);
    if(serverFromPoolForDisplay.host.empty())
    {
        parseError("Internal error, file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),std::string("serverFromPoolForDisplay.host.isEmpty() with Api_protocol::socketDisconnectedForReconnect()"));
        return std::string();
    }
    message("stageConnexion=CatchChallenger::Api_protocol::StageConnexion::Stage3 set at "+std::string(__FILE__)+":"+std::to_string(__LINE__));
    stageConnexion=CatchChallenger::Api_protocol::StageConnexion::Stage3;//prevent loop in stage2
    haveFirstHeader=false;
    //std::cout << "Api_protocol::socketDisconnectedForReconnect(), Try connect to: " << serverFromPoolForDisplay.host << ":" << serverFromPoolForDisplay.port << std::endl;
    return serverFromPoolForDisplay.host+":"+std::to_string(serverFromPoolForDisplay.port);
}

const std::vector<ServerFromPoolForDisplay> &Api_protocol::getServerOrdenedList()
{
    return serverOrdenedList;
}

bool Api_protocol::protocolWrong() const
{
    return have_send_protocol && !have_receive_protocol;
}

bool Api_protocol::tryLogin(const std::string &login, const std::string &pass)
{
    if(!have_send_protocol)
    {
        this->login=login;
        this->pass=pass;
        return true;
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
    char outputData[CATCHCHALLENGER_SHA224HASH_SIZE*2];
    {
        std::string loginAndSalt=login+/*salt*/"RtR3bm9Z1DFMfAC3";
        hashSha224(loginAndSalt.data(),loginAndSalt.size(),outputData);
    }
    {
        char passHash[CATCHCHALLENGER_SHA224HASH_SIZE];
        std::string passAndSaltAndLogin=pass+/*salt*/"AwjDvPIzfJPTTgHs"+login;
        hashSha224(passAndSaltAndLogin.data(),passAndSaltAndLogin.size(),passHash);

        std::string hashAndTokenString=std::string(passHash,CATCHCHALLENGER_SHA224HASH_SIZE)+token;
        hashSha224(hashAndTokenString.data(),hashAndTokenString.size(),outputData+CATCHCHALLENGER_SHA224HASH_SIZE);
    }

    packOutcommingQuery(0xA8,queryNumber(),outputData,sizeof(outputData));
    this->login.clear();
    this->pass.clear();
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
    char outputData[CATCHCHALLENGER_SHA224HASH_SIZE+passHash.size()];
    hashSha224(loginHash.data(),loginHash.size(),outputData);
    //pass
    memcpy(outputData+CATCHCHALLENGER_SHA224HASH_SIZE,passHash.data(),passHash.size());

    packOutcommingQuery(0xA9,queryNumber(),outputData,sizeof(outputData));
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
    char buffer[2];
    buffer[0]=moved_unit;
    buffer[1]=directionInt;
    packOutcommingData(0x02,buffer,sizeof(buffer));
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
    if(chatType!=Chat_type_local && chatType!=Chat_type_all && chatType!=Chat_type_clan && chatType!=Chat_type_system && chatType!=Chat_type_system_important)
    {
        std::cerr << "chatType wrong: " << chatType << std::endl;
        return;
    }
    if(text.size()>255)
    {
        std::cerr << "text in Utf8 too big, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    char buffer[2+text.size()];
    buffer[0]=(uint8_t)chatType;
    buffer[1]=(uint8_t)text.size();
    memcpy(buffer+2,text.data(),text.size());
    packOutcommingData(0x03,buffer,sizeof(buffer));
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
    if(text.size()>255)
    {
        std::cerr << "text in Utf8 too big, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    if(pseudo.size()>255)
    {
        std::cerr << "text in Utf8 too big, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return;
    }
    char buffer[2+text.size()+1+pseudo.size()];
    buffer[0]=(uint8_t)Chat_type_pm;
    buffer[1]=text.size();
    memcpy(buffer+2,text.data(),text.size());
    buffer[2+text.size()]=pseudo.size();
    memcpy(buffer+2+text.size()+1,pseudo.data(),pseudo.size());
    packOutcommingData(0x03,buffer,sizeof(buffer));
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
    postReplyData(teleportQueryInformation.queryId,NULL,0);
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
    char buffer[2+1+pseudo.size()+2];
    buffer[0]=(uint8_t)charactersGroupIndex;
    buffer[1]=(uint8_t)profileIndex;
    buffer[2]=pseudo.size();
    memcpy(buffer+3,pseudo.data(),pseudo.size());
    buffer[3+pseudo.size()]=(uint8_t)monsterGroupId;
    buffer[3+pseudo.size()+1]=(uint8_t)skinId;
    is_logged=packOutcommingQuery(0xAA,queryNumber(),buffer,sizeof(buffer));
    return true;
}

bool Api_protocol::removeCharacter(const uint8_t &charactersGroupIndex,const uint32_t &characterId)
{
    if(!is_logged)
    {
        std::cerr << "is not logged, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return false;
    }
    char buffer[1+4];
    buffer[0]=(uint8_t)charactersGroupIndex;
    const uint32_t &characterIdLittleEndian=htole32(characterId);
    memcpy(buffer+1,&characterIdLittleEndian,sizeof(characterIdLittleEndian));
    is_logged=packOutcommingQuery(0xAB,queryNumber(),buffer,sizeof(buffer));
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
        const ServerFromPoolForDisplay &server=serverOrdenedList.at(index);
        if(server.uniqueKey==serverUniqueKey && server.charactersGroupIndex==charactersGroupIndex)
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
    char buffer[1+4+4];
    buffer[0]=(uint8_t)charactersGroupIndex;
    const uint32_t &serverUniqueKeyLittleEndian=htole32(serverUniqueKey);
    memcpy(buffer+1,&serverUniqueKeyLittleEndian,sizeof(serverUniqueKeyLittleEndian));
    const uint32_t &characterIdLittleEndian=htole32(characterId);
    memcpy(buffer+1+4,&characterIdLittleEndian,sizeof(characterIdLittleEndian));
    is_logged=packOutcommingQuery(0xAC,queryNumber(),buffer,sizeof(buffer));
    this->selectedServerIndex=serverIndex;
    //std::cout << "this: " << this << ", socket: " << socket << ", select char: " << characterId << ", charactersGroupIndex: " << (uint32_t)charactersGroupIndex << ", serverUniqueKey: " << serverUniqueKey << ", line: " << __FILE__ << ": " << __LINE__ << std::endl;
    return true;
}

bool Api_protocol::selectCharacter(const uint32_t &serverIndex, const uint32_t &characterId)
{
    const std::vector<ServerFromPoolForDisplay> &serverOrdenedList=getServerOrdenedList();
    if(serverIndex>=serverOrdenedList.size())
    {
        std::cerr << "server index out of bound, line: " << __FILE__ << ": " << __LINE__ << std::endl;
        return false;
    }
    const uint8_t &charactersGroupIndex=serverOrdenedList.at(serverIndex).charactersGroupIndex;
    const uint32_t &serverUniqueKey=serverOrdenedList.at(serverIndex).uniqueKey;
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
    char buffer[1+4+4];
    buffer[0]=(uint8_t)charactersGroupIndex;
    const uint32_t &serverUniqueKeyLittleEndian=htole32(serverUniqueKey);
    memcpy(buffer+1,&serverUniqueKeyLittleEndian,sizeof(serverUniqueKeyLittleEndian));
    const uint32_t &characterIdLittleEndian=htole32(characterId);
    memcpy(buffer+1+4,&characterIdLittleEndian,sizeof(characterIdLittleEndian));
    is_logged=packOutcommingQuery(0xAC,queryNumber(),buffer,sizeof(buffer));
    this->selectedServerIndex=serverIndex;
    //std::cout << "this: " << this << ", socket: " << socket << ", select char: " << characterId << ", charactersGroupIndex: " << (uint32_t)charactersGroupIndex << ", serverUniqueKey: " << serverUniqueKey << ", line: " << __FILE__ << ": " << __LINE__ << std::endl;
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
    char outputData[1];
    outputData[0]=plant_id;
    packOutcommingData(0x19,outputData,sizeof(outputData));
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
    char buffer[2];
    buffer[0]=(uint8_t)0x01;
    buffer[1]=number;
    packOutcommingData(0x0D,buffer,sizeof(buffer));
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
    char buffer[2];
    buffer[0]=(uint8_t)monterPosition;
    packOutcommingData(0x0F,buffer,sizeof(buffer));
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
    char buffer[2];
    buffer[0]=(uint8_t)0x02;
    buffer[1]=number;
    packOutcommingData(0x0D,buffer,sizeof(buffer));
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
    char buffer[2+4];
    const uint16_t &objectLittleEndian=htole16(object);
    memcpy(buffer,&objectLittleEndian,sizeof(objectLittleEndian));
    const uint32_t &quantityLittleEndian=htole32(quantity);
    memcpy(buffer+2,&quantityLittleEndian,sizeof(quantityLittleEndian));
    packOutcommingData(0x13,buffer,sizeof(buffer));
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
    char buffer[2];
    const uint16_t &objectLittleEndian=htole16(object);
    memcpy(buffer,&objectLittleEndian,sizeof(objectLittleEndian));
    packOutcommingQuery(0x86,queryNumber(),buffer,sizeof(buffer));
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
    char buffer[2+1];
    const uint16_t &objectLittleEndian=htole16(object);
    memcpy(buffer,&objectLittleEndian,sizeof(objectLittleEndian));
    buffer[2]=monsterPosition;
    packOutcommingData(0x10,buffer,sizeof(buffer));
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
    char buffer[8+2+(2+4)+4+(4)+4+(4)];
    const uint64_t &cashLittleEndian=htole64(cash);
    memcpy(buffer,&cashLittleEndian,sizeof(cashLittleEndian));

    const uint16_t &index16=htole16(items.size());
    memcpy(buffer+8,&index16,sizeof(index16));
    unsigned int index=0;
    while(index<items.size())
    {
        const uint16_t &index16=htole16(items.at(index).first);
        memcpy(buffer+8+2+(2+4)*index,&index16,sizeof(index16));
        const uint32_t &index32=htole32(items.at(index).second);
        memcpy(buffer+8+2+(2+4)*index+2,&index32,sizeof(index32));
        index++;
    }

    uint32_t index32=htole32(withdrawMonsters.size());
    memcpy(buffer+8+2+(2+4)*items.size(),&index32,sizeof(index32));
    index=0;
    while(index<withdrawMonsters.size())
    {
        uint32_t index32=htole32(withdrawMonsters.at(index));
        memcpy(buffer+8+2+(2+4)*items.size()+4+4*index,&index32,sizeof(index32));
        index++;
    }
    index32=htole32(depositeMonsters.size());
    memcpy(buffer+8+2+(2+4)*items.size()+4+4*withdrawMonsters.size(),&index32,sizeof(index32));
    index=0;
    while(index<depositeMonsters.size())
    {
        index32=htole32(depositeMonsters.at(index));
        memcpy(buffer+8+2+(2+4)*items.size()+4+4*withdrawMonsters.size()+4+4*index,&index32,sizeof(index32));
        index++;
    }

    packOutcommingData(0x17,buffer,sizeof(buffer));
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
    char buffer[2];
    const uint16_t &shopIdLittleEndian=htole16(shopId);
    memcpy(buffer,&shopIdLittleEndian,sizeof(shopIdLittleEndian));
    packOutcommingQuery(0x87,queryNumber(),buffer,sizeof(buffer));
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
    char buffer[2+2+4+4];
    const uint16_t &shopIdLittleEndian=htole16(shopId);
    memcpy(buffer,&shopIdLittleEndian,sizeof(shopIdLittleEndian));
    const uint16_t &objectIdLittleEndian=htole16(objectId);
    memcpy(buffer+2,&objectIdLittleEndian,sizeof(objectIdLittleEndian));
    const uint32_t &quantityLittleEndian=htole32(quantity);
    memcpy(buffer+2+2,&quantityLittleEndian,sizeof(quantityLittleEndian));
    const uint32_t &priceLittleEndian=htole32(price);
    memcpy(buffer+2+2+4,&priceLittleEndian,sizeof(priceLittleEndian));
    packOutcommingQuery(0x88,queryNumber(),buffer,sizeof(buffer));
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
    char buffer[2+2+4+4];
    const uint16_t &shopIdLittleEndian=htole16(shopId);
    memcpy(buffer,&shopIdLittleEndian,sizeof(shopIdLittleEndian));
    const uint16_t &objectIdLittleEndian=htole16(objectId);
    memcpy(buffer+2,&objectIdLittleEndian,sizeof(objectIdLittleEndian));
    const uint32_t &quantityLittleEndian=htole32(quantity);
    memcpy(buffer+2+2,&quantityLittleEndian,sizeof(quantityLittleEndian));
    const uint32_t &priceLittleEndian=htole32(price);
    memcpy(buffer+2+2+4,&priceLittleEndian,sizeof(priceLittleEndian));
    packOutcommingQuery(0x89,queryNumber(),buffer,sizeof(buffer));
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
    char buffer[2];
    const uint16_t &factoryIdLittleEndian=htole16(factoryId);
    memcpy(buffer,&factoryIdLittleEndian,sizeof(factoryIdLittleEndian));
    packOutcommingQuery(0x8A,queryNumber(),buffer,sizeof(buffer));
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
    char buffer[2+2+4+4];
    const uint16_t &factoryIdLittleEndian=htole16(factoryId);
    memcpy(buffer,&factoryIdLittleEndian,sizeof(factoryIdLittleEndian));
    const uint16_t &objectIdLittleEndian=htole16(objectId);
    memcpy(buffer+2,&objectIdLittleEndian,sizeof(objectIdLittleEndian));
    const uint32_t &quantityLittleEndian=htole32(quantity);
    memcpy(buffer+2+2,&quantityLittleEndian,sizeof(quantityLittleEndian));
    const uint32_t &priceLittleEndian=htole32(price);
    memcpy(buffer+2+2+4,&priceLittleEndian,sizeof(priceLittleEndian));
    packOutcommingQuery(0x8B,queryNumber(),buffer,sizeof(buffer));
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
    char buffer[2+2+4+4];
    const uint16_t &factoryIdLittleEndian=htole16(factoryId);
    memcpy(buffer,&factoryIdLittleEndian,sizeof(factoryIdLittleEndian));
    const uint16_t &objectIdLittleEndian=htole16(objectId);
    memcpy(buffer+2,&objectIdLittleEndian,sizeof(objectIdLittleEndian));
    const uint32_t &quantityLittleEndian=htole32(quantity);
    memcpy(buffer+2+2,&quantityLittleEndian,sizeof(quantityLittleEndian));
    const uint32_t &priceLittleEndian=htole32(price);
    memcpy(buffer+2+2+4,&priceLittleEndian,sizeof(priceLittleEndian));
    packOutcommingQuery(0x8C,queryNumber(),buffer,sizeof(buffer));
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
    packOutcommingData(0x07,NULL,0);
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
    packOutcommingData(0x0B,NULL,0);
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
    char buffer[2];
    const uint16_t &fightIdLittleEndian=htole16(fightId);
    memcpy(buffer,&fightIdLittleEndian,sizeof(fightIdLittleEndian));
    packOutcommingData(0x0C,buffer,sizeof(buffer));
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
    char buffer[1];
    buffer[0]=(uint8_t)monsterPosition;
    packOutcommingData(0x0E,buffer,sizeof(buffer));
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
    char buffer[2];
    const uint16_t &skillLittleEndian=htole16(skill);
    memcpy(buffer,&skillLittleEndian,sizeof(skillLittleEndian));
    packOutcommingData(0x11,buffer,sizeof(buffer));
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
    char buffer[1+2];
    buffer[0]=monsterPosition;
    const uint16_t &skillLittleEndian=htole16(skill);
    memcpy(buffer+1,&skillLittleEndian,sizeof(skillLittleEndian));
    packOutcommingData(0x09,buffer,sizeof(buffer));
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
    char buffer[2];
    const uint16_t &questIdLittleEndian=htole16(questId);
    memcpy(buffer,&questIdLittleEndian,sizeof(questIdLittleEndian));
    packOutcommingData(0x1B,buffer,sizeof(buffer));
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
    char buffer[2];
    const uint16_t &questIdLittleEndian=htole16(questId);
    memcpy(buffer,&questIdLittleEndian,sizeof(questIdLittleEndian));
    packOutcommingData(0x1C,buffer,sizeof(buffer));
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
    char buffer[2];
    const uint16_t &questIdLittleEndian=htole16(questId);
    memcpy(buffer,&questIdLittleEndian,sizeof(questIdLittleEndian));
    packOutcommingData(0x1D,buffer,sizeof(buffer));
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
    char buffer[2];
    const uint16_t &questIdLittleEndian=htole16(questId);
    memcpy(buffer,&questIdLittleEndian,sizeof(questIdLittleEndian));
    packOutcommingData(0x1E,buffer,sizeof(buffer));
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
    if(name.size()>255 || name.empty())
    {
        std::cerr << "rawText too big or not compatible with utf8" << std::endl;
        return;
    }
    char buffer[1+1+name.size()];
    buffer[0]=(uint8_t)0x01;
    buffer[1]=name.size();
    memcpy(buffer+1+1,name.data(),name.size());
    packOutcommingQuery(0x92,queryNumber(),buffer,sizeof(buffer));
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
    char buffer[1];
    buffer[0]=(uint8_t)0x02;
    packOutcommingQuery(0x92,queryNumber(),buffer,sizeof(buffer));
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
    char buffer[1];
    buffer[0]=(uint8_t)0x03;
    packOutcommingQuery(0x92,queryNumber(),buffer,sizeof(buffer));
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
    if(pseudo.size()>255 || pseudo.empty())
    {
        std::cerr << "rawText too big or not compatible with utf8" << std::endl;
        return;
    }
    char buffer[1+1+pseudo.size()];
    buffer[0]=(uint8_t)0x04;
    buffer[1]=(uint8_t)pseudo.size();
    memcpy(buffer+1+1,pseudo.data(),pseudo.size());
    packOutcommingQuery(0x92,queryNumber(),buffer,sizeof(buffer));
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
    if(pseudo.size()>255 || pseudo.empty())
    {
        std::cerr << "rawText too big or not compatible with utf8" << std::endl;
        return;
    }
    char buffer[1+1+pseudo.size()];
    buffer[0]=(uint8_t)0x05;
    buffer[1]=(uint8_t)pseudo.size();
    memcpy(buffer+1+1,pseudo.data(),pseudo.size());
    packOutcommingQuery(0x92,queryNumber(),buffer,sizeof(buffer));
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
    char buffer[1];
    if(accept)
        buffer[0]=(uint8_t)0x01;
    else
        buffer[0]=(uint8_t)0x02;
    packOutcommingData(0x04,buffer,sizeof(buffer));
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
    char buffer[1];
    if(!cancel)
        buffer[0]=(uint8_t)0x00;
    else
        buffer[0]=(uint8_t)0x01;
    packOutcommingData(0x1F,buffer,sizeof(buffer));
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
    packOutcommingQuery(0x8D,queryNumber(),NULL,0);
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
    char buffer[1+4+4];
    buffer[0]=(uint8_t)0x01;
    const uint32_t &marketObjectIdLittleEndian=htole32(marketObjectId);
    memcpy(buffer+1,&marketObjectIdLittleEndian,sizeof(marketObjectIdLittleEndian));
    const uint32_t &quantityLittleEndian=htole32(quantity);
    memcpy(buffer+1+4,&quantityLittleEndian,sizeof(quantityLittleEndian));
    packOutcommingQuery(0x8E,queryNumber(),buffer,sizeof(buffer));
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
    char buffer[1+4];
    buffer[0]=(uint8_t)0x02;
    const uint32_t &monsterMarketIdLittleEndian=htole32(monsterMarketId);
    memcpy(buffer+1,&monsterMarketIdLittleEndian,sizeof(monsterMarketIdLittleEndian));
    packOutcommingQuery(0x8E,queryNumber(),buffer,sizeof(buffer));
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
    char buffer[1+2+4+8];
    buffer[0]=(uint8_t)0x01;
    const uint16_t &objectIdIdLittleEndian=htole16(objectId);
    memcpy(buffer+1,&objectIdIdLittleEndian,sizeof(objectIdIdLittleEndian));
    const uint32_t &quantityLittleEndian=htole32(quantity);
    memcpy(buffer+1+2,&quantityLittleEndian,sizeof(quantityLittleEndian));
    const uint64_t &priceLittleEndian=htole64(price);
    memcpy(buffer+1+2+4,&priceLittleEndian,sizeof(priceLittleEndian));
    packOutcommingQuery(0x8F,queryNumber(),buffer,sizeof(buffer));
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
    char buffer[1+1+8];
    buffer[0]=(uint8_t)0x02;
    buffer[1]=monsterPosition;
    const uint64_t &priceLittleEndian=htole64(price);
    memcpy(buffer+1+1,&priceLittleEndian,sizeof(priceLittleEndian));
    packOutcommingQuery(0x8F,queryNumber(),buffer,sizeof(buffer));
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
    packOutcommingQuery(0x90,queryNumber(),NULL,0);
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
    char buffer[1+2+4];
    buffer[0]=(uint8_t)0x01;
    const uint16_t &objectPositionLittleEndian=htole16(objectPosition);
    memcpy(buffer+1,&objectPositionLittleEndian,sizeof(objectPositionLittleEndian));
    const uint32_t &quantityLittleEndian=htole32(quantity);
    memcpy(buffer+1+2,&quantityLittleEndian,sizeof(quantityLittleEndian));
    packOutcommingQuery(0x91,queryNumber(),buffer,sizeof(buffer));
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
    char buffer[1+4];
    buffer[0]=(uint8_t)0x02;
    const uint32_t &monsterMarketIdLittleEndian=htole32(monsterMarketId);
    memcpy(buffer+1,&monsterMarketIdLittleEndian,sizeof(monsterMarketIdLittleEndian));
    packOutcommingQuery(0x91,queryNumber(),buffer,sizeof(buffer));
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
    packOutcommingData(0x1A,NULL,0);
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
    char buffer[2];
    const uint16_t &recipeIdLittleEndian=htole16(recipeId);
    memcpy(buffer,&recipeIdLittleEndian,sizeof(recipeIdLittleEndian));
    packOutcommingQuery(0x85,queryNumber(),buffer,sizeof(buffer));
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
    char buffer[1]={0x02};
    postReplyData(battleRequestId.front(),buffer,sizeof(buffer));
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
    char buffer[1]={0x01};
    postReplyData(battleRequestId.front(),buffer,sizeof(buffer));
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
    char buffer[1]={0x02};
    postReplyData(tradeRequestId.front(),buffer,sizeof(buffer));
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
    char buffer[1]={0x01};
    postReplyData(tradeRequestId.front(),buffer,sizeof(buffer));
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
    packOutcommingData(0x16,NULL,0);
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
    packOutcommingData(0x15,NULL,0);
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
    char buffer[1+8]={0x01};
    const uint64_t &cashLittleEndian=htole64(cash);
    memcpy(buffer+1,&cashLittleEndian,sizeof(cashLittleEndian));
    packOutcommingData(0x14,buffer,sizeof(buffer));
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
    char buffer[1+2+4]={0x02};
    const uint16_t &itemLittleEndian=htole16(item);
    memcpy(buffer+1,&itemLittleEndian,sizeof(itemLittleEndian));
    const uint32_t &quantityLittleEndian=htole32(quantity);
    memcpy(buffer+1+2,&quantityLittleEndian,sizeof(quantityLittleEndian));
    packOutcommingData(0x14,buffer,sizeof(buffer));
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
    char buffer[1+1]={0x03};
    buffer[1]=monsterPosition;
    packOutcommingData(0x14,buffer,sizeof(buffer));
}

Api_protocol::StageConnexion Api_protocol::stage() const
{
    return stageConnexion;
}

std::pair<uint16_t,uint16_t> Api_protocol::getLast_number_of_player()
{
    return std::pair<uint16_t,uint16_t>(last_players_number,last_max_players);
}

//to reset all
void Api_protocol::resetAll()
{
    if(stageConnexion==StageConnexion::Stage2)
        std::cerr << "Api_protocol::resetAll() Suspect internal bug" << std::endl;
    //status for the query
    chat_list.clear();
    token.clear();
    events.clear();
    stageConnexion=StageConnexion::Stage1;
    last_players_number=1;
    last_max_players=65535;

    haveFirstHeader=false;
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
    player_informations.public_informations.monsterId=0;
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
    login.clear();
    pass.clear();

    unloadSelection();
    isInTrade=false;
    tradeRequestId.clear();
    isInBattle=false;
    battleRequestId.clear();
    mDatapackBase="datapack/";
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

const ServerFromPoolForDisplay &Api_protocol::getCurrentServer(const unsigned int &index)
{
    if(index>=serverOrdenedList.size())
        abort();
    return serverOrdenedList.at(index);
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
    const auto loadOkay = domDocument.Parse(("<root>"+xml+"</root>").c_str());
    if(loadOkay!=0)
    {
        std::cerr << "Api_protocol::addLogicalGroup(): "+tinyxml2errordoc(&domDocument) << std::endl;
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
                    if(name->Attribute("lang")!=NULL && name->Attribute("lang")==language && name->GetText()!=NULL)
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
                        if(name->GetText()!=NULL)
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
    const auto loadOkay = domDocument.Parse(("<root>"+server.xml+"</root>").c_str());
    if(loadOkay!=0)
    {
        std::cerr << "Api_protocol::addLogicalServer(): "+tinyxml2errordoc(&domDocument) << std::endl;
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
                    if(name->Attribute("lang")!=NULL && name->Attribute("lang")==language && name->GetText()!=NULL)
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
                        if(name->GetText()!=NULL)
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
                    if(description->Attribute("lang")!=NULL && description->Attribute("lang")==language && description->GetText()!=NULL)
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
                        if(description->GetText()!=NULL)
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
        std::cerr << "out of range for addLogicalGroup: " << server.xml << ", server.logicalGroupIndex "
                  << server.logicalGroupIndex << " <= logicialGroupIndexList.size() " << server.xml
                  << " (defaulting to root folder)" << std::to_string(logicialGroupIndexList.size())
                  << std::endl;
        logicialGroupCursor=&logicialGroup;
    }
    else
        logicialGroupCursor=logicialGroupIndexList.at(server.logicalGroupIndex);

    //serverOrdenedListIndex, should always be set at Api_protocol_message //Internal server list for the current pool
    ServerFromPoolForDisplay newServer;

    newServer.host=server.host;
    newServer.port=server.port;
    newServer.uniqueKey=server.uniqueKey;

    newServer.name=nameString;
    newServer.description=descriptionString;
    newServer.charactersGroupIndex=server.charactersGroupIndex;
    newServer.maxPlayer=server.maxPlayer;
    newServer.currentPlayer=server.currentPlayer;
    newServer.playedTime=0;
    newServer.lastConnect=0;

    newServer.serverOrdenedListIndex=255;

    logicialGroupCursor->servers.push_back(newServer);
    return &logicialGroupCursor->servers.back();
}

LogicialGroup Api_protocol::getLogicialGroup() const
{
    return logicialGroup;
}

void Api_protocol::sslHandcheckIsFinished()
{
    connectTheExternalSocketInternal();
}

void Api_protocol::connectTheExternalSocketInternal()
{
    //continue the normal procedure
    if(stageConnexion==StageConnexion::Stage1)
        connectedOnLoginServer();
    if(stageConnexion==StageConnexion::Stage2 || stageConnexion==StageConnexion::Stage3)
    {
        message("stageConnexion=CatchChallenger::Api_protocol::StageConnexion::Stage3 set at "+std::string(__FILE__)+":"+std::to_string(__LINE__));
        stageConnexion=StageConnexion::Stage3;
        connectedOnGameServer();
    }
    //need wait the sslHandcheck
    sendProtocol();
}

bool Api_protocol::postReplyData(const uint8_t &queryNumber, const char * const data,const int &size)
{
    const uint8_t packetCode=inputQueryNumberToPacketCode[queryNumber];
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

bool Api_protocol::packOutcommingData(const uint8_t &packetCode,const char * const data,const unsigned int &size)
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


bool Api_protocol::packOutcommingQuery(const uint8_t &packetCode, const uint8_t &queryNumber, const char * const data, const unsigned int &size)
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
    if(number_of_map==0)
    {
        std::cerr << "This race case will produce infinity loop, have you well call setMapNumber() before have_main_and_sub_datapack_loaded()?" << std::endl;
        return;
    }
    if(!delayedLogin.data.empty())
    {
        bool returnCode=parseCharacterBlockCharacter(delayedLogin.packetCode,delayedLogin.queryNumber,
            delayedLogin.data.data(),delayedLogin.data.size());
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
        if(!parseMessage(delayedMessageTemp.packetCode,delayedMessageTemp.data.data(),delayedMessageTemp.data.size()))
        {
            delayedMessages.clear();
            return;
        }
        index++;
    }
    delayedMessages.clear();
}

int Api_protocol::dataToPlayerMonster(const char * const data,const unsigned int &size,PlayerMonster &monster)
{
    int pos=0;
    uint32_t sub_index;
    PlayerBuff buff;
    PlayerMonster::PlayerSkill skill;
    if((size-pos)<(int)sizeof(uint16_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster id, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return -1;
    }
    monster.monster=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
    pos+=sizeof(uint16_t);
    if((size-pos)<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster level, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return -1;
    }
    monster.level=*(data+pos);
    pos+=sizeof(uint8_t);
    if((size-pos)<(int)sizeof(uint32_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster remaining_xp, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return -1;
    }
    monster.remaining_xp=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
    pos+=sizeof(uint32_t);
    if((size-pos)<(int)sizeof(uint32_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster hp, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return -1;
    }
    monster.hp=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
    pos+=sizeof(uint32_t);
    if((size-pos)<(int)sizeof(uint32_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster sp, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return -1;
    }
    monster.sp=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
    pos+=sizeof(uint32_t);
    if((size-pos)<(int)sizeof(uint16_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster captured_with, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return -1;
    }
    monster.catched_with=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
    pos+=sizeof(uint16_t);
    if((size-pos)<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster captured_with, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return -1;
    }
    uint8_t gender=*(data+pos);
    pos+=sizeof(uint8_t);
    switch(gender)
    {
        case 0x01:
        case 0x02:
        case 0x03:
            monster.gender=(Gender)gender;
        break;
        default:
            parseError("Procotol wrong or corrupted",std::string("gender code wrong: ")+std::to_string(gender)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return -1;
        break;
    }
    if((size-pos)<(int)sizeof(uint32_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster egg_step, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return -1;
    }
    monster.egg_step=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
    pos+=sizeof(uint32_t);
    if((size-pos)<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster character_origin, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return -1;
    }
    {
        uint8_t character_origin=*(data+pos);
        pos+=sizeof(uint8_t);
        monster.character_origin=character_origin;
    }

    if((size-pos)<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster size of list of the buff monsters, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return -1;
    }
    uint8_t sub_size8=*(data+pos);
    pos+=sizeof(uint8_t);
    sub_index=0;
    while(sub_index<sub_size8)
    {
        if((size-pos)<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster buff, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return -1;
        }
        buff.buff=*(data+pos);
        pos+=sizeof(uint8_t);
        if((size-pos)<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster buff level, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return -1;
        }
        buff.level=*(data+pos);
        pos+=sizeof(uint8_t);
        if((size-pos)<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster buff level, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return -1;
        }
        buff.remainingNumberOfTurn=*(data+pos);
        pos+=sizeof(uint8_t);
        monster.buffs.push_back(buff);
        sub_index++;
    }

    if((size-pos)<(int)sizeof(uint16_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster size of list of the skill monsters, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return -1;
    }
    uint16_t sub_size16=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
    pos+=sizeof(uint16_t);
    sub_index=0;
    while(sub_index<sub_size16)
    {
        if((size-pos)<(int)sizeof(uint16_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster skill, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return -1;
        }
        skill.skill=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
        pos+=sizeof(uint16_t);
        if((size-pos)<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster skill level, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return -1;
        }
        skill.level=*(data+pos);
        pos+=sizeof(uint8_t);
        if((size-pos)<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster skill level, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return -1;
        }
        skill.endurance=*(data+pos);
        pos+=sizeof(uint8_t);
        monster.skills.push_back(skill);
        sub_index++;
    }
    return pos;
}

bool Api_protocol::setMapNumber(const unsigned int number_of_map)
{
    if(number_of_map==0)
    {
        std::cerr << "to reset this number use resetAll()" << std::endl;
        return false;
    }
    //std::cout << "number of map: " << std::to_string(number_of_map) << std::endl;
    this->number_of_map=number_of_map;
    return true;
}

void Api_protocol::add_system_text(CatchChallenger::Chat_type chat_type,std::string text)
{
    ChatEntry newEntry;
    newEntry.player_type=Player_type_normal;
    //newEntry.player_pseudo=QString();
    newEntry.chat_type=chat_type;
    newEntry.text=text;
    chat_list.push_back(newEntry);
    while(chat_list.size()>64)
        chat_list.erase(chat_list.cbegin());
}

void Api_protocol::add_chat_text(CatchChallenger::Chat_type chat_type,std::string text,std::string pseudo,CatchChallenger::Player_type player_type)
{
    ChatEntry newEntry;
    newEntry.player_type=player_type;
    newEntry.player_pseudo=pseudo;
    newEntry.chat_type=chat_type;
    newEntry.text=text;
    chat_list.push_back(newEntry);
    while(chat_list.size()>64)
        chat_list.erase(chat_list.cbegin());
}

const std::vector<Api_protocol::ChatEntry> &Api_protocol::getChatContent()
{
    return chat_list;
}
