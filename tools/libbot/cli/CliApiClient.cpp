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
    y(0),
    outputBuffer(),
    outputCursor(0),
    spamMode(false),
    spamActive(false),
    spamArmed(false),
    spamNeedPrepare(false),
    spamServerDirection(Direction_look_at_bottom),
    spamDirectionA(Direction_move_at_left),
    spamDirectionB(Direction_move_at_right),
    spamPendingSteps(0),
    spamOnSecondTile(false),
    spamMapIndex(0),
    spamX0(0),
    spamY0(0),
    spamX1(0),
    spamY1(0),
    moveCount(0)
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
    //ProtocolParsingBase disconnects the client on a short write, and blocking
    //here would freeze the whole single-threaded fleet on one slow socket. So
    //append, push what the kernel takes, and report the full size: the
    //remainder is flushed by flushOutput() when the fd is writable again.
    //Appending unconditionally (instead of trying a direct send first) is what
    //keeps the byte ORDER right when a flush is still pending.
    outputBuffer.insert(outputBuffer.end(),data,data+size);
    if(!flushOutput())
        return -1;
    return static_cast<ssize_t>(size);
}

bool CliApiClient::wantWrite() const
{
    return outputCursor<outputBuffer.size();
}

bool CliApiClient::flushOutput()
{
    if(!socket.isValid())
    {
        outputBuffer.clear();
        outputCursor=0;
        return false;
    }
    while(outputCursor<outputBuffer.size())
    {
        const ssize_t writtenSize=socket.writeSome(outputBuffer.data()+outputCursor,
                                                  outputBuffer.size()-outputCursor);
        if(writtenSize<0)
        {
            std::cerr << "[" << label << "] write failed: " << socket.lastError() << std::endl;
            socket.closeSocket();
            outputBuffer.clear();
            outputCursor=0;
            return false;
        }
        if(writtenSize==0)
            return true;//send buffer full, the rest stays queued
        outputCursor+=static_cast<size_t>(writtenSize);
    }
    //fully drained: reuse the allocation instead of erase()ing the front
    outputBuffer.clear();
    outputCursor=0;
    return true;
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
    //"Login already used" (0xA9 code 0x02) is the reply to a create-account that
    //LOST a race: several bots opened the same brand-new login at once and one
    //of them created it. The account exists now, so a plain login WOULD work —
    //but the server does not leave the connection open for it: loginIsWrong()
    //sends the 0x02 reply and then calls errorOutput(), which disconnects. So
    //there is nothing to retry on this socket; the fix is to not race at all,
    //which is why main() serialises the very first login of a shared account and
    //why --spam gives every bot its own login.
    if(reason=="Login already used")
    {
        fail("not logged: "+reason+" (lost a create-account race; the server "
             "disconnects the loser, so this bot cannot recover on this socket)");
        return;
    }
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
    //The server KICKS a client that asks for one character too many
    //("You can't create more account, you have already N on M allowed",
    //ClientHeavyLoadLogin2.cpp -> errorOutput()). max_character came with the
    //login reply, so refuse it here: a clear per-bot message beats N kicks, and
    //the ON_MAP/SURVIVORS counts then say honestly how many bots the account
    //could host. Use one login per bot (bot-bench --spam does) to lift the cap.
    const uint8_t maxCharacter=CommonSettingsCommon::commonSettingsCommon.max_character;
    if(characterSlot>=maxCharacter)
    {
        fail("this account is limited to "+std::to_string(static_cast<uint32_t>(maxCharacter))+
             " character(s) by the server, so the slot "+std::to_string(characterSlot)+
             " cannot be created");
        return false;
    }
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
    this->mapIndex=mapIndex;
    this->x=x;
    this->y=y;
    //The server's MapBasicMove::last_direction starts at this orientation and
    //it REFUSES a move packet repeating the current direction, so the first
    //move must differ from it. Keep it to seed the alternation.
    spamServerDirection=last_direction;
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

//---- saturation ("spam") mode ------------------------------------------

void CliApiClient::setSpamMode(const bool &spam)
{
    spamMode=spam;
    if(spam)
        moveCount=0;
}

bool CliApiClient::wasSpamArmed() const
{
    return spamArmed;
}

uint64_t CliApiClient::getMoveCount() const
{
    return moveCount;
}

bool CliApiClient::isSpamming() const
{
    return spamMode && spamActive && state==State_OnMap && socket.isValid();
}

void CliApiClient::spamRefused(const std::string &reason)
{
    //NOT fail(): the bot stays logged in and on the map, it just cannot walk.
    //Disconnecting it here would look exactly like a server kick in the report.
    if(failReason.empty())
        failReason=reason;
}

void CliApiClient::stopSpam(const std::string &reason)
{
    if(spamActive)
    {
        spamActive=false;
        if(verbose)
            std::cout << "[" << label << "] spam stopped: " << reason << std::endl;
    }
}

const char *CliApiClient::tileIsSafeForSpam(const CommonMap &map,const COORD_TYPE &x,const COORD_TYPE &y)
{
    if(x>=map.width || y>=map.height)
        return "out of the map";
    if(!MoveOnTheMap::isWalkable(map,x,y))
        return "not walkable";
    //A ledge is one-way: the server forces the walk to continue and then
    //rejects the packet ("Try pass on wrong ledge") -> kick.
    if(MoveOnTheMap::getLedge(map,x,y)!=ParsedLayerLedges_NoLedges)
        return "a ledge";
    //Dirt is a plantable tile; harmless to walk on but it is never a plain
    //floor, so skip it and keep the oscillation on ordinary ground.
    if(MoveOnTheMap::isDirt(map,x,y))
        return "dirt (plantable)";
    //A teleporter source would move the bot to another map behind its back.
    if(MoveOnTheMap::needBeTeleported(map,x,y))
        return "a teleporter source";
    //Grass/water/cave: the server starts a wild fight after a random number of
    //steps and NEVER tells the client (the client is expected to derive it from
    //the shared random seeds). Every following move is then answered with
    //"try move when is in fight" -> kick. A zone whose walkOn list is empty has
    //no monster at all, and one whose defaultMonsters are all empty cannot
    //start a fight either (CommonFightEngine::generateWildFightIfCollision()).
    const MonstersCollisionValue zone=MoveOnTheMap::getZoneCollision(map,x,y);
    size_t index=0;
    while(index<zone.walkOn.size())
    {
        if(index<zone.walkOnMonsters.size())
        {
            if(!zone.walkOnMonsters.at(index).defaultMonsters.empty())
                return "a wild-monster zone (grass/water/cave)";
        }
        index++;
    }
    const std::pair<uint8_t,uint8_t> position(x,y);
    //Same story for a fight-bot trigger tile.
    if(map.botsFightTrigger.find(position)!=map.botsFightTrigger.cend())
        return "a fight-bot trigger";
    //An item on the ground / a shop tile makes the server answer the move with
    //extra packets; keep the loop measuring the move path only.
    if(map.items.find(position)!=map.items.cend())
        return "an item on the ground";
    if(map.shops.find(position)!=map.shops.cend())
        return "a shop";
    return NULL;
}

bool CliApiClient::prepareSpam()
{
    spamActive=false;
    spamNeedPrepare=false;
    if(state!=State_OnMap)
    {
        spamRefused("prepareSpam() called while not on the map");
        return false;
    }
    const std::vector<CommonMap> * const maps=CliDatapack::mapList();
    if(maps==NULL)
    {
        spamRefused("prepareSpam(): the datapack maps are not loaded");
        return false;
    }
    if(mapIndex>=maps->size())
    {
        spamRefused("prepareSpam(): map index "+std::to_string(mapIndex)+" is out of the "+
                    std::to_string(maps->size())+" loaded maps");
        return false;
    }
    const CommonMap &map=maps->at(mapIndex);
    const char * const startProblem=tileIsSafeForSpam(map,x,y);
    if(startProblem!=NULL)
    {
        spamRefused("prepareSpam(): the start tile ("+std::to_string(static_cast<uint32_t>(x))+
                    ","+std::to_string(static_cast<uint32_t>(y))+") of map "+
                    std::to_string(mapIndex)+" is "+std::string(startProblem)+
                    ", so it cannot be used for a move loop");
        return false;
    }
    //Try the four opposite pairs. The bot must come back to the SAME tile, so
    //the pair has to be a real axis (left/right, top/bottom) and the step must
    //not leave the map: a border crossing would land on a map whose tiles are
    //not the ones validated here.
    Direction candidates[4];
    candidates[0]=Direction_move_at_left;
    candidates[1]=Direction_move_at_right;
    candidates[2]=Direction_move_at_top;
    candidates[3]=Direction_move_at_bottom;
    uint8_t index=0;
    while(index<4)
    {
        const Direction first=candidates[index];
        CATCHCHALLENGER_TYPE_MAPID targetMap=mapIndex;
        COORD_TYPE targetX=x;
        COORD_TYPE targetY=y;
        //The very first packet turns the character towards `first` without
        //walking, and the server rejects a packet repeating its current
        //direction: a pair starting with that direction is unusable. Skipping
        //it (instead of swapping the pair) keeps the FIRST walked tile the one
        //validated just below.
        //allowTeleport=false so a step onto a teleporter source is refused
        //here rather than discovered by the server.
        if(first!=spamServerDirection &&
           MoveOnTheMap::canGoTo<CommonMap>(*maps,first,map,x,y,true,false))
        {
            if(MoveOnTheMap::moveWithoutTeleport<CommonMap>(*maps,first,targetMap,targetX,targetY,true,false))
            {
                if(targetMap==mapIndex && tileIsSafeForSpam(map,targetX,targetY)==NULL)
                {
                    //the way back is the opposite direction of the pair
                    Direction back=Direction_move_at_right;
                    if(first==Direction_move_at_left)
                        back=Direction_move_at_right;
                    else
                    {
                        if(first==Direction_move_at_right)
                            back=Direction_move_at_left;
                        else
                        {
                            if(first==Direction_move_at_top)
                                back=Direction_move_at_bottom;
                            else
                                back=Direction_move_at_top;
                        }
                    }
                    spamDirectionA=first;
                    spamDirectionB=back;
                    spamMapIndex=mapIndex;
                    spamX0=x;
                    spamY0=y;
                    spamX1=targetX;
                    spamY1=targetY;
                    spamOnSecondTile=false;
                    //the first packet only turns the character: 0 step
                    spamPendingSteps=0;
                    spamActive=true;
                    spamArmed=true;
                    if(verbose)
                        std::cout << "[" << label << "] spam axis "
                                  << MoveOnTheMap::directionToString(spamDirectionA) << "/"
                                  << MoveOnTheMap::directionToString(spamDirectionB)
                                  << " on map " << mapIndex
                                  << " between (" << std::to_string(spamX0) << "," << std::to_string(spamY0)
                                  << ") and (" << std::to_string(spamX1) << "," << std::to_string(spamY1)
                                  << ")" << std::endl;
                    return true;
                }
            }
        }
        index++;
    }
    spamRefused("prepareSpam(): no safe neighbour tile around ("+
                std::to_string(static_cast<uint32_t>(x))+","+
                std::to_string(static_cast<uint32_t>(y))+") on map "+std::to_string(mapIndex));
    return false;
}

void CliApiClient::sendOneMove()
{
    //0x02 = (steps already walked in the CURRENT direction, NEW direction).
    //The server executes those steps and then turns; repeating the current
    //direction is a protocol error on its side, hence the strict alternation.
    Direction next=spamDirectionA;
    if(spamServerDirection==spamDirectionA)
        next=spamDirectionB;
    char buffer[2];
    buffer[0]=static_cast<char>(spamPendingSteps);
    buffer[1]=static_cast<char>(static_cast<uint8_t>(next));
    if(!packOutcommingData(0x02,buffer,sizeof(buffer)))
    {
        stopSpam("the protocol layer refused the move packet");
        return;
    }
    //the announced steps are now done on the server: mirror them locally
    if(spamPendingSteps>0)
    {
        spamOnSecondTile=!spamOnSecondTile;
        if(spamOnSecondTile)
        {
            x=spamX1;
            y=spamY1;
        }
        else
        {
            x=spamX0;
            y=spamY0;
        }
        mapIndex=spamMapIndex;
    }
    spamServerDirection=next;
    spamPendingSteps=1;
    moveCount++;
}

uint32_t CliApiClient::pushSpamMoves(const uint32_t &maxMoves)
{
    uint32_t sent=0;
    if(spamNeedPrepare)
    {
        //a teleport moved the bot: re-validate the tiles before moving again
        if(!prepareSpam())
            return 0;
    }
    while(sent<maxMoves)
    {
        if(!isSpamming())
            return sent;
        //Closed loop: only queue the next move when the kernel took the
        //previous one. When the server stops draining, wantWrite() stays true
        //and the caller waits for POLLOUT instead of burning CPU on EAGAIN.
        if(wantWrite())
            return sent;
        const uint64_t before=moveCount;
        sendOneMove();
        if(moveCount==before)
            return sent;//sendOneMove() gave up (socket or protocol error)
        sent++;
    }
    return sent;
}

void CliApiClient::teleportTo(const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,
                              const COORD_TYPE &y,const Direction &direction)
{
    //A teleport is a server QUERY (0xE1): it MUST be answered or the server
    //runs out of query ids and kicks us ("no free query number"). teleportDone()
    //posts that reply and re-seeds Api_protocol's own direction state.
    this->mapIndex=mapId;
    this->x=x;
    this->y=y;
    spamServerDirection=direction;
    spamPendingSteps=0;
    if(!teleportDone())
        fail("teleportDone() refused to answer the teleport query");
    else
    {
        //The validated tile pair belongs to the OLD map: re-pick, but outside
        //the parser (prepareSpam() must not run while parseIncommingData() owns
        //the input buffer).
        if(spamActive)
        {
            spamActive=false;
            spamNeedPrepare=true;
        }
        if(verbose)
            std::cout << "[" << label << "] teleported to map " << mapId
                      << " (" << std::to_string(x) << "," << std::to_string(y) << ")" << std::endl;
    }
}
