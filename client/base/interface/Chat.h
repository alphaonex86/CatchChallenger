#ifndef CHAT_H
#define CHAT_H

#include "../../../general/base/ChatParsing.h"
#include "../../../general/base/GeneralStructures.h"
#include "../Api_protocol.h"

#include <QWidget>
#include <QTimer>
#include <QString>

namespace Ui {
class Chat;
}

namespace CatchChallenger {
class Chat : public QWidget
{
    Q_OBJECT
public:
    explicit Chat(QWidget *parent);
    ~Chat();
    void resetAll();
    void setClan(const bool &haveClan);
    void setClient(CatchChallenger::Api_protocol *client);
public slots:
    void new_system_text(CatchChallenger::Chat_type chat_type,QString text);
    void new_chat_text(CatchChallenger::Chat_type chat_type, QString text, QString pseudo, CatchChallenger::Player_type player_type);
    void setMultiPlayer(const bool & multiplayer);
private slots:
    void on_pushButtonChat_toggled(bool checked);
    void lineEdit_chat_text_returnPressed();
    void removeNumberForFlood();
    void update_chat();
    void comboBox_chat_type_currentIndexChanged(int index);
private:
    Ui::Chat *ui;
    QString lastMessageSend;
    QTimer stopFlood;
    int numberForFlood;

    struct ChatEntry
    {
        std::string player_pseudo;
        CatchChallenger::Player_type player_type;
        CatchChallenger::Chat_type chat_type;
        std::string text;
    };
    QList<ChatEntry> chat_list;
    CatchChallenger::Api_protocol *client;
};
}

#endif // CHAT_H
