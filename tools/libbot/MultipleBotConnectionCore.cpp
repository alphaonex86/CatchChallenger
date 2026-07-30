#include "MultipleBotConnectionCore.h"
#include "BotAbort.h"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/ProtocolParsing.hpp"

#include <iostream>
#include <stdlib.h>

/// Replace every occurrence of from by to. Hand rolled on purpose: pulling
/// <regex> in for a literal substitution costs a lot of code size and runtime
/// for nothing.
static void replaceAll(std::string &subject,const std::string &from,const std::string &to)
{
    if(!from.empty())
    {
        size_t pos=subject.find(from);
        while(pos!=std::string::npos)
        {
            subject.replace(pos,from.size(),to);
            pos=subject.find(from,pos+to.size());
        }
    }
}

MultipleBotConnectionCore::BotClient::BotClient() :
    link(NULL),
    have_informations(false),
    haveShowDisconnectionReason(false),
    haveBeenDiscounted(false),
    number(0),
    selectedCharacter(false),
    stat(Status_None)
{
    preferences.plant=100;
    preferences.item=100;
    preferences.fight=100;
    preferences.shop=100;
    preferences.wild=100;
}

MultipleBotConnectionCore::BotClient::~BotClient()
{
}

MultipleBotConnectionCore::MultipleBotConnectionCore() :
    botInterface(NULL),
    numberToChangeLoginForMultipleConnexion(1),
    numberOfBotConnected(0),
    numberOfSelectedCharacter(0),
    numberOfStartSelectingCharacter(0),
    numberOfHaveDatapackCharacter(0),
    numberOfStartCreatingCharacter(0),
    numberOfStartCreatedCharacter(0),
    mHaveAnError(false),
    charactersGroupIndex(0),
    serverUniqueKey(-1),
    serverIsSelected(false)
{
    CatchChallenger::ProtocolParsing::initialiseTheVariable();
    CatchChallenger::ProtocolParsing::setMaxPlayers(65535);
}

MultipleBotConnectionCore::~MultipleBotConnectionCore()
{
    unsigned int index=0;
    while(index<clientList.size())
    {
        BotClient * const client=clientList.at(index);
        //The link owns the protocol object and the socket(s): destroying it is
        //what releases them.
        delete client->link;
        client->link=NULL;
        delete client;
        index++;
    }
    clientList.clear();
}

bool MultipleBotConnectionCore::haveAnError()
{
    return mHaveAnError;
}

/* --- progress notifications, no-op by default -------------------------- */
void MultipleBotConnectionCore::notifyNumberOfBotConnected(const uint16_t value)
{
    (void)value;
}

void MultipleBotConnectionCore::notifyNumberOfSelectedCharacter(const uint16_t value)
{
    (void)value;
}

void MultipleBotConnectionCore::notifyNumberOfStartSelectingCharacter(const uint16_t value)
{
    (void)value;
}

void MultipleBotConnectionCore::notifyNumberOfHaveDatapackCharacter(const uint16_t value)
{
    (void)value;
}

void MultipleBotConnectionCore::notifyNumberOfStartCreatingCharacter(const uint16_t value)
{
    (void)value;
}

void MultipleBotConnectionCore::notifyNumberOfStartCreatedCharacter(const uint16_t value)
{
    (void)value;
}

void MultipleBotConnectionCore::notifyUpdateClientListStatus()
{
}

void MultipleBotConnectionCore::notifyAllPlayerConnected()
{
}

void MultipleBotConnectionCore::notifyAllPlayerOnMap()
{
}

/* --- the state machine ------------------------------------------------- */
MultipleBotConnectionCore::BotClient * MultipleBotConnectionCore::createClientCore()
{
    if(mHaveAnError)
    {
        stopConnectTimer();
        return NULL;
    }
    BotClient * const client=allocClient();
    if(client==NULL)
    {
        std::cerr << "MultipleBotConnectionCore::createClientCore(): allocClient() returned NULL" << std::endl;
        mHaveAnError=true;
        return NULL;
    }
    if(client->link==NULL)
    {
        std::cerr << "MultipleBotConnectionCore::createClientCore(): allocClient() returned a client without link" << std::endl;
        mHaveAnError=true;
        delete client;
        return NULL;
    }
    client->stat=Status_None;
    notifyUpdateClientListStatus();

    if(!client->link->connectToHost(host(),port(),proxy(),proxyport()))
    {
        std::cerr << "MultipleBotConnectionCore::createClientCore(): unable to start the connection to "
                  << host() << ":" << port() << std::endl;
        mHaveAnError=true;
    }
    client->stat=Status_Connecting;
    notifyUpdateClientListStatus();
    connectTheExternalSocket(client);
    return client;
}

void MultipleBotConnectionCore::connectTheExternalSocket(BotClient *client)
{
    if(client->link->api()==NULL)
    {
        std::cerr << "connectTheExternalSocket client->link->api()==NULL" << std::endl;
        BOT_ABORT();
    }
    client->link->api()->setDatapackPath(datapackPath());
    client->haveShowDisconnectionReason=false;
    client->haveBeenDiscounted=false;
    client->have_informations=false;
    client->number=numberToChangeLoginForMultipleConnexion;
    client->selectedCharacter=false;
    numberToChangeLoginForMultipleConnexion++;
    clientList.push_back(client);
    tryLink(client);
}

void MultipleBotConnectionCore::tryLink(BotClient *client)
{
    numberOfBotConnected++;
    notifyNumberOfBotConnected(numberOfBotConnected);

    if(client->link->api()==NULL)
    {
        std::cerr << "tryLink client->link->api()==NULL" << std::endl;
        BOT_ABORT();
    }
    if(!multipleConnexion())
    {
        client->login=login();
        client->pass=pass();
    }
    else
    {
        std::string loginString=login();
        std::string passString=pass();
        const std::string numberString=std::to_string(client->number);
        replaceAll(loginString,"%NUMBER%",numberString);
        replaceAll(passString,"%NUMBER%",numberString);
        client->login=loginString;
        client->pass=passString;
    }
}

void MultipleBotConnectionCore::protocol_is_good_with_client(BotClient *client)
{
    if(client->link->api()==NULL)
    {
        std::cerr << "protocol_is_good_with_client client->link->api()==NULL" << std::endl;
        BOT_ABORT();
    }
    client->stat=Status_WaitLogin;
    notifyUpdateClientListStatus();
    client->link->api()->tryLogin(client->login,client->pass);
}

void MultipleBotConnectionCore::insert_player_with_client(BotClient *client,const CatchChallenger::Player_public_informations &player,const uint8_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction)
{
    client->stat=Status_OnMap;
    notifyUpdateClientListStatus();
    (void)mapId;
    (void)x;
    (void)y;
    (void)direction;
    (void)player;

    client->have_informations=true;
}

void MultipleBotConnectionCore::haveCharacter_with_client(BotClient *client,const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE &y,const CatchChallenger::Direction &direction)
{
    if(client->selectedCharacter!=true)
    {
        std::cout << "selected character at " << __FILE__ << ":" << __LINE__ << std::endl;
        client->selectedCharacter=true;
        numberOfSelectedCharacter++;
        notifyNumberOfSelectedCharacter(numberOfSelectedCharacter);
        client->stat=Status_SelectedCharacter;
        notifyUpdateClientListStatus();
    }

    if(multipleConnexion())
    {
        if(numberOfBotConnected>=numberOfSelectedCharacter)
        {
            const uint32_t diff=numberOfBotConnected-numberOfSelectedCharacter;
            if(diff==0 && (int)numberOfSelectedCharacter>=connexionCountTarget())
                notifyAllPlayerOnMap();
        }
    }
    else
        notifyAllPlayerOnMap();

    //Register the bot's OWN player, the same way MapControllerMP::loadCurrentPlayer()
    //does for the real client. Without it the bot keeps the placeholder entry
    //MainWindow creates (mapId 0, 0/0, canMoveOnMap false) and ActionsAction never
    //starts moveTimer, so doMove() never walks the step the AI computes into
    //target.localStep: the bot stands still for the whole run.
    if(botInterface!=NULL && client->link->api()!=NULL)
        botInterface->insert_player(client->link->api(),
                                    client->link->api()->get_player_informations().public_informations,
                                    mapId,x,y,direction);
}

std::string MultipleBotConnectionCore::getNewPseudo()
{
    return std::string("bot")+CatchChallenger::FacilityLibGeneral::randomPassword("abcdefghijklmnopqurstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890",CommonSettingsCommon::commonSettingsCommon.max_pseudo_size-3);
}

void MultipleBotConnectionCore::createCharacterFor(BotClient *client)
{
    if(CatchChallenger::CommonDatapack::commonDatapack.get_profileList().empty())
        std::cerr << "Profile list is empty, unable to create a character" << std::endl;
    else
    {
        std::cout << client->login << " create new character" << std::endl;
        const uint8_t profileIndex=rand()%CatchChallenger::CommonDatapack::commonDatapack.get_profileList().size();
        const std::string pseudo=getNewPseudo();
        uint8_t skinId;
        const CatchChallenger::Profile &profile=CatchChallenger::CommonDatapack::commonDatapack.get_profileList().at(profileIndex);
        if(!profile.forcedskin.empty())
            skinId=profile.forcedskin.at(rand()%profile.forcedskin.size());
        else
        {
            // Skin count from the parsed datapack (CommonDatapack::get_skins()),
            // the single client-side source -- the datapack CACHE is server-only,
            // the client never sees it. It can be 0: a headless server ships
            // skin/fighter/ as empty folders and datapack sync transfers files
            // only, so the client receives no skin subdirs. Guard rand()%0
            // (SIGFPE); skinId 0 is still valid (the server validates against
            // ITS own skin count).
            const size_t skinCount=CatchChallenger::CommonDatapack::commonDatapack.get_skins().size();
            if(skinCount==0)
                skinId=0;
            else
                skinId=rand()%skinCount;
        }
        uint8_t monstergroupId;
        if(profile.monstergroup.empty())
            monstergroupId=0;
        else
            monstergroupId=rand()%profile.monstergroup.size();
        client->link->api()->addCharacter(charactersGroupIndex,profileIndex,pseudo,monstergroupId,skinId);
        numberOfStartCreatingCharacter++;
        notifyNumberOfStartCreatingCharacter(numberOfStartCreatingCharacter);
        client->stat=Status_CreatingCharacter;
        notifyUpdateClientListStatus();
    }
}

void MultipleBotConnectionCore::logged_with_client(BotClient *client)
{
    if(!serverIsSelected)
    {
        std::cerr << "logged_with_client(): !serverIsSelected" << std::endl;
        return;
    }
    client->stat=Status_Logged;
    notifyUpdateClientListStatus();
    if(client->link->api()==NULL)
    {
        std::cerr << "logged_with_client client->link->api()==NULL" << std::endl;
        BOT_ABORT();
    }
    if(client->charactersList.empty() || client->charactersList.at(charactersGroupIndex).size()<=0)
    {
        std::cout << client->login << " have not character" << std::endl;
        if((autoCreateCharacter() || multipleConnexion()) && serverIsSelected)
            createCharacterFor(client);
        return;
    }
    if(multipleConnexion() && serverIsSelected)
    {
        if(!client->charactersList.empty() && charactersGroupIndex<client->charactersList.size())
        {
            const uint32_t character_id=client->charactersList.at(charactersGroupIndex).at(rand()%client->charactersList.at(charactersGroupIndex).size()).character_id;
            if(characterOnMap.find(character_id)==characterOnMap.cend())
            {
                if(!client->link->api()->selectCharacter(charactersGroupIndex,serverUniqueKey,character_id))
                    std::cerr << "Unable to do automatic character selection: " << character_id << std::endl;
                else
                {
                    numberOfStartSelectingCharacter++;
                    notifyNumberOfStartSelectingCharacter(numberOfStartSelectingCharacter);
                    client->stat=Status_SelectingCharacter;
                    notifyUpdateClientListStatus();
                    //if not the first bot
                    if(!tempMapList.empty())
                    {
                        //then do fake init normaly done into haveDatapackMainSubCode_with_client()
                        if(!client->link->api()->setMapNumber(tempMapList.size()))
                            BOT_ABORT();
                        client->link->api()->have_main_and_sub_datapack_loaded();
                    }
                    characterOnMap.insert(character_id);
                }
            }
        }
        return;
    }
}

void MultipleBotConnectionCore::haveTheDatapack_with_client(BotClient *client)
{
    if(client->link->api()==NULL)
    {
        std::cerr << "haveTheDatapack_with_client client->link->api()==NULL" << std::endl;
        BOT_ABORT();
    }
    numberOfHaveDatapackCharacter++;
    notifyNumberOfHaveDatapackCharacter(numberOfHaveDatapackCharacter);
    if(botInterface!=NULL)
        std::cout << "haveTheDatapack_with_client(): Bot version: " << botInterface->name() << " " << botInterface->version() << std::endl;
    //load the profil list
    {
        CatchChallenger::CommonDatapack::commonDatapack.parseDatapack(datapackPath());//load always after the rates
        // No separate skin folder-scan: the skin list is whatever
        // parseDatapack() loaded into CommonDatapack::get_skins() (used at
        // character creation, guarded against an empty list).
    }

    if(client->charactersList.size()<=0 || charactersGroupIndex>=client->charactersList.size() || client->charactersList.at(charactersGroupIndex).empty())
    {
        if(serverIsSelected)
        {
            std::cout << client->login << " have not character" << std::endl;
            if(autoCreateCharacter() || multipleConnexion())
                createCharacterFor(client);
        }
        else
        {
            std::cerr << client->login << " have no character and no server selected: " << client->charactersList.size()
                      << ", charactersGroupIndex: " << (unsigned int)charactersGroupIndex << std::endl;
            if(client->charactersList.size()>0 && charactersGroupIndex<client->charactersList.size())
                std::cerr << client->login << " client->charactersList.at(charactersGroupIndex).empty()" << std::endl;
        }
        return;
    }
    //the actual client
    const uint32_t character_id=client->charactersList.at(charactersGroupIndex).at(rand()%client->charactersList.at(charactersGroupIndex).size()).character_id;
    if(characterOnMap.find(character_id)==characterOnMap.cend())
    {
        if(multipleConnexion() && serverIsSelected)
        {
            if(!client->link->api()->selectCharacter(charactersGroupIndex,serverUniqueKey,character_id))
                std::cerr << "Unable to select character after datapack loading: " << character_id << std::endl;
            else
            {
                characterOnMap.insert(character_id);
                std::cout << "haveTheDatapack_with_client: select character after datapack loading: " << character_id << std::endl;
            }
        }
    }
}

void MultipleBotConnectionCore::haveDatapackMainSubCode_with_client(BotClient *client)
{
    if(botInterface!=NULL)
        std::cout << "haveDatapackMainSubCode_with_client(): Bot version: " << botInterface->name() << " " << botInterface->version() << std::endl;
    if(client->link->api()==NULL)
    {
        std::cerr << "haveDatapackMainSubCode_with_client client->link->api()==NULL" << std::endl;
        BOT_ABORT();
    }
    client->link->sendDatapackContentMainSub();
}

void MultipleBotConnectionCore::haveTheDatapackMainSub_with_client(BotClient *client)
{
    if(client->link->api()==NULL)
    {
        std::cerr << "haveTheDatapackMainSub_with_client client->link->api()==NULL" << std::endl;
        BOT_ABORT();
    }
    if(botInterface!=NULL)
        std::cout << "haveTheDatapackMainSub_with_client(): Bot version: " << botInterface->name() << " " << botInterface->version() << std::endl;
    {
        if(CommonSettingsServer::commonSettingsServer.mainDatapackCode=="[main]")
        {
            std::cerr << "CommonSettingsServer::commonSettingsServer.mainDatapackCode==[main]" << std::endl;
            #ifdef CATCHCHALLENGER_HARDENED
            BOT_ABORT();
            #else
            return;
            #endif
        }
        if(CommonSettingsServer::commonSettingsServer.subDatapackCode=="[sub]")
        {
            std::cerr << "CommonSettingsServer::commonSettingsServer.subDatapackCode==[sub]" << std::endl;
            #ifdef CATCHCHALLENGER_HARDENED
            BOT_ABORT();
            #else
            return;
            #endif
        }
    }
    //load the datapack
    if(!tempMapList.empty())
    {
        std::cerr << "haveTheDatapackMainSub_with_client() !tempMapList.empty() internal bug" << std::endl;
        BOT_ABORT();
    }
    {
        const std::string &datapackPathString=datapackPath();
        CatchChallenger::CommonDatapack::commonDatapack.parseDatapack(datapackPathString);

        const std::vector<std::string> &returnList=
                    CatchChallenger::FacilityLibGeneral::listFolder(
                        (datapackPathString+"map/main/"+CommonSettingsServer::commonSettingsServer.mainDatapackCode+"/")
                        );

        //load the map: keep the .tmx files, drop the ones with a quote in the
        //name (they would break the xml the map id list is fed into), and index
        //them by the name without the extension. Plain string tests: a regex
        //here would be pure code bloat for a suffix compare.
        const size_t size=returnList.size();
        size_t index=0;
        catchchallenger_datapack_map<std::string, CATCHCHALLENGER_TYPE_MAPID> mapPathToId;
        while(index<size)
        {
            const std::string &fileName=returnList.at(index);
            if(fileName.size()>4 && fileName.compare(fileName.size()-4,4,".tmx")==0 &&
                    fileName.find('"')==std::string::npos && fileName.find('\'')==std::string::npos)
            {
                const std::string sortFileName=fileName.substr(0,fileName.size()-4);
                mapPathToId[sortFileName]=(CATCHCHALLENGER_TYPE_MAPID)tempMapList.size();
                tempMapList.push_back(sortFileName);
            }
            index++;
        }
        CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.parseDatapackAfterZoneAndMap(datapackPathString,CommonSettingsServer::commonSettingsServer.mainDatapackCode,CommonSettingsServer::commonSettingsServer.subDatapackCode,mapPathToId);
    }
    if(!client->link->api()->setMapNumber(tempMapList.size()))
        BOT_ABORT();
    client->link->api()->have_main_and_sub_datapack_loaded();
    ifMultipleConnexionStartCreation();
}

void MultipleBotConnectionCore::ifMultipleConnexionStartCreation()
{
    if(multipleConnexion())
    {
        if(!connectTimerIsActive())
        {
            //The tick is wired once by the front-end. Here we only (re)start the
            //timer: connectTimerTick() is idempotent once the target count is
            //reached, so a late re-entry on a slow server (the ESP32, after the
            //timer was stopped) is harmless.
            int temp_ms=1000/connectBySeconds();
            if(temp_ms<1)
                temp_ms=1;
            startConnectTimer((unsigned int)temp_ms);
            std::cout << "ifMultipleConnexionStartCreation(): start the multiple timer co " << temp_ms << std::endl;
            return;
        }
        else
            std::cout << "ifMultipleConnexionStartCreation(): connectTimerIsActive()" << std::endl;
    }
    else
    {
        std::cout << "ifMultipleConnexionStartCreation(): !multipleConnexion()" << std::endl;
        notifyAllPlayerConnected();
    }
}

void MultipleBotConnectionCore::connectTimerTick()
{
    const int connexionCountVar=connexionCountTarget();
    if((int)clientList.size()<connexionCountVar && (int)numberOfBotConnected<connexionCountVar)
    {
        if(numberOfBotConnected<numberOfSelectedCharacter)
        {
            std::cerr << "connectTimerTick(): numberOfBotConnected(" << numberOfBotConnected
                      << ")<numberOfSelectedCharacter(" << numberOfSelectedCharacter << ")" << std::endl;
            mHaveAnError=true;
            stopConnectTimer();
        }
        else
        {
            const uint32_t diff=numberOfBotConnected-numberOfSelectedCharacter;
            if(diff<=(uint32_t)maxDiffConnectedSelected())
                createClientCore();
        }
    }
    else
    {
        std::cout << "connectTimerTick(): finish, stop it" << std::endl;
        notifyAllPlayerConnected();
        stopConnectTimer();
    }
}

void MultipleBotConnectionCore::newCharacterId_with_client(BotClient *client,const uint8_t &returnCode,const uint32_t &characterId)
{
    if(client->link->api()==NULL)
    {
        std::cerr << "newCharacterId_with_client client->link->api()==NULL" << std::endl;
        BOT_ABORT();
    }
    if(returnCode!=0x00)
    {
        std::cerr << "new character not created, server have returned a failed: " << (unsigned int)returnCode << std::endl;
        return;
    }
    if(serverUniqueKey==-1)
    {
        std::cerr << "Unable to select the freshly created char because don't have select the server: " << (unsigned int)returnCode << std::endl;
        return;
    }

    numberOfStartCreatedCharacter++;
    notifyNumberOfStartCreatedCharacter(numberOfStartCreatedCharacter);
    client->stat=Status_CreatedCharacter;
    notifyUpdateClientListStatus();
    if(characterOnMap.find(characterId)==characterOnMap.cend())
    {
        if(!client->link->api()->selectCharacter(charactersGroupIndex,serverUniqueKey,characterId))
            std::cerr << "Unable to select character after creation: " << characterId << std::endl;
        else
        {
            numberOfStartSelectingCharacter++;
            notifyNumberOfStartSelectingCharacter(numberOfStartSelectingCharacter);
            client->stat=Status_SelectingCharacterAfterCreation;
            notifyUpdateClientListStatus();
            characterOnMap.insert(characterId);
            std::cout << "Select new character created after creation: " << characterId << std::endl;
            ifMultipleConnexionStartCreation();
        }
    }
    else
        std::cout << client->login << " new character is already on map" << std::endl;
}

void MultipleBotConnectionCore::have_current_player_info_with_client(BotClient *client,const CatchChallenger::Player_private_and_public_informations &informations)
{
    if(client->selectedCharacter==true)
        return;
    std::cout << "selected character at " << __FILE__ << ":" << __LINE__ << std::endl;
    client->selectedCharacter=true;
    //This is literally "the client now has its informations". It was only set
    //in insert_player_with_client(), i.e. when the server pushed some OTHER
    //player's insert to this bot -- a bot that had not seen anyone yet stayed
    //false and BotTargetList's constructor then skipped it, so the bot list
    //showed 7 rows for 8 connected bots.
    client->have_informations=true;
    numberOfSelectedCharacter++;
    notifyNumberOfSelectedCharacter(numberOfSelectedCharacter);
    client->stat=Status_SelectedCharacter;
    notifyUpdateClientListStatus();

    //A FRESHLY CREATED character completes here (have_current_player_info),
    //NOT in haveCharacter() -- haveCharacter() is driven by the protocol's
    //haveCharacter, which the server sends only for characters that ALREADY
    //exist at login. So for a cold server where every bot creates its character
    //(e.g. the ESP32 all-in-one on a fresh boot), the on-map notification must
    //be evaluated here too, otherwise notifyAllPlayerOnMap() never fires and
    //the run times out. haveCharacter_with_client() keeps the SAME check for
    //the warm/existing-character path; the consumer is idempotent, so whichever
    //path completes the last selection wins and a second call is a no-op.
    if(multipleConnexion())
    {
        if(numberOfBotConnected>=numberOfSelectedCharacter)
        {
            const uint32_t diff=numberOfBotConnected-numberOfSelectedCharacter;
            if(diff==0 && (int)numberOfSelectedCharacter>=connexionCountTarget())
                notifyAllPlayerOnMap();
        }
    }
    else
        notifyAllPlayerOnMap();

    (void)informations;
}

void MultipleBotConnectionCore::newError_with_client(BotClient *client,const std::string &error,const std::string &detailedError)
{
    (void)error;
    (void)detailedError;
    mHaveAnError=true;
    client->link->disconnectFromHost();
}

void MultipleBotConnectionCore::newSocketError_with_client(BotClient *client,const int error)
{
    std::cerr << "newSocketError() " << error << std::endl;
    mHaveAnError=true;
    (void)client;
}

void MultipleBotConnectionCore::disconnected_with_client(BotClient *client)
{
    std::cout << "disconnected()" << std::endl;
    if(client==NULL)
    {
        mHaveAnError=true;
        std::cerr << "disconnected(): error, from unknown" << std::endl;
        return;
    }
    CatchChallenger::Api_protocol * const api=client->link->api();
    if(api==NULL)
    {
        mHaveAnError=true;
        std::cerr << "disconnected(): error, api null" << std::endl;
        return;
    }
    if(api->stage()==CatchChallenger::Api_protocol::StageConnexion::Stage2 ||
            api->stage()==CatchChallenger::Api_protocol::StageConnexion::Stage3)
    {
        std::cout << "disconnected(): For reason: api->socketDisconnectedForReconnect()" << std::endl;
        api->socketDisconnectedForReconnect();
        return;
    }
    mHaveAnError=true;
    if(client->haveBeenDiscounted==false)
    {
        client->haveBeenDiscounted=true;
        numberOfBotConnected--;
        notifyNumberOfBotConnected(numberOfBotConnected);
        std::cout << "disconnected(): numberOfBotConnected--: " << api->player_informations.public_informations.pseudo << std::endl;
    }
}

void MultipleBotConnectionCore::notLoggedInternal(const std::string &reason)
{
    (void)reason;
    mHaveAnError=true;

    unsigned int index=0;
    while(index<clientList.size())
    {
        BotClient * const client=clientList.at(index);
        if(client->link!=NULL)
        {
            if(client->link->api()!=NULL)
                client->link->api()->disconnectClient();
        }
        index++;
    }
}
