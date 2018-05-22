#ifndef GLOBALCONTROLER_H
#define GLOBALCONTROLER_H

#include <QObject>
#include <QObject>
#include <QTimer>
#include <QList>
#include <QSemaphore>
#include <QByteArray>
#include <QSettings>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../bot/MultipleBotConnectionImplForGui.h"
#include "../../client/base/ClientStructures.h"

class GlobalControler : public QObject
{
    Q_OBJECT
public:
    explicit GlobalControler(QObject *parent = 0);
    ~GlobalControler();
private:
    QSettings settings;
    QTimer slowDownTimer;
    QTimer autoConnect;
    QTimer timeoutTimer;

    MultipleBotConnectionImplForGui multipleBotConnexion;
    QHash<uint8_t/*character group index*/,QPair<uint8_t/*server count*/,uint8_t/*temp Index to display*/> > serverByCharacterGroup;
    std::vector<CatchChallenger::ServerFromPoolForDisplay *> serverOrdenedList;
    std::vector<std::vector<CatchChallenger::CharacterEntry> > characterEntryList;

    QString login,pass,host,proxy,character;
    uint32_t serverUniqueKey,charactersGroupIndex,port,proxyport,character_id,timeout,char_count;
public slots:
    void detectSlowDown(uint32_t, uint32_t worseTime);
private slots:
    void lastReplyTime(const quint32 &time);
    void on_connect_clicked();
    void timeoutSlot();
    void logged(CatchChallenger::Api_client_real *senderObject,
                const std::vector<CatchChallenger::ServerFromPoolForDisplay *> &serverOrdenedList,
                const std::vector<std::vector<CatchChallenger::CharacterEntry> > &characterEntryList,
                bool haveTheDatapack);
    void updateServerList(CatchChallenger::Api_client_real *);
    void statusError(QString error);
    void datapackIsReady();
    void datapackMainSubIsReady();
    void all_player_on_map();
signals:
    void isDisconnected();
};

#endif // GLOBALCONTROLER_H
