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
private:
    void showEvent(QShowEvent * event);
    Ui::SocialChat *ui;
    QHash<QString,ActionsBotInterface::Player *> pseudoToBot;
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
        CatchChallenger::Chat_type type;
        std::string text;
    };
    QList<ChatEntry> chat_list;
};

#endif // SOCIALCHAT_H
