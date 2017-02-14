#ifndef SOCIALCHAT_H
#define SOCIALCHAT_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QHash>
#include <QString>
#include <QPixmap>
#include "../bot/actions/ActionsAction.h"
#include <vector>
#include <unordered_set>
#include <string>
#include <QCompleter>

namespace Ui {
class SocialChat;
}

class SocialChat : public QMainWindow
{
    Q_OBJECT

public:
    explicit SocialChat();
    ~SocialChat();
    static SocialChat *socialChat;
private slots:
    void on_bots_itemSelectionChanged();
    void loadPlayerInformation();
    QPixmap getFrontSkin(const QString &skinName);
    QPixmap getFrontSkin(const uint32_t &skinId);
    QString getFrontSkinPath(const QString &skin);
    QPixmap getBackSkin(const QString &skinName);
    QPixmap getBackSkin(const uint32_t &skinId);
    QString getBackSkinPath(const QString &skin);
    QPixmap getTrainerSkin(const QString &skinName);
    QPixmap getTrainerSkin(const uint32_t &skinId);
    QString getTrainerSkinPath(const QString &skin);
    QString getSkinPath(const QString &skinName,const QString &type);
    void on_note_textChanged();
    //chat
    void new_chat_text(const CatchChallenger::Chat_type &chat_type,const QString &text,const QString &pseudo,const CatchChallenger::Player_type &player_type);
    void new_chat_text_internal(const CatchChallenger::Chat_type &chat_type,const QString &text,const QString &pseudo,const CatchChallenger::Player_type &player_type);
    void new_system_text(const CatchChallenger::Chat_type &chat_type,const QString &text);
    void update_chat();
    void removeNumberForFlood();
    void on_globalChatText_returnPressed();
    //map view
    void insert_player(const CatchChallenger::Player_public_informations &player,const uint32_t &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction);
    void updatePlayerKnownList(CatchChallenger::Api_protocol *api);
    void on_globalChat_anchorClicked(const QUrl &arg1);

private:
    void showEvent(QShowEvent * event);
    void focusInEvent(QFocusEvent * event);
    Ui::SocialChat *ui;
    QHash<QString,CatchChallenger::Api_protocol *> pseudoToBot;
    QSqlDatabase database;

    //skin
    QHash<QString,QPixmap> frontSkinCacheString;
    QHash<QString,QPixmap> backSkinCacheString;
    QHash<QString,QPixmap> trainerSkinCacheString;

    //chat
    QString lastMessageSend;
    QTimer stopFlood;
    int numberForFlood;
    std::vector<std::unordered_set<std::string> > lastText;
    struct ChatEntry
    {
        std::string player_pseudo;
        CatchChallenger::Player_type player_type;
        CatchChallenger::Chat_type chat_type;
        std::string text;
    };
    QList<ChatEntry> chat_list;
    QSet<QString> knownGlobalChatPlayers;
    QCompleter *completer;
};

#endif // SOCIALCHAT_H
