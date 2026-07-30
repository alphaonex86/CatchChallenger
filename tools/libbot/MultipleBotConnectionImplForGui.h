#ifndef MULTIPLEBOTCONNECTIONIMPLFOPRGUI_H
#define MULTIPLEBOTCONNECTIONIMPLFOPRGUI_H

#include "MultipleBotConnection.h"

class MultipleBotConnectionImplForGui : public MultipleBotConnection
{
    Q_OBJECT
public:
    explicit MultipleBotConnectionImplForGui();
    ~MultipleBotConnectionImplForGui();
    void characterSelectForFirstCharacter(const quint32 &charId);
    void serverSelect(const uint8_t &charactersGroupIndex,const quint32 &serverUniqueKey);
public slots:
    void detectSlowDown();
public:
    //Kept as QString: they are filled straight from the GUI widgets / QSettings
    //of the front-ends. The Qt-free core only sees the std::string accessors
    //below.
    QString mLogin;
    QString mPass;
    bool mMultipleConnexion;
    bool mAutoCreateCharacter;
    int mConnectBySeconds;
    int mConnexionCountTarget;
    int mMaxDiffConnectedSelected;
    QString mProxy;
    quint16 mProxyport;
    QString mHost;
    quint16 mPort;
    bool firstCharacterSelected;
    static bool displayingError;
private:
    std::string login();
    std::string pass();
    bool multipleConnexion();
    bool autoCreateCharacter();
    int connectBySeconds();
    int connexionCountTarget();
    int maxDiffConnectedSelected();
    std::string proxy();
    uint16_t proxyport();
    std::string host();
    uint16_t port();
private:
    virtual void insert_player(const uint8_t &simplifiedIndex,const CatchChallenger::Player_public_informations &player,const uint8_t &mapId,const uint8_t &x,const uint8_t &y,
                               const CatchChallenger::Direction &direction);
    virtual void remove_player(const uint8_t &id);
    virtual void dropAllPlayerOnTheMap();
    virtual void notLogged(const std::string &reason);
    virtual void logged(const std::vector<std::vector<CatchChallenger::CharacterEntry> > &characterEntryList);
    virtual void newCharacterId(const quint8 &returnCode, const quint32 &characterId);
    virtual void haveTheDatapack();
    virtual void haveTheDatapackMainSub();
    virtual void haveTheDatapackMainSubCode();
    virtual void sslErrors(const QList<QSslError> &errors);
    virtual void protocol_is_good();
    virtual void newSocketError(QAbstractSocket::SocketError error);
    virtual void newError(const std::string &error,const std::string &detailedError);
    virtual void have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations);
    virtual void disconnected();
    virtual void connectTimerSlot();
    virtual void haveCharacter(const CATCHCHALLENGER_TYPE_MAPID &mapId,const COORD_TYPE &x,const COORD_TYPE &y,const CatchChallenger::Direction &direction);
    virtual void connectTheExternalSocket(MultipleBotConnectionCore::BotClient *client);
signals:
    void loggedDone(CatchChallenger::Api_client_real *senderObject,
                    const std::vector<CatchChallenger::ServerFromPoolForDisplay> &serverOrdenedList,
                    const std::vector<std::vector<CatchChallenger::CharacterEntry> > &characterEntryList,
                    bool haveTheDatapack);
    void statusError(QString error);
    //emitted when the master refuses the login/character-select (notLogged);
    //carries the human reason so a controller can distinguish a retryable
    //reconnect-cooldown ("Too recently disconnected"/"Already logged") from a
    //hard failure instead of waiting out the global timeout.
    void notLoggedReason(QString reason);
    void chat_text(const CatchChallenger::Chat_type &chat_type,const std::string &text,const std::string &pseudo,const CatchChallenger::Player_type &type) const;
    void emit_detectSlowDown(uint32_t queryCount,uint32_t worseTime);
    void datapackIsReady();
    void datapackMainSubIsReady();
};

#endif // MULTIPLEBOTCONNECTIONIMPLFOPRGUI_H
