/********************************************************************************
** Form generated from reading UI file 'NewGame.ui'
**
** Created: Fri Oct 5 13:40:00 2012
**      by: Qt User Interface Compiler version 4.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_NEWGAME_H
#define UI_NEWGAME_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QToolButton>
#include <QtGui/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_NewGame
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QLineEdit *gameName;
    QGridLayout *gridLayout;
    QSpacerItem *verticalSpacer_2;
    QToolButton *previousSkin;
    QSpacerItem *horizontalSpacer;
    QLabel *skin;
    QSpacerItem *horizontalSpacer_5;
    QToolButton *nextSkin;
    QSpacerItem *verticalSpacer;
    QLineEdit *pseudo;
    QHBoxLayout *horizontalLayout_3;
    QPushButton *ok;
    QPushButton *cancel;

    void setupUi(QDialog *NewGame)
    {
        if (NewGame->objectName().isEmpty())
            NewGame->setObjectName(QString::fromUtf8("NewGame"));
        NewGame->resize(420, 340);
        NewGame->setMinimumSize(QSize(420, 340));
        NewGame->setMaximumSize(QSize(420, 340));
        NewGame->setStyleSheet(QString::fromUtf8("background-image: url(:/images/player-select.png);"));
        verticalLayout = new QVBoxLayout(NewGame);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        label = new QLabel(NewGame);
        label->setObjectName(QString::fromUtf8("label"));
        label->setStyleSheet(QString::fromUtf8("background-image: url(:/images/empty.png);"));

        horizontalLayout->addWidget(label);

        gameName = new QLineEdit(NewGame);
        gameName->setObjectName(QString::fromUtf8("gameName"));
        gameName->setStyleSheet(QString::fromUtf8("background-image: url(:/images/light-white.png);\n"
"border-image: url(:/images/empty.png);"));

        horizontalLayout->addWidget(gameName);


        verticalLayout->addLayout(horizontalLayout);

        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        verticalSpacer_2 = new QSpacerItem(20, 118, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer_2, 0, 2, 1, 1);

        previousSkin = new QToolButton(NewGame);
        previousSkin->setObjectName(QString::fromUtf8("previousSkin"));
        previousSkin->setEnabled(false);
        previousSkin->setMinimumSize(QSize(28, 28));
        previousSkin->setMaximumSize(QSize(28, 28));
        previousSkin->setStyleSheet(QString::fromUtf8("QToolButton {\n"
"     border-color:#000;\n"
"     border: 1px outset #999;\n"
"     background-color: rgb(255, 255, 255);\n"
"     padding:1px 10px;\n"
"	 background-image: url(:/images/empty.png);\n"
"}\n"
" QToolButton:pressed {\n"
"     background-color: rgb(240,240,240);\n"
"     border-style: inset;\n"
" }"));

        gridLayout->addWidget(previousSkin, 1, 0, 1, 1);

        horizontalSpacer = new QSpacerItem(104, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer, 1, 1, 1, 1);

        skin = new QLabel(NewGame);
        skin->setObjectName(QString::fromUtf8("skin"));
        skin->setMinimumSize(QSize(160, 160));
        skin->setMaximumSize(QSize(160, 192));
        skin->setStyleSheet(QString::fromUtf8("background-image: url(:/images/empty.png);"));

        gridLayout->addWidget(skin, 1, 2, 1, 1);

        horizontalSpacer_5 = new QSpacerItem(104, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_5, 1, 3, 1, 1);

        nextSkin = new QToolButton(NewGame);
        nextSkin->setObjectName(QString::fromUtf8("nextSkin"));
        nextSkin->setMinimumSize(QSize(28, 28));
        nextSkin->setMaximumSize(QSize(28, 28));
        nextSkin->setStyleSheet(QString::fromUtf8("QToolButton {\n"
"     border-color:#000;\n"
"     border: 1px outset #999;\n"
"     background-color: rgb(255, 255, 255);\n"
"     padding:1px 10px;\n"
"	 background-image: url(:/images/empty.png);\n"
"}\n"
" QToolButton:pressed {\n"
"     background-color: rgb(240,240,240);\n"
"     border-style: inset;\n"
" }"));

        gridLayout->addWidget(nextSkin, 1, 4, 1, 1);

        verticalSpacer = new QSpacerItem(20, 118, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer, 2, 2, 1, 1);


        verticalLayout->addLayout(gridLayout);

        pseudo = new QLineEdit(NewGame);
        pseudo->setObjectName(QString::fromUtf8("pseudo"));
        pseudo->setStyleSheet(QString::fromUtf8("background-image: url(:/images/light-white.png);\n"
"border-image: url(:/images/empty.png);"));

        verticalLayout->addWidget(pseudo);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        ok = new QPushButton(NewGame);
        ok->setObjectName(QString::fromUtf8("ok"));
        ok->setEnabled(false);
        ok->setStyleSheet(QString::fromUtf8("QPushButton {\n"
"     border-color:#000;\n"
"     border: 1px outset #999;\n"
"     background-color: rgb(255, 255, 255);\n"
"     padding:1px 10px;\n"
"	 background-image: url(:/images/empty.png);\n"
"}\n"
" QPushButton:pressed {\n"
"     background-color: rgb(240,240,240);\n"
"     border-style: inset;\n"
" }"));

        horizontalLayout_3->addWidget(ok);

        cancel = new QPushButton(NewGame);
        cancel->setObjectName(QString::fromUtf8("cancel"));
        cancel->setStyleSheet(QString::fromUtf8("QPushButton {\n"
"     border-color:#000;\n"
"     border: 1px outset #999;\n"
"     background-color: rgb(255, 255, 255);\n"
"     padding:1px 10px;\n"
"	 background-image: url(:/images/empty.png);\n"
"}\n"
" QPushButton:pressed {\n"
"     background-color: rgb(240,240,240);\n"
"     border-style: inset;\n"
" }"));

        horizontalLayout_3->addWidget(cancel);


        verticalLayout->addLayout(horizontalLayout_3);


        retranslateUi(NewGame);
        QObject::connect(cancel, SIGNAL(clicked()), NewGame, SLOT(reject()));

        QMetaObject::connectSlotsByName(NewGame);
    } // setupUi

    void retranslateUi(QDialog *NewGame)
    {
        NewGame->setWindowTitle(QApplication::translate("NewGame", "Dialog", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("NewGame", "Game name:", 0, QApplication::UnicodeUTF8));
        gameName->setText(QApplication::translate("NewGame", "New game", 0, QApplication::UnicodeUTF8));
        gameName->setPlaceholderText(QApplication::translate("NewGame", "game name", 0, QApplication::UnicodeUTF8));
        previousSkin->setText(QApplication::translate("NewGame", "<", 0, QApplication::UnicodeUTF8));
        nextSkin->setText(QApplication::translate("NewGame", ">", 0, QApplication::UnicodeUTF8));
        pseudo->setPlaceholderText(QApplication::translate("NewGame", "pseudo", 0, QApplication::UnicodeUTF8));
        ok->setText(QApplication::translate("NewGame", "Ok", 0, QApplication::UnicodeUTF8));
        cancel->setText(QApplication::translate("NewGame", "Cancel", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class NewGame: public Ui_NewGame {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_NEWGAME_H
