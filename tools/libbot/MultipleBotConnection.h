#ifndef MULTIPLEBOTCONNECTION_H
#define MULTIPLEBOTCONNECTION_H

#include "../../client/libqtcatchchallenger/ConnectedSocket.hpp"
#include "../../client/libqtcatchchallenger/Api_client_real.hpp"
#include "../../client/libcatchchallenger/ClientStructures.hpp"
#include "MultipleBotConnectionCore.h"

#include <QTimer>
#include <QObject>
#include <QHash>
#include <QSslSocket>
#include <QSslError>

/// \brief Qt transport for ONE bot: QSslSocket -> ConnectedSocket -> Api_client_real.
///
/// The whole point of the class is that the bot core never sees any of those
/// three types. It is not a QObject: it has no signal of its own, the signals of
/// the objects it owns are wired by MultipleBotConnection.
class QtBotClientLink : public BotClientLink
{
public:
    QtBotClientLink();
    ~QtBotClientLink();
    CatchChallenger::Api_protocol *api() const;
    bool connectToHost(const std::string &host,const uint16_t port,
                       const std::string &proxyHost,const uint16_t proxyPort);
    void disconnectFromHost();
    void sendDatapackContentMainSub();

    QSslSocket *sslSocket;
    CatchChallenger::ConnectedSocket *socket;
    CatchChallenger::Api_client_real *apiClient;
};

/// \brief Qt front-end of MultipleBotConnectionCore.
///
/// It owns everything Qt the bot needs: the retry QTimer, the QSslSocket based
/// links, and the sender()->client resolution (the three QHash indexes below).
/// The protocol notifications arrive as Qt slots (the pure virtuals at the
/// bottom, implemented by MultipleBotConnectionImplForGui), get resolved to a
/// client and are handed to the toolkit-free core.
class MultipleBotConnection : public QObject, public MultipleBotConnectionCore
{
    Q_OBJECT
public:
    explicit MultipleBotConnection();
    ~MultipleBotConnection();

    /// \brief A core BotClient plus the Qt handle its consumers need.
    /// api mirrors QtBotClientLink::apiClient; it is kept here because the GUI
    /// needs the concrete Qt type to connect to the api signals.
    struct CatchChallengerClient : public MultipleBotConnectionCore::BotClient
    {
        CatchChallengerClient();
        CatchChallenger::Api_client_real *api;
    };

    QHash<CatchChallenger::Api_client_real *,CatchChallengerClient *> apiToCatchChallengerClient;
    QHash<CatchChallenger::ConnectedSocket *,CatchChallengerClient *> connectedSocketToCatchChallengerClient;
    QHash<QSslSocket *,CatchChallengerClient *> sslSocketToCatchChallengerClient;

    virtual CatchChallengerClient *createClient();
protected:
    QTimer connectTimer;

    /// The link of a client this class created, without RTTI: allocClient() is
    /// the only place a link is made, and it always makes a QtBotClientLink.
    static QtBotClientLink *qtLink(MultipleBotConnectionCore::BotClient *client);

    /* --- MultipleBotConnectionCore front-end hooks ---------------------- */
    virtual MultipleBotConnectionCore::BotClient *allocClient();
    virtual void startConnectTimer(const unsigned int intervalMs);
    virtual void stopConnectTimer();
    virtual bool connectTimerIsActive() const;
    virtual std::string datapackPath() const;
    virtual void connectTheExternalSocket(MultipleBotConnectionCore::BotClient *client);

    virtual void notifyNumberOfBotConnected(const uint16_t value);
    virtual void notifyNumberOfSelectedCharacter(const uint16_t value);
    virtual void notifyNumberOfStartSelectingCharacter(const uint16_t value);
    virtual void notifyNumberOfHaveDatapackCharacter(const uint16_t value);
    virtual void notifyNumberOfStartCreatingCharacter(const uint16_t value);
    virtual void notifyNumberOfStartCreatedCharacter(const uint16_t value);
    virtual void notifyUpdateClientListStatus();
    virtual void notifyAllPlayerConnected();
    virtual void notifyAllPlayerOnMap();

    /* --- Qt slots: resolve the sender, then call the core --------------- */
    virtual void disconnected();
    virtual void lastReplyTime(const quint32 &time);
    virtual void notLogged(const std::string &reason);
    virtual void connectTimerSlot();
    //Declared with the position parameters on purpose: Api_protocol_loadchar
    //emits QthaveCharacter(mapId,x,y,direction) at the end of the character
    //load and that is the only place the bot ever learns where it is. Declaring
    //the slot without those parameters made Qt drop them silently.
    virtual void haveCharacter(const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE &y,const CatchChallenger::Direction &direction);

    /* --- implemented by the GUI/CLI subclass --------------------------- */
    virtual void insert_player(const uint8_t &simplifiedIndex,const CatchChallenger::Player_public_informations &player,const uint8_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction) = 0;
    virtual void remove_player(const uint8_t &id) = 0;
    virtual void dropAllPlayerOnTheMap() = 0;
    virtual void logged(const std::vector<std::vector<CatchChallenger::CharacterEntry> > &characterEntryList) = 0;
    virtual void newCharacterId(const quint8 &returnCode, const quint32 &characterId) = 0;
    virtual void haveTheDatapack() = 0;
    virtual void haveTheDatapackMainSub() = 0;
    virtual void haveTheDatapackMainSubCode() = 0;
    virtual void sslErrors(const QList<QSslError> &errors) = 0;
    virtual void protocol_is_good() = 0;
    virtual void newSocketError(QAbstractSocket::SocketError error) = 0;
    virtual void newError(const std::string &error,const std::string &detailedError) = 0;
    virtual void have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations) = 0;
signals:
    void emit_numberOfBotConnected(quint16 numberOfBotConnected);
    void emit_numberOfSelectedCharacter(quint16 numberOfSelectedCharacter);
    void emit_numberOfStartSelectingCharacter(quint16 numberOfStartSelectingCharacter);
    void emit_numberOfHaveDatapackCharacter(quint16 numberOfHaveDatapackCharacter);
    void emit_numberOfStartCreatingCharacter(quint16 numberOfStartCreatingCharacter);
    void emit_numberOfStartCreatedCharacter(quint16 numberOfStartCreatedCharacter);
    void updateClientListStatus();
    void emit_lastReplyTime(const quint32 &time);
    void emit_all_player_connected();
    void emit_all_player_on_map();
};

#endif // MULTIPLEBOTCONNECTION_H
