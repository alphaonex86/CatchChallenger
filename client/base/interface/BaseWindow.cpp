#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../../general/base/FacilityLib.h"

#include <QListWidgetItem>

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

    mapController=new MapController(client,true,false,true,false);
    mapController->setScale(2);
    ProtocolParsing::initialiseTheVariable();
    ui->setupUi(this);

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
    connect(client,SIGNAL(have_inventory(QHash<quint32,quint32>)),this,SLOT(have_inventory(QHash<quint32,quint32>)),Qt::QueuedConnection);

    //chat
    connect(client,SIGNAL(new_chat_text(Pokecraft::Chat_type,QString,QString,Pokecraft::Player_type)),this,SLOT(new_chat_text(Pokecraft::Chat_type,QString,QString,Pokecraft::Player_type)),Qt::QueuedConnection);
    connect(client,SIGNAL(new_system_text(Pokecraft::Chat_type,QString)),this,SLOT(new_system_text(Pokecraft::Chat_type,QString)),Qt::QueuedConnection);

    connect(client,SIGNAL(number_of_player(quint16,quint16)),this,SLOT(number_of_player(quint16,quint16)),Qt::QueuedConnection);
    //connect(client,SIGNAL(new_player_info()),this,SLOT(update_chat()),Qt::QueuedConnection);
    connect(&stopFlood,SIGNAL(timeout()),this,SLOT(removeNumberForFlood()),Qt::QueuedConnection);

    //connect the datapack loader
    connect(&datapackLoader,SIGNAL(datapackParsed()),this,SLOT(datapackParsed()),Qt::QueuedConnection);
    connect(this,SIGNAL(parseDatapack(QString)),&datapackLoader,SLOT(parseDatapack(QString)),Qt::QueuedConnection);

    stopFlood.setSingleShot(false);
    stopFlood.start(1500);
    numberForFlood=0;
    ui->horizontalLayout_mainDisplay->addWidget(mapController);
    mapController->setFocus();
    mapController->setDatapackPath(client->get_datapack_base_name());
    resetAll();
}

BaseWindow::~BaseWindow()
{
    datapackLoader.quit();
    datapackLoader.wait();
    delete ui;
    delete mapController;
}

void BaseWindow::setMultiPlayer(bool multiplayer)
{
    ui->frame_main_display_right->setVisible(multiplayer);
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
    ui->textBrowser_chat->clear();
    ui->comboBox_chat_type->setCurrentIndex(1);
        ui->lineEdit_chat_text->setText("");
    update_chat();
    lastMessageSend="";
    mapController->resetAll();
    haveDatapack=false;
    havePlayerInformations=false;
    datapackLoader.resetAll();
    haveInventory=false;
    datapackIsParsed=false;
    ui->inventory->clear();
    items_graphical.clear();
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

void BaseWindow::on_lineEdit_chat_text_returnPressed()
{
    QString text=ui->lineEdit_chat_text->text();
    if(text.isEmpty())
        return;
    if(text.contains(QRegExp("^ +$")))
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
    ui->lineEdit_chat_text->setText("");
    if(!text.startsWith("/pm "))
    {
        Chat_type chat_type;
        switch(ui->comboBox_chat_type->currentIndex())
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
    qDebug() << QString("new_system_text: %1").arg(text);
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
    qDebug() << QString("new_chat_text: %1 by %2").arg(text).arg(pseudo);
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
    qDebug() << QString("update_chat(): %1").arg(chat_list_player_pseudo.size());
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
    ui->textBrowser_chat->setHtml(nameHtml);
    //ui->textBrowser_chat->scrollToAnchor(QString::number(index-1));
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

void BaseWindow::on_comboBox_chat_type_currentIndexChanged(int index)
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

void BaseWindow::on_lineEdit_chat_text_lostFocus()
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

void BaseWindow::have_current_player_info()
{
    qDebug() << "BaseWindow::have_current_player_info()";
    if(havePlayerInformations)
        return;
    havePlayerInformations=true;
    Player_private_and_public_informations informations=client->get_player_informations();
    ui->player_informations_pseudo->setText(informations.public_informations.pseudo);
    ui->player_informations_cash->setText(QString("%1$").arg(informations.cash));
    updatePlayerImage();
    updateConnectingStatus();
}

void BaseWindow::haveTheDatapack()
{
    qDebug() << "BaseWindow::haveTheDatapack()";
    if(haveDatapack)
        return;
    haveDatapack=true;
    updatePlayerImage();

    emit parseDatapack(client->get_datapack_base_name());
    updateConnectingStatus();
}

void BaseWindow::have_inventory(const QHash<quint32,quint32> &items)
{
    qDebug() << "BaseWindow::have_inventory()";
    this->items=items;
    haveInventory=true;
    updateConnectingStatus();
    load_inventory();
}

void BaseWindow::load_inventory()
{
    qDebug() << "BaseWindow::load_inventory()";
    if(haveInventory && datapackIsParsed)
    {
        QHashIterator<quint32,quint32> i(items);
         while (i.hasNext()) {
             i.next();
             QListWidgetItem *item=new QListWidgetItem();
             items_graphical[item]=i.key();
             if(DatapackClientLoader::items.contains(i.key()))
             {
                 item->setIcon(DatapackClientLoader::items[i.key()].image);
                 if(i.value()>1)
                     item->setText(QString::number(i.value()));
             }
             else
             {
                 item->setIcon(datapackLoader.defaultInventoryImage());
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
    qDebug() << "BaseWindow::datapackParsed()";
    datapackIsParsed=true;
    load_inventory();
    updateConnectingStatus();
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

void Pokecraft::BaseWindow::on_pushButton_interface_bag_clicked()
{
    ui->stackedWidget->setCurrentIndex(3);
}

void Pokecraft::BaseWindow::on_toolButton_quit_interface_2_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void Pokecraft::BaseWindow::on_inventory_itemSelectionChanged()
{
    qDebug() << "on_inventory_itemSelectionChanged()";
    QList<QListWidgetItem *> items=ui->inventory->selectedItems();
    if(items.size()!=1)
    {
        ui->inventory_image->setPixmap(datapackLoader.defaultInventoryImage());
        ui->inventory_name->setText("");
        ui->inventory_description->setText(tr("Select an object"));
        return;
    }
    QListWidgetItem *item=items.first();
    const DatapackClientLoader::item &content=DatapackClientLoader::items[items_graphical[item]];
    ui->inventory_image->setPixmap(content.image);
    ui->inventory_name->setText(content.name);
    ui->inventory_description->setText(content.description);
}
