#include "Chat.h"
#include "ui_Chat.h"
#include "../Api_client_real.h"

#include <QRegularExpression>
#include <QScrollBar>

using namespace CatchChallenger;

Chat::Chat(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Chat)
{
    ui->setupUi(this);
    connect(&stopFlood,&QTimer::timeout,this,&Chat::removeNumberForFlood,Qt::QueuedConnection);
    connect(ui->comboBox_chat_type,static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,&Chat::comboBox_chat_type_currentIndexChanged);
    connect(ui->lineEdit_chat_text,&QLineEdit::returnPressed,this,&Chat::lineEdit_chat_text_returnPressed);

    stopFlood.setSingleShot(false);
    stopFlood.start(1500);
    numberForFlood=0;
    client=NULL;

    setClan(false);
}

Chat::~Chat()
{
    delete ui;
}

void Chat::resetAll()
{
    chat_list.clear();
    ui->textBrowser_chat->clear();
    ui->comboBox_chat_type->setCurrentIndex(1);
    ui->lineEdit_chat_text->setText("");
    update_chat();
    client=NULL;
}

void Chat::setClient(CatchChallenger::Api_protocol *client)
{
    this->client=client;
}

void Chat::setClan(const bool &haveClan)
{
    ui->comboBox_chat_type->clear();
    ui->comboBox_chat_type->addItem(QApplication::translate("Chat", "All", 0));
    ui->comboBox_chat_type->setItemData(0,0,99);
    ui->comboBox_chat_type->addItem(QApplication::translate("Chat", "Local", 0));
    ui->comboBox_chat_type->setItemData(1,1,99);
    if(haveClan)
    {
        ui->comboBox_chat_type->addItem(QApplication::translate("Chat", "Clan", 0));
        ui->comboBox_chat_type->setItemData(2,2,99);
    }
}

void Chat::comboBox_chat_type_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    update_chat();
}

void Chat::on_pushButtonChat_toggled(bool checked)
{
    ui->textBrowser_chat->setVisible(checked);
    ui->lineEdit_chat_text->setVisible(checked);
    ui->comboBox_chat_type->setVisible(checked);
}

void Chat::lineEdit_chat_text_returnPressed()
{
    QString text=ui->lineEdit_chat_text->text();
    text.remove("\n");
    text.remove("\r");
    text.remove("\t");
    if(text.isEmpty())
        return;
    if(text.contains(QRegularExpression("^ +$")))
    {
        ui->lineEdit_chat_text->clear();
        new_system_text(Chat_type_system,"Space text not allowed");
        return;
    }
    if(text.size()>256)
    {
        ui->lineEdit_chat_text->clear();
        new_system_text(Chat_type_system,"Message too long");
        return;
    }
    if(!text.startsWith('/'))
    {
        if(text==lastMessageSend)
        {
            ui->lineEdit_chat_text->clear();
            new_system_text(Chat_type_system,"Send message like as previous");
            return;
        }
        if(numberForFlood>2)
        {
            ui->lineEdit_chat_text->clear();
            new_system_text(Chat_type_system,"Stop flood");
            return;
        }
    }
    numberForFlood++;
    lastMessageSend=text;
    ui->lineEdit_chat_text->setText(QString());
    if(!text.startsWith("/pm "))
    {
        Chat_type chat_type;
        switch(ui->comboBox_chat_type->itemData(ui->comboBox_chat_type->currentIndex(),99).toUInt())
        {
        default:
        case 0:
            chat_type=Chat_type_all;
        break;
        case 1:
            chat_type=Chat_type_local;
        break;
        case 2:
            chat_type=Chat_type_clan;
        break;
        }
        client->sendChatText(chat_type,text);
        if(!text.startsWith('/'))
            new_chat_text(chat_type,text,QString::fromStdString(client->player_informations.public_informations.pseudo),client->player_informations.public_informations.type);
    }
    else if(text.contains(QRegularExpression("^/pm [^ ]+ .+$")))
    {
        QString pseudo=text;
        pseudo.replace(QRegularExpression("^/pm ([^ ]+) .+$"), "\\1");
        text.replace(QRegularExpression("^/pm [^ ]+ (.+)$"), "\\1");
        client->sendPM(text,pseudo);
        new_chat_text(Chat_type_pm,text,tr("To: ")+pseudo,Player_type_normal);
    }
}

void Chat::removeNumberForFlood()
{
    if(numberForFlood<=0)
        return;
    numberForFlood--;
}

void Chat::new_system_text(CatchChallenger::Chat_type chat_type,QString text)
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << QStringLiteral("new_system_text: %1").arg(text);
    #endif
    ChatEntry newEntry;
    newEntry.player_type=Player_type_normal;
    //newEntry.player_pseudo=QString();
    newEntry.type=chat_type;
    newEntry.text=text.toStdString();
    chat_list << newEntry;
    while(chat_list.size()>64)
        chat_list.removeFirst();
    update_chat();
}

void Chat::new_chat_text(CatchChallenger::Chat_type chat_type,QString text,QString pseudo,CatchChallenger::Player_type player_type)
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << QStringLiteral("new_chat_text: %1 by %2").arg(text).arg(pseudo);
    #endif
    ChatEntry newEntry;
    newEntry.player_type=player_type;
    newEntry.player_pseudo=pseudo.toStdString();
    newEntry.type=chat_type;
    newEntry.text=text.toStdString();
    chat_list << newEntry;
    while(chat_list.size()>64)
        chat_list.removeFirst();
    update_chat();
}

void Chat::setMultiPlayer(const bool & multiplayer)
{
    ui->pushButtonChat->setChecked(multiplayer);
}

void Chat::update_chat()
{
    QString nameHtml;
    int index=0;
    while(index<chat_list.size())
    {
        const ChatEntry &entry=chat_list.at(index);
        bool addPlayerInfo=true;
        if(entry.type==Chat_type_system || entry.type==Chat_type_system_important)
            addPlayerInfo=false;
        if(!addPlayerInfo)
            nameHtml+=QString::fromStdString(ChatParsing::new_chat_message(std::string(),Player_type_normal,entry.type,entry.text));
        else
            nameHtml+=QString::fromStdString(ChatParsing::new_chat_message(entry.player_pseudo,entry.player_type,entry.type,entry.text));
        index++;
    }
    ui->textBrowser_chat->setHtml(nameHtml);
    ui->textBrowser_chat->verticalScrollBar()->setValue(ui->textBrowser_chat->verticalScrollBar()->maximum());
}
