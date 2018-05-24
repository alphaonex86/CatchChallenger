#ifndef MULTIPLEBOTCONNECTION_H
#define MULTIPLEBOTCONNECTION_H

#include "../../general/base/QFakeSocket.h"
#include "../../general/base/ConnectedSocket.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonDatapackServerSpec.h"
#include "../../client/base/Api_client_real.h"
#include "../../client/base/ClientStructures.h"
#include "BotInterface.h"

#include <QTimer>
#include <QObject>

class MultipleBotConnection : public QObject
{
    Q_OBJECT
public:
    explicit MultipleBotConnection();
    ~MultipleBotConnection();

    BotInterface *botInterface;
    struct CatchChallengerClient
    {
        QSslSocket *sslSocket;
        CatchChallenger::ConnectedSocket *socket;
        CatchChallenger::Api_client_real *api;
        bool have_informations;
        bool haveShowDisconnectionReason;
        bool haveBeenDiscounted;
        //CatchChallenger::Direction direction;
        std::vector<std::vector<CatchChallenger::CharacterEntry> > charactersList;
        quint16 number;
        QString login;
        QString pass;
        bool selectedCharacter;
        //bool haveFirstHeader;->put into Api_protocol

        struct Preferences
        {
            unsigned int plant;
            unsigned int item;
            unsigned int fight;
            unsigned int shop;
            unsigned int wild;
        };
        Preferences preferences;
    };
    QHash<CatchChallenger::Api_client_real *,CatchChallengerClient *> apiToCatchChallengerClient;
    QHash<CatchChallenger::ConnectedSocket *,CatchChallengerClient *> connectedSocketToCatchChallengerClient;
    QHash<QSslSocket *,CatchChallengerClient *> sslSocketToCatchChallengerClient;

    virtual CatchChallengerClient *createClient();
    bool haveAnError();
protected:
    BotInterface *pluginLoaderInstance;

    QTimer connectTimer;

    QFileInfoList skinsList;
    quint16 numberToChangeLoginForMultipleConnexion;
    QSet<quint32> characterOnMap;//protect mutual call: characterSelectForFirstCharacter(), logged_with_client(), haveTheDatapack_with_client()
    quint16 numberOfBotConnected;
    quint16 numberOfSelectedCharacter;
    bool mHaveAnError;
    uint8_t charactersGroupIndex;
    int64_t/*to have -1*/ serverUniqueKey;
    bool serverIsSelected;

    QStringList tempMapList;
protected:
    virtual void insert_player_with_client(CatchChallengerClient *client,const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction);
    virtual void haveCharacter();
    virtual void logged_with_client(CatchChallengerClient *client);
    virtual std::string getNewPseudo();
    void have_current_player_info_with_client(CatchChallengerClient *client, const CatchChallenger::Player_private_and_public_informations &informations);
    void newError_with_client(CatchChallengerClient *client, QString error,QString detailedError);
    void newSocketError_with_client(CatchChallengerClient *client, QAbstractSocket::SocketError error);
    virtual void disconnected();
    virtual void lastReplyTime(const quint32 &time);
    virtual void notLogged(const std::string &reason);
    virtual void tryLink(CatchChallengerClient *client);
    virtual void protocol_is_good_with_client(CatchChallengerClient *client);
    virtual void haveTheDatapack_with_client(CatchChallengerClient *client);
    virtual void haveTheDatapackMainSub_with_client(CatchChallengerClient *client);
    virtual void haveDatapackMainSubCode_with_client(CatchChallengerClient *client);
    virtual void ifMultipleConnexionStartCreation();
    virtual void connectTimerSlot();
    void newCharacterId_with_client(CatchChallengerClient *client,const quint8 &returnCode, const quint32 &characterId);
    virtual void connectTheExternalSocket(CatchChallengerClient *client);

    virtual void insert_player(const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction) = 0;
    virtual void remove_player(const uint16_t &id) = 0;
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

    virtual QString login() = 0;
    virtual QString pass() = 0;
    virtual bool multipleConnexion() = 0;
    virtual bool autoCreateCharacter() = 0;
    virtual int connectBySeconds() = 0;
    virtual int connexionCountTarget() = 0;
    virtual int maxDiffConnectedSelected() = 0;
    virtual QString proxy() = 0;
    virtual quint16 proxyport() = 0;
    virtual QString host() = 0;
    virtual quint16 port() = 0;

    QString getResolvedPluginName(const QString &name);
signals:
    void emit_numberOfBotConnected(quint16 numberOfBotConnected);
    void emit_numberOfSelectedCharacter(quint16 numberOfSelectedCharacter);
    void emit_lastReplyTime(const quint32 &time);
    void emit_all_player_connected();
    void emit_all_player_on_map();
};

#endif // MULTIPLEBOTCONNECTION_H
