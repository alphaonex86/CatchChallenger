#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../../general/base/FacilityLib.h"
#include "../ClientVariable.h"
#include "DatapackClientLoader.h"

#include <QListWidgetItem>
#include <QBuffer>

using namespace Pokecraft;

BaseWindow::BaseWindow(Api_protocol *client) :
    ui(new Ui::BaseWindowUI)
{
    qRegisterMetaType<Pokecraft::Chat_type>("Pokecraft::Chat_type");
    qRegisterMetaType<Pokecraft::Player_type>("Pokecraft::Player_type");
    qRegisterMetaType<Pokecraft::Player_private_and_public_informations>("Pokecraft::Player_private_and_public_informations");
    qRegisterMetaType<QHash<quint32,quint32> >("QHash<quint32,quint32>");

    this->client=client;
    socketState=QAbstractSocket::UnconnectedState;

    frame_main_display_right=NULL;
    mapController=new MapController(client,true,false,true,false);
    mapController->setScale(2);
    ProtocolParsing::initialiseTheVariable();
    ui->setupUi(this);
    setupChatUI();

    connect(client,SIGNAL(protocol_is_good()),this,SLOT(protocol_is_good()),Qt::QueuedConnection);
    connect(client,SIGNAL(disconnected(QString)),this,SLOT(disconnected(QString)),Qt::QueuedConnection);
    connect(client,SIGNAL(error(QString)),this,SLOT(error(QString)),Qt::QueuedConnection);
    connect(client,SIGNAL(message(QString)),this,SLOT(message(QString)),Qt::QueuedConnection);
    connect(client,SIGNAL(notLogged(QString)),this,SLOT(notLogged(QString)),Qt::QueuedConnection);
    connect(client,SIGNAL(logged()),this,SLOT(logged()),Qt::QueuedConnection);
    connect(client,SIGNAL(haveTheDatapack()),this,SLOT(haveTheDatapack()),Qt::QueuedConnection);
    connect(client,SIGNAL(newError(QString,QString)),this,SLOT(newError(QString,QString)),Qt::QueuedConnection);

    //connect the map controler
    connect(client,SIGNAL(have_current_player_info(Pokecraft::Player_private_and_public_informations)),this,SLOT(have_current_player_info()),Qt::QueuedConnection);

    //inventory
    connect(client,SIGNAL(have_inventory(QHash<quint32,quint32>)),this,SLOT(have_inventory(QHash<quint32,quint32>)),Qt::QueuedConnection);
    connect(client,SIGNAL(add_to_inventory(QHash<quint32,quint32>)),this,SLOT(add_to_inventory(QHash<quint32,quint32>)),Qt::QueuedConnection);

    //chat
    connect(client,SIGNAL(new_chat_text(Pokecraft::Chat_type,QString,QString,Pokecraft::Player_type)),this,SLOT(new_chat_text(Pokecraft::Chat_type,QString,QString,Pokecraft::Player_type)),Qt::QueuedConnection);
    connect(client,SIGNAL(new_system_text(Pokecraft::Chat_type,QString)),this,SLOT(new_system_text(Pokecraft::Chat_type,QString)),Qt::QueuedConnection);

    connect(client,SIGNAL(number_of_player(quint16,quint16)),this,SLOT(number_of_player(quint16,quint16)),Qt::QueuedConnection);
    //connect(client,SIGNAL(new_player_info()),this,SLOT(update_chat()),Qt::QueuedConnection);
    connect(&stopFlood,SIGNAL(timeout()),this,SLOT(removeNumberForFlood()),Qt::QueuedConnection);

    //connect the datapack loader
    connect(&DatapackClientLoader::datapackLoader,SIGNAL(datapackParsed()),this,SLOT(datapackParsed()),Qt::QueuedConnection);
    connect(this,SIGNAL(parseDatapack(QString)),&DatapackClientLoader::datapackLoader,SLOT(parseDatapack(QString)),Qt::QueuedConnection);
    connect(&DatapackClientLoader::datapackLoader,SIGNAL(datapackParsed()),mapController,SLOT(datapackParsed()),Qt::QueuedConnection);

    //render
    connect(mapController,SIGNAL(stopped_in_front_of(Pokecraft::Map_client,quint8,quint8)),this,SLOT(stopped_in_front_of(Pokecraft::Map_client,quint8,quint8)));
    connect(mapController,SIGNAL(actionOn(Pokecraft::Map_client,quint8,quint8)),this,SLOT(actionOn(Pokecraft::Map_client,quint8,quint8)));

    connect(this,SIGNAL(useSeed(quint8)),client,SLOT(useSeed(quint8)));
    connect(this,SIGNAL(collectMaturePlant()),client,SLOT(collectMaturePlant()));

    stopFlood.setSingleShot(false);
    stopFlood.start(1500);
    numberForFlood=0;

    tip_timeout.setInterval(TIMETODISPLAY_TIP);
    gain_timeout.setInterval(TIMETODISPLAY_GAIN);
    tip_timeout.setSingleShot(true);
    gain_timeout.setSingleShot(true);
    connect(&tip_timeout,SIGNAL(timeout()),this,SLOT(tipTimeout()));
    connect(&gain_timeout,SIGNAL(timeout()),this,SLOT(gainTimeout()));

    mapController->setFocus();
    mapController->setDatapackPath(client->get_datapack_base_name());

    renderFrame = new QFrame(ui->page_3);
    renderFrame->setObjectName(QString::fromUtf8("renderFrame"));
    renderFrame->setMinimumSize(QSize(600, 520));
    QVBoxLayout *renderLayout = new QVBoxLayout(renderFrame);
    renderLayout->setSpacing(0);
    renderLayout->setContentsMargins(0, 0, 0, 0);
    renderLayout->setObjectName(QString::fromUtf8("renderLayout"));
    renderLayout->addWidget(mapController);
    renderFrame->setGeometry(QRect(0, 52, 800, 516));
    renderFrame->lower();
    renderFrame->lower();
    renderFrame->lower();

    resetAll();
}

BaseWindow::~BaseWindow()
{
    delete ui;
    delete mapController;
}

void BaseWindow::setMultiPlayer(bool multiplayer)
{
    emit sendsetMultiPlayer(multiplayer);
    //frame_main_display_right->setVisible(multiplayer);
    ui->frame_main_display_interface_player->setVisible(multiplayer);
}

void BaseWindow::resetAll()
{
    ui->frame_main_display_interface_player->hide();
    ui->label_interface_number_of_player->setText("?/?");
    ui->stackedWidget->setCurrentIndex(0);
    chat_list_player_pseudo.clear();
    chat_list_player_type.clear();
    chat_list_type.clear();
    chat_list_text.clear();
    textBrowser_chat->clear();
    comboBox_chat_type->setCurrentIndex(1);
        lineEdit_chat_text->setText("");
    update_chat();
    lastMessageSend="";
    mapController->resetAll();
    haveDatapack=false;
    havePlayerInformations=false;
    DatapackClientLoader::datapackLoader.resetAll();
    haveInventory=false;
    datapackIsParsed=false;
    ui->inventory->clear();
    items_graphical.clear();
    items_to_graphical.clear();
    ui->tip->setVisible(false);
    ui->gain->setVisible(false);
    ui->IG_dialog->setVisible(false);
    seedWait=false;
    inSelection=false;
}

void BaseWindow::serverIsLoading()
{
    ui->label_connecting_status->setText(tr("Preparing the game data"));
}

void BaseWindow::serverIsReady()
{
    ui->label_connecting_status->setText(tr("Game data is ready"));
}

QString BaseWindow::lastLocation() const
{
    return mapController->lastLocation();
}

void BaseWindow::disconnected(QString reason)
{
    if(haveShowDisconnectionReason)
        return;
    haveShowDisconnectionReason=true;
    QMessageBox::information(this,tr("Disconnected"),tr("Disconnected by the reason: %1").arg(reason));
    resetAll();
}

void BaseWindow::notLogged(QString reason)
{
    QMessageBox::information(this,tr("Unable to login"),tr("Unable to login: %1").arg(reason));
}

void BaseWindow::logged()
{
    updateConnectingStatus();
    client->sendDatapackContent();
}

void BaseWindow::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
    break;
    default:
    break;
    }
}

void BaseWindow::message(QString message)
{
    qDebug() << message;
}

void BaseWindow::protocol_is_good()
{
    ui->label_connecting_status->setText(tr("Try login..."));
}

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
    QFrame *frame_main_display_right = new QFrame(ui->page_3);
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

void BaseWindow::number_of_player(quint16 number,quint16 max)
{
    ui->frame_main_display_interface_player->show();
    ui->label_interface_number_of_player->setText(QString("%1/%2").arg(number).arg(max));
}

void BaseWindow::comboBox_chat_type_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    update_chat();
}

void BaseWindow::on_toolButton_interface_quit_clicked()
{
    client->tryDisconnect();
}

void BaseWindow::on_toolButton_quit_interface_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void BaseWindow::on_pushButton_interface_trainer_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
}

void BaseWindow::lineEdit_chat_text_lostFocus()
{
    mapController->setFocus();
}

void BaseWindow::stateChanged(QAbstractSocket::SocketState socketState)
{
    if(this->socketState==socketState)
        return;
    this->socketState=socketState;

    if(socketState!=QAbstractSocket::UnconnectedState)
    {
        if(socketState==QAbstractSocket::ConnectedState)
        {
            haveShowDisconnectionReason=false;
            ui->label_connecting_status->setText(tr("Try initialise the protocol..."));
            client->sendProtocol();
        }
        else
            ui->label_connecting_status->setText(tr("Connecting to the server..."));
    }
}

//return ok, itemId
void BaseWindow::selectObject(const ObjectType &objectType)
{
    inSelection=true;
    waitedObjectType=objectType;
    ui->stackedWidget->setCurrentIndex(3);
    load_inventory();
}

void BaseWindow::objectSelection(const bool &ok,const quint32 &itemId)
{
    switch(waitedObjectType)
    {
        case ObjectType_Seed:
            if(!ok)
                return;
            if(!items.contains(itemId))
            {
                qDebug() << "item id is not into the inventory";
                return;
            }
            items[itemId]--;
            if(items[itemId]==0)
                items.remove(itemId);
            seed_in_waiting=itemId;
            showTip(tr("Seed in planting..."));
            seedWait=true;
            load_inventory();
        break;
        default:
        qDebug() << "waitedObjectType is unknow";
        return;
    }
}

void BaseWindow::have_current_player_info()
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::have_current_player_info()";
    #endif
    if(havePlayerInformations)
        return;
    havePlayerInformations=true;
    Player_private_and_public_informations informations=client->get_player_informations();
    ui->player_informations_pseudo->setText(informations.public_informations.pseudo);
    ui->player_informations_cash->setText(QString("%1$").arg(informations.cash));
    DebugClass::debugConsole(QString("%1 is logged with id: %2, cash: %3").arg(informations.public_informations.pseudo).arg(informations.public_informations.simplifiedId).arg(informations.cash));
    updatePlayerImage();
    updateConnectingStatus();
}

void BaseWindow::haveTheDatapack()
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::haveTheDatapack()";
    #endif
    if(haveDatapack)
        return;
    haveDatapack=true;

    emit parseDatapack(client->get_datapack_base_name());
}

void BaseWindow::have_inventory(const QHash<quint32,quint32> &items)
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::have_inventory()";
    #endif
    this->items=items;
    haveInventory=true;
    updateConnectingStatus();
    load_inventory();
    updateConnectingStatus();
}

void BaseWindow::add_to_inventory(const QHash<quint32,quint32> &items)
{
    QString html=tr("You have obtained: ");
    QStringList objects;
    QHashIterator<quint32,quint32> i(items);
    while (i.hasNext()) {
        i.next();

        //add really to the list
        if(this->items.contains(i.key()))
            this->items[i.key()]+=i.value();
        else
            this->items[i.key()]=i.value();

        QPixmap image;
        QString name;
        if(DatapackClientLoader::datapackLoader.items.contains(i.key()))
        {
            image=DatapackClientLoader::datapackLoader.items[i.key()].image;
            name=DatapackClientLoader::datapackLoader.items[i.key()].name;
        }
        else
        {
            image=DatapackClientLoader::datapackLoader.defaultInventoryImage();
            name=QString("id: %1").arg(i.key());
        }

        image=image.scaled(24,24);
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        image.save(&buffer, "PNG");
        if(objects.size()<16)
        {
            if(i.value()>1)
                objects << QString("<b>%2x</b> %3 <img src=\"data:image/png;base64,%1\" />").arg(QString(byteArray.toBase64())).arg(i.value()).arg(name);
            else
                objects << QString("%2 <img src=\"data:image/png;base64,%1\" />").arg(QString(byteArray.toBase64())).arg(name);
        }
    }
    if(objects.size()==16)
        objects << "...";
    html+=objects.join(", ");
    showGain(html);

    load_inventory();
}

void BaseWindow::load_inventory()
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::load_inventory()";
    #endif
    if(!haveInventory || !datapackIsParsed)
        return;
    ui->inventory->clear();
    items_graphical.clear();
    items_to_graphical.clear();
    QHashIterator<quint32,quint32> i(items);
    while (i.hasNext()) {
        i.next();

        bool show=!inSelection;
        if(inSelection)
        {
            switch(waitedObjectType)
            {
                case ObjectType_Seed:
                    if(DatapackClientLoader::datapackLoader.itemToplants.contains(i.key()))
                        show=true;
                break;
                default:
                qDebug() << "waitedObjectType is unknow into load_inventory()";
                break;
            }
        }
        if(show)
        {
            QListWidgetItem *item;
            if(!items_to_graphical.contains(i.key()))
            {
                item=new QListWidgetItem();
                items_to_graphical[i.key()]=item;
                items_graphical[item]=i.key();
            }
            else
                item=items_to_graphical[i.key()];
            items_graphical[item]=i.key();
            if(DatapackClientLoader::datapackLoader.items.contains(i.key()))
            {
                item->setIcon(DatapackClientLoader::datapackLoader.items[i.key()].image);
                if(i.value()>1)
                    item->setText(QString::number(i.value()));
            }
            else
            {
                item->setIcon(DatapackClientLoader::datapackLoader.defaultInventoryImage());
                if(i.value()>1)
                    item->setText(QString("id: %1 (x%2)").arg(i.key()).arg(i.value()));
                else
                    item->setText(QString("id: %1").arg(i.key()));
            }
            ui->inventory->addItem(item);
        }
    }
}

void BaseWindow::datapackParsed()
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::datapackParsed()";
    #endif
    datapackIsParsed=true;
    load_inventory();
    updateConnectingStatus();
    updatePlayerImage();
}

void BaseWindow::updateConnectingStatus()
{
    QStringList waitedData;
    if(!haveInventory || !havePlayerInformations)
        waitedData << tr("loading the player informations");
    if(!haveDatapack)
        waitedData << tr("loading the datapack");
    else if(!datapackIsParsed)
        waitedData << tr("opening the datapack");
    if(waitedData.isEmpty())
    {
        this->setWindowTitle(tr("Pokecraft - %1").arg(client->getPseudo()));
        ui->stackedWidget->setCurrentIndex(1);
        showTip(tr("Welcome <b><i>%1</i></b> on pokecraft").arg(client->getPseudo()));
        return;
    }
    ui->label_connecting_status->setText(tr("Waiting: %1").arg(waitedData.join(", ")));
}

void BaseWindow::newError(QString error,QString detailedError)
{
    qDebug() << detailedError.toLocal8Bit();
    if(socketState!=QAbstractSocket::ConnectedState)
        return;
    client->tryDisconnect();
    resetAll();
    QMessageBox::critical(this,tr("Error"),error);
}

void BaseWindow::error(QString error)
{
    newError("Error with the protocol",error);
}

void BaseWindow::updatePlayerImage()
{
    if(havePlayerInformations && haveDatapack)
    {
        Player_public_informations informations=client->get_player_informations().public_informations;
        skinFolderList=Pokecraft::FacilityLib::skinIdList(client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN);
        QPixmap playerImage;
        if(informations.skinId<skinFolderList.size())
        {
            playerImage=QPixmap(client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(informations.skinId)+"/front.png");
            if(!playerImage.isNull())
                ui->player_informations_front->setPixmap(playerImage);
            else
            {
                ui->player_informations_front->setPixmap(QPixmap(":/images/player_default/front.png"));
                qDebug() << "Unable to load the player image: "+client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN+skinFolderList.at(informations.skinId)+"/front.png";
            }
        }
        else
        {
            ui->player_informations_front->setPixmap(QPixmap(":/images/player_default/front.png"));
            qDebug() << "The skin id: "+QString::number(informations.skinId)+", into a list of: "+QString::number(skinFolderList.size())+" item(s) into BaseWindow::updatePlayerImage()";
        }
    }
}

void BaseWindow::on_pushButton_interface_bag_clicked()
{
    if(inSelection)
    {
        qDebug() << "BaseWindow::on_pushButton_interface_bag_clicked() in selection, can't click here";
        return;
    }
    ui->stackedWidget->setCurrentIndex(3);
}

void BaseWindow::on_toolButton_quit_inventory_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    if(inSelection)
        objectSelection(false,0);
}

void BaseWindow::on_inventory_itemSelectionChanged()
{
    qDebug() << "on_inventory_itemSelectionChanged()";
    QList<QListWidgetItem *> items=ui->inventory->selectedItems();
    if(items.size()!=1)
    {
        ui->inventory_image->setPixmap(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        ui->inventory_name->setText("");
        ui->inventory_description->setText(tr("Select an object"));
        return;
    }
    QListWidgetItem *item=items.first();
    const DatapackClientLoader::Item &content=DatapackClientLoader::datapackLoader.items[items_graphical[item]];
    ui->inventory_image->setPixmap(content.image);
    ui->inventory_name->setText(content.name);
    ui->inventory_description->setText(content.description);
}

void BaseWindow::tipTimeout()
{
    ui->tip->setVisible(false);
}

void BaseWindow::gainTimeout()
{
    ui->gain->setVisible(false);
}

void BaseWindow::showTip(const QString &tip)
{
    ui->tip->setVisible(true);
    ui->tip->setText(tip);
    tip_timeout.start();
}

void BaseWindow::showGain(const QString &gain)
{
    ui->gain->setVisible(true);
    ui->gain->setText(gain);
    gain_timeout.start();
}

void BaseWindow::on_toolButton_quit_options_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void BaseWindow::stopped_in_front_of(const Pokecraft::Map_client &map,const quint8 &x,const quint8 &y)
{
    if(Pokecraft::MoveOnTheMap::isDirt(map,x,y))
    {
        int index=0;
        while(index<map.plantList.size())
        {
            if(map.plantList.at(index).x==x && map.plantList.at(index).y==y)
            {
                quint64 current_time=QDateTime::currentMSecsSinceEpoch()/1000;
                if(map.plantList.at(index).mature_at<current_time)
                    showTip(tr("To recolt the plant press <i>Enter</i>"));
                else
                    showTip(tr("This plant is growing and can't be collected"));
                return;
            }
            else
                index++;
        }
        showTip(tr("To plant a seed press <i>Enter</i>"));
        return;
    }
}

void BaseWindow::actionOn(const Pokecraft::Map_client &map,const quint8 &x,const quint8 &y)
{
    if(Pokecraft::MoveOnTheMap::isDirt(map,x,y))
    {
        int index=0;
        while(index<map.plantList.size())
        {
            if(map.plantList.at(index).x==x && map.plantList.at(index).y==y)
            {
                quint64 current_time=QDateTime::currentMSecsSinceEpoch()/1000;
                if(map.plantList.at(index).mature_at<current_time)
                {
                    showTip(tr("Plant collecting..."));
                    emit collectMaturePlant();
                }
                else
                    showTip(tr("This plant is growing and can't be collected"));
                return;
            }
            else
                index++;
        }
        if(seedWait)
        {
            showTip(tr("Wait to finish to plant the previous seed"));
            return;
        }
        selectObject(ObjectType_Seed);
        return;
    }
}

void BaseWindow::on_inventory_itemActivated(QListWidgetItem *item)
{
    if(!items_graphical.contains(item))
    {
        qDebug() << "BaseWindow::on_inventory_itemActivated(): activated item not found";
        return;
    }
    if(!inSelection)
    {
        qDebug() << "BaseWindow::on_inventory_itemActivated(): not in selection, use is not done actually";
        return;
    }
    objectSelection(true,items_graphical[item]);
}

void BaseWindow::seed_planted(const bool &ok)
{
    seedWait=false;
    if(ok)
        /// \todo add to the map here, and don't send on the server
        showTip(tr("Seed correctly planted"));
    else
    {
        if(items.contains(seed_in_waiting))
            items[seed_in_waiting]++;
        else
            items[seed_in_waiting]=1;
        showTip(tr("Seed cannot be planted"));
        load_inventory();
    }
}

void BaseWindow::plant_collected(const Pokecraft::Plant_collect &stat)
{
    switch(stat)
    {
        case Plant_collect_correctly_collected:
            showTip(tr("Plant collected"));
        break;
        case Plant_collect_empty_dirt:
            showTip(tr("Try collected empty dirt"));
        break;
        case Plant_collect_owned_by_another_player:
            showTip(tr("This plant had been planted recently by another player"));
        break;
        case Plant_collect_impossible:
            showTip(tr("This plant can't be collected"));
        break;
        default:
        qDebug() << "BaseWindow::plant_collected(): unkonw return";
        return;
    }
}
