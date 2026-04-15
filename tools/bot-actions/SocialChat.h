#ifndef SOCIALCHAT_H
#define SOCIALCHAT_H

#include <QMainWindow>
#include <QHash>
#include <QString>
#include <QPixmap>
#include "../libbot/actions/ActionsAction.h"
#include <vector>
#include <unordered_set>
#include <string>
#include <QCompleter>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

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
    void new_chat_text(const CatchChallenger::Chat_type &chat_type,const std::string &text,const std::string &pseudo,const CatchChallenger::Player_type &player_type);
    void new_chat_text_internal(const CatchChallenger::Chat_type &chat_type, const QString &text, const QString &pseudo, const QString &theotherpseudo, const CatchChallenger::Player_type &player_type);
    void new_system_text(const CatchChallenger::Chat_type &chat_type, const std::string &text);
    void update_chat();
    void removeNumberForFlood();
    void on_globalChatText_returnPressed();
    //map view
    void insert_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &simplifiedIndex,const CatchChallenger::Player_public_informations &player,const SIMPLIFIED_PLAYER_ID_FOR_MAP &mapId,const uint8_t &x,const uint8_t &y,const CatchChallenger::Direction &direction);
    void remove_player(const SIMPLIFIED_PLAYER_ID_FOR_MAP &id);
    void dropAllPlayerOnTheMap();
    void updatePlayerKnownList(CatchChallenger::Api_protocol_Qt *api);
    void updateVisiblePlayers(CatchChallenger::Api_protocol_Qt *api);
    void on_globalChat_anchorClicked(const QUrl &arg1);
    void globalChatText_updateCompleter();
    void on_chatSpecText_returnPressed();
    void on_listWidgetChatType_itemSelectionChanged();
    //ollama
    void on_checkBoxOllama_toggled(bool checked);
    void on_lineEditOllamaUrl_textChanged(const QString &text);
    void on_lineEditOllamaModel_textChanged(const QString &text);
    void ollamaReplyFinished(QNetworkReply *reply);
    void requestOllamaResponse(const QString &botPseudo, const QString &senderPseudo, const QString &message, const CatchChallenger::Chat_type &chatType);
    void sendOllamaReply(const QString &replyText, const QString &botPseudo, const CatchChallenger::Chat_type &chatType, const QString &targetPseudo);

private:
    void showEvent(QShowEvent * event);
    void focusInEvent(QFocusEvent * event);
    Ui::SocialChat *ui;
    QHash<QString,CatchChallenger::Api_protocol_Qt *> pseudoToBot;

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

    //ollama
    QNetworkAccessManager *ollamaNetworkManager;
    struct OllamaPendingReply
    {
        QString botPseudo;
        CatchChallenger::Chat_type chatType;
        QString targetPseudo;
        qint64 requestTimestamp;
    };
    QHash<QNetworkReply*,OllamaPendingReply> ollamaPendingReplies;
};

#endif // SOCIALCHAT_H
