/********************************************************************************
** Form generated from reading UI file 'Chat.ui'
**
** Created: Fri Apr 26 14:18:41 2013
**      by: Qt User Interface Compiler version 4.8.4
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CHAT_H
#define UI_CHAT_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QTextBrowser>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Chat
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QPushButton *pushButtonChat;
    QSpacerItem *horizontalSpacer;
    QTextBrowser *textBrowser_chat;
    QHBoxLayout *horizontalLayout_2;
    QLineEdit *lineEdit_chat_text;
    QComboBox *comboBox_chat_type;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *Chat)
    {
        if (Chat->objectName().isEmpty())
            Chat->setObjectName(QString::fromUtf8("Chat"));
        Chat->resize(400, 300);
        verticalLayout = new QVBoxLayout(Chat);
        verticalLayout->setSpacing(2);
        verticalLayout->setContentsMargins(2, 2, 2, 2);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(2);
        horizontalLayout->setContentsMargins(2, 2, 2, 2);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        pushButtonChat = new QPushButton(Chat);
        pushButtonChat->setObjectName(QString::fromUtf8("pushButtonChat"));
        pushButtonChat->setCheckable(true);
        pushButtonChat->setChecked(true);

        horizontalLayout->addWidget(pushButtonChat);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);


        verticalLayout->addLayout(horizontalLayout);

        textBrowser_chat = new QTextBrowser(Chat);
        textBrowser_chat->setObjectName(QString::fromUtf8("textBrowser_chat"));
        textBrowser_chat->setStyleSheet(QString::fromUtf8("#textBrowser_chat { background-color: qlineargradient(spread:pad, x1:0, y1:0.3, x2:0, y2:1, stop:0 rgba(255, 255, 255, 150), stop:1 rgba(221, 221, 221, 150));\n"
"border-color: rgb(204, 204, 204);\n"
"border-radius: 5px;\n"
"padding:5px;\n"
"}"));
        textBrowser_chat->setFrameShape(QFrame::NoFrame);
        textBrowser_chat->setFrameShadow(QFrame::Plain);

        verticalLayout->addWidget(textBrowser_chat);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(2);
        horizontalLayout_2->setContentsMargins(2, 2, 2, 2);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        lineEdit_chat_text = new QLineEdit(Chat);
        lineEdit_chat_text->setObjectName(QString::fromUtf8("lineEdit_chat_text"));

        horizontalLayout_2->addWidget(lineEdit_chat_text);

        comboBox_chat_type = new QComboBox(Chat);
        comboBox_chat_type->setObjectName(QString::fromUtf8("comboBox_chat_type"));

        horizontalLayout_2->addWidget(comboBox_chat_type);


        verticalLayout->addLayout(horizontalLayout_2);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);


        retranslateUi(Chat);

        comboBox_chat_type->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(Chat);
    } // setupUi

    void retranslateUi(QWidget *Chat)
    {
        Chat->setWindowTitle(QApplication::translate("Chat", "Form", 0, QApplication::UnicodeUTF8));
        pushButtonChat->setText(QApplication::translate("Chat", "Chat", 0, QApplication::UnicodeUTF8));
        comboBox_chat_type->clear();
        comboBox_chat_type->insertItems(0, QStringList()
         << QApplication::translate("Chat", "All", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Chat", "Local", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Chat", "Clan", 0, QApplication::UnicodeUTF8)
        );
    } // retranslateUi

};

namespace Ui {
    class Chat: public Ui_Chat {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CHAT_H
