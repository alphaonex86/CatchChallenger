/********************************************************************************
** Form generated from reading UI file 'BaseWindow.ui'
**
** Created: Fri Apr 26 14:18:41 2013
**      by: Qt User Interface Compiler version 4.8.4
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_BASEWINDOW_H
#define UI_BASEWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QFormLayout>
#include <QtGui/QFrame>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QListWidget>
#include <QtGui/QProgressBar>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QSpinBox>
#include <QtGui/QStackedWidget>
#include <QtGui/QTabWidget>
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
    QWidget *page_init;
    QVBoxLayout *verticalLayout_2;
    QSpacerItem *verticalSpacer_loading_4;
    QHBoxLayout *horizontalLayout_loading_7;
    QSpacerItem *horizontalSpacer_loading_9;
    QLabel *label_connecting_status;
    QSpacerItem *horizontalSpacer_loading_10;
    QWidget *page_map;
    QVBoxLayout *verticalLayout_4;
    QHBoxLayout *horizontalLayout_16;
    QSpacerItem *horizontalSpacer_13;
    QLabel *gain;
    QSpacerItem *horizontalSpacer_14;
    QSpacerItem *verticalSpacer_5;
    QHBoxLayout *horizontalLayout_18;
    QSpacerItem *horizontalSpacer_11;
    QFrame *IG_dialog;
    QVBoxLayout *verticalLayout_5;
    QHBoxLayout *horizontalLayout_19;
    QSpacerItem *horizontalSpacer_17;
    QToolButton *close_IG_dialog;
    QSpacerItem *verticalSpacer_9;
    QLabel *IG_dialog_text;
    QSpacerItem *verticalSpacer_10;
    QSpacerItem *horizontalSpacer_12;
    QSpacerItem *verticalSpacer_8;
    QHBoxLayout *horizontalLayout_persistant_tip;
    QSpacerItem *horizontalSpacer_18;
    QLabel *tip;
    QSpacerItem *horizontalSpacer_19;
    QHBoxLayout *horizontalLayout_17;
    QSpacerItem *horizontalSpacer_15;
    QLabel *persistant_tip;
    QSpacerItem *horizontalSpacer_16;
    QFrame *frame_main_display_bottom;
    QHBoxLayout *horizontalLayout_10;
    QPushButton *pushButton_interface_bag;
    QPushButton *pushButton_interface_monster_list;
    QPushButton *pushButton_interface_trainer;
    QPushButton *pushButton_interface_crafting;
    QPushButton *pushButton_interface_monsters;
    QToolButton *toolButtonOptions;
    QToolButton *toolButton_interface_map;
    QSpacerItem *horizontalSpacer_main_display_11;
    QToolButton *toolButton_interface_quit;
    QFrame *frame_main_display_interface_player;
    QHBoxLayout *horizontalLayout_11;
    QLabel *label_interface_player_img;
    QLabel *label_interface_number_of_player;
    QWidget *page_player;
    QVBoxLayout *verticalLayout_8;
    QSpacerItem *verticalSpacer_3;
    QHBoxLayout *horizontalLayout_12;
    QSpacerItem *horizontalSpacer_10;
    QTabWidget *tabWidgetTrainerCard;
    QWidget *tabWidgetTrainerCardPage1;
    QGridLayout *gridLayout_2;
    QFrame *frame_2;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_3;
    QSpacerItem *horizontalSpacer_2;
    QLabel *label_2;
    QSpacerItem *horizontalSpacer_3;
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
    QLabel *player_informations_front;
    QHBoxLayout *horizontalLayout_4;
    QLabel *label_4;
    QLabel *label_badges;
    QSpacerItem *horizontalSpacer_5;
    QWidget *tabWidgetTrainerCardPage2;
    QHBoxLayout *horizontalLayout_29;
    QLabel *labelReputation;
    QWidget *tabWidgetTrainerCardPage3;
    QHBoxLayout *horizontalLayout_36;
    QListWidget *questsList;
    QTextBrowser *questDetails;
    QSpacerItem *horizontalSpacer_9;
    QSpacerItem *verticalSpacer_2;
    QHBoxLayout *horizontalLayout_9;
    QSpacerItem *horizontalSpacer_8;
    QToolButton *toolButton_quit_interface;
    QWidget *page_inventory;
    QVBoxLayout *verticalLayout_3;
    QFrame *inventory_layout;
    QLabel *inventory_image;
    QLabel *label_8;
    QLabel *label_9;
    QLabel *inventory_description;
    QListWidget *inventory;
    QToolButton *toolButton_quit_inventory;
    QLabel *inventory_name;
    QFrame *frame_3;
    QVBoxLayout *verticalLayout_12;
    QPushButton *inventoryInformation;
    QPushButton *inventoryUse;
    QPushButton *inventoryDestroy;
    QSpacerItem *verticalSpacer_11;
    QWidget *page_options;
    QVBoxLayout *verticalLayout_13;
    QGridLayout *gridLayout;
    QGroupBox *groupBox_Options_Displayer;
    QVBoxLayout *verticalLayout_6;
    QHBoxLayout *horizontalLayout_6;
    QCheckBox *checkBoxLimitFPS;
    QSpinBox *spinBoxMaxFPS;
    QCheckBox *checkBoxZoom;
    QSpacerItem *verticalSpacer_4;
    QGroupBox *groupBox_Options_Multiplayer;
    QVBoxLayout *verticalLayout_10;
    QCheckBox *checkBoxShowPseudo;
    QCheckBox *checkBoxShowPlayerType;
    QSpacerItem *verticalSpacer_6;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout_11;
    QHBoxLayout *horizontalLayout_15;
    QLabel *label;
    QSpacerItem *horizontalSpacer_4;
    QComboBox *comboBoxLanguage;
    QSpacerItem *verticalSpacer_13;
    QGroupBox *groupBox_2;
    QVBoxLayout *verticalLayout_9;
    QHBoxLayout *horizontalLayout_13;
    QLabel *label_11;
    QSpacerItem *horizontalSpacer_20;
    QLabel *labelInput;
    QHBoxLayout *horizontalLayout_20;
    QLabel *label_12;
    QSpacerItem *horizontalSpacer_21;
    QLabel *labelOutput;
    QSpacerItem *verticalSpacer_12;
    QSpacerItem *verticalSpacer_7;
    QHBoxLayout *horizontalLayout_14;
    QSpacerItem *horizontalSpacer;
    QToolButton *toolButton_quit_options;
    QWidget *page_plants;
    QFrame *framePlantsLeft;
    QLabel *labelPlantImage;
    QLabel *labelPlantName;
    QLabel *labelPlantDescription;
    QWidget *formLayoutWidget;
    QFormLayout *formLayout;
    QLabel *labelPlantFruitImage;
    QLabel *labelPlantFruitText;
    QLabel *labelFruitsImage;
    QLabel *labelFruitsText;
    QLabel *labelFloweringImage;
    QLabel *labelFloweringText;
    QWidget *formLayoutWidget_2;
    QFormLayout *formLayout_2;
    QLabel *labelPlantedImage;
    QLabel *labelPlantedText;
    QLabel *labelSproutedImage;
    QLabel *labelSproutedText;
    QLabel *labelTallerImage;
    QLabel *labelTallerText;
    QPushButton *plantUse;
    QFrame *framePlantsRight;
    QLabel *labelPlantInventoryTitle;
    QListWidget *listPlantList;
    QToolButton *toolButton_quit_plants;
    QWidget *page_crafting;
    QFrame *frameCraftingLeft;
    QLabel *labelCraftingImage;
    QLabel *labelCraftingDetails;
    QLabel *label_13;
    QListWidget *listCraftingMaterials;
    QPushButton *craftingUse;
    QFrame *frameCraftingRight;
    QListWidget *listCraftingList;
    QLabel *label_10;
    QToolButton *toolButton_quit_crafting;
    QWidget *page_shop;
    QVBoxLayout *verticalLayout_14;
    QFrame *shop_layout;
    QLabel *label_14;
    QLabel *shopImage;
    QLabel *shopDescription;
    QLabel *shopSellerImage;
    QLabel *shopName;
    QToolButton *toolButton_quit_shop;
    QListWidget *shopItemList;
    QPushButton *shopBuy;
    QLabel *shopCash;
    QWidget *page_battle;
    QStackedWidget *stackedWidgetFightBottomBar;
    QWidget *stackedWidgetFightBottomBarPageEnter;
    QHBoxLayout *horizontalLayout_22;
    QLabel *labelFightEnter;
    QSpacerItem *horizontalSpacer_23;
    QPushButton *pushButtonFightEnterNext;
    QWidget *stackedWidgetFightBottomBarPageMain;
    QGridLayout *gridLayout_3;
    QLabel *label_15;
    QVBoxLayout *verticalLayout_16;
    QSpacerItem *verticalSpacer_15;
    QPushButton *pushButtonFightAttack;
    QPushButton *pushButtonFightMonster;
    QPushButton *pushButtonFightBag;
    QSpacerItem *verticalSpacer_14;
    QToolButton *toolButtonFightQuit;
    QWidget *stackedWidgetFightBottomBarPageAttack;
    QHBoxLayout *horizontalLayout_23;
    QListWidget *listWidgetFightAttack;
    QLabel *labelFightAttackDetails;
    QVBoxLayout *verticalLayout_17;
    QSpacerItem *verticalSpacer_16;
    QPushButton *pushButtonFightAttackConfirmed;
    QPushButton *pushButtonFightReturn;
    QFrame *frameFightBackground;
    QLabel *labelFightPlateformBottom;
    QLabel *labelFightPlateformTop;
    QLabel *labelFightMonsterBottom;
    QLabel *labelFightMonsterTop;
    QFrame *frameFightTop;
    QVBoxLayout *verticalLayout_18;
    QHBoxLayout *horizontalLayout_25;
    QLabel *labelFightTopName;
    QSpacerItem *horizontalSpacer_24;
    QLabel *labelFightTopLevel;
    QHBoxLayout *horizontalLayout_24;
    QLabel *label_18;
    QProgressBar *progressBarFightTopHP;
    QSpacerItem *verticalSpacer_17;
    QFrame *frameFightBottom;
    QVBoxLayout *verticalLayout_19;
    QHBoxLayout *horizontalLayout_28;
    QLabel *labelFightBottomName;
    QSpacerItem *horizontalSpacer_25;
    QLabel *labelFightBottomLevel;
    QHBoxLayout *horizontalLayout_27;
    QLabel *label_21;
    QProgressBar *progressBarFightBottomHP;
    QLabel *labelFightBottomHP;
    QHBoxLayout *horizontalLayout_26;
    QLabel *label_23;
    QProgressBar *progressBarFightBottopExp;
    QSpacerItem *verticalSpacer_18;
    QWidget *page_monster;
    QVBoxLayout *verticalLayout_15;
    QListWidget *monsterList;
    QHBoxLayout *horizontalLayout_21;
    QSpacerItem *horizontalSpacer_22;
    QPushButton *selectMonster;
    QSpacerItem *horizontalSpacer_31;
    QToolButton *toolButton_monster_list_quit;
    QWidget *page_bioscan;
    QToolButton *toolButton_bioscan_quit;
    QWidget *page_trade;
    QVBoxLayout *verticalLayout_22;
    QHBoxLayout *horizontalLayout_33;
    QVBoxLayout *verticalLayout_20;
    QLabel *tradePlayerImage;
    QLabel *tradePlayerPseudo;
    QHBoxLayout *horizontalLayout_30;
    QLabel *label_22;
    QSpinBox *tradePlayerCash;
    QSpacerItem *horizontalSpacer_26;
    QListWidget *tradePlayerItems;
    QPushButton *tradeAddItem;
    QListWidget *tradePlayerMonsters;
    QPushButton *tradeAddMonster;
    QHBoxLayout *horizontalLayout_32;
    QSpacerItem *horizontalSpacer_28;
    QPushButton *tradeValidate;
    QSpacerItem *horizontalSpacer_29;
    QVBoxLayout *verticalLayout_21;
    QLabel *tradeOtherImage;
    QLabel *tradeOtherPseudo;
    QHBoxLayout *horizontalLayout_31;
    QLabel *label_24;
    QSpinBox *tradeOtherCash;
    QSpacerItem *horizontalSpacer_27;
    QListWidget *tradeOtherItems;
    QListWidget *tradeOtherMonsters;
    QLabel *tradeOtherStat;
    QHBoxLayout *horizontalLayout_34;
    QSpacerItem *horizontalSpacer_30;
    QToolButton *tradeCancel;
    QWidget *page_learn;
    QHBoxLayout *horizontalLayout_35;
    QFrame *learn_layout;
    QToolButton *learnQuit;
    QLabel *label_16;
    QLabel *learnDescription;
    QPushButton *learnValidate;
    QLabel *learnMonster;
    QListWidget *learnAttackList;
    QLabel *learnSP;

    void setupUi(QWidget *BaseWindowUI)
    {
        if (BaseWindowUI->objectName().isEmpty())
            BaseWindowUI->setObjectName(QString::fromUtf8("BaseWindowUI"));
        BaseWindowUI->resize(800, 600);
        BaseWindowUI->setMinimumSize(QSize(800, 600));
        BaseWindowUI->setMaximumSize(QSize(800, 600));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/images/catchchallenger.png"), QSize(), QIcon::Normal, QIcon::Off);
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
        stackedWidget->setStyleSheet(QString::fromUtf8("#page_bioscan{background-image: url(:/images/interface/player/background.png);}"));
        page_init = new QWidget();
        page_init->setObjectName(QString::fromUtf8("page_init"));
        page_init->setStyleSheet(QString::fromUtf8("#page_init{background-image: url(:/images/background.png);}"));
        verticalLayout_2 = new QVBoxLayout(page_init);
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

        label_connecting_status = new QLabel(page_init);
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

        stackedWidget->addWidget(page_init);
        page_map = new QWidget();
        page_map->setObjectName(QString::fromUtf8("page_map"));
        verticalLayout_4 = new QVBoxLayout(page_map);
        verticalLayout_4->setSpacing(0);
        verticalLayout_4->setContentsMargins(0, 0, 0, 0);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        horizontalLayout_16 = new QHBoxLayout();
        horizontalLayout_16->setSpacing(6);
        horizontalLayout_16->setObjectName(QString::fromUtf8("horizontalLayout_16"));
        horizontalSpacer_13 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_16->addItem(horizontalSpacer_13);

        gain = new QLabel(page_map);
        gain->setObjectName(QString::fromUtf8("gain"));
        gain->setStyleSheet(QString::fromUtf8("#gain {\n"
"background-color: qlineargradient(spread:pad, x1:0, y1:0.3, x2:0, y2:1, stop:0 rgba(255, 255, 255, 150), stop:1 rgba(221, 221, 221, 150));\n"
"border-color: rgb(204, 204, 204);\n"
"border-radius: 5px;\n"
"padding:5px;\n"
"}"));

        horizontalLayout_16->addWidget(gain);

        horizontalSpacer_14 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_16->addItem(horizontalSpacer_14);


        verticalLayout_4->addLayout(horizontalLayout_16);

        verticalSpacer_5 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_4->addItem(verticalSpacer_5);

        horizontalLayout_18 = new QHBoxLayout();
        horizontalLayout_18->setSpacing(6);
        horizontalLayout_18->setObjectName(QString::fromUtf8("horizontalLayout_18"));
        horizontalSpacer_11 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_18->addItem(horizontalSpacer_11);

        IG_dialog = new QFrame(page_map);
        IG_dialog->setObjectName(QString::fromUtf8("IG_dialog"));
        IG_dialog->setMinimumSize(QSize(100, 100));
        IG_dialog->setStyleSheet(QString::fromUtf8("#IG_dialog {\n"
"background-color: qlineargradient(spread:pad, x1:0, y1:0.3, x2:0, y2:1, stop:0 rgba(255, 255, 255, 200), stop:1 rgba(221, 221, 221, 200));\n"
"border-color: rgb(204, 204, 204);\n"
"border-radius: 5px;\n"
"}"));
        verticalLayout_5 = new QVBoxLayout(IG_dialog);
        verticalLayout_5->setSpacing(2);
        verticalLayout_5->setContentsMargins(2, 2, 2, 2);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        horizontalLayout_19 = new QHBoxLayout();
        horizontalLayout_19->setSpacing(6);
        horizontalLayout_19->setObjectName(QString::fromUtf8("horizontalLayout_19"));
        horizontalSpacer_17 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_19->addItem(horizontalSpacer_17);

        close_IG_dialog = new QToolButton(IG_dialog);
        close_IG_dialog->setObjectName(QString::fromUtf8("close_IG_dialog"));
        close_IG_dialog->setMinimumSize(QSize(22, 25));
        close_IG_dialog->setMaximumSize(QSize(22, 25));
        close_IG_dialog->setStyleSheet(QString::fromUtf8("QToolButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QToolButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QToolButton { \n"
"background-image: url(:/images/interface/quit.png);\n"
"border-image: url(:/images/empty.png);\n"
"}"));

        horizontalLayout_19->addWidget(close_IG_dialog);


        verticalLayout_5->addLayout(horizontalLayout_19);

        verticalSpacer_9 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_5->addItem(verticalSpacer_9);

        IG_dialog_text = new QLabel(IG_dialog);
        IG_dialog_text->setObjectName(QString::fromUtf8("IG_dialog_text"));
        IG_dialog_text->setTextFormat(Qt::RichText);

        verticalLayout_5->addWidget(IG_dialog_text);

        verticalSpacer_10 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_5->addItem(verticalSpacer_10);


        horizontalLayout_18->addWidget(IG_dialog);

        horizontalSpacer_12 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_18->addItem(horizontalSpacer_12);


        verticalLayout_4->addLayout(horizontalLayout_18);

        verticalSpacer_8 = new QSpacerItem(20, 221, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_4->addItem(verticalSpacer_8);

        horizontalLayout_persistant_tip = new QHBoxLayout();
        horizontalLayout_persistant_tip->setSpacing(6);
        horizontalLayout_persistant_tip->setObjectName(QString::fromUtf8("horizontalLayout_persistant_tip"));
        horizontalSpacer_18 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_persistant_tip->addItem(horizontalSpacer_18);

        tip = new QLabel(page_map);
        tip->setObjectName(QString::fromUtf8("tip"));
        tip->setStyleSheet(QString::fromUtf8("#tip { background-color: qlineargradient(spread:pad, x1:0, y1:0.3, x2:0, y2:1, stop:0 rgba(255, 255, 255, 150), stop:1 rgba(221, 221, 221, 150));\n"
"border-color: rgb(204, 204, 204);\n"
"border-radius: 5px;\n"
"padding:5px;\n"
"}"));

        horizontalLayout_persistant_tip->addWidget(tip);

        horizontalSpacer_19 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_persistant_tip->addItem(horizontalSpacer_19);


        verticalLayout_4->addLayout(horizontalLayout_persistant_tip);

        horizontalLayout_17 = new QHBoxLayout();
        horizontalLayout_17->setSpacing(6);
        horizontalLayout_17->setObjectName(QString::fromUtf8("horizontalLayout_17"));
        horizontalSpacer_15 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_17->addItem(horizontalSpacer_15);

        persistant_tip = new QLabel(page_map);
        persistant_tip->setObjectName(QString::fromUtf8("persistant_tip"));
        persistant_tip->setStyleSheet(QString::fromUtf8("#persistant_tip { background-color: qlineargradient(spread:pad, x1:0, y1:0.3, x2:0, y2:1, stop:0 rgba(255, 255, 255, 150), stop:1 rgba(221, 221, 221, 150));\n"
"border-color: rgb(204, 204, 204);\n"
"border-radius: 5px;\n"
"padding:5px;\n"
"}"));

        horizontalLayout_17->addWidget(persistant_tip);

        horizontalSpacer_16 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_17->addItem(horizontalSpacer_16);


        verticalLayout_4->addLayout(horizontalLayout_17);

        frame_main_display_bottom = new QFrame(page_map);
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
"background-image: url(:/images/interface/option-bioscan.png);\n"
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

        pushButton_interface_crafting = new QPushButton(frame_main_display_bottom);
        pushButton_interface_crafting->setObjectName(QString::fromUtf8("pushButton_interface_crafting"));
        pushButton_interface_crafting->setMinimumSize(QSize(110, 25));
        pushButton_interface_crafting->setMaximumSize(QSize(110, 25));
        pushButton_interface_crafting->setStyleSheet(QString::fromUtf8("QPushButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QPushButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QPushButton { \n"
"background-image: url(:/images/interface/option-craft.png);\n"
"border-image: url(:/images/empty.png);\n"
"color: rgb(255, 255, 255);\n"
"padding-left:25px;\n"
"padding-right:4px;\n"
"padding-top:8px;\n"
"padding-bottom:3px;\n"
"}"));

        horizontalLayout_10->addWidget(pushButton_interface_crafting);

        pushButton_interface_monsters = new QPushButton(frame_main_display_bottom);
        pushButton_interface_monsters->setObjectName(QString::fromUtf8("pushButton_interface_monsters"));
        pushButton_interface_monsters->setMinimumSize(QSize(110, 25));
        pushButton_interface_monsters->setMaximumSize(QSize(110, 25));
        pushButton_interface_monsters->setStyleSheet(QString::fromUtf8("QPushButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QPushButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QPushButton { \n"
"	background-image: url(:/images/interface/button-generic.png);\n"
"border-image: url(:/images/empty.png);\n"
"color: rgb(255, 255, 255);\n"
"padding-left:25px;\n"
"padding-right:4px;\n"
"padding-top:8px;\n"
"padding-bottom:3px;\n"
"}"));

        horizontalLayout_10->addWidget(pushButton_interface_monsters);

        toolButtonOptions = new QToolButton(frame_main_display_bottom);
        toolButtonOptions->setObjectName(QString::fromUtf8("toolButtonOptions"));
        toolButtonOptions->setMinimumSize(QSize(22, 25));
        toolButtonOptions->setMaximumSize(QSize(22, 25));
        toolButtonOptions->setStyleSheet(QString::fromUtf8("QToolButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QToolButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QToolButton { \n"
"background-image: url(:/images/interface/option-options.png);\n"
"border-image: url(:/images/empty.png);\n"
"}"));

        horizontalLayout_10->addWidget(toolButtonOptions);

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
        frame_main_display_interface_player->setStyleSheet(QString::fromUtf8("#frame_main_display_interface_player\n"
"{\n"
"border: 1px solid #343434;\n"
"background-image: url(:/images/light-white.png);\n"
"}"));
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

        stackedWidget->addWidget(page_map);
        page_player = new QWidget();
        page_player->setObjectName(QString::fromUtf8("page_player"));
        page_player->setStyleSheet(QString::fromUtf8("#page_player{background-image: url(:/images/interface/player/background.png);}"));
        verticalLayout_8 = new QVBoxLayout(page_player);
        verticalLayout_8->setSpacing(0);
        verticalLayout_8->setContentsMargins(3, 3, 3, 3);
        verticalLayout_8->setObjectName(QString::fromUtf8("verticalLayout_8"));
        verticalSpacer_3 = new QSpacerItem(20, 175, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_8->addItem(verticalSpacer_3);

        horizontalLayout_12 = new QHBoxLayout();
        horizontalLayout_12->setSpacing(6);
        horizontalLayout_12->setObjectName(QString::fromUtf8("horizontalLayout_12"));
        horizontalSpacer_10 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_12->addItem(horizontalSpacer_10);

        tabWidgetTrainerCard = new QTabWidget(page_player);
        tabWidgetTrainerCard->setObjectName(QString::fromUtf8("tabWidgetTrainerCard"));
        tabWidgetTrainerCard->setStyleSheet(QString::fromUtf8("#tabWidgetTrainerCardPage2{\n"
"background-color: qlineargradient(spread:pad, x1:0, y1:0.3, x2:0, y2:1, stop:0 rgba(255, 255, 255, 200), stop:1 rgba(221, 221, 221, 200));\n"
"border-color: rgb(204, 204, 204);\n"
"border-radius: 5px;\n"
"}"));
        tabWidgetTrainerCardPage1 = new QWidget();
        tabWidgetTrainerCardPage1->setObjectName(QString::fromUtf8("tabWidgetTrainerCardPage1"));
        tabWidgetTrainerCardPage1->setStyleSheet(QString::fromUtf8("#tabWidgetTrainerCardPage1{\n"
"background-color: qlineargradient(spread:pad, x1:0, y1:0.3, x2:0, y2:1, stop:0 rgba(255, 255, 255, 200), stop:1 rgba(221, 221, 221, 200));\n"
"border-color: rgb(204, 204, 204);\n"
"border-radius: 5px;\n"
"}"));
        gridLayout_2 = new QGridLayout(tabWidgetTrainerCardPage1);
        gridLayout_2->setSpacing(6);
        gridLayout_2->setContentsMargins(11, 11, 11, 11);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        frame_2 = new QFrame(tabWidgetTrainerCardPage1);
        frame_2->setObjectName(QString::fromUtf8("frame_2"));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(frame_2->sizePolicy().hasHeightForWidth());
        frame_2->setSizePolicy(sizePolicy);
        frame_2->setMinimumSize(QSize(415, 174));
        frame_2->setMaximumSize(QSize(415, 174));
        verticalLayout = new QVBoxLayout(frame_2);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(6);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalSpacer_2 = new QSpacerItem(88, 14, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_2);

        label_2 = new QLabel(frame_2);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setMinimumSize(QSize(0, 16));
        label_2->setMaximumSize(QSize(16777215, 16));
        label_2->setStyleSheet(QString::fromUtf8("background-color: rgb(104, 184, 248);\n"
"border: 1px solid rgb(64, 136, 192);"));

        horizontalLayout_3->addWidget(label_2);

        horizontalSpacer_3 = new QSpacerItem(87, 14, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_3);


        verticalLayout->addLayout(horizontalLayout_3);

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
        player_informations_pseudo->setMinimumSize(QSize(150, 0));
        player_informations_pseudo->setMaximumSize(QSize(150, 16777215));

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
        player_informations_cash->setMinimumSize(QSize(150, 0));
        player_informations_cash->setMaximumSize(QSize(150, 16777215));

        horizontalLayout_8->addWidget(player_informations_cash);

        horizontalSpacer_7 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_8->addItem(horizontalSpacer_7);


        verticalLayout_7->addLayout(horizontalLayout_8);

        verticalSpacer = new QSpacerItem(20, 43, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_7->addItem(verticalSpacer);


        horizontalLayout_5->addWidget(frame_5);

        player_informations_front = new QLabel(frame_2);
        player_informations_front->setObjectName(QString::fromUtf8("player_informations_front"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(150);
        sizePolicy1.setVerticalStretch(100);
        sizePolicy1.setHeightForWidth(player_informations_front->sizePolicy().hasHeightForWidth());
        player_informations_front->setSizePolicy(sizePolicy1);
        player_informations_front->setMinimumSize(QSize(160, 100));
        player_informations_front->setMaximumSize(QSize(160, 100));
        player_informations_front->setStyleSheet(QString::fromUtf8("background-image: url(:/images/interface/player/ball.png);\n"
"background-repeat:no-repeat;\n"
"background-position:center; "));
        player_informations_front->setPixmap(QPixmap(QString::fromUtf8(":/images/player_default/front.png")));
        player_informations_front->setAlignment(Qt::AlignCenter);

        horizontalLayout_5->addWidget(player_informations_front);


        verticalLayout->addLayout(horizontalLayout_5);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setSpacing(6);
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        label_4 = new QLabel(frame_2);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setMinimumSize(QSize(0, 16));
        label_4->setMaximumSize(QSize(16777215, 16));
        label_4->setStyleSheet(QString::fromUtf8("background-image: url(:/images/empty.png);"));

        horizontalLayout_4->addWidget(label_4);

        label_badges = new QLabel(frame_2);
        label_badges->setObjectName(QString::fromUtf8("label_badges"));

        horizontalLayout_4->addWidget(label_badges);

        horizontalSpacer_5 = new QSpacerItem(345, 14, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_5);


        verticalLayout->addLayout(horizontalLayout_4);


        gridLayout_2->addWidget(frame_2, 0, 0, 1, 1);

        tabWidgetTrainerCard->addTab(tabWidgetTrainerCardPage1, QString());
        tabWidgetTrainerCardPage2 = new QWidget();
        tabWidgetTrainerCardPage2->setObjectName(QString::fromUtf8("tabWidgetTrainerCardPage2"));
        tabWidgetTrainerCardPage2->setStyleSheet(QString::fromUtf8("#tabWidgetTrainerCardPage2{\n"
"background-color: qlineargradient(spread:pad, x1:0, y1:0.3, x2:0, y2:1, stop:0 rgba(255, 255, 255, 200), stop:1 rgba(221, 221, 221, 200));\n"
"border-color: rgb(204, 204, 204);\n"
"border-radius: 5px;\n"
"}"));
        horizontalLayout_29 = new QHBoxLayout(tabWidgetTrainerCardPage2);
        horizontalLayout_29->setSpacing(6);
        horizontalLayout_29->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_29->setObjectName(QString::fromUtf8("horizontalLayout_29"));
        labelReputation = new QLabel(tabWidgetTrainerCardPage2);
        labelReputation->setObjectName(QString::fromUtf8("labelReputation"));
        labelReputation->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        horizontalLayout_29->addWidget(labelReputation);

        tabWidgetTrainerCard->addTab(tabWidgetTrainerCardPage2, QString());
        tabWidgetTrainerCardPage3 = new QWidget();
        tabWidgetTrainerCardPage3->setObjectName(QString::fromUtf8("tabWidgetTrainerCardPage3"));
        tabWidgetTrainerCardPage3->setStyleSheet(QString::fromUtf8("#tabWidgetTrainerCardPage3{\n"
"background-color: qlineargradient(spread:pad, x1:0, y1:0.3, x2:0, y2:1, stop:0 rgba(255, 255, 255, 200), stop:1 rgba(221, 221, 221, 200));\n"
"border-color: rgb(204, 204, 204);\n"
"border-radius: 5px;\n"
"}"));
        horizontalLayout_36 = new QHBoxLayout(tabWidgetTrainerCardPage3);
        horizontalLayout_36->setSpacing(6);
        horizontalLayout_36->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_36->setObjectName(QString::fromUtf8("horizontalLayout_36"));
        questsList = new QListWidget(tabWidgetTrainerCardPage3);
        questsList->setObjectName(QString::fromUtf8("questsList"));

        horizontalLayout_36->addWidget(questsList);

        questDetails = new QTextBrowser(tabWidgetTrainerCardPage3);
        questDetails->setObjectName(QString::fromUtf8("questDetails"));
        questDetails->setStyleSheet(QString::fromUtf8("background-image: url(:/images/empty.png);"));
        questDetails->setFrameShape(QFrame::NoFrame);

        horizontalLayout_36->addWidget(questDetails);

        tabWidgetTrainerCard->addTab(tabWidgetTrainerCardPage3, QString());

        horizontalLayout_12->addWidget(tabWidgetTrainerCard);

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

        toolButton_quit_interface = new QToolButton(page_player);
        toolButton_quit_interface->setObjectName(QString::fromUtf8("toolButton_quit_interface"));
        toolButton_quit_interface->setMinimumSize(QSize(22, 25));
        toolButton_quit_interface->setMaximumSize(QSize(22, 25));
        toolButton_quit_interface->setStyleSheet(QString::fromUtf8("QToolButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QToolButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QToolButton { \n"
"background-image: url(:/images/interface/quit.png);\n"
"border-image: url(:/images/empty.png);\n"
"}"));

        horizontalLayout_9->addWidget(toolButton_quit_interface);


        verticalLayout_8->addLayout(horizontalLayout_9);

        stackedWidget->addWidget(page_player);
        page_inventory = new QWidget();
        page_inventory->setObjectName(QString::fromUtf8("page_inventory"));
        page_inventory->setStyleSheet(QString::fromUtf8("#page_inventory{background-image: url(:/images/inventory/background.png);}\n"
""));
        verticalLayout_3 = new QVBoxLayout(page_inventory);
        verticalLayout_3->setSpacing(0);
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        inventory_layout = new QFrame(page_inventory);
        inventory_layout->setObjectName(QString::fromUtf8("inventory_layout"));
        inventory_layout->setStyleSheet(QString::fromUtf8("#inventory_layout{background-image: url(:/images/inventory/interface.png);}"));
        inventory_image = new QLabel(inventory_layout);
        inventory_image->setObjectName(QString::fromUtf8("inventory_image"));
        inventory_image->setGeometry(QRect(47, 477, 72, 72));
        inventory_image->setPixmap(QPixmap(QString::fromUtf8(":/images/inventory/unknow-object.png")));
        inventory_image->setAlignment(Qt::AlignCenter);
        label_8 = new QLabel(inventory_layout);
        label_8->setObjectName(QString::fromUtf8("label_8"));
        label_8->setGeometry(QRect(340, 5, 357, 51));
        QFont font1;
        font1.setPointSize(22);
        label_8->setFont(font1);
        label_8->setStyleSheet(QString::fromUtf8("background-image: url(:/images/inventory/type.png);"));
        label_8->setAlignment(Qt::AlignCenter);
        label_9 = new QLabel(inventory_layout);
        label_9->setObjectName(QString::fromUtf8("label_9"));
        label_9->setGeometry(QRect(40, 150, 140, 190));
        label_9->setPixmap(QPixmap(QString::fromUtf8(":/images/inventory/bag1.png")));
        inventory_description = new QLabel(inventory_layout);
        inventory_description->setObjectName(QString::fromUtf8("inventory_description"));
        inventory_description->setGeometry(QRect(150, 530, 491, 51));
        inventory_description->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        inventory = new QListWidget(inventory_layout);
        inventory->setObjectName(QString::fromUtf8("inventory"));
        inventory->setGeometry(QRect(250, 70, 531, 391));
        inventory->setStyleSheet(QString::fromUtf8("background-image: url(:/images/empty.png);"));
        inventory->setFrameShape(QFrame::NoFrame);
        inventory->setIconSize(QSize(48, 48));
        inventory->setMovement(QListView::Static);
        inventory->setViewMode(QListView::IconMode);
        toolButton_quit_inventory = new QToolButton(inventory_layout);
        toolButton_quit_inventory->setObjectName(QString::fromUtf8("toolButton_quit_inventory"));
        toolButton_quit_inventory->setGeometry(QRect(770, 560, 22, 25));
        toolButton_quit_inventory->setMinimumSize(QSize(22, 25));
        toolButton_quit_inventory->setMaximumSize(QSize(22, 25));
        toolButton_quit_inventory->setStyleSheet(QString::fromUtf8("QToolButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QToolButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QToolButton { \n"
"background-image: url(:/images/interface/quit.png);\n"
"border-image: url(:/images/empty.png);\n"
"}"));
        inventory_name = new QLabel(inventory_layout);
        inventory_name->setObjectName(QString::fromUtf8("inventory_name"));
        inventory_name->setGeometry(QRect(150, 510, 491, 16));
        QFont font2;
        font2.setBold(true);
        font2.setWeight(75);
        inventory_name->setFont(font2);
        frame_3 = new QFrame(inventory_layout);
        frame_3->setObjectName(QString::fromUtf8("frame_3"));
        frame_3->setGeometry(QRect(650, 510, 111, 81));
        frame_3->setFrameShadow(QFrame::Raised);
        verticalLayout_12 = new QVBoxLayout(frame_3);
        verticalLayout_12->setSpacing(0);
        verticalLayout_12->setContentsMargins(0, 0, 0, 0);
        verticalLayout_12->setObjectName(QString::fromUtf8("verticalLayout_12"));
        inventoryInformation = new QPushButton(frame_3);
        inventoryInformation->setObjectName(QString::fromUtf8("inventoryInformation"));
        inventoryInformation->setMinimumSize(QSize(110, 25));
        inventoryInformation->setMaximumSize(QSize(110, 25));
        inventoryInformation->setStyleSheet(QString::fromUtf8("QPushButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QPushButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QPushButton { \n"
"background-image: url(:/images/interface/button-generic.png);\n"
"border-image: url(:/images/empty.png);\n"
"color: rgb(255, 255, 255);\n"
"padding-left:4px;\n"
"padding-right:4px;\n"
"padding-top:8px;\n"
"padding-bottom:3px;\n"
"}"));

        verticalLayout_12->addWidget(inventoryInformation);

        inventoryUse = new QPushButton(frame_3);
        inventoryUse->setObjectName(QString::fromUtf8("inventoryUse"));
        inventoryUse->setMinimumSize(QSize(110, 25));
        inventoryUse->setMaximumSize(QSize(110, 25));
        inventoryUse->setStyleSheet(QString::fromUtf8("QPushButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QPushButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QPushButton { \n"
"background-image: url(:/images/interface/button-generic.png);\n"
"border-image: url(:/images/empty.png);\n"
"color: rgb(255, 255, 255);\n"
"padding-left:4px;\n"
"padding-right:4px;\n"
"padding-top:8px;\n"
"padding-bottom:3px;\n"
"}"));

        verticalLayout_12->addWidget(inventoryUse);

        inventoryDestroy = new QPushButton(frame_3);
        inventoryDestroy->setObjectName(QString::fromUtf8("inventoryDestroy"));
        inventoryDestroy->setMinimumSize(QSize(110, 25));
        inventoryDestroy->setMaximumSize(QSize(110, 25));
        inventoryDestroy->setStyleSheet(QString::fromUtf8("QPushButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QPushButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QPushButton { \n"
"background-image: url(:/images/interface/button-generic.png);\n"
"border-image: url(:/images/empty.png);\n"
"color: rgb(255, 255, 255);\n"
"padding-left:4px;\n"
"padding-right:4px;\n"
"padding-top:8px;\n"
"padding-bottom:3px;\n"
"}"));

        verticalLayout_12->addWidget(inventoryDestroy);

        verticalSpacer_11 = new QSpacerItem(20, 3, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_12->addItem(verticalSpacer_11);


        verticalLayout_3->addWidget(inventory_layout);

        stackedWidget->addWidget(page_inventory);
        page_options = new QWidget();
        page_options->setObjectName(QString::fromUtf8("page_options"));
        page_options->setStyleSheet(QString::fromUtf8("#page_options{background-image: url(:/images/background.png);}"));
        verticalLayout_13 = new QVBoxLayout(page_options);
        verticalLayout_13->setSpacing(6);
        verticalLayout_13->setContentsMargins(11, 11, 11, 11);
        verticalLayout_13->setObjectName(QString::fromUtf8("verticalLayout_13"));
        gridLayout = new QGridLayout();
        gridLayout->setSpacing(6);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        groupBox_Options_Displayer = new QGroupBox(page_options);
        groupBox_Options_Displayer->setObjectName(QString::fromUtf8("groupBox_Options_Displayer"));
        groupBox_Options_Displayer->setStyleSheet(QString::fromUtf8("background-image: url(:/images/light-white.png);"));
        verticalLayout_6 = new QVBoxLayout(groupBox_Options_Displayer);
        verticalLayout_6->setSpacing(6);
        verticalLayout_6->setContentsMargins(11, 11, 11, 11);
        verticalLayout_6->setObjectName(QString::fromUtf8("verticalLayout_6"));
        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setSpacing(6);
        horizontalLayout_6->setObjectName(QString::fromUtf8("horizontalLayout_6"));
        checkBoxLimitFPS = new QCheckBox(groupBox_Options_Displayer);
        checkBoxLimitFPS->setObjectName(QString::fromUtf8("checkBoxLimitFPS"));
        checkBoxLimitFPS->setChecked(true);

        horizontalLayout_6->addWidget(checkBoxLimitFPS);

        spinBoxMaxFPS = new QSpinBox(groupBox_Options_Displayer);
        spinBoxMaxFPS->setObjectName(QString::fromUtf8("spinBoxMaxFPS"));
        spinBoxMaxFPS->setMaximumSize(QSize(50, 16777215));
        spinBoxMaxFPS->setMinimum(16);
        spinBoxMaxFPS->setValue(25);

        horizontalLayout_6->addWidget(spinBoxMaxFPS);


        verticalLayout_6->addLayout(horizontalLayout_6);

        checkBoxZoom = new QCheckBox(groupBox_Options_Displayer);
        checkBoxZoom->setObjectName(QString::fromUtf8("checkBoxZoom"));
        checkBoxZoom->setChecked(true);

        verticalLayout_6->addWidget(checkBoxZoom);

        verticalSpacer_4 = new QSpacerItem(20, 164, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_6->addItem(verticalSpacer_4);


        gridLayout->addWidget(groupBox_Options_Displayer, 0, 0, 1, 1);

        groupBox_Options_Multiplayer = new QGroupBox(page_options);
        groupBox_Options_Multiplayer->setObjectName(QString::fromUtf8("groupBox_Options_Multiplayer"));
        groupBox_Options_Multiplayer->setStyleSheet(QString::fromUtf8("background-image: url(:/images/light-white.png);"));
        verticalLayout_10 = new QVBoxLayout(groupBox_Options_Multiplayer);
        verticalLayout_10->setSpacing(6);
        verticalLayout_10->setContentsMargins(11, 11, 11, 11);
        verticalLayout_10->setObjectName(QString::fromUtf8("verticalLayout_10"));
        checkBoxShowPseudo = new QCheckBox(groupBox_Options_Multiplayer);
        checkBoxShowPseudo->setObjectName(QString::fromUtf8("checkBoxShowPseudo"));
        checkBoxShowPseudo->setEnabled(false);
        checkBoxShowPseudo->setChecked(false);

        verticalLayout_10->addWidget(checkBoxShowPseudo);

        checkBoxShowPlayerType = new QCheckBox(groupBox_Options_Multiplayer);
        checkBoxShowPlayerType->setObjectName(QString::fromUtf8("checkBoxShowPlayerType"));
        checkBoxShowPlayerType->setEnabled(false);
        checkBoxShowPlayerType->setChecked(false);

        verticalLayout_10->addWidget(checkBoxShowPlayerType);

        verticalSpacer_6 = new QSpacerItem(20, 167, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_10->addItem(verticalSpacer_6);


        gridLayout->addWidget(groupBox_Options_Multiplayer, 0, 1, 1, 1);

        groupBox = new QGroupBox(page_options);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        groupBox->setStyleSheet(QString::fromUtf8("background-image: url(:/images/light-white.png);"));
        verticalLayout_11 = new QVBoxLayout(groupBox);
        verticalLayout_11->setSpacing(6);
        verticalLayout_11->setContentsMargins(11, 11, 11, 11);
        verticalLayout_11->setObjectName(QString::fromUtf8("verticalLayout_11"));
        horizontalLayout_15 = new QHBoxLayout();
        horizontalLayout_15->setSpacing(6);
        horizontalLayout_15->setObjectName(QString::fromUtf8("horizontalLayout_15"));
        label = new QLabel(groupBox);
        label->setObjectName(QString::fromUtf8("label"));

        horizontalLayout_15->addWidget(label);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_15->addItem(horizontalSpacer_4);

        comboBoxLanguage = new QComboBox(groupBox);
        comboBoxLanguage->setObjectName(QString::fromUtf8("comboBoxLanguage"));

        horizontalLayout_15->addWidget(comboBoxLanguage);


        verticalLayout_11->addLayout(horizontalLayout_15);

        verticalSpacer_13 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_11->addItem(verticalSpacer_13);


        gridLayout->addWidget(groupBox, 1, 0, 1, 1);

        groupBox_2 = new QGroupBox(page_options);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        groupBox_2->setStyleSheet(QString::fromUtf8("background-image: url(:/images/light-white.png);"));
        verticalLayout_9 = new QVBoxLayout(groupBox_2);
        verticalLayout_9->setSpacing(6);
        verticalLayout_9->setContentsMargins(11, 11, 11, 11);
        verticalLayout_9->setObjectName(QString::fromUtf8("verticalLayout_9"));
        horizontalLayout_13 = new QHBoxLayout();
        horizontalLayout_13->setSpacing(6);
        horizontalLayout_13->setObjectName(QString::fromUtf8("horizontalLayout_13"));
        label_11 = new QLabel(groupBox_2);
        label_11->setObjectName(QString::fromUtf8("label_11"));

        horizontalLayout_13->addWidget(label_11);

        horizontalSpacer_20 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_13->addItem(horizontalSpacer_20);

        labelInput = new QLabel(groupBox_2);
        labelInput->setObjectName(QString::fromUtf8("labelInput"));

        horizontalLayout_13->addWidget(labelInput);


        verticalLayout_9->addLayout(horizontalLayout_13);

        horizontalLayout_20 = new QHBoxLayout();
        horizontalLayout_20->setSpacing(6);
        horizontalLayout_20->setObjectName(QString::fromUtf8("horizontalLayout_20"));
        label_12 = new QLabel(groupBox_2);
        label_12->setObjectName(QString::fromUtf8("label_12"));

        horizontalLayout_20->addWidget(label_12);

        horizontalSpacer_21 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_20->addItem(horizontalSpacer_21);

        labelOutput = new QLabel(groupBox_2);
        labelOutput->setObjectName(QString::fromUtf8("labelOutput"));

        horizontalLayout_20->addWidget(labelOutput);


        verticalLayout_9->addLayout(horizontalLayout_20);

        verticalSpacer_12 = new QSpacerItem(20, 24, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_9->addItem(verticalSpacer_12);


        gridLayout->addWidget(groupBox_2, 1, 1, 1, 1);


        verticalLayout_13->addLayout(gridLayout);

        verticalSpacer_7 = new QSpacerItem(20, 230, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_13->addItem(verticalSpacer_7);

        horizontalLayout_14 = new QHBoxLayout();
        horizontalLayout_14->setSpacing(6);
        horizontalLayout_14->setObjectName(QString::fromUtf8("horizontalLayout_14"));
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_14->addItem(horizontalSpacer);

        toolButton_quit_options = new QToolButton(page_options);
        toolButton_quit_options->setObjectName(QString::fromUtf8("toolButton_quit_options"));
        toolButton_quit_options->setMinimumSize(QSize(22, 25));
        toolButton_quit_options->setMaximumSize(QSize(22, 25));
        toolButton_quit_options->setStyleSheet(QString::fromUtf8("QToolButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QToolButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QToolButton { \n"
"background-image: url(:/images/interface/quit.png);\n"
"border-image: url(:/images/empty.png);\n"
"}"));

        horizontalLayout_14->addWidget(toolButton_quit_options);


        verticalLayout_13->addLayout(horizontalLayout_14);

        stackedWidget->addWidget(page_options);
        page_plants = new QWidget();
        page_plants->setObjectName(QString::fromUtf8("page_plants"));
        page_plants->setStyleSheet(QString::fromUtf8("#page_plants{background-image: url(:/images/interface/player/background.png);}"));
        framePlantsLeft = new QFrame(page_plants);
        framePlantsLeft->setObjectName(QString::fromUtf8("framePlantsLeft"));
        framePlantsLeft->setGeometry(QRect(0, 0, 355, 590));
        framePlantsLeft->setMinimumSize(QSize(355, 590));
        framePlantsLeft->setMaximumSize(QSize(355, 590));
        framePlantsLeft->setStyleSheet(QString::fromUtf8("#framePlantsLeft{background-image: url(:/images/interface/plants-left.png);}"));
        labelPlantImage = new QLabel(framePlantsLeft);
        labelPlantImage->setObjectName(QString::fromUtf8("labelPlantImage"));
        labelPlantImage->setGeometry(QRect(20, 20, 72, 72));
        labelPlantImage->setMinimumSize(QSize(72, 72));
        labelPlantImage->setMaximumSize(QSize(72, 72));
        labelPlantName = new QLabel(framePlantsLeft);
        labelPlantName->setObjectName(QString::fromUtf8("labelPlantName"));
        labelPlantName->setGeometry(QRect(140, 40, 181, 41));
        QFont font3;
        font3.setPointSize(13);
        font3.setBold(false);
        font3.setWeight(50);
        labelPlantName->setFont(font3);
        labelPlantName->setAlignment(Qt::AlignCenter);
        labelPlantDescription = new QLabel(framePlantsLeft);
        labelPlantDescription->setObjectName(QString::fromUtf8("labelPlantDescription"));
        labelPlantDescription->setGeometry(QRect(26, 413, 300, 150));
        labelPlantDescription->setMinimumSize(QSize(300, 150));
        labelPlantDescription->setMaximumSize(QSize(300, 150));
        labelPlantDescription->setWordWrap(true);
        formLayoutWidget = new QWidget(framePlantsLeft);
        formLayoutWidget->setObjectName(QString::fromUtf8("formLayoutWidget"));
        formLayoutWidget->setGeometry(QRect(180, 150, 160, 186));
        formLayout = new QFormLayout(formLayoutWidget);
        formLayout->setSpacing(6);
        formLayout->setContentsMargins(11, 11, 11, 11);
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        formLayout->setContentsMargins(0, 0, 0, 0);
        labelPlantFruitImage = new QLabel(formLayoutWidget);
        labelPlantFruitImage->setObjectName(QString::fromUtf8("labelPlantFruitImage"));
        labelPlantFruitImage->setMinimumSize(QSize(48, 48));
        labelPlantFruitImage->setMaximumSize(QSize(48, 48));
        labelPlantFruitImage->setScaledContents(true);

        formLayout->setWidget(2, QFormLayout::LabelRole, labelPlantFruitImage);

        labelPlantFruitText = new QLabel(formLayoutWidget);
        labelPlantFruitText->setObjectName(QString::fromUtf8("labelPlantFruitText"));

        formLayout->setWidget(2, QFormLayout::FieldRole, labelPlantFruitText);

        labelFruitsImage = new QLabel(formLayoutWidget);
        labelFruitsImage->setObjectName(QString::fromUtf8("labelFruitsImage"));
        labelFruitsImage->setMinimumSize(QSize(32, 64));
        labelFruitsImage->setMaximumSize(QSize(32, 64));
        labelFruitsImage->setScaledContents(true);

        formLayout->setWidget(1, QFormLayout::LabelRole, labelFruitsImage);

        labelFruitsText = new QLabel(formLayoutWidget);
        labelFruitsText->setObjectName(QString::fromUtf8("labelFruitsText"));

        formLayout->setWidget(1, QFormLayout::FieldRole, labelFruitsText);

        labelFloweringImage = new QLabel(formLayoutWidget);
        labelFloweringImage->setObjectName(QString::fromUtf8("labelFloweringImage"));
        labelFloweringImage->setMinimumSize(QSize(32, 64));
        labelFloweringImage->setMaximumSize(QSize(32, 64));
        labelFloweringImage->setScaledContents(true);

        formLayout->setWidget(0, QFormLayout::LabelRole, labelFloweringImage);

        labelFloweringText = new QLabel(formLayoutWidget);
        labelFloweringText->setObjectName(QString::fromUtf8("labelFloweringText"));

        formLayout->setWidget(0, QFormLayout::FieldRole, labelFloweringText);

        formLayoutWidget_2 = new QWidget(framePlantsLeft);
        formLayoutWidget_2->setObjectName(QString::fromUtf8("formLayoutWidget_2"));
        formLayoutWidget_2->setGeometry(QRect(20, 140, 160, 202));
        formLayout_2 = new QFormLayout(formLayoutWidget_2);
        formLayout_2->setSpacing(6);
        formLayout_2->setContentsMargins(11, 11, 11, 11);
        formLayout_2->setObjectName(QString::fromUtf8("formLayout_2"));
        formLayout_2->setContentsMargins(0, 0, 0, 0);
        labelPlantedImage = new QLabel(formLayoutWidget_2);
        labelPlantedImage->setObjectName(QString::fromUtf8("labelPlantedImage"));
        labelPlantedImage->setMinimumSize(QSize(32, 64));
        labelPlantedImage->setMaximumSize(QSize(32, 64));
        labelPlantedImage->setScaledContents(true);

        formLayout_2->setWidget(0, QFormLayout::LabelRole, labelPlantedImage);

        labelPlantedText = new QLabel(formLayoutWidget_2);
        labelPlantedText->setObjectName(QString::fromUtf8("labelPlantedText"));

        formLayout_2->setWidget(0, QFormLayout::FieldRole, labelPlantedText);

        labelSproutedImage = new QLabel(formLayoutWidget_2);
        labelSproutedImage->setObjectName(QString::fromUtf8("labelSproutedImage"));
        labelSproutedImage->setMinimumSize(QSize(32, 64));
        labelSproutedImage->setMaximumSize(QSize(32, 64));
        labelSproutedImage->setScaledContents(true);

        formLayout_2->setWidget(1, QFormLayout::LabelRole, labelSproutedImage);

        labelSproutedText = new QLabel(formLayoutWidget_2);
        labelSproutedText->setObjectName(QString::fromUtf8("labelSproutedText"));

        formLayout_2->setWidget(1, QFormLayout::FieldRole, labelSproutedText);

        labelTallerImage = new QLabel(formLayoutWidget_2);
        labelTallerImage->setObjectName(QString::fromUtf8("labelTallerImage"));
        labelTallerImage->setMinimumSize(QSize(32, 64));
        labelTallerImage->setMaximumSize(QSize(32, 64));
        labelTallerImage->setScaledContents(true);

        formLayout_2->setWidget(2, QFormLayout::LabelRole, labelTallerImage);

        labelTallerText = new QLabel(formLayoutWidget_2);
        labelTallerText->setObjectName(QString::fromUtf8("labelTallerText"));

        formLayout_2->setWidget(2, QFormLayout::FieldRole, labelTallerText);

        plantUse = new QPushButton(framePlantsLeft);
        plantUse->setObjectName(QString::fromUtf8("plantUse"));
        plantUse->setGeometry(QRect(130, 370, 110, 25));
        plantUse->setMinimumSize(QSize(110, 25));
        plantUse->setMaximumSize(QSize(110, 25));
        plantUse->setStyleSheet(QString::fromUtf8("QPushButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QPushButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QPushButton { \n"
"background-image: url(:/images/interface/button-generic.png);\n"
"border-image: url(:/images/empty.png);\n"
"color: rgb(255, 255, 255);\n"
"padding-left:4px;\n"
"padding-right:4px;\n"
"padding-top:8px;\n"
"padding-bottom:3px;\n"
"}"));
        framePlantsRight = new QFrame(page_plants);
        framePlantsRight->setObjectName(QString::fromUtf8("framePlantsRight"));
        framePlantsRight->setGeometry(QRect(360, 0, 434, 568));
        framePlantsRight->setMinimumSize(QSize(434, 568));
        framePlantsRight->setMaximumSize(QSize(434, 568));
        framePlantsRight->setStyleSheet(QString::fromUtf8("#framePlantsRight{background-image: url(:/images/interface/plants-right.png);}"));
        labelPlantInventoryTitle = new QLabel(framePlantsRight);
        labelPlantInventoryTitle->setObjectName(QString::fromUtf8("labelPlantInventoryTitle"));
        labelPlantInventoryTitle->setGeometry(QRect(142, 15, 171, 31));
        QFont font4;
        font4.setPointSize(13);
        labelPlantInventoryTitle->setFont(font4);
        labelPlantInventoryTitle->setAlignment(Qt::AlignCenter);
        listPlantList = new QListWidget(framePlantsRight);
        listPlantList->setObjectName(QString::fromUtf8("listPlantList"));
        listPlantList->setGeometry(QRect(40, 70, 371, 481));
        listPlantList->setStyleSheet(QString::fromUtf8("background-image: url(:/images/empty.png);"));
        listPlantList->setFrameShape(QFrame::NoFrame);
        listPlantList->setFrameShadow(QFrame::Plain);
        listPlantList->setIconSize(QSize(48, 48));
        toolButton_quit_plants = new QToolButton(page_plants);
        toolButton_quit_plants->setObjectName(QString::fromUtf8("toolButton_quit_plants"));
        toolButton_quit_plants->setGeometry(QRect(770, 570, 22, 25));
        toolButton_quit_plants->setMinimumSize(QSize(22, 25));
        toolButton_quit_plants->setMaximumSize(QSize(22, 25));
        toolButton_quit_plants->setStyleSheet(QString::fromUtf8("QToolButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QToolButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QToolButton { \n"
"background-image: url(:/images/interface/quit.png);\n"
"border-image: url(:/images/empty.png);\n"
"}"));
        stackedWidget->addWidget(page_plants);
        page_crafting = new QWidget();
        page_crafting->setObjectName(QString::fromUtf8("page_crafting"));
        page_crafting->setStyleSheet(QString::fromUtf8("#page_crafting{background-image: url(:/images/interface/player/background.png);}"));
        frameCraftingLeft = new QFrame(page_crafting);
        frameCraftingLeft->setObjectName(QString::fromUtf8("frameCraftingLeft"));
        frameCraftingLeft->setGeometry(QRect(0, 0, 355, 598));
        frameCraftingLeft->setMinimumSize(QSize(355, 598));
        frameCraftingLeft->setMaximumSize(QSize(355, 598));
        frameCraftingLeft->setStyleSheet(QString::fromUtf8("#frameCraftingLeft{background-image: url(:/images/interface/crafting-left.png);}"));
        labelCraftingImage = new QLabel(frameCraftingLeft);
        labelCraftingImage->setObjectName(QString::fromUtf8("labelCraftingImage"));
        labelCraftingImage->setGeometry(QRect(34, 28, 72, 72));
        labelCraftingImage->setMinimumSize(QSize(72, 72));
        labelCraftingImage->setMaximumSize(QSize(72, 72));
        labelCraftingDetails = new QLabel(frameCraftingLeft);
        labelCraftingDetails->setObjectName(QString::fromUtf8("labelCraftingDetails"));
        labelCraftingDetails->setGeometry(QRect(130, 20, 181, 91));
        labelCraftingDetails->setTextFormat(Qt::RichText);
        label_13 = new QLabel(frameCraftingLeft);
        label_13->setObjectName(QString::fromUtf8("label_13"));
        label_13->setGeometry(QRect(40, 150, 91, 16));
        listCraftingMaterials = new QListWidget(frameCraftingLeft);
        listCraftingMaterials->setObjectName(QString::fromUtf8("listCraftingMaterials"));
        listCraftingMaterials->setGeometry(QRect(30, 190, 301, 341));
        listCraftingMaterials->setStyleSheet(QString::fromUtf8("background-image: url(:/images/empty.png);"));
        listCraftingMaterials->setFrameShape(QFrame::NoFrame);
        listCraftingMaterials->setFrameShadow(QFrame::Plain);
        listCraftingMaterials->setSelectionMode(QAbstractItemView::NoSelection);
        listCraftingMaterials->setIconSize(QSize(24, 24));
        craftingUse = new QPushButton(frameCraftingLeft);
        craftingUse->setObjectName(QString::fromUtf8("craftingUse"));
        craftingUse->setGeometry(QRect(120, 550, 110, 25));
        craftingUse->setMinimumSize(QSize(110, 25));
        craftingUse->setMaximumSize(QSize(110, 25));
        craftingUse->setStyleSheet(QString::fromUtf8("QPushButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QPushButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QPushButton { \n"
"background-image: url(:/images/interface/button-generic.png);\n"
"border-image: url(:/images/empty.png);\n"
"color: rgb(255, 255, 255);\n"
"padding-left:4px;\n"
"padding-right:4px;\n"
"padding-top:8px;\n"
"padding-bottom:3px;\n"
"}"));
        frameCraftingRight = new QFrame(page_crafting);
        frameCraftingRight->setObjectName(QString::fromUtf8("frameCraftingRight"));
        frameCraftingRight->setGeometry(QRect(360, 0, 434, 568));
        frameCraftingRight->setMinimumSize(QSize(434, 568));
        frameCraftingRight->setMaximumSize(QSize(434, 568));
        frameCraftingRight->setStyleSheet(QString::fromUtf8("#frameCraftingRight{background-image: url(:/images/interface/crafting-right.png);}"));
        listCraftingList = new QListWidget(frameCraftingRight);
        listCraftingList->setObjectName(QString::fromUtf8("listCraftingList"));
        listCraftingList->setGeometry(QRect(40, 70, 371, 481));
        listCraftingList->setStyleSheet(QString::fromUtf8("background-image: url(:/images/empty.png);"));
        listCraftingList->setFrameShape(QFrame::NoFrame);
        listCraftingList->setFrameShadow(QFrame::Plain);
        listCraftingList->setIconSize(QSize(48, 48));
        label_10 = new QLabel(frameCraftingRight);
        label_10->setObjectName(QString::fromUtf8("label_10"));
        label_10->setGeometry(QRect(100, 20, 251, 21));
        label_10->setFont(font4);
        label_10->setAlignment(Qt::AlignCenter);
        toolButton_quit_crafting = new QToolButton(page_crafting);
        toolButton_quit_crafting->setObjectName(QString::fromUtf8("toolButton_quit_crafting"));
        toolButton_quit_crafting->setGeometry(QRect(770, 570, 22, 25));
        toolButton_quit_crafting->setMinimumSize(QSize(22, 25));
        toolButton_quit_crafting->setMaximumSize(QSize(22, 25));
        toolButton_quit_crafting->setStyleSheet(QString::fromUtf8("QToolButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QToolButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QToolButton { \n"
"background-image: url(:/images/interface/quit.png);\n"
"border-image: url(:/images/empty.png);\n"
"}"));
        stackedWidget->addWidget(page_crafting);
        page_shop = new QWidget();
        page_shop->setObjectName(QString::fromUtf8("page_shop"));
        page_shop->setStyleSheet(QString::fromUtf8("#page_shop{background-image: url(:/images/inventory/background.png);}\n"
""));
        verticalLayout_14 = new QVBoxLayout(page_shop);
        verticalLayout_14->setSpacing(0);
        verticalLayout_14->setContentsMargins(0, 0, 0, 0);
        verticalLayout_14->setObjectName(QString::fromUtf8("verticalLayout_14"));
        shop_layout = new QFrame(page_shop);
        shop_layout->setObjectName(QString::fromUtf8("shop_layout"));
        shop_layout->setStyleSheet(QString::fromUtf8("#shop_layout{background-image: url(:/images/inventory/shop.png);}"));
        shop_layout->setFrameShape(QFrame::NoFrame);
        shop_layout->setFrameShadow(QFrame::Plain);
        label_14 = new QLabel(shop_layout);
        label_14->setObjectName(QString::fromUtf8("label_14"));
        label_14->setGeometry(QRect(350, 5, 357, 51));
        label_14->setMinimumSize(QSize(357, 51));
        label_14->setMaximumSize(QSize(357, 51));
        label_14->setFont(font1);
        label_14->setStyleSheet(QString::fromUtf8("background-image: url(:/images/inventory/type.png);"));
        label_14->setAlignment(Qt::AlignCenter);
        shopImage = new QLabel(shop_layout);
        shopImage->setObjectName(QString::fromUtf8("shopImage"));
        shopImage->setGeometry(QRect(47, 476, 72, 72));
        shopImage->setPixmap(QPixmap(QString::fromUtf8(":/images/inventory/unknow-object.png")));
        shopDescription = new QLabel(shop_layout);
        shopDescription->setObjectName(QString::fromUtf8("shopDescription"));
        shopDescription->setGeometry(QRect(150, 530, 611, 61));
        shopDescription->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        shopSellerImage = new QLabel(shop_layout);
        shopSellerImage->setObjectName(QString::fromUtf8("shopSellerImage"));
        shopSellerImage->setGeometry(QRect(30, 160, 160, 160));
        shopSellerImage->setPixmap(QPixmap(QString::fromUtf8(":/images/player_default/front.png")));
        shopSellerImage->setAlignment(Qt::AlignCenter);
        shopName = new QLabel(shop_layout);
        shopName->setObjectName(QString::fromUtf8("shopName"));
        shopName->setGeometry(QRect(150, 510, 521, 16));
        toolButton_quit_shop = new QToolButton(shop_layout);
        toolButton_quit_shop->setObjectName(QString::fromUtf8("toolButton_quit_shop"));
        toolButton_quit_shop->setGeometry(QRect(770, 560, 22, 25));
        toolButton_quit_shop->setMinimumSize(QSize(22, 25));
        toolButton_quit_shop->setMaximumSize(QSize(22, 25));
        toolButton_quit_shop->setStyleSheet(QString::fromUtf8("QToolButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QToolButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QToolButton { \n"
"background-image: url(:/images/interface/quit.png);\n"
"border-image: url(:/images/empty.png);\n"
"}"));
        shopItemList = new QListWidget(shop_layout);
        shopItemList->setObjectName(QString::fromUtf8("shopItemList"));
        shopItemList->setGeometry(QRect(250, 70, 531, 381));
        shopItemList->setStyleSheet(QString::fromUtf8("background-image: url(:/images/empty.png);"));
        shopItemList->setFrameShape(QFrame::NoFrame);
        shopItemList->setIconSize(QSize(48, 48));
        shopBuy = new QPushButton(shop_layout);
        shopBuy->setObjectName(QString::fromUtf8("shopBuy"));
        shopBuy->setGeometry(QRect(680, 503, 110, 25));
        shopBuy->setMinimumSize(QSize(110, 25));
        shopBuy->setMaximumSize(QSize(110, 25));
        shopBuy->setStyleSheet(QString::fromUtf8("QPushButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QPushButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QPushButton { \n"
"background-image: url(:/images/interface/button-generic.png);\n"
"border-image: url(:/images/empty.png);\n"
"color: rgb(255, 255, 255);\n"
"padding-left:4px;\n"
"padding-right:4px;\n"
"padding-top:8px;\n"
"padding-bottom:3px;\n"
"}"));
        shopCash = new QLabel(shop_layout);
        shopCash->setObjectName(QString::fromUtf8("shopCash"));
        shopCash->setGeometry(QRect(6, 10, 201, 20));
        shopCash->setAlignment(Qt::AlignCenter);

        verticalLayout_14->addWidget(shop_layout);

        stackedWidget->addWidget(page_shop);
        page_battle = new QWidget();
        page_battle->setObjectName(QString::fromUtf8("page_battle"));
        stackedWidgetFightBottomBar = new QStackedWidget(page_battle);
        stackedWidgetFightBottomBar->setObjectName(QString::fromUtf8("stackedWidgetFightBottomBar"));
        stackedWidgetFightBottomBar->setGeometry(QRect(0, 440, 800, 160));
        stackedWidgetFightBottomBar->setMinimumSize(QSize(800, 160));
        stackedWidgetFightBottomBar->setMaximumSize(QSize(800, 160));
        stackedWidgetFightBottomBar->setStyleSheet(QString::fromUtf8("#stackedWidgetFightBottomBar{background-image: url(:/images/interface/fight/fight-bottom-bar.png);}"));
        stackedWidgetFightBottomBarPageEnter = new QWidget();
        stackedWidgetFightBottomBarPageEnter->setObjectName(QString::fromUtf8("stackedWidgetFightBottomBarPageEnter"));
        horizontalLayout_22 = new QHBoxLayout(stackedWidgetFightBottomBarPageEnter);
        horizontalLayout_22->setSpacing(6);
        horizontalLayout_22->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_22->setObjectName(QString::fromUtf8("horizontalLayout_22"));
        labelFightEnter = new QLabel(stackedWidgetFightBottomBarPageEnter);
        labelFightEnter->setObjectName(QString::fromUtf8("labelFightEnter"));

        horizontalLayout_22->addWidget(labelFightEnter);

        horizontalSpacer_23 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_22->addItem(horizontalSpacer_23);

        pushButtonFightEnterNext = new QPushButton(stackedWidgetFightBottomBarPageEnter);
        pushButtonFightEnterNext->setObjectName(QString::fromUtf8("pushButtonFightEnterNext"));

        horizontalLayout_22->addWidget(pushButtonFightEnterNext);

        stackedWidgetFightBottomBar->addWidget(stackedWidgetFightBottomBarPageEnter);
        stackedWidgetFightBottomBarPageMain = new QWidget();
        stackedWidgetFightBottomBarPageMain->setObjectName(QString::fromUtf8("stackedWidgetFightBottomBarPageMain"));
        gridLayout_3 = new QGridLayout(stackedWidgetFightBottomBarPageMain);
        gridLayout_3->setSpacing(6);
        gridLayout_3->setContentsMargins(11, 11, 11, 11);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        label_15 = new QLabel(stackedWidgetFightBottomBarPageMain);
        label_15->setObjectName(QString::fromUtf8("label_15"));

        gridLayout_3->addWidget(label_15, 0, 0, 2, 1);

        verticalLayout_16 = new QVBoxLayout();
        verticalLayout_16->setSpacing(6);
        verticalLayout_16->setObjectName(QString::fromUtf8("verticalLayout_16"));
        verticalSpacer_15 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_16->addItem(verticalSpacer_15);

        pushButtonFightAttack = new QPushButton(stackedWidgetFightBottomBarPageMain);
        pushButtonFightAttack->setObjectName(QString::fromUtf8("pushButtonFightAttack"));

        verticalLayout_16->addWidget(pushButtonFightAttack);

        pushButtonFightMonster = new QPushButton(stackedWidgetFightBottomBarPageMain);
        pushButtonFightMonster->setObjectName(QString::fromUtf8("pushButtonFightMonster"));

        verticalLayout_16->addWidget(pushButtonFightMonster);

        pushButtonFightBag = new QPushButton(stackedWidgetFightBottomBarPageMain);
        pushButtonFightBag->setObjectName(QString::fromUtf8("pushButtonFightBag"));
        pushButtonFightBag->setEnabled(false);

        verticalLayout_16->addWidget(pushButtonFightBag);

        verticalSpacer_14 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_16->addItem(verticalSpacer_14);


        gridLayout_3->addLayout(verticalLayout_16, 0, 1, 2, 1);

        toolButtonFightQuit = new QToolButton(stackedWidgetFightBottomBarPageMain);
        toolButtonFightQuit->setObjectName(QString::fromUtf8("toolButtonFightQuit"));
        toolButtonFightQuit->setMinimumSize(QSize(22, 25));
        toolButtonFightQuit->setMaximumSize(QSize(22, 25));
        toolButtonFightQuit->setStyleSheet(QString::fromUtf8("QToolButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QToolButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QToolButton { \n"
"background-image: url(:/images/interface/quit.png);\n"
"border-image: url(:/images/empty.png);\n"
"}"));

        gridLayout_3->addWidget(toolButtonFightQuit, 1, 2, 1, 1);

        stackedWidgetFightBottomBar->addWidget(stackedWidgetFightBottomBarPageMain);
        stackedWidgetFightBottomBarPageAttack = new QWidget();
        stackedWidgetFightBottomBarPageAttack->setObjectName(QString::fromUtf8("stackedWidgetFightBottomBarPageAttack"));
        horizontalLayout_23 = new QHBoxLayout(stackedWidgetFightBottomBarPageAttack);
        horizontalLayout_23->setSpacing(6);
        horizontalLayout_23->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_23->setObjectName(QString::fromUtf8("horizontalLayout_23"));
        listWidgetFightAttack = new QListWidget(stackedWidgetFightBottomBarPageAttack);
        listWidgetFightAttack->setObjectName(QString::fromUtf8("listWidgetFightAttack"));

        horizontalLayout_23->addWidget(listWidgetFightAttack);

        labelFightAttackDetails = new QLabel(stackedWidgetFightBottomBarPageAttack);
        labelFightAttackDetails->setObjectName(QString::fromUtf8("labelFightAttackDetails"));
        labelFightAttackDetails->setMinimumSize(QSize(200, 0));

        horizontalLayout_23->addWidget(labelFightAttackDetails);

        verticalLayout_17 = new QVBoxLayout();
        verticalLayout_17->setSpacing(6);
        verticalLayout_17->setObjectName(QString::fromUtf8("verticalLayout_17"));
        verticalSpacer_16 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_17->addItem(verticalSpacer_16);

        pushButtonFightAttackConfirmed = new QPushButton(stackedWidgetFightBottomBarPageAttack);
        pushButtonFightAttackConfirmed->setObjectName(QString::fromUtf8("pushButtonFightAttackConfirmed"));

        verticalLayout_17->addWidget(pushButtonFightAttackConfirmed);

        pushButtonFightReturn = new QPushButton(stackedWidgetFightBottomBarPageAttack);
        pushButtonFightReturn->setObjectName(QString::fromUtf8("pushButtonFightReturn"));

        verticalLayout_17->addWidget(pushButtonFightReturn);


        horizontalLayout_23->addLayout(verticalLayout_17);

        stackedWidgetFightBottomBar->addWidget(stackedWidgetFightBottomBarPageAttack);
        frameFightBackground = new QFrame(page_battle);
        frameFightBackground->setObjectName(QString::fromUtf8("frameFightBackground"));
        frameFightBackground->setGeometry(QRect(0, 0, 800, 440));
        frameFightBackground->setStyleSheet(QString::fromUtf8("#frameFightBackground{background-image: url(:/images/interface/fight/background.png);}"));
        labelFightPlateformBottom = new QLabel(frameFightBackground);
        labelFightPlateformBottom->setObjectName(QString::fromUtf8("labelFightPlateformBottom"));
        labelFightPlateformBottom->setGeometry(QRect(30, 350, 230, 90));
        labelFightPlateformBottom->setMinimumSize(QSize(230, 90));
        labelFightPlateformBottom->setMaximumSize(QSize(230, 90));
        labelFightPlateformBottom->setPixmap(QPixmap(QString::fromUtf8(":/images/interface/fight/plateform-background.png")));
        labelFightPlateformTop = new QLabel(frameFightBackground);
        labelFightPlateformTop->setObjectName(QString::fromUtf8("labelFightPlateformTop"));
        labelFightPlateformTop->setGeometry(QRect(460, 170, 260, 90));
        labelFightPlateformTop->setMinimumSize(QSize(260, 90));
        labelFightPlateformTop->setMaximumSize(QSize(260, 90));
        labelFightPlateformTop->setPixmap(QPixmap(QString::fromUtf8(":/images/interface/fight/plateform-front.png")));
        labelFightMonsterBottom = new QLabel(frameFightBackground);
        labelFightMonsterBottom->setObjectName(QString::fromUtf8("labelFightMonsterBottom"));
        labelFightMonsterBottom->setGeometry(QRect(60, 280, 160, 160));
        labelFightMonsterBottom->setMinimumSize(QSize(160, 160));
        labelFightMonsterBottom->setMaximumSize(QSize(160, 160));
        labelFightMonsterBottom->setPixmap(QPixmap(QString::fromUtf8(":/images/player_default/back.png")));
        labelFightMonsterBottom->setAlignment(Qt::AlignCenter);
        labelFightMonsterTop = new QLabel(frameFightBackground);
        labelFightMonsterTop->setObjectName(QString::fromUtf8("labelFightMonsterTop"));
        labelFightMonsterTop->setGeometry(QRect(510, 90, 160, 160));
        labelFightMonsterTop->setMinimumSize(QSize(160, 160));
        labelFightMonsterTop->setMaximumSize(QSize(160, 160));
        labelFightMonsterTop->setPixmap(QPixmap(QString::fromUtf8(":/images/player_default/front.png")));
        labelFightMonsterTop->setAlignment(Qt::AlignCenter);
        frameFightTop = new QFrame(frameFightBackground);
        frameFightTop->setObjectName(QString::fromUtf8("frameFightTop"));
        frameFightTop->setGeometry(QRect(70, 70, 300, 88));
        frameFightTop->setMinimumSize(QSize(300, 88));
        frameFightTop->setMaximumSize(QSize(300, 88));
        frameFightTop->setStyleSheet(QString::fromUtf8("#frameFightTop{\n"
"background-image: url(:/images/interface/fight/labelTop.png);\n"
"padding:6px 14px 6px 6px;\n"
"}"));
        verticalLayout_18 = new QVBoxLayout(frameFightTop);
        verticalLayout_18->setSpacing(6);
        verticalLayout_18->setContentsMargins(11, 11, 11, 11);
        verticalLayout_18->setObjectName(QString::fromUtf8("verticalLayout_18"));
        horizontalLayout_25 = new QHBoxLayout();
        horizontalLayout_25->setSpacing(6);
        horizontalLayout_25->setObjectName(QString::fromUtf8("horizontalLayout_25"));
        labelFightTopName = new QLabel(frameFightTop);
        labelFightTopName->setObjectName(QString::fromUtf8("labelFightTopName"));
        labelFightTopName->setText(QString::fromUtf8("Name"));

        horizontalLayout_25->addWidget(labelFightTopName);

        horizontalSpacer_24 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_25->addItem(horizontalSpacer_24);

        labelFightTopLevel = new QLabel(frameFightTop);
        labelFightTopLevel->setObjectName(QString::fromUtf8("labelFightTopLevel"));
        labelFightTopLevel->setText(QString::fromUtf8("Level 100"));

        horizontalLayout_25->addWidget(labelFightTopLevel);


        verticalLayout_18->addLayout(horizontalLayout_25);

        horizontalLayout_24 = new QHBoxLayout();
        horizontalLayout_24->setSpacing(6);
        horizontalLayout_24->setObjectName(QString::fromUtf8("horizontalLayout_24"));
        label_18 = new QLabel(frameFightTop);
        label_18->setObjectName(QString::fromUtf8("label_18"));

        horizontalLayout_24->addWidget(label_18);

        progressBarFightTopHP = new QProgressBar(frameFightTop);
        progressBarFightTopHP->setObjectName(QString::fromUtf8("progressBarFightTopHP"));
        progressBarFightTopHP->setMinimumSize(QSize(0, 10));
        progressBarFightTopHP->setMaximumSize(QSize(16777215, 10));
        progressBarFightTopHP->setTextVisible(false);

        horizontalLayout_24->addWidget(progressBarFightTopHP);


        verticalLayout_18->addLayout(horizontalLayout_24);

        verticalSpacer_17 = new QSpacerItem(20, 19, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_18->addItem(verticalSpacer_17);

        frameFightBottom = new QFrame(frameFightBackground);
        frameFightBottom->setObjectName(QString::fromUtf8("frameFightBottom"));
        frameFightBottom->setGeometry(QRect(480, 310, 300, 88));
        frameFightBottom->setMinimumSize(QSize(300, 88));
        frameFightBottom->setMaximumSize(QSize(300, 88));
        frameFightBottom->setStyleSheet(QString::fromUtf8("#frameFightBottom{\n"
"background-image: url(:/images/interface/fight/labelBottom.png);\n"
"padding:6px 6px 6px 14px;\n"
"}\n"
""));
        verticalLayout_19 = new QVBoxLayout(frameFightBottom);
        verticalLayout_19->setSpacing(6);
        verticalLayout_19->setContentsMargins(11, 11, 11, 11);
        verticalLayout_19->setObjectName(QString::fromUtf8("verticalLayout_19"));
        horizontalLayout_28 = new QHBoxLayout();
        horizontalLayout_28->setSpacing(6);
        horizontalLayout_28->setObjectName(QString::fromUtf8("horizontalLayout_28"));
        labelFightBottomName = new QLabel(frameFightBottom);
        labelFightBottomName->setObjectName(QString::fromUtf8("labelFightBottomName"));
        labelFightBottomName->setText(QString::fromUtf8("Name"));

        horizontalLayout_28->addWidget(labelFightBottomName);

        horizontalSpacer_25 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_28->addItem(horizontalSpacer_25);

        labelFightBottomLevel = new QLabel(frameFightBottom);
        labelFightBottomLevel->setObjectName(QString::fromUtf8("labelFightBottomLevel"));
        labelFightBottomLevel->setText(QString::fromUtf8("Level 100"));

        horizontalLayout_28->addWidget(labelFightBottomLevel);


        verticalLayout_19->addLayout(horizontalLayout_28);

        horizontalLayout_27 = new QHBoxLayout();
        horizontalLayout_27->setSpacing(6);
        horizontalLayout_27->setObjectName(QString::fromUtf8("horizontalLayout_27"));
        label_21 = new QLabel(frameFightBottom);
        label_21->setObjectName(QString::fromUtf8("label_21"));

        horizontalLayout_27->addWidget(label_21);

        progressBarFightBottomHP = new QProgressBar(frameFightBottom);
        progressBarFightBottomHP->setObjectName(QString::fromUtf8("progressBarFightBottomHP"));
        progressBarFightBottomHP->setMinimumSize(QSize(0, 10));
        progressBarFightBottomHP->setMaximumSize(QSize(16777215, 10));
        progressBarFightBottomHP->setTextVisible(false);

        horizontalLayout_27->addWidget(progressBarFightBottomHP);

        labelFightBottomHP = new QLabel(frameFightBottom);
        labelFightBottomHP->setObjectName(QString::fromUtf8("labelFightBottomHP"));
        labelFightBottomHP->setText(QString::fromUtf8("?/?"));

        horizontalLayout_27->addWidget(labelFightBottomHP);


        verticalLayout_19->addLayout(horizontalLayout_27);

        horizontalLayout_26 = new QHBoxLayout();
        horizontalLayout_26->setSpacing(6);
        horizontalLayout_26->setObjectName(QString::fromUtf8("horizontalLayout_26"));
        label_23 = new QLabel(frameFightBottom);
        label_23->setObjectName(QString::fromUtf8("label_23"));

        horizontalLayout_26->addWidget(label_23);

        progressBarFightBottopExp = new QProgressBar(frameFightBottom);
        progressBarFightBottopExp->setObjectName(QString::fromUtf8("progressBarFightBottopExp"));
        progressBarFightBottopExp->setMinimumSize(QSize(0, 10));
        progressBarFightBottopExp->setMaximumSize(QSize(16777215, 10));
        progressBarFightBottopExp->setTextVisible(false);

        horizontalLayout_26->addWidget(progressBarFightBottopExp);


        verticalLayout_19->addLayout(horizontalLayout_26);

        verticalSpacer_18 = new QSpacerItem(20, 5, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_19->addItem(verticalSpacer_18);

        stackedWidget->addWidget(page_battle);
        page_monster = new QWidget();
        page_monster->setObjectName(QString::fromUtf8("page_monster"));
        page_monster->setStyleSheet(QString::fromUtf8("#page_monster{background-image: url(:/images/interface/player/background.png);}"));
        verticalLayout_15 = new QVBoxLayout(page_monster);
        verticalLayout_15->setSpacing(6);
        verticalLayout_15->setContentsMargins(11, 11, 11, 11);
        verticalLayout_15->setObjectName(QString::fromUtf8("verticalLayout_15"));
        monsterList = new QListWidget(page_monster);
        monsterList->setObjectName(QString::fromUtf8("monsterList"));
        monsterList->setIconSize(QSize(80, 80));
        monsterList->setMovement(QListView::Static);

        verticalLayout_15->addWidget(monsterList);

        horizontalLayout_21 = new QHBoxLayout();
        horizontalLayout_21->setSpacing(6);
        horizontalLayout_21->setObjectName(QString::fromUtf8("horizontalLayout_21"));
        horizontalSpacer_22 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_21->addItem(horizontalSpacer_22);

        selectMonster = new QPushButton(page_monster);
        selectMonster->setObjectName(QString::fromUtf8("selectMonster"));

        horizontalLayout_21->addWidget(selectMonster);

        horizontalSpacer_31 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_21->addItem(horizontalSpacer_31);

        toolButton_monster_list_quit = new QToolButton(page_monster);
        toolButton_monster_list_quit->setObjectName(QString::fromUtf8("toolButton_monster_list_quit"));
        toolButton_monster_list_quit->setMinimumSize(QSize(22, 25));
        toolButton_monster_list_quit->setMaximumSize(QSize(22, 25));
        toolButton_monster_list_quit->setStyleSheet(QString::fromUtf8("QToolButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QToolButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QToolButton { \n"
"background-image: url(:/images/interface/quit.png);\n"
"border-image: url(:/images/empty.png);\n"
"}"));

        horizontalLayout_21->addWidget(toolButton_monster_list_quit);


        verticalLayout_15->addLayout(horizontalLayout_21);

        stackedWidget->addWidget(page_monster);
        page_bioscan = new QWidget();
        page_bioscan->setObjectName(QString::fromUtf8("page_bioscan"));
        toolButton_bioscan_quit = new QToolButton(page_bioscan);
        toolButton_bioscan_quit->setObjectName(QString::fromUtf8("toolButton_bioscan_quit"));
        toolButton_bioscan_quit->setGeometry(QRect(760, 560, 22, 25));
        toolButton_bioscan_quit->setMinimumSize(QSize(22, 25));
        toolButton_bioscan_quit->setMaximumSize(QSize(22, 25));
        toolButton_bioscan_quit->setStyleSheet(QString::fromUtf8("QToolButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QToolButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QToolButton { \n"
"background-image: url(:/images/interface/quit.png);\n"
"border-image: url(:/images/empty.png);\n"
"}"));
        stackedWidget->addWidget(page_bioscan);
        page_trade = new QWidget();
        page_trade->setObjectName(QString::fromUtf8("page_trade"));
        page_trade->setStyleSheet(QString::fromUtf8("#page_trade{background-image: url(:/images/interface/player/background.png);}"));
        verticalLayout_22 = new QVBoxLayout(page_trade);
        verticalLayout_22->setSpacing(6);
        verticalLayout_22->setContentsMargins(11, 11, 11, 11);
        verticalLayout_22->setObjectName(QString::fromUtf8("verticalLayout_22"));
        horizontalLayout_33 = new QHBoxLayout();
        horizontalLayout_33->setSpacing(6);
        horizontalLayout_33->setObjectName(QString::fromUtf8("horizontalLayout_33"));
        verticalLayout_20 = new QVBoxLayout();
        verticalLayout_20->setSpacing(6);
        verticalLayout_20->setObjectName(QString::fromUtf8("verticalLayout_20"));
        tradePlayerImage = new QLabel(page_trade);
        tradePlayerImage->setObjectName(QString::fromUtf8("tradePlayerImage"));
        tradePlayerImage->setMinimumSize(QSize(0, 80));
        tradePlayerImage->setPixmap(QPixmap(QString::fromUtf8(":/images/player_default/front.png")));
        tradePlayerImage->setAlignment(Qt::AlignCenter);

        verticalLayout_20->addWidget(tradePlayerImage);

        tradePlayerPseudo = new QLabel(page_trade);
        tradePlayerPseudo->setObjectName(QString::fromUtf8("tradePlayerPseudo"));
        tradePlayerPseudo->setText(QString::fromUtf8("Pseudo"));
        tradePlayerPseudo->setAlignment(Qt::AlignCenter);

        verticalLayout_20->addWidget(tradePlayerPseudo);

        horizontalLayout_30 = new QHBoxLayout();
        horizontalLayout_30->setSpacing(6);
        horizontalLayout_30->setObjectName(QString::fromUtf8("horizontalLayout_30"));
        label_22 = new QLabel(page_trade);
        label_22->setObjectName(QString::fromUtf8("label_22"));

        horizontalLayout_30->addWidget(label_22);

        tradePlayerCash = new QSpinBox(page_trade);
        tradePlayerCash->setObjectName(QString::fromUtf8("tradePlayerCash"));
        tradePlayerCash->setMaximum(999999999);

        horizontalLayout_30->addWidget(tradePlayerCash);

        horizontalSpacer_26 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_30->addItem(horizontalSpacer_26);


        verticalLayout_20->addLayout(horizontalLayout_30);

        tradePlayerItems = new QListWidget(page_trade);
        tradePlayerItems->setObjectName(QString::fromUtf8("tradePlayerItems"));
        tradePlayerItems->setIconSize(QSize(48, 48));
        tradePlayerItems->setMovement(QListView::Static);
        tradePlayerItems->setViewMode(QListView::IconMode);

        verticalLayout_20->addWidget(tradePlayerItems);

        tradeAddItem = new QPushButton(page_trade);
        tradeAddItem->setObjectName(QString::fromUtf8("tradeAddItem"));

        verticalLayout_20->addWidget(tradeAddItem);

        tradePlayerMonsters = new QListWidget(page_trade);
        tradePlayerMonsters->setObjectName(QString::fromUtf8("tradePlayerMonsters"));
        tradePlayerMonsters->setIconSize(QSize(80, 80));
        tradePlayerMonsters->setMovement(QListView::Static);
        tradePlayerMonsters->setViewMode(QListView::IconMode);

        verticalLayout_20->addWidget(tradePlayerMonsters);

        tradeAddMonster = new QPushButton(page_trade);
        tradeAddMonster->setObjectName(QString::fromUtf8("tradeAddMonster"));

        verticalLayout_20->addWidget(tradeAddMonster);

        horizontalLayout_32 = new QHBoxLayout();
        horizontalLayout_32->setSpacing(6);
        horizontalLayout_32->setObjectName(QString::fromUtf8("horizontalLayout_32"));
        horizontalSpacer_28 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_32->addItem(horizontalSpacer_28);

        tradeValidate = new QPushButton(page_trade);
        tradeValidate->setObjectName(QString::fromUtf8("tradeValidate"));

        horizontalLayout_32->addWidget(tradeValidate);

        horizontalSpacer_29 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_32->addItem(horizontalSpacer_29);


        verticalLayout_20->addLayout(horizontalLayout_32);


        horizontalLayout_33->addLayout(verticalLayout_20);

        verticalLayout_21 = new QVBoxLayout();
        verticalLayout_21->setSpacing(6);
        verticalLayout_21->setObjectName(QString::fromUtf8("verticalLayout_21"));
        tradeOtherImage = new QLabel(page_trade);
        tradeOtherImage->setObjectName(QString::fromUtf8("tradeOtherImage"));
        tradeOtherImage->setMinimumSize(QSize(0, 80));
        tradeOtherImage->setPixmap(QPixmap(QString::fromUtf8(":/images/player_default/front.png")));
        tradeOtherImage->setAlignment(Qt::AlignCenter);

        verticalLayout_21->addWidget(tradeOtherImage);

        tradeOtherPseudo = new QLabel(page_trade);
        tradeOtherPseudo->setObjectName(QString::fromUtf8("tradeOtherPseudo"));
        tradeOtherPseudo->setText(QString::fromUtf8("Pseudo"));
        tradeOtherPseudo->setAlignment(Qt::AlignCenter);

        verticalLayout_21->addWidget(tradeOtherPseudo);

        horizontalLayout_31 = new QHBoxLayout();
        horizontalLayout_31->setSpacing(6);
        horizontalLayout_31->setObjectName(QString::fromUtf8("horizontalLayout_31"));
        label_24 = new QLabel(page_trade);
        label_24->setObjectName(QString::fromUtf8("label_24"));

        horizontalLayout_31->addWidget(label_24);

        tradeOtherCash = new QSpinBox(page_trade);
        tradeOtherCash->setObjectName(QString::fromUtf8("tradeOtherCash"));
        tradeOtherCash->setReadOnly(true);
        tradeOtherCash->setButtonSymbols(QAbstractSpinBox::NoButtons);
        tradeOtherCash->setMaximum(999999999);

        horizontalLayout_31->addWidget(tradeOtherCash);

        horizontalSpacer_27 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_31->addItem(horizontalSpacer_27);


        verticalLayout_21->addLayout(horizontalLayout_31);

        tradeOtherItems = new QListWidget(page_trade);
        tradeOtherItems->setObjectName(QString::fromUtf8("tradeOtherItems"));
        tradeOtherItems->setIconSize(QSize(48, 48));
        tradeOtherItems->setMovement(QListView::Static);
        tradeOtherItems->setViewMode(QListView::IconMode);

        verticalLayout_21->addWidget(tradeOtherItems);

        tradeOtherMonsters = new QListWidget(page_trade);
        tradeOtherMonsters->setObjectName(QString::fromUtf8("tradeOtherMonsters"));
        tradeOtherMonsters->setIconSize(QSize(80, 80));
        tradeOtherMonsters->setMovement(QListView::Static);
        tradeOtherMonsters->setViewMode(QListView::IconMode);

        verticalLayout_21->addWidget(tradeOtherMonsters);

        tradeOtherStat = new QLabel(page_trade);
        tradeOtherStat->setObjectName(QString::fromUtf8("tradeOtherStat"));

        verticalLayout_21->addWidget(tradeOtherStat);


        horizontalLayout_33->addLayout(verticalLayout_21);


        verticalLayout_22->addLayout(horizontalLayout_33);

        horizontalLayout_34 = new QHBoxLayout();
        horizontalLayout_34->setSpacing(6);
        horizontalLayout_34->setObjectName(QString::fromUtf8("horizontalLayout_34"));
        horizontalSpacer_30 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_34->addItem(horizontalSpacer_30);

        tradeCancel = new QToolButton(page_trade);
        tradeCancel->setObjectName(QString::fromUtf8("tradeCancel"));
        tradeCancel->setMinimumSize(QSize(22, 25));
        tradeCancel->setMaximumSize(QSize(22, 25));
        tradeCancel->setStyleSheet(QString::fromUtf8("QToolButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QToolButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QToolButton { \n"
"background-image: url(:/images/interface/quit.png);\n"
"border-image: url(:/images/empty.png);\n"
"}"));

        horizontalLayout_34->addWidget(tradeCancel);


        verticalLayout_22->addLayout(horizontalLayout_34);

        stackedWidget->addWidget(page_trade);
        page_learn = new QWidget();
        page_learn->setObjectName(QString::fromUtf8("page_learn"));
        page_learn->setStyleSheet(QString::fromUtf8("#page_learn{background-image: url(:/images/inventory/background.png);}\n"
""));
        horizontalLayout_35 = new QHBoxLayout(page_learn);
        horizontalLayout_35->setSpacing(0);
        horizontalLayout_35->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_35->setObjectName(QString::fromUtf8("horizontalLayout_35"));
        learn_layout = new QFrame(page_learn);
        learn_layout->setObjectName(QString::fromUtf8("learn_layout"));
        learn_layout->setStyleSheet(QString::fromUtf8("#learn_layout{background-image: url(:/images/interface/learn.png);}"));
        learnQuit = new QToolButton(learn_layout);
        learnQuit->setObjectName(QString::fromUtf8("learnQuit"));
        learnQuit->setGeometry(QRect(770, 560, 22, 25));
        learnQuit->setMinimumSize(QSize(22, 25));
        learnQuit->setMaximumSize(QSize(22, 25));
        learnQuit->setStyleSheet(QString::fromUtf8("QToolButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QToolButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QToolButton { \n"
"background-image: url(:/images/interface/quit.png);\n"
"border-image: url(:/images/empty.png);\n"
"}"));
        label_16 = new QLabel(learn_layout);
        label_16->setObjectName(QString::fromUtf8("label_16"));
        label_16->setGeometry(QRect(360, 5, 357, 51));
        label_16->setMinimumSize(QSize(357, 51));
        label_16->setMaximumSize(QSize(357, 51));
        label_16->setFont(font1);
        label_16->setStyleSheet(QString::fromUtf8("background-image: url(:/images/inventory/type.png);"));
        label_16->setAlignment(Qt::AlignCenter);
        learnDescription = new QLabel(learn_layout);
        learnDescription->setObjectName(QString::fromUtf8("learnDescription"));
        learnDescription->setGeometry(QRect(10, 510, 661, 81));
        learnValidate = new QPushButton(learn_layout);
        learnValidate->setObjectName(QString::fromUtf8("learnValidate"));
        learnValidate->setGeometry(QRect(680, 510, 110, 25));
        learnValidate->setMinimumSize(QSize(110, 25));
        learnValidate->setMaximumSize(QSize(110, 25));
        learnValidate->setStyleSheet(QString::fromUtf8("QPushButton::hover {\n"
"background-position:left center;\n"
"}\n"
"QPushButton::pressed {\n"
"background-position:left bottom;\n"
"}\n"
"QPushButton { \n"
"background-image: url(:/images/interface/button-generic.png);\n"
"border-image: url(:/images/empty.png);\n"
"color: rgb(255, 255, 255);\n"
"padding-left:4px;\n"
"padding-right:4px;\n"
"padding-top:8px;\n"
"padding-bottom:3px;\n"
"}"));
        learnMonster = new QLabel(learn_layout);
        learnMonster->setObjectName(QString::fromUtf8("learnMonster"));
        learnMonster->setGeometry(QRect(30, 170, 160, 160));
        learnMonster->setMinimumSize(QSize(160, 160));
        learnMonster->setMaximumSize(QSize(160, 160));
        learnMonster->setPixmap(QPixmap(QString::fromUtf8(":/images/monsters/default/front.png")));
        learnMonster->setAlignment(Qt::AlignCenter);
        learnAttackList = new QListWidget(learn_layout);
        learnAttackList->setObjectName(QString::fromUtf8("learnAttackList"));
        learnAttackList->setGeometry(QRect(250, 70, 531, 391));
        learnAttackList->setStyleSheet(QString::fromUtf8("background-image: url(:/images/empty.png);"));
        learnAttackList->setFrameShape(QFrame::NoFrame);
        learnSP = new QLabel(learn_layout);
        learnSP->setObjectName(QString::fromUtf8("learnSP"));
        learnSP->setGeometry(QRect(60, 10, 57, 14));
        learnSP->setText(QString::fromUtf8("Sp: %1"));

        horizontalLayout_35->addWidget(learn_layout);

        stackedWidget->addWidget(page_learn);

        horizontalLayout->addWidget(stackedWidget);


        horizontalLayout_2->addLayout(horizontalLayout);


        retranslateUi(BaseWindowUI);
        QObject::connect(close_IG_dialog, SIGNAL(clicked()), IG_dialog, SLOT(hide()));

        stackedWidget->setCurrentIndex(2);
        tabWidgetTrainerCard->setCurrentIndex(2);
        stackedWidgetFightBottomBar->setCurrentIndex(2);


        QMetaObject::connectSlotsByName(BaseWindowUI);
    } // setupUi

    void retranslateUi(QWidget *BaseWindowUI)
    {
        BaseWindowUI->setWindowTitle(QApplication::translate("BaseWindowUI", "CatchChallenger", 0, QApplication::UnicodeUTF8));
        close_IG_dialog->setText(QApplication::translate("BaseWindowUI", "...", 0, QApplication::UnicodeUTF8));
        pushButton_interface_bag->setText(QApplication::translate("BaseWindowUI", "Bag", 0, QApplication::UnicodeUTF8));
        pushButton_interface_monster_list->setText(QApplication::translate("BaseWindowUI", "Bioscan", 0, QApplication::UnicodeUTF8));
        pushButton_interface_trainer->setText(QApplication::translate("BaseWindowUI", "Player", 0, QApplication::UnicodeUTF8));
        pushButton_interface_crafting->setText(QApplication::translate("BaseWindowUI", "Crafting", 0, QApplication::UnicodeUTF8));
        pushButton_interface_monsters->setText(QApplication::translate("BaseWindowUI", "Monsters", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        toolButtonOptions->setToolTip(QApplication::translate("BaseWindowUI", "Options", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        toolButtonOptions->setText(QApplication::translate("BaseWindowUI", "...", 0, QApplication::UnicodeUTF8));
        toolButton_interface_map->setText(QString());
#ifndef QT_NO_TOOLTIP
        toolButton_interface_quit->setToolTip(QApplication::translate("BaseWindowUI", "Quit", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        toolButton_interface_quit->setText(QApplication::translate("BaseWindowUI", "...", 0, QApplication::UnicodeUTF8));
        label_interface_number_of_player->setText(QApplication::translate("BaseWindowUI", "?/?", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("BaseWindowUI", "Trainer card", 0, QApplication::UnicodeUTF8));
        label_5->setText(QApplication::translate("BaseWindowUI", "Name: ", 0, QApplication::UnicodeUTF8));
        label_7->setText(QApplication::translate("BaseWindowUI", "Cash: ", 0, QApplication::UnicodeUTF8));
        label_4->setText(QApplication::translate("BaseWindowUI", "Badges:", 0, QApplication::UnicodeUTF8));
        label_badges->setText(QApplication::translate("BaseWindowUI", "<span style=\" color:#a0a0a0;\">None</span>", 0, QApplication::UnicodeUTF8));
        tabWidgetTrainerCard->setTabText(tabWidgetTrainerCard->indexOf(tabWidgetTrainerCardPage1), QApplication::translate("BaseWindowUI", "Informations", 0, QApplication::UnicodeUTF8));
        tabWidgetTrainerCard->setTabText(tabWidgetTrainerCard->indexOf(tabWidgetTrainerCardPage2), QApplication::translate("BaseWindowUI", "Reputation", 0, QApplication::UnicodeUTF8));
        tabWidgetTrainerCard->setTabText(tabWidgetTrainerCard->indexOf(tabWidgetTrainerCardPage3), QApplication::translate("BaseWindowUI", "Quests", 0, QApplication::UnicodeUTF8));
        toolButton_quit_interface->setText(QApplication::translate("BaseWindowUI", "...", 0, QApplication::UnicodeUTF8));
        label_8->setText(QApplication::translate("BaseWindowUI", "Inventory", 0, QApplication::UnicodeUTF8));
        inventory_description->setText(QApplication::translate("BaseWindowUI", "Select an object", 0, QApplication::UnicodeUTF8));
        toolButton_quit_inventory->setText(QApplication::translate("BaseWindowUI", "...", 0, QApplication::UnicodeUTF8));
        inventoryInformation->setText(QApplication::translate("BaseWindowUI", "Information", 0, QApplication::UnicodeUTF8));
        inventoryUse->setText(QApplication::translate("BaseWindowUI", "Use", 0, QApplication::UnicodeUTF8));
        inventoryDestroy->setText(QApplication::translate("BaseWindowUI", "Destroy", 0, QApplication::UnicodeUTF8));
        groupBox_Options_Displayer->setTitle(QApplication::translate("BaseWindowUI", "Display", 0, QApplication::UnicodeUTF8));
        checkBoxLimitFPS->setText(QApplication::translate("BaseWindowUI", "Limit FPS to:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        checkBoxZoom->setToolTip(QApplication::translate("BaseWindowUI", "The other player is displayed in zoom x2 range.\n"
"Without it will be show into the screen, with it it show at the border", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        checkBoxZoom->setText(QApplication::translate("BaseWindowUI", "Zoom x2 (see tooltip)", 0, QApplication::UnicodeUTF8));
        groupBox_Options_Multiplayer->setTitle(QApplication::translate("BaseWindowUI", "Multiplayer", 0, QApplication::UnicodeUTF8));
        checkBoxShowPseudo->setText(QApplication::translate("BaseWindowUI", "Show the pseudo", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        checkBoxShowPlayerType->setToolTip(QApplication::translate("BaseWindowUI", "Show if is admin, developer, premium, normal", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        checkBoxShowPlayerType->setText(QApplication::translate("BaseWindowUI", "Show the player type", 0, QApplication::UnicodeUTF8));
        groupBox->setTitle(QApplication::translate("BaseWindowUI", "Misc", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("BaseWindowUI", "Language:", 0, QApplication::UnicodeUTF8));
        comboBoxLanguage->clear();
        comboBoxLanguage->insertItems(0, QStringList()
         << QApplication::translate("BaseWindowUI", "English", 0, QApplication::UnicodeUTF8)
        );
        groupBox_2->setTitle(QApplication::translate("BaseWindowUI", "Informations", 0, QApplication::UnicodeUTF8));
        label_11->setText(QApplication::translate("BaseWindowUI", "Input:", 0, QApplication::UnicodeUTF8));
        labelInput->setText(QApplication::translate("BaseWindowUI", "0KB/s", 0, QApplication::UnicodeUTF8));
        label_12->setText(QApplication::translate("BaseWindowUI", "Output:", 0, QApplication::UnicodeUTF8));
        labelOutput->setText(QApplication::translate("BaseWindowUI", "0KB/s", 0, QApplication::UnicodeUTF8));
        toolButton_quit_options->setText(QApplication::translate("BaseWindowUI", "...", 0, QApplication::UnicodeUTF8));
        plantUse->setText(QApplication::translate("BaseWindowUI", "Use", 0, QApplication::UnicodeUTF8));
        labelPlantInventoryTitle->setText(QApplication::translate("BaseWindowUI", "Plants", 0, QApplication::UnicodeUTF8));
        toolButton_quit_plants->setText(QApplication::translate("BaseWindowUI", "...", 0, QApplication::UnicodeUTF8));
        label_13->setText(QApplication::translate("BaseWindowUI", "Material(s):", 0, QApplication::UnicodeUTF8));
        craftingUse->setText(QApplication::translate("BaseWindowUI", "Create", 0, QApplication::UnicodeUTF8));
        label_10->setText(QApplication::translate("BaseWindowUI", "Crafting", 0, QApplication::UnicodeUTF8));
        toolButton_quit_crafting->setText(QApplication::translate("BaseWindowUI", "...", 0, QApplication::UnicodeUTF8));
        label_14->setText(QApplication::translate("BaseWindowUI", "Shop", 0, QApplication::UnicodeUTF8));
        shopDescription->setText(QApplication::translate("BaseWindowUI", "Select an object", 0, QApplication::UnicodeUTF8));
        shopName->setText(QString());
        toolButton_quit_shop->setText(QApplication::translate("BaseWindowUI", "...", 0, QApplication::UnicodeUTF8));
        shopBuy->setText(QApplication::translate("BaseWindowUI", "Buy", 0, QApplication::UnicodeUTF8));
        pushButtonFightEnterNext->setText(QApplication::translate("BaseWindowUI", "Next", 0, QApplication::UnicodeUTF8));
        label_15->setText(QApplication::translate("BaseWindowUI", "What do you do?", 0, QApplication::UnicodeUTF8));
        pushButtonFightAttack->setText(QApplication::translate("BaseWindowUI", "Attack", 0, QApplication::UnicodeUTF8));
        pushButtonFightMonster->setText(QApplication::translate("BaseWindowUI", "Monster", 0, QApplication::UnicodeUTF8));
        pushButtonFightBag->setText(QApplication::translate("BaseWindowUI", "Bag", 0, QApplication::UnicodeUTF8));
        toolButtonFightQuit->setText(QApplication::translate("BaseWindowUI", "...", 0, QApplication::UnicodeUTF8));
        pushButtonFightAttackConfirmed->setText(QApplication::translate("BaseWindowUI", "Attack", 0, QApplication::UnicodeUTF8));
        pushButtonFightReturn->setText(QApplication::translate("BaseWindowUI", "Return", 0, QApplication::UnicodeUTF8));
        label_18->setText(QApplication::translate("BaseWindowUI", "HP", 0, QApplication::UnicodeUTF8));
        label_21->setText(QApplication::translate("BaseWindowUI", "HP", 0, QApplication::UnicodeUTF8));
        label_23->setText(QApplication::translate("BaseWindowUI", "Exp", 0, QApplication::UnicodeUTF8));
        selectMonster->setText(QApplication::translate("BaseWindowUI", "Select", 0, QApplication::UnicodeUTF8));
        toolButton_monster_list_quit->setText(QApplication::translate("BaseWindowUI", "...", 0, QApplication::UnicodeUTF8));
        toolButton_bioscan_quit->setText(QApplication::translate("BaseWindowUI", "...", 0, QApplication::UnicodeUTF8));
        label_22->setText(QApplication::translate("BaseWindowUI", "Cash:", 0, QApplication::UnicodeUTF8));
        tradeAddItem->setText(QApplication::translate("BaseWindowUI", "Add item", 0, QApplication::UnicodeUTF8));
        tradeAddMonster->setText(QApplication::translate("BaseWindowUI", "Add monster", 0, QApplication::UnicodeUTF8));
        tradeValidate->setText(QApplication::translate("BaseWindowUI", "Validate", 0, QApplication::UnicodeUTF8));
        label_24->setText(QApplication::translate("BaseWindowUI", "Cash:", 0, QApplication::UnicodeUTF8));
        tradeOtherStat->setText(QApplication::translate("BaseWindowUI", "TextLabel", 0, QApplication::UnicodeUTF8));
        tradeCancel->setText(QApplication::translate("BaseWindowUI", "...", 0, QApplication::UnicodeUTF8));
        learnQuit->setText(QApplication::translate("BaseWindowUI", "...", 0, QApplication::UnicodeUTF8));
        label_16->setText(QApplication::translate("BaseWindowUI", "Learn", 0, QApplication::UnicodeUTF8));
        learnDescription->setText(QApplication::translate("BaseWindowUI", "Select attack to learn", 0, QApplication::UnicodeUTF8));
        learnValidate->setText(QApplication::translate("BaseWindowUI", "Learn", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class BaseWindowUI: public Ui_BaseWindowUI {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_BASEWINDOW_H
