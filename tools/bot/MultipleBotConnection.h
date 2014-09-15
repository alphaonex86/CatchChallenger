#ifndef MULTIPLEBOTCONNECTION_H
#define MULTIPLEBOTCONNECTION_H

#include "../../general/base/QFakeSocket.h"
#include "../../general/base/ConnectedSocket.h"
#include "../../general/base/CommonDatapack.h"
#include "../../client/base/Api_client_real.h"

#include <QTimer>
#include <QObject>

class MultipleBotConnection
{
public:
    explicit MultipleBotConnection();
    ~MultipleBotConnection();
protected:
    struct CatchChallengerClient
    {
        QSslSocket *sslSocket;
        CatchChallenger::ConnectedSocket *socket;
        CatchChallenger::Api_client_real *api;
        bool have_informations;
        bool haveShowDisconnectionReason;
        CatchChallenger::Direction direction;
        QList<CatchChallenger::CharacterEntry> charactersList;
        quint16 number;
        QString login;
        QString pass;
        bool selectedCharacter;
        bool haveFirstHeader;
    };
    QHash<CatchChallenger::Api_client_real *,CatchChallengerClient *> apiToCatchChallengerClient;
    QHash<CatchChallenger::ConnectedSocket *,CatchChallengerClient *> connectedSocketToCatchChallengerClient;
    QHash<QSslSocket *,CatchChallengerClient *> sslSocketToCatchChallengerClient;

    QTimer connectTimer;

    QFileInfoList skinsList;
    quint16 number;
    QSet<quint32> characterOnMap;
    quint16 numberOfBotConnected;
    quint16 numberOfSelectedCharacter;
    bool haveEnError;
protected slots:
    void createClient();
    virtual void insert_player(CatchChallengerClient *client,const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction);
    virtual void insert_player(const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction) = 0;
    virtual void haveCharacter();
    virtual void logged(CatchChallengerClient *client,const QList<CatchChallenger::CharacterEntry> &characterEntryList);
    virtual void logged(const QList<CatchChallenger::CharacterEntry> &characterEntryList) = 0;
    void have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations);
    void newError(QString error,QString detailedError);
    void newSocketError(QAbstractSocket::SocketError error);
    virtual void disconnected();
    virtual void lastReplyTime(const quint32 &time);
    void tryLink(CatchChallengerClient *client);
    void protocol_is_good(CatchChallengerClient *client);
    void sslErrors(const QList<QSslError> &errors);
    void on_characterSelect_clicked();
    virtual void haveTheDatapack(CatchChallengerClient *client);
    virtual void haveTheDatapack() = 0;
    void ifMultipleConnexionStartCreation();
    void connectTimerSlot();
    void newCharacterId(const quint8 &returnCode, const quint32 &characterId);
    void sslHandcheckIsFinished();
    void readForFirstHeader();
    virtual void connectTheExternalSocket(CatchChallengerClient *client);

    virtual QString login() = 0;
    virtual QString pass() = 0;
    virtual bool multipleConnexion() = 0;
    virtual bool autoCreateCharacter() = 0;
};

#endif // MULTIPLEBOTCONNECTION_H
