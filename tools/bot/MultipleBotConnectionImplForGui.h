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
    QString login();
    QString pass();
    bool multipleConnexion();
    bool autoCreateCharacter();
    int connectBySeconds();
    int connexionCountTarget();
    int maxDiffConnectedSelected();
    QString proxy();
    quint16 proxyport();
    QString host();
    quint16 port();
private:
    virtual void insert_player(const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction);
    virtual void remove_player(const uint16_t &id);
    virtual void dropAllPlayerOnTheMap();
    virtual void logged(const QList<CatchChallenger::ServerFromPoolForDisplay *> &serverOrdenedList,const QList<QList<CatchChallenger::CharacterEntry> > &characterEntryList);
    virtual void newCharacterId(const quint8 &returnCode, const quint32 &characterId);
    virtual void haveTheDatapack();
    virtual void haveTheDatapackMainSub();
    virtual void haveTheDatapackMainSubCode();
    virtual void sslErrors(const QList<QSslError> &errors);
    virtual void protocol_is_good();
    virtual void newSocketError(QAbstractSocket::SocketError error);
    virtual void newError(QString error,QString detailedError);
    virtual void have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations);
    virtual void disconnected();
    virtual void connectTimerSlot();
    virtual void haveCharacter();
    virtual void connectTheExternalSocket(CatchChallengerClient * client);
signals:
    void loggedDone(CatchChallenger::Api_client_real *senderObject,const QList<CatchChallenger::ServerFromPoolForDisplay *> &serverOrdenedList,const QList<QList<CatchChallenger::CharacterEntry> > &characterEntryList,bool haveTheDatapack);
    void statusError(QString error);
    void chat_text(const CatchChallenger::Chat_type &chat_type,const QString &text,const QString &pseudo,const CatchChallenger::Player_type &type) const;
    void emit_detectSlowDown(uint32_t queryCount,uint32_t worseTime);
    void datapackIsReady();
    void datapackMainSubIsReady();
};

#endif // MULTIPLEBOTCONNECTIONIMPLFOPRGUI_H
