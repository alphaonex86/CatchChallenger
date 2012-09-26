#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDir>

#include "../base/render/MapVisualiserPlayer.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
	qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
	qRegisterMetaType<Pokecraft::Chat_type>("Pokecraft::Chat_type");
	qRegisterMetaType<Pokecraft::Player_type>("Pokecraft::Player_type");

    QDir().mkdir(QCoreApplication::applicationDirPath()+"/savegames/");

    mapController=new MapController(NULL,true,false,true,false);
	Pokecraft::ProtocolParsing::initialiseTheVariable();
	client=new Pokecraft::Api_client_real(&socket);
	ui->setupUi(this);
    /*if(settings.contains("login"))
		ui->lineEditLogin->setText(settings.value("login").toString());
	if(settings.contains("pass"))
	{
		ui->lineEditPass->setText(settings.value("pass").toString());
		ui->checkBoxRememberPassword->setChecked(true);
	}
	if(settings.contains("server_list"))
		server_list=settings.value("server_list").toStringList();
	if(server_list.size()==0)
		server_list << client->getHost()+":"+QString::number(client->getPort());
	ui->comboBoxServerList->addItems(server_list);
	if(settings.contains("last_server"))
	{
		int index=ui->comboBoxServerList->findText(settings.value("last_server").toString());
		if(index!=-1)
			ui->comboBoxServerList->setCurrentIndex(index);
    }*/

    Pokecraft::ServerSettings settings;
    internalServer.setSettings(settings);

	connect(client,SIGNAL(disconnected(QString)),this,SLOT(disconnected(QString)));
	connect(client,SIGNAL(error(QString)),this,SLOT(error(QString)));
	connect(client,SIGNAL(message(QString)),this,SLOT(message(QString)));
	connect(&socket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(error(QAbstractSocket::SocketError)),Qt::QueuedConnection);
	connect(client,SIGNAL(notLogged(QString)),this,SLOT(notLogged(QString)));
	connect(client,SIGNAL(logged()),this,SLOT(logged()));
	connect(client,SIGNAL(protocol_is_good()),this,SLOT(protocol_is_good()));
	connect(&socket,SIGNAL(stateChanged(QAbstractSocket::SocketState)),SLOT(stateChanged(QAbstractSocket::SocketState)));
    connect(client,SIGNAL(haveTheDatapack()),this,SLOT(haveTheDatapack()));
    connect(client,SIGNAL(newError(QString,QString)),this,SLOT(newError(QString,QString)));

    //connect the map controler
    connect(client,SIGNAL(have_current_player_info(Pokecraft::Player_private_and_public_informations)),mapController,SLOT(have_current_player_info(Pokecraft::Player_private_and_public_informations)));
    connect(client,SIGNAL(insert_player(Pokecraft::Player_public_informations,QString,quint16,quint16,Pokecraft::Direction)),mapController,SLOT(insert_player(Pokecraft::Player_public_informations,QString,quint16,quint16,Pokecraft::Direction)));
    connect(mapController,SIGNAL(send_player_direction(Pokecraft::Direction)),client,SLOT(send_player_direction(Pokecraft::Direction)));

	//chat
	connect(client,SIGNAL(new_chat_text(Pokecraft::Chat_type,QString,QString,Pokecraft::Player_type)),this,SLOT(new_chat_text(Pokecraft::Chat_type,QString,QString,Pokecraft::Player_type)));
	connect(client,SIGNAL(new_system_text(Pokecraft::Chat_type,QString)),this,SLOT(new_system_text(Pokecraft::Chat_type,QString)));

	//connect(client,SIGNAL(new_player_info()),this,SLOT(update_chat()));
	connect(&stopFlood,SIGNAL(timeout()),this,SLOT(removeNumberForFlood()));
	stateChanged(QAbstractSocket::UnconnectedState);
	stopFlood.setSingleShot(false);
	stopFlood.start(1500);
	numberForFlood=0;
	haveShowDisconnectionReason=false;
    ui->horizontalLayout_mainDisplay->addWidget(mapController);
    mapController->setFocus();
	resetAll();
}

MainWindow::~MainWindow()
{
	client->tryDisconnect();
	delete client;
	delete ui;
    delete mapController;
}

void MainWindow::resetAll()
{
/*	ui->frame_main_display_interface_player->hide();
	ui->label_interface_number_of_player->setText("?/?");
	ui->stackedWidget->setCurrentIndex(0);
	ui->textBrowser_chat->clear();
	ui->comboBox_chat_type->setCurrentIndex(0);
        ui->lineEdit_chat_text->setText("");
	update_chat();
	lastMessageSend="";
	if(ui->lineEditLogin->text().isEmpty())
		ui->lineEditLogin->setFocus();
	else if(ui->lineEditPass->text().isEmpty())
		ui->lineEditPass->setFocus();
	else
        ui->pushButtonTryLogin->setFocus();*/
    chat_list_player_pseudo.clear();
    chat_list_player_type.clear();
    chat_list_type.clear();
    chat_list_text.clear();
}

void MainWindow::have_current_player_info()
{
    qDebug() << "have_current_player_info()";
	Pokecraft::Player_public_informations informations=client->get_player_informations().public_informations;
	ui->label_connecting_status->setText(tr("Loading the datapack..."));
	ui->player_informations_id->setText(tr("N°ID/%1").arg(0));
	ui->player_informations_pseudo->setText(informations.pseudo);
	ui->player_informations_cash->setText("0$");
	QPixmap playerImage(client->get_datapack_base_name()+"skin/mainCaracter/"+informations.skin+"/front.png");
	if(!playerImage.isNull())
		ui->player_informations_front->setPixmap(playerImage);
}

void MainWindow::haveTheDatapack()
{
	ui->label_connecting_status->setText(tr("Loading the player informations..."));
	this->setWindowTitle(tr("Pokecraft - %1").arg(client->getPseudo()));

	ui->stackedWidget->setCurrentIndex(2);
}

void MainWindow::disconnected(QString reason)
{
	QMessageBox::information(this,tr("Disconnected"),tr("Disconnected by the reason: %1").arg(reason));
	haveShowDisconnectionReason=true;
	resetAll();
}

void MainWindow::notLogged(QString reason)
{
	QMessageBox::information(this,tr("Unable to login"),tr("Unable to login: %1").arg(reason));
}

void MainWindow::logged()
{
	ui->label_connecting_status->setText(tr("Loading the starting data..."));
	client->sendDatapackContent();
}

void MainWindow::changeEvent(QEvent *e)
{
	QMainWindow::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
	break;
	default:
	break;
	}
}

void MainWindow::stateChanged(QAbstractSocket::SocketState socketState)
{
	if(socketState==QAbstractSocket::UnconnectedState)
        resetAll();
	else
	{
		ui->stackedWidget->setCurrentIndex(1);
		if(socketState==QAbstractSocket::ConnectedState)
		{
			ui->label_connecting_status->setText(tr("Try initialise the protocol..."));
			client->sendProtocol();
		}
		else
			ui->label_connecting_status->setText(tr("Connecting to the server..."));
	}
}

void MainWindow::error(QAbstractSocket::SocketError socketError)
{
	resetAll();
	switch(socketError)
	{
	case QAbstractSocket::RemoteHostClosedError:
		if(haveShowDisconnectionReason)
		{
			haveShowDisconnectionReason=false;
			return;
		}
		QMessageBox::information(this,tr("Connection closed"),tr("Connection closed by the server"));
	break;
	case QAbstractSocket::ConnectionRefusedError:
		QMessageBox::information(this,tr("Connection closed"),tr("Connection refused by the server"));
	break;
	case QAbstractSocket::SocketTimeoutError:
		QMessageBox::information(this,tr("Connection closed"),tr("Socket time out, server too long"));
	break;
	case QAbstractSocket::HostNotFoundError:
		QMessageBox::information(this,tr("Connection closed"),tr("The host address was not found."));
	break;
	case QAbstractSocket::SocketAccessError:
		QMessageBox::information(this,tr("Connection closed"),tr("The socket operation failed because the application lacked the required privileges."));
	break;
	case QAbstractSocket::SocketResourceError:
		QMessageBox::information(this,tr("Connection closed"),tr("The local system ran out of resources"));
	break;
	case QAbstractSocket::NetworkError:
		QMessageBox::information(this,tr("Connection closed"),tr("An error occurred with the network"));
	break;
	case QAbstractSocket::UnsupportedSocketOperationError:
		QMessageBox::information(this,tr("Connection closed"),tr("The requested socket operation is not supported by the local operating system (e.g., lack of IPv6 support)"));
	break;
	case QAbstractSocket::SslHandshakeFailedError:
		QMessageBox::information(this,tr("Connection closed"),tr("The SSL/TLS handshake failed, so the connection was closed"));
	break;
	default:
		QMessageBox::information(this,tr("Connection error"),tr("Connection error: %1").arg(socketError));
	}
}

void MainWindow::haveNewError()
{
//	QMessageBox::critical(this,tr("Error"),client->errorString());
}

void MainWindow::newError(QString error,QString detailedError)
{
    qDebug() << detailedError.toLocal8Bit();
    socket.close();
    resetAll();
    QMessageBox::critical(this,tr("Error"),error);
}

void MainWindow::error(QString error)
{
	newError("Error with the protocol",error);
}

void MainWindow::message(QString message)
{
	qDebug() << message;
}

void MainWindow::protocol_is_good()
{
//	client->tryLogin(ui->lineEditLogin->text(),ui->lineEditPass->text());
	ui->label_connecting_status->setText(tr("Try login..."));
}

void MainWindow::removeNumberForFlood()
{
	if(numberForFlood<=0)
		return;
	numberForFlood--;
}

void MainWindow::new_system_text(Pokecraft::Chat_type chat_type,QString text)
{
	qDebug() << QString("new_system_text: %1").arg(text);
	chat_list_player_type << Pokecraft::Player_type_normal;
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

void MainWindow::new_chat_text(Pokecraft::Chat_type chat_type,QString text,QString pseudo,Pokecraft::Player_type type)
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

void MainWindow::update_chat()
{
	QString nameHtml;
	int index=0;
	while(index<chat_list_player_pseudo.size())
	{
		bool addPlayerInfo=true;
		if(chat_list_type.at(index)==Pokecraft::Chat_type_system || chat_list_type.at(index)==Pokecraft::Chat_type_system_important)
			addPlayerInfo=false;
		int index_player=-1;
		if(!addPlayerInfo)
			nameHtml+=Pokecraft::ChatParsing::new_chat_message("",Pokecraft::Player_type_normal,chat_list_type.at(index),chat_list_text.at(index));
		else if(index_player!=-1)
			nameHtml+=Pokecraft::ChatParsing::new_chat_message(chat_list_player_pseudo.at(index),chat_list_player_type.at(index),chat_list_type.at(index),chat_list_text.at(index));
		index++;
	}
/*	ui->textBrowser_chat->setHtml(nameHtml);
    ui->textBrowser_chat->scrollToAnchor(QString::number(index-1));*/
}

QString MainWindow::toHtmlEntities(QString text)
{
	text.replace("&","&amp;");
	text.replace("\"","&quot;");
	text.replace("'","&#039;");
	text.replace("<","&lt;");
	text.replace(">","&gt;");
	return text;
}

QString MainWindow::toSmilies(QString text)
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

void MainWindow::on_pushButton_interface_bag_pressed()
{
	QString css=ui->pushButton_interface_bag->styleSheet();
	css.replace(".1.png",".2.png");
	ui->pushButton_interface_bag->setStyleSheet(css);
}

void MainWindow::on_pushButton_interface_bag_released()
{
	QString css=ui->pushButton_interface_bag->styleSheet();
	css.replace(".2.png",".1.png");
	ui->pushButton_interface_bag->setStyleSheet(css);
}

void MainWindow::on_pushButton_interface_monster_list_pressed()
{
	QString css=ui->pushButton_interface_monster_list->styleSheet();
	css.replace(".1.png",".2.png");
	ui->pushButton_interface_monster_list->setStyleSheet(css);
}

void MainWindow::on_pushButton_interface_monster_list_released()
{
	QString css=ui->pushButton_interface_monster_list->styleSheet();
	css.replace(".2.png",".1.png");
	ui->pushButton_interface_monster_list->setStyleSheet(css);
}

void MainWindow::on_pushButton_interface_trainer_pressed()
{
	QString css=ui->pushButton_interface_trainer->styleSheet();
	css.replace(".1.png",".2.png");
	ui->pushButton_interface_trainer->setStyleSheet(css);
}

void MainWindow::on_pushButton_interface_trainer_released()
{
	QString css=ui->pushButton_interface_trainer->styleSheet();
	css.replace(".2.png",".1.png");
	ui->pushButton_interface_trainer->setStyleSheet(css);
}

void MainWindow::on_toolButton_interface_map_pressed()
{
	QString css=ui->toolButton_interface_map->styleSheet();
	css.replace(".1.png",".2.png");
	ui->toolButton_interface_map->setStyleSheet(css);
}

void MainWindow::on_toolButton_interface_map_released()
{
	QString css=ui->toolButton_interface_map->styleSheet();
	css.replace(".2.png",".1.png");
	ui->toolButton_interface_map->setStyleSheet(css);
}

void MainWindow::on_toolButton_interface_quit_clicked()
{
	client->tryDisconnect();
}

void MainWindow::on_toolButton_interface_quit_pressed()
{
	QString css=ui->toolButton_interface_quit->styleSheet();
	css.replace(".1.png",".2.png");
	ui->toolButton_interface_quit->setStyleSheet(css);
}

void MainWindow::on_toolButton_interface_quit_released()
{
	QString css=ui->toolButton_interface_quit->styleSheet();
	css.replace(".2.png",".1.png");
	ui->toolButton_interface_quit->setStyleSheet(css);
}

void MainWindow::on_toolButton_quit_interface_clicked()
{
	ui->stackedWidget->setCurrentIndex(2);
}

void MainWindow::on_toolButton_quit_interface_pressed()
{
	QString css=ui->toolButton_quit_interface->styleSheet();
	css.replace(".1.png",".2.png");
	ui->toolButton_quit_interface->setStyleSheet(css);
}

void MainWindow::on_toolButton_quit_interface_released()
{
	QString css=ui->toolButton_quit_interface->styleSheet();
	css.replace(".2.png",".1.png");
	ui->toolButton_quit_interface->setStyleSheet(css);
}


void MainWindow::on_pushButton_interface_trainer_clicked()
{
	ui->stackedWidget->setCurrentIndex(3);
}

void MainWindow::on_SaveGame_New_clicked()
{
/*    int index=0;
    QString saveGameFolder;
    do
    {
        saveGameFolder=QCoreApplication::applicationDirPath()+"/savegames/"+QString::number(index)+"/";
        index++;
    } while(QDir(saveGameFolder).exists());
    QDir(saveGameFolder).mkdir();*/
}
