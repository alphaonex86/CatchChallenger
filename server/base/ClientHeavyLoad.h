#ifndef CATCHCHALLENGER_CLIENTHEAVYLOAD_H
#define CATCHCHALLENGER_CLIENTHEAVYLOAD_H

#include <QObject>
#include <QSqlDatabase>
#include <QStringList>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QCoreApplication>
#include <QFile>
#include <QCryptographicHash>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QFileInfo>
#include <QDateTime>
#include <QDir>
#include <QRegularExpression>

#include "ServerStructures.h"
#include "../VariableServer.h"
#include "../../general/base/DebugClass.h"

namespace CatchChallenger {
class ClientHeavyLoad : public QObject
{
    Q_OBJECT
public:
    explicit ClientHeavyLoad();
    ~ClientHeavyLoad();
    void setVariable(Player_internal_informations *player_informations);
    static QList<quint16> simplifiedIdList;
    static quint8 tempDatapackListReplySize;
    static QByteArray tempDatapackListReplyArray;
    static quint8 tempDatapackListReply;
    static int tempDatapackListReplyTestCount;
    static QByteArray rawFiles,compressedFiles;
    static int rawFilesCount,compressedFilesCount;
    static QSet<QString> compressedExtension;
    static QHash<quint32,quint16> clanConnectedCount;
    static QString single_quote;
    static QString antislash_single_quote;
    static QString text_dotslash;
    static QString text_slash;
    static QString text_double_slash;
    static QString text_antislash;
public slots:
    virtual void askLogin(const quint8 &query_id, const QByteArray &login_org, const QByteArray &pass_org);
    virtual void askLoginBot(const quint8 &query_id);
    virtual void deleteCharacterNow(const quint32 &characterId);
    //check each element of the datapack, determine if need be removed, updated, add as new file all the missing file
    void datapackList(const quint8 &query_id, const QStringList &files, const QList<quint64> &timestamps);
    void addDatapackListReply(const bool &fileRemove);
    void purgeDatapackListReply(const quint8 &query_id);
    void sendFileContent();
    void sendCompressedFileContent();
    void dbQuery(const QString &queryText);
    void askedRandomNumber();
    //character
    void addCharacter(const quint8 &query_id, const quint8 &profileIndex,const QString &pseudo,const QString &skin);
    void removeCharacter(const quint8 &query_id, const quint32 &characterId);
    void selectCharacter(const quint8 &query_id, const quint32 &characterId);
    //normal slots
    void askIfIsReadyToStop();
private:
    // ------------------------------
    bool sendFile(const QString &fileName, const quint64 &mtime);
    QString SQL_text_quote(QString text);
    // ------------------------------
    Player_internal_informations *player_informations;
    bool loadTheRawUTF8String();
    void loginIsRight(const quint8 &query_id, quint32 characterId, Map* map, const /*COORD_TYPE*/ quint8 &x, const /*COORD_TYPE*/ quint8 &y, const Orientation &orientation);
    void loginIsRightWithParsedRescue(const quint8 &query_id, quint32 characterId, Map* map, const /*COORD_TYPE*/ quint8 &x, const /*COORD_TYPE*/ quint8 &y, const Orientation &orientation,
                      Map* rescue_map, const /*COORD_TYPE*/ quint8 &rescue_x, const /*COORD_TYPE*/ quint8 &rescue_y, const Orientation &rescue_orientation,
                      Map* unvalidated_rescue_map, const /*COORD_TYPE*/ quint8 &unvalidated_rescue_x, const /*COORD_TYPE*/ quint8 &unvalidated_rescue_y, const Orientation &unvalidated_rescue_orientation
                      );
    void loginIsRightWithRescue(const quint8 &query_id,quint32 characterId,Map* map,const /*COORD_TYPE*/ quint8 &x,const /*COORD_TYPE*/ quint8 &y,const Orientation &orientation,
                      const QVariant &rescue_map,const QVariant &rescue_x,const QVariant &rescue_y,const QVariant &rescue_orientation,
                      const QVariant &unvalidated_rescue_map,const QVariant &unvalidated_rescue_x,const QVariant &unvalidated_rescue_y,const QVariant &unvalidated_rescue_orientation
                      );
    void loginIsWrong(const quint8 &query_id,const QString &messageToSend,const QString &debugMessage);
    void characterSelectionIsWrong(const quint8 &query_id,const QString &messageToSend,const QString &debugMessage);
    //load linked data (like item, quests, ...)
    void loadLinkedData();
    void loadItems();
    void loadRecipes();
    void loadMonsters();
    void loadReputation();
    void loadQuests();
    void loadBotAlreadyBeaten();
    void sendInventory();
    QList<PlayerBuff> loadMonsterBuffs(const quint32 &monsterId);
    QList<PlayerMonster::PlayerSkill> loadMonsterSkills(const quint32 &monsterId);
    static QRegularExpression fileNameStartStringRegex;
signals:
    //normal signals
    void error(const QString &error) const;
    void message(const QString &message) const;
    void isReadyToStop() const;
    //send packet on network
    void sendFullPacket(const quint8 &mainIdent,const quint16 &subIdent,const QByteArray &data=QByteArray()) const;
    void sendPacket(const quint8 &mainIdent,const QByteArray &data=QByteArray()) const;
    //send reply
    void postReply(const quint8 &queryNumber,const QByteArray &data) const;
    //login linked signals
    void send_player_informations() const;
    void isLogged() const;
    void put_on_the_map(Map* map,const /*COORD_TYPE*/ quint8 &x,const /*COORD_TYPE*/ quint8 &y,const Orientation &orientation) const;
    //random linked signals
    void newRandomNumber(const QByteArray &randomData) const;
    void haveClanInfo(const quint32 &clanId,const QString &clanName,const quint64 &cash);
};
}

#endif // CLIENTHEAVYLOAD_H
