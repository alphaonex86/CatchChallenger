/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created: Fri Apr 26 14:18:41 2013
**      by: Qt User Interface Compiler version 4.8.4
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QFrame>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QPushButton>
#include <QtGui/QScrollArea>
#include <QtGui/QSpacerItem>
#include <QtGui/QStackedWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QHBoxLayout *horizontalLayout;
    QStackedWidget *stackedWidget;
    QWidget *page;
    QVBoxLayout *verticalLayout_3;
    QSpacerItem *verticalSpacer_4;
    QHBoxLayout *horizontalLayout_login_1;
    QSpacerItem *horizontalSpacer_login_8;
    QFrame *frame_login_2;
    QHBoxLayout *horizontalLayout_6;
    QFrame *frame_login;
    QVBoxLayout *verticalLayout;
    QScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QVBoxLayout *verticalLayout_5;
    QLabel *savegameEmpty;
    QHBoxLayout *horizontalLayout_11;
    QSpacerItem *horizontalSpacer_11;
    QPushButton *SaveGame_Play;
    QSpacerItem *horizontalSpacer_12;
    QHBoxLayout *horizontalLayout_login_4;
    QSpacerItem *horizontalSpacer_login_3;
    QPushButton *SaveGame_New;
    QSpacerItem *horizontalSpacer_login_5;
    QPushButton *SaveGame_Rename;
    QSpacerItem *horizontalSpacer;
    QPushButton *SaveGame_Copy;
    QSpacerItem *horizontalSpacer_login_6;
    QPushButton *SaveGame_Delete;
    QSpacerItem *horizontalSpacer_login_4;
    QSpacerItem *horizontalSpacer_login_9;
    QSpacerItem *verticalSpacer_5;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(800, 600);
        MainWindow->setMinimumSize(QSize(800, 600));
        MainWindow->setMaximumSize(QSize(800, 600));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/images/catchchallenger.png"), QSize(), QIcon::Normal, QIcon::Off);
        MainWindow->setWindowIcon(icon);
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        horizontalLayout = new QHBoxLayout(centralWidget);
        horizontalLayout->setSpacing(0);
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        stackedWidget = new QStackedWidget(centralWidget);
        stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
        page = new QWidget();
        page->setObjectName(QString::fromUtf8("page"));
        page->setStyleSheet(QString::fromUtf8("#page{background-image: url(:/images/background.png);}"));
        verticalLayout_3 = new QVBoxLayout(page);
        verticalLayout_3->setSpacing(6);
        verticalLayout_3->setContentsMargins(11, 11, 11, 11);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        verticalSpacer_4 = new QSpacerItem(20, 99, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_3->addItem(verticalSpacer_4);

        horizontalLayout_login_1 = new QHBoxLayout();
        horizontalLayout_login_1->setSpacing(6);
        horizontalLayout_login_1->setObjectName(QString::fromUtf8("horizontalLayout_login_1"));
        horizontalSpacer_login_8 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_login_1->addItem(horizontalSpacer_login_8);

        frame_login_2 = new QFrame(page);
        frame_login_2->setObjectName(QString::fromUtf8("frame_login_2"));
        frame_login_2->setMinimumSize(QSize(777, 379));
        frame_login_2->setMaximumSize(QSize(777, 379));
        frame_login_2->setStyleSheet(QString::fromUtf8("#frame_login_2{background-image: url(:/images/savegame-select.png);}"));
        horizontalLayout_6 = new QHBoxLayout(frame_login_2);
        horizontalLayout_6->setSpacing(0);
        horizontalLayout_6->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_6->setObjectName(QString::fromUtf8("horizontalLayout_6"));
        frame_login = new QFrame(frame_login_2);
        frame_login->setObjectName(QString::fromUtf8("frame_login"));
        frame_login->setMinimumSize(QSize(760, 360));
        frame_login->setMaximumSize(QSize(760, 360));
        frame_login->setFrameShape(QFrame::NoFrame);
        frame_login->setFrameShadow(QFrame::Raised);
        verticalLayout = new QVBoxLayout(frame_login);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        scrollArea = new QScrollArea(frame_login);
        scrollArea->setObjectName(QString::fromUtf8("scrollArea"));
        scrollArea->setStyleSheet(QString::fromUtf8("QScrollArea {\n"
"	background-image: url(:/images/light-white.png);\n"
"border: 1px solid #000;\n"
"}\n"
"QLabel { padding:2px; }"));
        scrollArea->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName(QString::fromUtf8("scrollAreaWidgetContents"));
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 750, 292));
        verticalLayout_5 = new QVBoxLayout(scrollAreaWidgetContents);
        verticalLayout_5->setSpacing(6);
        verticalLayout_5->setContentsMargins(11, 11, 11, 11);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        savegameEmpty = new QLabel(scrollAreaWidgetContents);
        savegameEmpty->setObjectName(QString::fromUtf8("savegameEmpty"));

        verticalLayout_5->addWidget(savegameEmpty);

        scrollArea->setWidget(scrollAreaWidgetContents);

        verticalLayout->addWidget(scrollArea);

        horizontalLayout_11 = new QHBoxLayout();
        horizontalLayout_11->setSpacing(6);
        horizontalLayout_11->setObjectName(QString::fromUtf8("horizontalLayout_11"));
        horizontalSpacer_11 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_11->addItem(horizontalSpacer_11);

        SaveGame_Play = new QPushButton(frame_login);
        SaveGame_Play->setObjectName(QString::fromUtf8("SaveGame_Play"));
        SaveGame_Play->setEnabled(false);

        horizontalLayout_11->addWidget(SaveGame_Play);

        horizontalSpacer_12 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_11->addItem(horizontalSpacer_12);


        verticalLayout->addLayout(horizontalLayout_11);

        horizontalLayout_login_4 = new QHBoxLayout();
        horizontalLayout_login_4->setSpacing(6);
        horizontalLayout_login_4->setObjectName(QString::fromUtf8("horizontalLayout_login_4"));
        horizontalSpacer_login_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_login_4->addItem(horizontalSpacer_login_3);

        SaveGame_New = new QPushButton(frame_login);
        SaveGame_New->setObjectName(QString::fromUtf8("SaveGame_New"));

        horizontalLayout_login_4->addWidget(SaveGame_New);

        horizontalSpacer_login_5 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_login_4->addItem(horizontalSpacer_login_5);

        SaveGame_Rename = new QPushButton(frame_login);
        SaveGame_Rename->setObjectName(QString::fromUtf8("SaveGame_Rename"));
        SaveGame_Rename->setEnabled(false);

        horizontalLayout_login_4->addWidget(SaveGame_Rename);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_login_4->addItem(horizontalSpacer);

        SaveGame_Copy = new QPushButton(frame_login);
        SaveGame_Copy->setObjectName(QString::fromUtf8("SaveGame_Copy"));
        SaveGame_Copy->setEnabled(false);

        horizontalLayout_login_4->addWidget(SaveGame_Copy);

        horizontalSpacer_login_6 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_login_4->addItem(horizontalSpacer_login_6);

        SaveGame_Delete = new QPushButton(frame_login);
        SaveGame_Delete->setObjectName(QString::fromUtf8("SaveGame_Delete"));
        SaveGame_Delete->setEnabled(false);

        horizontalLayout_login_4->addWidget(SaveGame_Delete);

        horizontalSpacer_login_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_login_4->addItem(horizontalSpacer_login_4);


        verticalLayout->addLayout(horizontalLayout_login_4);


        horizontalLayout_6->addWidget(frame_login);


        horizontalLayout_login_1->addWidget(frame_login_2);

        horizontalSpacer_login_9 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_login_1->addItem(horizontalSpacer_login_9);


        verticalLayout_3->addLayout(horizontalLayout_login_1);

        verticalSpacer_5 = new QSpacerItem(20, 98, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_3->addItem(verticalSpacer_5);

        stackedWidget->addWidget(page);

        horizontalLayout->addWidget(stackedWidget);

        MainWindow->setCentralWidget(centralWidget);

        retranslateUi(MainWindow);

        stackedWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "CatchChallenger", 0, QApplication::UnicodeUTF8));
        savegameEmpty->setText(QApplication::translate("MainWindow", "<html><head/><body><p align=\"center\"><span style=\" font-size:12pt; color:#a0a0a0;\">Empty</span></p></body></html>", 0, QApplication::UnicodeUTF8));
        SaveGame_Play->setText(QApplication::translate("MainWindow", "Play selected game", 0, QApplication::UnicodeUTF8));
        SaveGame_New->setText(QApplication::translate("MainWindow", "New game", 0, QApplication::UnicodeUTF8));
        SaveGame_Rename->setText(QApplication::translate("MainWindow", "Rename", 0, QApplication::UnicodeUTF8));
        SaveGame_Copy->setText(QApplication::translate("MainWindow", "Copy", 0, QApplication::UnicodeUTF8));
        SaveGame_Delete->setText(QApplication::translate("MainWindow", "Delete", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
