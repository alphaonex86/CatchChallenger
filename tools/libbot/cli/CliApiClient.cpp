#include "CliApiClient.hpp"
#include "CliDatapack.hpp"

#include <iostream>

#include "../../../general/base/CommonDatapack.hpp"
#include "../../../general/base/CommonSettingsCommon.hpp"
#include "../../../general/base/CommonSettingsServer.hpp"
#include "../../../general/base/cpp11addition.hpp"

using namespace CatchChallenger;

CliApiClient::CliApiClient() :
    Api_protocol(),
    socket(),
    state(State_Idle),
    pendingWork(PendingWork_None),
    identityLogin(),
    identityPass(),
    datapackPath(),
    failReason(),
    label(),
    verbose(false),
    stateChanged(false),
    listOnly(false),
    charactersGroupIndex(0),
    characterSlot(0),
    characterCount(0),
    mapIndex(0),
    x(0),
    y(0)
{
}

CliApiClient::~CliApiClient()
{
    socket.closeSocket();
}

void CliApiClient::setIdentity(const std::string &login,const std::string &pass,
                               const std::string &datapackPath)
{
    identityLogin=login;
    identityPass=pass;
    //Every datapack path is used by concatenation ("<path>items/", …), so the
    //trailing '/' is mandatory. Api_protocol::setDatapackPath() adds it to its
    //own copy; normalise ours too or the loader looks for "datapackitems/".
    if(stringEndsWith(datapackPath,'/'))
        this->datapackPath=datapackPath;
    else
        this->datapackPath=datapackPath+"/";
    setDatapackPath(this->datapackPath);
}

void CliApiClient::setCharacterSlot(const uint32_t &slot)
{
    characterSlot=slot;
}

void CliApiClient::setListOnly(const bool &listOnly)
{
    this->listOnly=listOnly;
}

uint32_t CliApiClient::getCharacterCount() const
{
    return characterCount;
}

const std::string &CliApiClient::getLabel() const
{
    return label;
}

void CliApiClient::setLabel(const std::string &label)
{
    this->label=label;
}

void CliApiClient::setVerbose(const bool &verbose)
{
    this->verbose=verbose;
}

const char *CliApiClient::stateToString(const State &state)
{
    switch(state)
    {
        case State_Idle:                return "IDLE";
        case State_ConnectingLogin:     return "CONNECTING_LOGIN";
        case State_LoginProtocol:       return "LOGIN_PROTOCOL";
        case State_LoggingIn:           return "LOGGING_IN";
        case State_CreatingCharacter:   return "CREATING_CHARACTER";
        case State_SelectingCharacter:  return "SELECTING_CHARACTER";
        case State_NeedReconnect:       return "NEED_RECONNECT";
        case State_ConnectingGame:      return "CONNECTING_GAME";
        case State_GameProtocol:        return "GAME_PROTOCOL";
        case State_NeedDatapack:        return "NEED_DATAPACK";
        case State_OnMap:               return "ON_MAP";
        case State_Failed:              return "FAILED";
        case State_Listed:              return "LISTED";
        default:                        return "UNKNOWN";
    }
}

void CliApiClient::setState(const State &state)
{
    if(this->state!=state)
    {
        this->state=state;
        stateChanged=true;
    }
}

bool CliApiClient::takeStateChanged()
{
    const bool returnValue=stateChanged;
    stateChanged=false;
    return returnValue;
}

void CliApiClient::fail(const std::string &reason)
{
    //keep the FIRST reason: it is the cause, the later ones are consequences
    if(failReason.empty())
        failReason=reason;
    pendingWork=PendingWork_None;
    setState(State_Failed);
    socket.closeSocket();
}

CliSocket &CliApiClient::getSocket()
{
    return socket;
}

CliApiClient::State CliApiClient::getState() const
{
    return state;
}

CliApiClient::PendingWork CliApiClient::getPendingWork() const
{
    return pendingWork;
}

bool CliApiClient::isFinished() const
{
    return state==State_OnMap || state==State_Failed || state==State_Listed;
}

const std::string &CliApiClient::getFailReason() const
{
    return failReason;
}

CATCHCHALLENGER_TYPE_MAPID CliApiClient::getMapIndex() const
{
    return mapIndex;
}

COORD_TYPE CliApiClient::getX() const
{
    return x;
}

COORD_TYPE CliApiClient::getY() const
{
    return y;
}

bool CliApiClient::connectToLoginServer(const std::string &host,const uint16_t &port)
{
    //tryLogin() before the protocol is sent only STORES the credentials;
    //Api_protocol replays them itself when the 0xA0 reply lands.
    if(!tryLogin(identityLogin,identityPass))
    {
        fail("tryLogin() refused the credentials");
        return false;
    }
    if(!socket.startConnect(host,port))
    {
        fail("connect to the login server failed: "+socket.lastError());
        return false;
    }
    setState(State_ConnectingLogin);
    return true;
}

void CliApiClient::socketWritable()
{
    if(!socket.finishConnect())
    {
        fail("connect failed: "+socket.lastError());
        return;
    }
    //The TCP session is up. readForFirstHeader() flips haveFirstHeader and
    //calls connectTheExternalSocketInternal() -> sendProtocol() (0xA0).
    readForFirstHeader();
}

void CliApiClient::socketReadyRead()
{
    parseIncommingData();
    //readFromSocket() reports a peer close by invalidating the socket; the
    //Api_protocol callbacks (disconnected/notLogged) may already have set a
    //better reason, so only report the raw close when nothing else did.
    if(!socket.isValid() && !isFinished() && pendingWork==PendingWork_None)
        fail("socket closed: "+socket.lastError());
}

bool CliApiClient::runPendingWork()
{
    const PendingWork work=pendingWork;
    pendingWork=PendingWork_None;
    if(work==PendingWork_Reconnect)
        return doReconnect();
    else
    {
        if(work==PendingWork_LoadDatapack)
            return doLoadDatapack();
        else
            return true;
    }
}

bool CliApiClient::doReconnect()
{
    //Api_protocol already called closeSocket() from the 0xAC reply; make sure
    //the fd is really gone before asking for the game server coordinates.
    socket.closeSocket();
    const std::string hostAndPort=socketDisconnectedForReconnect();
    if(hostAndPort.empty())
    {
        fail("socketDisconnectedForReconnect() returned no game server address");
        return false;
    }
    const size_t separator=hostAndPort.rfind(':');
    if(separator==std::string::npos)
    {
        fail("game server address without a port: "+hostAndPort);
        return false;
    }
    const std::string host=hostAndPort.substr(0,separator);
    bool ok=false;
    const uint16_t port=stringtouint16(hostAndPort.substr(separator+1),&ok);
    if(!ok || port==0)
    {
        fail("game server port is not a number: "+hostAndPort);
        return false;
    }
    if(verbose)
        std::cout << "[" << label << "] reconnect to the game server " << host << ":" << port << std::endl;
    if(!socket.startConnect(host,port))
    {
        fail("connect to the game server failed: "+socket.lastError());
        return false;
    }
    setState(State_ConnectingGame);
    return true;
}

bool CliApiClient::doLoadDatapack()
{
    //The datapack is on local disk (client and server share the machine), so
    //nothing is fetched over the protocol nor over HTTP.
    if(!CliDatapack::loadBase(datapackPath))
    {
        fail("datapack base: "+CliDatapack::lastError());
        return false;
    }
    if(!CliDatapack::loadMainSub(CommonSettingsServer::commonSettingsServer.mainDatapackCode,
                                 CommonSettingsServer::commonSettingsServer.subDatapackCode))
    {
        fail("datapack main/sub: "+CliDatapack::lastError());
        return false;
    }
    //ORDER MATTERS: have_main_and_sub_datapack_loaded() bails out (and the
    //character block stays queued forever) when number_of_map is still 0.
    if(!setMapNumber(CliDatapack::mapCount()))
    {
        fail("setMapNumber("+std::to_string(CliDatapack::mapCount())+") refused");
        return false;
    }
    //replays the queued character block -> haveCharacter() -> ON_MAP
    have_main_and_sub_datapack_loaded();
    return true;
}

//---- transport seam ----------------------------------------------------

ssize_t CliApiClient::readFromSocket(char *data,const size_t &size)
{
    const ssize_t readSize=socket.readData(data,size);
    if(readSize<0)
        socket.closeSocket();
    return readSize;
}

ssize_t CliApiClient::writeToSocket(const char * const data,const size_t &size)
{
    const ssize_t writtenSize=socket.writeData(data,size);
    if(writtenSize<0)
    {
        //the protocol layer turns a short write into disconnectClient()
        std::cerr << "[" << label << "] write failed: " << socket.lastError() << std::endl;
        socket.closeSocket();
    }
    return writtenSize;
}

void CliApiClient::closeSocket()
{
    socket.closeSocket();
}

void CliApiClient::readForFirstHeader()
{
    if(!haveFirstHeader)
    {
        //Mirrors Api_protocol_Qt::readForFirstHeader(): the SSL preamble byte
        //is gone, so go straight to the post-handshake path.
        if(stageConnexion==StageConnexion::Stage2)
            stageConnexion=StageConnexion::Stage3;
        haveFirstHeader=true;
        connectTheExternalSocketInternal();
    }
}

void CliApiClient::tryDisconnect()
{
    socket.closeSocket();
}

void CliApiClient::defineMaxPlayers(const uint16_t &maxPlayers)
{
    ProtocolParsing::setMaxPlayers(maxPlayers);
}

//---- state machine hooks ----------------------------------------------

void CliApiClient::newError(const std::string &error,const std::string &detailedError)
{
    fail(error+": "+detailedError);
}

void CliApiClient::message(const std::string &message)
{
    if(verbose)
        std::cout << "[" << label << "] " << message << std::endl;
}

void CliApiClient::disconnected(const std::string &reason)
{
    fail("disconnected: "+reason);
}

void CliApiClient::notLogged(const std::string &reason)
{
    fail("not logged: "+reason);
}

void CliApiClient::protocol_is_good()
{
    setState(State_LoggingIn);
}

void CliApiClient::connectedOnLoginServer()
{
    setState(State_LoginProtocol);
}

void CliApiClient::connectingOnGameServer()
{
    //fired from inside the 0xAC reply, i.e. from inside the parser: defer the
    //actual socket work to the event loop.
    setState(State_NeedReconnect);
    pendingWork=PendingWork_Reconnect;
}

void CliApiClient::connectedOnGameServer()
{
    setState(State_GameProtocol);
}

void CliApiClient::haveDatapackMainSubCode()
{
    //Called from parseCharacterBlockCharacter() BEFORE it checks whether the
    //datapack is parsed. Loading here would re-enter the parser, so only flag
    //it; the queued block is released by doLoadDatapack().
    setState(State_NeedDatapack);
    pendingWork=PendingWork_LoadDatapack;
}

void CliApiClient::logged(const std::vector<std::vector<CharacterEntry> > &characterEntryList)
{
    if(listOnly)
    {
        //Count only. The socket stays open until the event loop notices the
        //terminal state and the caller destroys us: closing it from inside a
        //protocol callback would pull the buffer out from under the parser.
        characterCount=0;
        size_t groupIndex=0;
        while(groupIndex<characterEntryList.size())
        {
            characterCount+=static_cast<uint32_t>(characterEntryList.at(groupIndex).size());
            groupIndex++;
        }
        setState(State_Listed);
        return;
    }
    selectOrCreateCharacter(characterEntryList);
}

void CliApiClient::selectOrCreateCharacter(const std::vector<std::vector<CharacterEntry> > &characterEntryList)
{
    const std::vector<ServerFromPoolForDisplay> &servers=getServerOrdenedList();
    if(servers.empty())
    {
        fail("the login server returned an empty server list");
        return;
    }
    //one server per benchmark run: take the first of the ordered list
    const ServerFromPoolForDisplay &server=servers.at(0);
    charactersGroupIndex=server.charactersGroupIndex;
    if(charactersGroupIndex<characterEntryList.size() &&
       characterSlot<characterEntryList.at(charactersGroupIndex).size())
    {
        const CharacterEntry &character=characterEntryList.at(charactersGroupIndex).at(characterSlot);
        if(verbose)
            std::cout << "[" << label << "] select character " << character.character_id
                      << " (" << character.pseudo << ")" << std::endl;
        if(!selectCharacter(charactersGroupIndex,server.uniqueKey,character.character_id,0))
        {
            fail("selectCharacter("+std::to_string(character.character_id)+") refused");
            return;
        }
        setState(State_SelectingCharacter);
    }
    else
    {
        if(!createCharacter())
            return;
        setState(State_CreatingCharacter);
    }
}

bool CliApiClient::createCharacter()
{
    //Character creation validates profileIndex and skinId against the LOCAL
    //datapack, so it has to be parsed before we can ask for a character.
    if(!CliDatapack::loadBase(datapackPath))
    {
        fail("datapack base: "+CliDatapack::lastError());
        return false;
    }
    const std::vector<Profile> &profileList=CommonDatapack::commonDatapack.get_profileList();
    if(profileList.empty())
    {
        fail("the datapack has no profile, no character can be created");
        return false;
    }
    const Profile &profile=profileList.at(0);
    uint8_t skinId=0;
    if(!profile.forcedskin.empty())
        skinId=profile.forcedskin.at(0);
    else
    {
        if(CommonDatapack::commonDatapack.get_skins().empty())
        {
            fail("the datapack has no skin, no character can be created");
            return false;
        }
    }
    //The pseudo is unique SERVER-WIDE, not per account, so derive it from the
    //login AND the slot; two accounts on the same server would otherwise
    //collide on "bench0". Keep the slot suffix and truncate the login part to
    //fit the server-announced limit.
    const std::string suffix=std::to_string(characterSlot);
    const uint8_t maxPseudoSize=CommonSettingsCommon::commonSettingsCommon.max_pseudo_size;
    std::string pseudo=identityLogin;
    if(maxPseudoSize>0)
    {
        if(suffix.size()>=maxPseudoSize)
            pseudo.clear();
        else
        {
            if(pseudo.size()>(maxPseudoSize-suffix.size()))
                pseudo=pseudo.substr(0,maxPseudoSize-suffix.size());
        }
    }
    pseudo+=suffix;
    if(maxPseudoSize>0 && pseudo.size()>maxPseudoSize)
        pseudo=pseudo.substr(0,maxPseudoSize);
    if(verbose)
        std::cout << "[" << label << "] create character " << pseudo << std::endl;
    if(!addCharacter(charactersGroupIndex,0,pseudo,0,skinId))
    {
        fail("addCharacter("+pseudo+") refused");
        return false;
    }
    return true;
}

void CliApiClient::newCharacterId(const uint8_t &returnCode,const uint32_t &characterId)
{
    //0x00 is the SUCCESS code here (see BaseWindow::newCharacterId): 0x01 is
    //"pseudo already taken", 0x02 "already at the max character count".
    if(returnCode!=0x00)
    {
        if(returnCode==0x01)
            fail("character creation refused: the pseudo is already taken");
        else
        {
            if(returnCode==0x02)
                fail("character creation refused: the account is at its max character count");
            else
                fail("character creation refused, return code "+std::to_string(static_cast<uint32_t>(returnCode)));
        }
        return;
    }
    if(characterId==0)
    {
        fail("character creation returned the id 0");
        return;
    }
    const std::vector<ServerFromPoolForDisplay> &servers=getServerOrdenedList();
    if(servers.empty())
    {
        fail("the login server returned an empty server list");
        return;
    }
    if(!selectCharacter(charactersGroupIndex,servers.at(0).uniqueKey,characterId,0))
    {
        fail("selectCharacter("+std::to_string(characterId)+") refused after creation");
        return;
    }
    setState(State_SelectingCharacter);
}

void CliApiClient::haveCharacter(const CATCHCHALLENGER_TYPE_MAPID &mapIndex,const COORD_TYPE &x,
                                const COORD_TYPE &y,const Direction &last_direction)
{
    (void)last_direction;
    this->mapIndex=mapIndex;
    this->x=x;
    this->y=y;
    setState(State_OnMap);
}

std::string CliApiClient::getLanguage() const
{
    return std::string("en");
}

bool CliApiClient::haveBeatBot(const CATCHCHALLENGER_TYPE_MAPID &mapId,
                               const CATCHCHALLENGER_TYPE_BOTID &botFightId) const
{
    (void)mapId;
    (void)botFightId;
    //a benchmark bot never fights, so no bot has ever been beaten
    return false;
}
