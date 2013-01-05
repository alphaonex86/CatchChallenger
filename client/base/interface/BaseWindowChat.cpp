#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../../general/base/FacilityLib.h"
#include "../ClientVariable.h"
#include "DatapackClientLoader.h"

#include <QListWidgetItem>
#include <QBuffer>
#include <QInputDialog>
#include <QMessageBox>
#include <QDesktopServices>

//do buy queue
//do sell queue

using namespace Pokecraft;

void BaseWindow::lineEdit_chat_text_returnPressed()
{
    QString text=lineEdit_chat_text->text();
    if(text.isEmpty())
        return;
    if(text.contains(QRegExp("^ +$")))
    {
        lineEdit_chat_text->clear();
        new_system_text(Chat_type_system,"Space text not allowed");
        return;
    }
    if(text.size()>256)
    {
        lineEdit_chat_text->clear();
        new_system_text(Chat_type_system,"Message too long");
        return;
    }
    if(!text.startsWith('/'))
    {
        if(text==lastMessageSend)
        {
            lineEdit_chat_text->clear();
            new_system_text(Chat_type_system,"Send message like as previous");
            return;
        }
        if(numberForFlood>2)
        {
            lineEdit_chat_text->clear();
            new_system_text(Chat_type_system,"Stop flood");
            return;
        }
    }
    numberForFlood++;
    lastMessageSend=text;
    lineEdit_chat_text->setText("");
    if(!text.startsWith("/pm "))
    {
        Chat_type chat_type;
        switch(comboBox_chat_type->currentIndex())
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
    }
    else if(text.contains(QRegExp("^/pm [^ ]+ .+$")))
    {
        QString pseudo=text;
        pseudo.replace(QRegExp("^/pm ([^ ]+) .+$"), "\\1");
        text.replace(QRegExp("^/pm [^ ]+ (.+)$"), "\\1");
        client->sendPM(text,pseudo);
    }
}

void BaseWindow::removeNumberForFlood()
{
    if(numberForFlood<=0)
        return;
    numberForFlood--;
}

void BaseWindow::new_system_text(Chat_type chat_type,QString text)
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << QString("new_system_text: %1").arg(text);
    #endif
    chat_list_player_type << Player_type_normal;
    chat_list_player_pseudo << "";
    chat_list_type << chat_type;
    chat_list_text << text;
    while(chat_list_player_type.size()>64)
    {
        chat_list_player_type.removeFirst();
        chat_list_player_pseudo.removeFirst();
        chat_list_type.removeFirst();
        chat_list_text.removeFirst();
    }
    update_chat();
}

void BaseWindow::new_chat_text(Chat_type chat_type,QString text,QString pseudo,Player_type type)
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << QString("new_chat_text: %1 by %2").arg(text).arg(pseudo);
    #endif
    chat_list_player_type << type;
    chat_list_player_pseudo << pseudo;
    chat_list_type << chat_type;
    chat_list_text << text;
    while(chat_list_player_type.size()>64)
    {
        chat_list_player_type.removeFirst();
        chat_list_player_pseudo.removeFirst();
        chat_list_type.removeFirst();
        chat_list_text.removeFirst();
    }
    update_chat();
}

void BaseWindow::update_chat()
{
    QString nameHtml;
    int index=0;
    while(index<chat_list_player_pseudo.size())
    {
        bool addPlayerInfo=true;
        if(chat_list_type.at(index)==Chat_type_system || chat_list_type.at(index)==Chat_type_system_important)
            addPlayerInfo=false;
        if(!addPlayerInfo)
            nameHtml+=ChatParsing::new_chat_message("",Player_type_normal,chat_list_type.at(index),chat_list_text.at(index));
        else
            nameHtml+=ChatParsing::new_chat_message(chat_list_player_pseudo.at(index),chat_list_player_type.at(index),chat_list_type.at(index),chat_list_text.at(index));
        index++;
    }
    textBrowser_chat->setHtml(nameHtml);
    //textBrowser_chat->scrollToAnchor(QString::number(index-1));
}

void BaseWindow::setupChatUI()
{
    QFrame *frame_main_display_right = new QFrame(ui->page_map);
    frame_main_display_right->setMinimumSize(QSize(200, 0));
    frame_main_display_right->setMaximumSize(QSize(200, 16777215));
    frame_main_display_right->setStyleSheet(QString::fromUtf8("background-image: url(:/images/interface/baroption.png);"));
    QVBoxLayout *verticalLayout_5 = new QVBoxLayout(frame_main_display_right);
    verticalLayout_5->setSpacing(2);
    verticalLayout_5->setContentsMargins(2, 0, 2, 0);
    textBrowser_chat = new QTextBrowser(frame_main_display_right);
    textBrowser_chat->setMinimumSize(QSize(196, 275));
    textBrowser_chat->setMaximumSize(QSize(196, 275));
    textBrowser_chat->setStyleSheet(QString::fromUtf8("border-image: url(:/images/empty.png);\n"
"background-image: url(:/images/light-white.png);"));
    textBrowser_chat->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textBrowser_chat->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    verticalLayout_5->addWidget(textBrowser_chat);

    QHBoxLayout *horizontalLayout_main_display_9 = new QHBoxLayout();
    horizontalLayout_main_display_9->setSpacing(0);
    lineEdit_chat_text = new QLineEdit(frame_main_display_right);
    lineEdit_chat_text->setObjectName(QString::fromUtf8("lineEdit_chat_text"));
    lineEdit_chat_text->setMinimumSize(QSize(0, 25));
    lineEdit_chat_text->setMaximumSize(QSize(16777215, 25));
    lineEdit_chat_text->setStyleSheet(QString::fromUtf8("background-image: url(:/images/chat/background-input.png);\n"
"border-image: url(:/images/empty.png);"));
    lineEdit_chat_text->setMaxLength(256);
    horizontalLayout_main_display_9->addWidget(lineEdit_chat_text);
    comboBox_chat_type = new QComboBox(frame_main_display_right);
    comboBox_chat_type->setObjectName(QString::fromUtf8("comboBox_chat_type"));
    horizontalLayout_main_display_9->addWidget(comboBox_chat_type);

    verticalLayout_5->addLayout(horizontalLayout_main_display_9);
    QSpacerItem *verticalSpacer_5 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    verticalLayout_5->addItem(verticalSpacer_5);

    comboBox_chat_type->insertItems(0, QStringList()
     << QApplication::translate("BaseWindowUI", "All", 0, QApplication::UnicodeUTF8)
     << QApplication::translate("BaseWindowUI", "Local", 0, QApplication::UnicodeUTF8)
     << QApplication::translate("BaseWindowUI", "Clan", 0, QApplication::UnicodeUTF8)
    );

    frame_main_display_right->setGeometry(QRect(0, 150, 200, 250));

    connect(comboBox_chat_type,SIGNAL(currentIndexChanged(int)),this,SLOT(comboBox_chat_type_currentIndexChanged(int)));
    connect(lineEdit_chat_text,SIGNAL(returnPressed()),this,SLOT(lineEdit_chat_text_returnPressed()));
    connect(lineEdit_chat_text,SIGNAL(lostFocus()),this,SLOT(lineEdit_chat_text_lostFocus()));
    connect(this,SIGNAL(sendsetMultiPlayer(bool)),frame_main_display_right,SLOT(setVisible(bool)),Qt::QueuedConnection);
}

QString BaseWindow::toHtmlEntities(QString text)
{
    text.replace("&","&amp;");
    text.replace("\"","&quot;");
    text.replace("'","&#039;");
    text.replace("<","&lt;");
    text.replace(">","&gt;");
    return text;
}

QString BaseWindow::toSmilies(QString text)
{
    text.replace(":)","<img src=\":/images/smiles/face-smile.png\" alt=\"\" style=\"vertical-align:middle;\" />");
    text.replace(":|","<img src=\":/images/smiles/face-plain.png\" alt=\"\" style=\"vertical-align:middle;\" />");
    text.replace(":(","<img src=\":/images/smiles/face-sad.png\" alt=\"\" style=\"vertical-align:middle;\" />");
    text.replace(":P","<img src=\":/images/smiles/face-raspberry.png\" alt=\"\" style=\"vertical-align:middle;\" />");
    text.replace(":p","<img src=\":/images/smiles/face-raspberry.png\" alt=\"\" style=\"vertical-align:middle;\" />");
    text.replace(":D","<img src=\":/images/smiles/face-laugh.png\" alt=\"\" style=\"vertical-align:middle;\" />");
    text.replace(":o","<img src=\":/images/smiles/face-surprise.png\" alt=\"\" style=\"vertical-align:middle;\" />");
    text.replace(";)","<img src=\":/images/smiles/face-wink.png\" alt=\"\" style=\"vertical-align:middle;\" />");
    return text;
}