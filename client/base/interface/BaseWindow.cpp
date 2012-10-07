#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../../general/base/FacilityLib.h"

using namespace Pokecraft;

BaseWindow::BaseWindow(Api_protocol *client) :
    ui(new Ui::BaseWindowUI)
{
    qRegisterMetaType<Pokecraft::Chat_type>("Pokecraft::Chat_type");
    qRegisterMetaType<Pokecraft::Player_type>("Pokecraft::Player_type");
    qRegisterMetaType<Pokecraft::Player_private_and_public_informations>("Pokecraft::Player_private_and_public_informations");

    this->client=client;
    socketState=QAbstractSocket::UnconnectedState;

    mapController=new MapController(client,true,false,true,false);
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
    //chat
    connect(client,SIGNAL(new_chat_text(Pokecraft::Chat_type,QString,QString,Pokecraft::Player_type)),this,SLOT(new_chat_text(Pokecraft::Chat_type,QString,QString,Pokecraft::Player_type)),Qt::QueuedConnection);
    connect(client,SIGNAL(new_system_text(Pokecraft::Chat_type,QString)),this,SLOT(new_system_text(Pokecraft::Chat_type,QString)),Qt::QueuedConnection);

    connect(client,SIGNAL(number_of_player(quint16,quint16)),this,SLOT(number_of_player(quint16,quint16)),Qt::QueuedConnection);
    //connect(client,SIGNAL(new_player_info()),this,SLOT(update_chat()),Qt::QueuedConnection);
    connect(&stopFlood,SIGNAL(timeout()),this,SLOT(removeNumberForFlood()),Qt::QueuedConnection);

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
    ui->comboBox_chat_type->setCurrentIndex(0);
        ui->lineEdit_chat_text->setText("");
    update_chat();
    lastMessageSend="";
    mapController->resetAll();
}

void BaseWindow::serverIsLoading()
{
    ui->label_connecting_status->setText(tr("Preparing the game data"));
}

void BaseWindow::serverIsReady()
{
    ui->label_connecting_status->setText(tr("Game data is ready"));
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
    ui->label_connecting_status->setText(tr("Loading the starting data..."));
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
        client->sendChatText((Chat_type)(ui->comboBox_chat_type->currentIndex()+2),text);
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
    qDebug() << QString("new_chat_text: %1 by  %2").arg(text).arg(pseudo);
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
        int index_player=-1;
        if(!addPlayerInfo)
            nameHtml+=ChatParsing::new_chat_message("",Player_type_normal,chat_list_type.at(index),chat_list_text.at(index));
        else if(index_player!=-1)
            nameHtml+=ChatParsing::new_chat_message(chat_list_player_pseudo.at(index),chat_list_player_type.at(index),chat_list_type.at(index),chat_list_text.at(index));
        index++;
    }
    ui->textBrowser_chat->setHtml(nameHtml);
    ui->textBrowser_chat->scrollToAnchor(QString::number(index-1));
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
    qDebug() << "have_current_player_info()";
    skinFolderList=Pokecraft::FacilityLib::skinIdList(client->get_datapack_base_name()+DATAPACK_BASE_PATH_MAP);
    Player_public_informations informations=client->get_player_informations().public_informations;
    ui->label_connecting_status->setText(tr("Loading the datapack..."));
    ui->player_informations_pseudo->setText(informations.pseudo);
    ui->player_informations_cash->setText("0$");
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
        qDebug() << "The skin id: "+QString::number(informations.skinId)+"into a list of: "+skinFolderList.size()+" item(s)";
    }
}

void BaseWindow::haveTheDatapack()
{
    skinFolderList=Pokecraft::FacilityLib::skinIdList(client->get_datapack_base_name()+DATAPACK_BASE_PATH_MAP);

    ui->label_connecting_status->setText(tr("Loading the player informations..."));
    this->setWindowTitle(tr("Pokecraft - %1").arg(client->getPseudo()));

    ui->stackedWidget->setCurrentIndex(1);
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
