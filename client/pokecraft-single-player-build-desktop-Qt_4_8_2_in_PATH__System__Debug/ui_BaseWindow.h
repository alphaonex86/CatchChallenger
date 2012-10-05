/********************************************************************************
** Form generated from reading UI file 'BaseWindow.ui'
**
** Created: Fri Oct 5 13:40:00 2012
**      by: Qt User Interface Compiler version 4.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_BASEWINDOW_H
#define UI_BASEWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QFrame>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QStackedWidget>
#include <QtGui/QTextBrowser>
#include <QtGui/QToolButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_BaseWindowUI
{
public:
    QHBoxLayout *horizontalLayout_2;
    QHBoxLayout *horizontalLayout;
    QStackedWidget *stackedWidget;
    QWidget *page_2;
    QVBoxLayout *verticalLayout_2;
    QSpacerItem *verticalSpacer_loading_4;
    QHBoxLayout *horizontalLayout_loading_7;
    QSpacerItem *horizontalSpacer_loading_9;
    QLabel *label_connecting_status;
    QSpacerItem *horizontalSpacer_loading_10;
    QWidget *page_3;
    QVBoxLayout *verticalLayout_4;
    QFrame *frame_main_display_top;
    QHBoxLayout *horizontalLayout_mainDisplay;
    QFrame *frame_main_display_right;
    QVBoxLayout *verticalLayout_5;
    QTextBrowser *textBrowser_chat;
    QHBoxLayout *horizontalLayout_main_display_9;
    QLineEdit *lineEdit_chat_text;
    QComboBox *comboBox_chat_type;
    QFrame *frame_main_display_player_info;
    QFrame *frame_main_display_bottom;
    QHBoxLayout *horizontalLayout_10;
    QPushButton *pushButton_interface_bag;
    QPushButton *pushButton_interface_monster_list;
    QPushButton *pushButton_interface_trainer;
    QToolButton *toolButton_interface_map;
    QSpacerItem *horizontalSpacer_main_display_11;
    QToolButton *toolButton_interface_quit;
    QFrame *frame_main_display_interface_player;
    QHBoxLayout *horizontalLayout_11;
    QLabel *label_interface_player_img;
    QLabel *label_interface_number_of_player;
    QWidget *page_4;
    QVBoxLayout *verticalLayout_8;
    QSpacerItem *verticalSpacer_3;
    QHBoxLayout *horizontalLayout_12;
    QSpacerItem *horizontalSpacer_10;
    QFrame *frame;
    QGridLayout *gridLayout_2;
    QFrame *frame_2;
    QVBoxLayout *verticalLayout_6;
    QFrame *frame_3;
    QHBoxLayout *horizontalLayout_3;
    QSpacerItem *horizontalSpacer_2;
    QLabel *label_2;
    QSpacerItem *horizontalSpacer_3;
    QLabel *player_informations_id;
    QSpacerItem *horizontalSpacer_4;
    QHBoxLayout *horizontalLayout_5;
    QFrame *frame_5;
    QVBoxLayout *verticalLayout_7;
    QHBoxLayout *horizontalLayout_7;
    QLabel *label_3;
    QLabel *label_5;
    QLabel *player_informations_pseudo;
    QSpacerItem *horizontalSpacer_6;
    QHBoxLayout *horizontalLayout_8;
    QLabel *label_6;
    QLabel *label_7;
    QLabel *player_informations_cash;
    QSpacerItem *horizontalSpacer_7;
    QSpacerItem *verticalSpacer;
    QFrame *frame_6;
    QHBoxLayout *horizontalLayout_13;
    QLabel *player_informations_front;
    QFrame *frame_4;
    QHBoxLayout *horizontalLayout_4;
    QLabel *label_4;
    QSpacerItem *horizontalSpacer_5;
    QSpacerItem *horizontalSpacer_9;
    QSpacerItem *verticalSpacer_2;
    QHBoxLayout *horizontalLayout_9;
    QSpacerItem *horizontalSpacer_8;
    QToolButton *toolButton_quit_interface;

    void setupUi(QWidget *BaseWindowUI)
    {
        if (BaseWindowUI->objectName().isEmpty())
            BaseWindowUI->setObjectName(QString::fromUtf8("BaseWindowUI"));
        BaseWindowUI->resize(800, 600);
        BaseWindowUI->setMinimumSize(QSize(800, 600));
        BaseWindowUI->setMaximumSize(QSize(800, 600));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/images/pokecraft.png"), QSize(), QIcon::Normal, QIcon::Off);
        BaseWindowUI->setWindowIcon(icon);
        horizontalLayout_2 = new QHBoxLayout(BaseWindowUI);
        horizontalLayout_2->setSpacing(0);
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(0);
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        stackedWidget = new QStackedWidget(BaseWindowUI);
        stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
        stackedWidget->setStyleSheet(QString::fromUtf8(""));
        page_2 = new QWidget();
        page_2->setObjectName(QString::fromUtf8("page_2"));
        page_2->setStyleSheet(QString::fromUtf8("background-image: url(:/images/background.png);"));
        verticalLayout_2 = new QVBoxLayout(page_2);
        verticalLayout_2->setSpacing(6);
        verticalLayout_2->setContentsMargins(11, 11, 11, 11);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(-1, -1, -1, 75);
        verticalSpacer_loading_4 = new QSpacerItem(20, 488, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_2->addItem(verticalSpacer_loading_4);

        horizontalLayout_loading_7 = new QHBoxLayout();
        horizontalLayout_loading_7->setSpacing(6);
        horizontalLayout_loading_7->setObjectName(QString::fromUtf8("horizontalLayout_loading_7"));
        horizontalSpacer_loading_9 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_loading_7->addItem(horizontalSpacer_loading_9);

        label_connecting_status = new QLabel(page_2);
        label_connecting_status->setObjectName(QString::fromUtf8("label_connecting_status"));
        QFont font;
        font.setPointSize(15);
        font.setBold(true);
        font.setWeight(75);
        label_connecting_status->setFont(font);
        label_connecting_status->setStyleSheet(QString::fromUtf8("background-image: url(:/images/empty.png);\n"
"color: rgb(255, 255, 255);"));

        horizontalLayout_loading_7->addWidget(label_connecting_status);

        horizontalSpacer_loading_10 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_loading_7->addItem(horizontalSpacer_loading_10);


        verticalLayout_2->addLayout(horizontalLayout_loading_7);

        stackedWidget->addWidget(page_2);
        page_3 = new QWidget();
        page_3->setObjectName(QString::fromUtf8("page_3"));
        verticalLayout_4 = new QVBoxLayout(page_3);
        verticalLayout_4->setSpacing(0);
        verticalLayout_4->setContentsMargins(0, 0, 0, 0);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        frame_main_display_top = new QFrame(page_3);
        frame_main_display_top->setObjectName(QString::fromUtf8("frame_main_display_top"));
        frame_main_display_top->setMinimumSize(QSize(0, 52));
        frame_main_display_top->setMaximumSize(QSize(16777215, 52));
        frame_main_display_top->setStyleSheet(QString::fromUtf8("background-image: url(:/images/interface/pokemonbar.png);"));

        verticalLayout_4->addWidget(frame_main_display_top);

        horizontalLayout_mainDisplay = new QHBoxLayout();
        horizontalLayout_mainDisplay->setSpacing(0);
        horizontalLayout_mainDisplay->setObjectName(QString::fromUtf8("horizontalLayout_mainDisplay"));
        frame_main_display_right = new QFrame(page_3);
        frame_main_display_right->setObjectName(QString::fromUtf8("frame_main_display_right"));
        frame_main_display_right->setMinimumSize(QSize(200, 0));
        frame_main_display_right->setMaximumSize(QSize(200, 16777215));
        frame_main_display_right->setStyleSheet(QString::fromUtf8("background-image: url(:/images/interface/baroption.png);"));
        verticalLayout_5 = new QVBoxLayout(frame_main_display_right);
        verticalLayout_5->setSpacing(2);
        verticalLayout_5->setContentsMargins(11, 11, 11, 11);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        verticalLayout_5->setContentsMargins(2, 0, 2, 0);
        textBrowser_chat = new QTextBrowser(frame_main_display_right);
        textBrowser_chat->setObjectName(QString::fromUtf8("textBrowser_chat"));
        textBrowser_chat->setMinimumSize(QSize(196, 275));
        textBrowser_chat->setMaximumSize(QSize(196, 275));
        textBrowser_chat->setStyleSheet(QString::fromUtf8("border-image: url(:/images/empty.png);\n"
"background-image: url(:/images/light-white.png);"));
        textBrowser_chat->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        textBrowser_chat->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        verticalLayout_5->addWidget(textBrowser_chat);

        horizontalLayout_main_display_9 = new QHBoxLayout();
        horizontalLayout_main_display_9->setSpacing(0);
        horizontalLayout_main_display_9->setObjectName(QString::fromUtf8("horizontalLayout_main_display_9"));
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

        frame_main_display_player_info = new QFrame(frame_main_display_right);
        frame_main_display_player_info->setObjectName(QString::fromUtf8("frame_main_display_player_info"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(frame_main_display_player_info->sizePolicy().hasHeightForWidth());
        frame_main_display_player_info->setSizePolicy(sizePolicy);

        verticalLayout_5->addWidget(frame_main_display_player_info);


        horizontalLayout_mainDisplay->addWidget(frame_main_display_right);


        verticalLayout_4->addLayout(horizontalLayout_mainDisplay);

        frame_main_display_bottom = new QFrame(page_3);
        frame_main_display_bottom->setObjectName(QString::fromUtf8("frame_main_display_bottom"));
        frame_main_display_bottom->setMinimumSize(QSize(0, 32));
        frame_main_display_bottom->setMaximumSize(QSize(16777215, 32));
        frame_main_display_bottom->setStyleSheet(QString::fromUtf8("background-image: url(:/images/interface/baroption.png);"));
        horizontalLayout_10 = new QHBoxLayout(frame_main_display_bottom);
        horizontalLayout_10->setSpacing(6);
        horizontalLayout_10->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_10->setObjectName(QString::fromUtf8("horizontalLayout_10"));
        horizontalLayout_10->setContentsMargins(-1, 0, -1, 0);
        pushButton_interface_bag = new QPushButton(frame_main_display_bottom);
        pushButton_interface_bag->setObjectName(QString::fromUtf8("pushButton_interface_bag"));
        pushButton_interface_bag->setMinimumSize(QSize(110, 25));
        pushButton_interface_bag->setMaximumSize(QSize(110, 25));
        pushButton_interface_bag->setStyleSheet(QString::fromUtf8("QPushButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QPushButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QPushButton { \n"
"background-image: url(:/images/interface/option-chest.png);\n"
"border-image: url(:/images/empty.png);\n"
"color: rgb(255, 255, 255);\n"
"padding-left:28px;\n"
"padding-right:4px;\n"
"padding-top:8px;\n"
"padding-bottom:3px;\n"
"background-attachment:fixed;\n"
"}\n"
""));

        horizontalLayout_10->addWidget(pushButton_interface_bag);

        pushButton_interface_monster_list = new QPushButton(frame_main_display_bottom);
        pushButton_interface_monster_list->setObjectName(QString::fromUtf8("pushButton_interface_monster_list"));
        pushButton_interface_monster_list->setMinimumSize(QSize(110, 25));
        pushButton_interface_monster_list->setMaximumSize(QSize(110, 25));
        pushButton_interface_monster_list->setStyleSheet(QString::fromUtf8("QPushButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QPushButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QPushButton { \n"
"background-image: url(:/images/interface/option-monstropedia.png);\n"
"border-image: url(:/images/empty.png);\n"
"color: rgb(255, 255, 255);\n"
"padding-left:19px;\n"
"padding-right:4px;\n"
"padding-top:8px;\n"
"padding-bottom:3px;\n"
"}"));

        horizontalLayout_10->addWidget(pushButton_interface_monster_list);

        pushButton_interface_trainer = new QPushButton(frame_main_display_bottom);
        pushButton_interface_trainer->setObjectName(QString::fromUtf8("pushButton_interface_trainer"));
        pushButton_interface_trainer->setMinimumSize(QSize(110, 25));
        pushButton_interface_trainer->setMaximumSize(QSize(110, 25));
        pushButton_interface_trainer->setStyleSheet(QString::fromUtf8("QPushButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QPushButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QPushButton { \n"
"background-image: url(:/images/interface/option-player.png);\n"
"border-image: url(:/images/empty.png);\n"
"color: rgb(255, 255, 255);\n"
"padding-left:25px;\n"
"padding-right:4px;\n"
"padding-top:8px;\n"
"padding-bottom:3px;\n"
"}"));

        horizontalLayout_10->addWidget(pushButton_interface_trainer);

        toolButton_interface_map = new QToolButton(frame_main_display_bottom);
        toolButton_interface_map->setObjectName(QString::fromUtf8("toolButton_interface_map"));
        toolButton_interface_map->setMinimumSize(QSize(28, 25));
        toolButton_interface_map->setMaximumSize(QSize(28, 25));
        toolButton_interface_map->setStyleSheet(QString::fromUtf8("background-image: url(:/images/interface/pkmnoption5.1.png);\n"
"border-image: url(:/images/empty.png);"));

        horizontalLayout_10->addWidget(toolButton_interface_map);

        horizontalSpacer_main_display_11 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_10->addItem(horizontalSpacer_main_display_11);

        toolButton_interface_quit = new QToolButton(frame_main_display_bottom);
        toolButton_interface_quit->setObjectName(QString::fromUtf8("toolButton_interface_quit"));
        toolButton_interface_quit->setMinimumSize(QSize(22, 25));
        toolButton_interface_quit->setMaximumSize(QSize(22, 25));
        toolButton_interface_quit->setStyleSheet(QString::fromUtf8("QToolButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QToolButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QToolButton { \n"
"background-image: url(:/images/interface/quit.png);\n"
"border-image: url(:/images/empty.png);\n"
"}"));

        horizontalLayout_10->addWidget(toolButton_interface_quit);

        frame_main_display_interface_player = new QFrame(frame_main_display_bottom);
        frame_main_display_interface_player->setObjectName(QString::fromUtf8("frame_main_display_interface_player"));
        frame_main_display_interface_player->setMinimumSize(QSize(25, 25));
        frame_main_display_interface_player->setMaximumSize(QSize(16777215, 25));
        frame_main_display_interface_player->setStyleSheet(QString::fromUtf8("border: 1px solid #343434;\n"
"background-image: url(:/images/interface/background.png);"));
        horizontalLayout_11 = new QHBoxLayout(frame_main_display_interface_player);
        horizontalLayout_11->setSpacing(6);
        horizontalLayout_11->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_11->setObjectName(QString::fromUtf8("horizontalLayout_11"));
        horizontalLayout_11->setContentsMargins(2, 0, 2, 0);
        label_interface_player_img = new QLabel(frame_main_display_interface_player);
        label_interface_player_img->setObjectName(QString::fromUtf8("label_interface_player_img"));
        label_interface_player_img->setMinimumSize(QSize(0, 23));
        label_interface_player_img->setMaximumSize(QSize(16777215, 23));
        label_interface_player_img->setStyleSheet(QString::fromUtf8("border: 0px;\n"
"background-image: url(:/images/empty.png);"));
        label_interface_player_img->setPixmap(QPixmap(QString::fromUtf8(":/images/interface/player.png")));

        horizontalLayout_11->addWidget(label_interface_player_img);

        label_interface_number_of_player = new QLabel(frame_main_display_interface_player);
        label_interface_number_of_player->setObjectName(QString::fromUtf8("label_interface_number_of_player"));
        label_interface_number_of_player->setStyleSheet(QString::fromUtf8("border: 0px;\n"
"background-image: url(:/images/empty.png);"));

        horizontalLayout_11->addWidget(label_interface_number_of_player);


        horizontalLayout_10->addWidget(frame_main_display_interface_player);


        verticalLayout_4->addWidget(frame_main_display_bottom);

        stackedWidget->addWidget(page_3);
        page_4 = new QWidget();
        page_4->setObjectName(QString::fromUtf8("page_4"));
        page_4->setStyleSheet(QString::fromUtf8("background-image: url(:/images/interface/player/background.png);"));
        verticalLayout_8 = new QVBoxLayout(page_4);
        verticalLayout_8->setSpacing(6);
        verticalLayout_8->setContentsMargins(11, 11, 11, 11);
        verticalLayout_8->setObjectName(QString::fromUtf8("verticalLayout_8"));
        verticalSpacer_3 = new QSpacerItem(20, 175, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_8->addItem(verticalSpacer_3);

        horizontalLayout_12 = new QHBoxLayout();
        horizontalLayout_12->setSpacing(6);
        horizontalLayout_12->setObjectName(QString::fromUtf8("horizontalLayout_12"));
        horizontalSpacer_10 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_12->addItem(horizontalSpacer_10);

        frame = new QFrame(page_4);
        frame->setObjectName(QString::fromUtf8("frame"));
        frame->setMinimumSize(QSize(430, 195));
        frame->setMaximumSize(QSize(430, 195));
        frame->setStyleSheet(QString::fromUtf8("background-image: url(:/images/background-dialog.png);"));
        frame->setFrameShape(QFrame::NoFrame);
        frame->setFrameShadow(QFrame::Plain);
        gridLayout_2 = new QGridLayout(frame);
        gridLayout_2->setSpacing(6);
        gridLayout_2->setContentsMargins(11, 11, 11, 11);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        frame_2 = new QFrame(frame);
        frame_2->setObjectName(QString::fromUtf8("frame_2"));
        QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(frame_2->sizePolicy().hasHeightForWidth());
        frame_2->setSizePolicy(sizePolicy1);
        frame_2->setMinimumSize(QSize(415, 174));
        frame_2->setMaximumSize(QSize(415, 174));
        frame_2->setStyleSheet(QString::fromUtf8("background-image: url(:/images/empty.png);"));
        verticalLayout_6 = new QVBoxLayout(frame_2);
        verticalLayout_6->setSpacing(6);
        verticalLayout_6->setContentsMargins(11, 11, 11, 11);
        verticalLayout_6->setObjectName(QString::fromUtf8("verticalLayout_6"));
        frame_3 = new QFrame(frame_2);
        frame_3->setObjectName(QString::fromUtf8("frame_3"));
        frame_3->setMinimumSize(QSize(0, 25));
        frame_3->setMaximumSize(QSize(16777215, 25));
        frame_3->setStyleSheet(QString::fromUtf8("background-image: url(:/images/interface/player/bande.png);"));
        horizontalLayout_3 = new QHBoxLayout(frame_3);
        horizontalLayout_3->setSpacing(6);
        horizontalLayout_3->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalSpacer_2 = new QSpacerItem(88, 14, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_2);

        label_2 = new QLabel(frame_3);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setMinimumSize(QSize(0, 16));
        label_2->setMaximumSize(QSize(16777215, 16));
        label_2->setStyleSheet(QString::fromUtf8("background-color: rgb(104, 184, 248);\n"
"border: 1px solid rgb(64, 136, 192);"));

        horizontalLayout_3->addWidget(label_2);

        horizontalSpacer_3 = new QSpacerItem(87, 14, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_3);

        player_informations_id = new QLabel(frame_3);
        player_informations_id->setObjectName(QString::fromUtf8("player_informations_id"));
        player_informations_id->setMinimumSize(QSize(0, 16));
        player_informations_id->setMaximumSize(QSize(16777215, 16));
        player_informations_id->setStyleSheet(QString::fromUtf8("background-image: url(:/images/empty.png);"));

        horizontalLayout_3->addWidget(player_informations_id);

        horizontalSpacer_4 = new QSpacerItem(88, 14, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_4);


        verticalLayout_6->addWidget(frame_3);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setSpacing(6);
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        frame_5 = new QFrame(frame_2);
        frame_5->setObjectName(QString::fromUtf8("frame_5"));
        verticalLayout_7 = new QVBoxLayout(frame_5);
        verticalLayout_7->setSpacing(6);
        verticalLayout_7->setContentsMargins(11, 11, 11, 11);
        verticalLayout_7->setObjectName(QString::fromUtf8("verticalLayout_7"));
        horizontalLayout_7 = new QHBoxLayout();
        horizontalLayout_7->setSpacing(6);
        horizontalLayout_7->setObjectName(QString::fromUtf8("horizontalLayout_7"));
        label_3 = new QLabel(frame_5);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setMinimumSize(QSize(8, 8));
        label_3->setMaximumSize(QSize(8, 8));
        label_3->setPixmap(QPixmap(QString::fromUtf8(":/images/interface/player/point.png")));

        horizontalLayout_7->addWidget(label_3);

        label_5 = new QLabel(frame_5);
        label_5->setObjectName(QString::fromUtf8("label_5"));

        horizontalLayout_7->addWidget(label_5);

        player_informations_pseudo = new QLabel(frame_5);
        player_informations_pseudo->setObjectName(QString::fromUtf8("player_informations_pseudo"));

        horizontalLayout_7->addWidget(player_informations_pseudo);

        horizontalSpacer_6 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_7->addItem(horizontalSpacer_6);


        verticalLayout_7->addLayout(horizontalLayout_7);

        horizontalLayout_8 = new QHBoxLayout();
        horizontalLayout_8->setSpacing(6);
        horizontalLayout_8->setObjectName(QString::fromUtf8("horizontalLayout_8"));
        label_6 = new QLabel(frame_5);
        label_6->setObjectName(QString::fromUtf8("label_6"));
        label_6->setMinimumSize(QSize(8, 8));
        label_6->setMaximumSize(QSize(8, 8));
        label_6->setPixmap(QPixmap(QString::fromUtf8(":/images/interface/player/point.png")));

        horizontalLayout_8->addWidget(label_6);

        label_7 = new QLabel(frame_5);
        label_7->setObjectName(QString::fromUtf8("label_7"));

        horizontalLayout_8->addWidget(label_7);

        player_informations_cash = new QLabel(frame_5);
        player_informations_cash->setObjectName(QString::fromUtf8("player_informations_cash"));

        horizontalLayout_8->addWidget(player_informations_cash);

        horizontalSpacer_7 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_8->addItem(horizontalSpacer_7);


        verticalLayout_7->addLayout(horizontalLayout_8);

        verticalSpacer = new QSpacerItem(20, 43, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_7->addItem(verticalSpacer);


        horizontalLayout_5->addWidget(frame_5);

        frame_6 = new QFrame(frame_2);
        frame_6->setObjectName(QString::fromUtf8("frame_6"));
        horizontalLayout_13 = new QHBoxLayout(frame_6);
        horizontalLayout_13->setSpacing(6);
        horizontalLayout_13->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_13->setObjectName(QString::fromUtf8("horizontalLayout_13"));
        player_informations_front = new QLabel(frame_6);
        player_informations_front->setObjectName(QString::fromUtf8("player_informations_front"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy2.setHorizontalStretch(150);
        sizePolicy2.setVerticalStretch(100);
        sizePolicy2.setHeightForWidth(player_informations_front->sizePolicy().hasHeightForWidth());
        player_informations_front->setSizePolicy(sizePolicy2);
        player_informations_front->setMinimumSize(QSize(150, 100));
        player_informations_front->setStyleSheet(QString::fromUtf8("background-image: url(:/images/interface/player/ball.png);\n"
"background-repeat:no-repeat;\n"
"background-position:center; "));

        horizontalLayout_13->addWidget(player_informations_front);


        horizontalLayout_5->addWidget(frame_6);


        verticalLayout_6->addLayout(horizontalLayout_5);

        frame_4 = new QFrame(frame_2);
        frame_4->setObjectName(QString::fromUtf8("frame_4"));
        frame_4->setMinimumSize(QSize(0, 25));
        frame_4->setMaximumSize(QSize(16777215, 25));
        frame_4->setStyleSheet(QString::fromUtf8("background-image: url(:/images/interface/player/bande.png);\n"
""));
        horizontalLayout_4 = new QHBoxLayout(frame_4);
        horizontalLayout_4->setSpacing(6);
        horizontalLayout_4->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        label_4 = new QLabel(frame_4);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setMinimumSize(QSize(0, 16));
        label_4->setMaximumSize(QSize(16777215, 16));
        label_4->setStyleSheet(QString::fromUtf8("background-image: url(:/images/empty.png);"));

        horizontalLayout_4->addWidget(label_4);

        horizontalSpacer_5 = new QSpacerItem(345, 14, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_5);


        verticalLayout_6->addWidget(frame_4);


        gridLayout_2->addWidget(frame_2, 0, 0, 1, 1);


        horizontalLayout_12->addWidget(frame);

        horizontalSpacer_9 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_12->addItem(horizontalSpacer_9);


        verticalLayout_8->addLayout(horizontalLayout_12);

        verticalSpacer_2 = new QSpacerItem(20, 175, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_8->addItem(verticalSpacer_2);

        horizontalLayout_9 = new QHBoxLayout();
        horizontalLayout_9->setSpacing(6);
        horizontalLayout_9->setObjectName(QString::fromUtf8("horizontalLayout_9"));
        horizontalSpacer_8 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_9->addItem(horizontalSpacer_8);

        toolButton_quit_interface = new QToolButton(page_4);
        toolButton_quit_interface->setObjectName(QString::fromUtf8("toolButton_quit_interface"));
        toolButton_quit_interface->setMinimumSize(QSize(22, 25));
        toolButton_quit_interface->setMaximumSize(QSize(22, 25));
        toolButton_quit_interface->setStyleSheet(QString::fromUtf8("background-image: url(:/images/interface/quit-1.1.png);\n"
"border-image: url(:/images/empty.png);"));

        horizontalLayout_9->addWidget(toolButton_quit_interface);


        verticalLayout_8->addLayout(horizontalLayout_9);

        stackedWidget->addWidget(page_4);

        horizontalLayout->addWidget(stackedWidget);


        horizontalLayout_2->addLayout(horizontalLayout);


        retranslateUi(BaseWindowUI);

        stackedWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(BaseWindowUI);
    } // setupUi

    void retranslateUi(QWidget *BaseWindowUI)
    {
        BaseWindowUI->setWindowTitle(QApplication::translate("BaseWindowUI", "Pokecraft", 0, QApplication::UnicodeUTF8));
        lineEdit_chat_text->setPlaceholderText(QApplication::translate("BaseWindowUI", "Your message", 0, QApplication::UnicodeUTF8));
        comboBox_chat_type->clear();
        comboBox_chat_type->insertItems(0, QStringList()
         << QApplication::translate("BaseWindowUI", "All", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("BaseWindowUI", "Clan", 0, QApplication::UnicodeUTF8)
        );
        pushButton_interface_bag->setText(QApplication::translate("BaseWindowUI", "Bag", 0, QApplication::UnicodeUTF8));
        pushButton_interface_monster_list->setText(QApplication::translate("BaseWindowUI", "Monstropedia", 0, QApplication::UnicodeUTF8));
        pushButton_interface_trainer->setText(QApplication::translate("BaseWindowUI", "Player", 0, QApplication::UnicodeUTF8));
        toolButton_interface_map->setText(QString());
        toolButton_interface_quit->setText(QApplication::translate("BaseWindowUI", "...", 0, QApplication::UnicodeUTF8));
        label_interface_number_of_player->setText(QApplication::translate("BaseWindowUI", "?/?", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("BaseWindowUI", "Trainer card", 0, QApplication::UnicodeUTF8));
        player_informations_id->setText(QApplication::translate("BaseWindowUI", "N\302\260ID/%1", 0, QApplication::UnicodeUTF8));
        label_5->setText(QApplication::translate("BaseWindowUI", "Nom: ", 0, QApplication::UnicodeUTF8));
        label_7->setText(QApplication::translate("BaseWindowUI", "Cash: ", 0, QApplication::UnicodeUTF8));
        label_4->setText(QApplication::translate("BaseWindowUI", "Badges:", 0, QApplication::UnicodeUTF8));
        toolButton_quit_interface->setText(QApplication::translate("BaseWindowUI", "...", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class BaseWindowUI: public Ui_BaseWindowUI {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_BASEWINDOW_H
