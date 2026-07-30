/** \file MultipleBotConnectionCore.h
\brief Qt-free bot connection state machine
\licence GPL3, see the file COPYING */

#ifndef MULTIPLEBOTCONNECTIONCORE_H
#define MULTIPLEBOTCONNECTIONCORE_H

#include <string>
#include <vector>
#include <unordered_set>
#include <stdint.h>

#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../client/libcatchchallenger/ClientStructures.hpp"
#include "BotClientLink.h"
#include "BotInterface.h"

/// \brief Everything the bot does between "open a socket" and "the character is
/// on the map", with no toolkit dependency at all.
///
/// A front-end derives from it and supplies:
///  * the transport (allocClient() returns a client whose link is ready),
///  * the retry timer (startConnectTimer()/stopConnectTimer()/connectTimerIsActive()),
///  * the configuration (login()/pass()/host()/port()/...),
///  * the datapack folder (datapackPath()),
///  * optionally the progress notifications (notify*(), all no-op by default).
///
/// The front-end also owns the "who sent this?" resolution: the protocol
/// notifications enter through the *_with_client() methods, which take the
/// client explicitly instead of relying on a signal sender.
class MultipleBotConnectionCore
{
public:
    MultipleBotConnectionCore();
    virtual ~MultipleBotConnectionCore();

    BotInterface *botInterface;

    enum Status
    {
        Status_None,
        Status_Connecting,
        Status_Connected,
        Status_WaitProtocol,
        Status_WaitLogin,
        Status_Logged,
        Status_WaitDataPack,
        Status_HaveDatapack,
        Status_CreatingCharacter,
        Status_CreatedCharacter,
        Status_SelectingCharacter,
        Status_SelectingCharacterAfterCreation,
        Status_SelectedCharacter,
        Status_OnMap
    };

    /// \brief One bot, as the core sees it. A front-end may derive from it to
    /// staple its own (toolkit specific) handles onto the same object.
    struct BotClient
    {
        BotClient();
        virtual ~BotClient();

        BotClientLink *link;
        bool have_informations;
        bool haveShowDisconnectionReason;
        bool haveBeenDiscounted;
        std::vector<std::vector<CatchChallenger::CharacterEntry> > charactersList;
        uint16_t number;
        std::string login;
        std::string pass;
        bool selectedCharacter;

        struct Preferences
        {
            unsigned int plant;
            unsigned int item;
            unsigned int fight;
            unsigned int shop;
            unsigned int wild;
        };
        Preferences preferences;
        Status stat;
    };

    bool haveAnError();
protected:
    /// Every client ever created, in creation order. Owns them.
    std::vector<BotClient *> clientList;

    uint16_t numberToChangeLoginForMultipleConnexion;
    //protect mutual call: characterSelectForFirstCharacter(), logged_with_client(),
    //haveTheDatapack_with_client()
    std::unordered_set<uint32_t> characterOnMap;
    uint16_t numberOfBotConnected;
    uint16_t numberOfSelectedCharacter;
    uint16_t numberOfStartSelectingCharacter;
    uint16_t numberOfHaveDatapackCharacter;
    uint16_t numberOfStartCreatingCharacter;
    uint16_t numberOfStartCreatedCharacter;
    bool mHaveAnError;
    uint8_t charactersGroupIndex;
    int64_t/*to have -1*/ serverUniqueKey;
    bool serverIsSelected;

    std::vector<std::string> tempMapList;

protected:
    /* --- supplied by the front-end ------------------------------------- */
    /// \brief Allocate a client with a ready-to-connect link. NULL on failure.
    virtual BotClient *allocClient() = 0;
    virtual void startConnectTimer(const unsigned int intervalMs) = 0;
    virtual void stopConnectTimer() = 0;
    virtual bool connectTimerIsActive() const = 0;
    /// \brief Folder holding the downloaded datapack, with the trailing slash.
    virtual std::string datapackPath() const = 0;

    virtual std::string login() = 0;
    virtual std::string pass() = 0;
    virtual bool multipleConnexion() = 0;
    virtual bool autoCreateCharacter() = 0;
    virtual int connectBySeconds() = 0;
    virtual int connexionCountTarget() = 0;
    virtual int maxDiffConnectedSelected() = 0;
    virtual std::string proxy() = 0;
    virtual uint16_t proxyport() = 0;
    virtual std::string host() = 0;
    virtual uint16_t port() = 0;

    /* --- progress notifications, no-op by default ---------------------- */
    virtual void notifyNumberOfBotConnected(const uint16_t value);
    virtual void notifyNumberOfSelectedCharacter(const uint16_t value);
    virtual void notifyNumberOfStartSelectingCharacter(const uint16_t value);
    virtual void notifyNumberOfHaveDatapackCharacter(const uint16_t value);
    virtual void notifyNumberOfStartCreatingCharacter(const uint16_t value);
    virtual void notifyNumberOfStartCreatedCharacter(const uint16_t value);
    virtual void notifyUpdateClientListStatus();
    virtual void notifyAllPlayerConnected();
    virtual void notifyAllPlayerOnMap();

    /* --- the state machine --------------------------------------------- */
    BotClient *createClientCore();
    /// \brief Register the freshly built client and start its login.
    /// A front-end overrides it to wire its own notifications first, then
    /// calls this implementation.
    virtual void connectTheExternalSocket(BotClient *client);
    virtual void tryLink(BotClient *client);
    virtual std::string getNewPseudo();

    virtual void insert_player_with_client(BotClient *client,const CatchChallenger::Player_public_informations &player,const uint8_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction);
    //Carries the bot's OWN spawn position: the protocol reports
    //haveCharacter(mapId,x,y,direction) at the end of the character load, and
    //that is the only place the bot ever learns where it is (the server never
    //sends a client an insert_player for itself).
    virtual void haveCharacter_with_client(BotClient *client,const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE &y,const CatchChallenger::Direction &direction);
    virtual void logged_with_client(BotClient *client);
    void have_current_player_info_with_client(BotClient *client,const CatchChallenger::Player_private_and_public_informations &informations);
    void newError_with_client(BotClient *client,const std::string &error,const std::string &detailedError);
    void newSocketError_with_client(BotClient *client,const int error);
    void disconnected_with_client(BotClient *client);
    void notLoggedInternal(const std::string &reason);
    virtual void protocol_is_good_with_client(BotClient *client);
    virtual void haveTheDatapack_with_client(BotClient *client);
    virtual void haveTheDatapackMainSub_with_client(BotClient *client);
    virtual void haveDatapackMainSubCode_with_client(BotClient *client);
    virtual void ifMultipleConnexionStartCreation();
    void connectTimerTick();
    void newCharacterId_with_client(BotClient *client,const uint8_t &returnCode,const uint32_t &characterId);
private:
    /// Shared body of the two "this account has no character" branches.
    void createCharacterFor(BotClient *client);
};

#endif // MULTIPLEBOTCONNECTIONCORE_H
