#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "pokecraft-clients/gamedata.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
	qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
	Pokecraft::ProtocolParsing::initialiseTheVariable();
	client=new Pokecraft::Api_client_real(&socket);
	ui->setupUi(this);
	if(settings.contains("login"))
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
	}
	connect(client,SIGNAL(disconnected(QString)),this,SLOT(disconnected(QString)));
	connect(client,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(error(QAbstractSocket::SocketError)),Qt::QueuedConnection);
	connect(client,SIGNAL(notLogged(QString)),this,SLOT(notLogged(QString)));
	connect(client,SIGNAL(logged()),this,SLOT(logged()));
	connect(client,SIGNAL(protocol_is_good()),this,SLOT(protocol_is_good()));
	connect(client,SIGNAL(stateChanged(QAbstractSocket::SocketState)),SLOT(stateChanged(QAbstractSocket::SocketState)));
	connect(client,SIGNAL(haveTheDatapack()),SLOT(haveTheDatapack()));
//	connect(client,SIGNAL(new_chat_text(quint32,quint8,QString)),this,SLOT(new_chat_text(quint32,quint8,QString)));
	connect(client,SIGNAL(number_of_player(quint16,quint16)),this,SLOT(number_of_player(quint16,quint16)));
	//connect(client,SIGNAL(new_player_info()),this,SLOT(update_chat()));
	connect(&stopFlood,SIGNAL(timeout()),this,SLOT(removeNumberForFlood()));
        subclient = NULL;
        graphicsview = NULL;
	stateChanged(QAbstractSocket::UnconnectedState);
	stopFlood.setSingleShot(false);
	stopFlood.start(1500);
	numberForFlood=0;
	haveShowDisconnectionReason=false;
	resetAll();
}

MainWindow::~MainWindow()
{
	client->tryDisconnect();
	delete client;
	delete ui;
}

void MainWindow::resetAll()
{
	ui->label_interface_number_of_player->setText("?/?");
	ui->stackedWidget->setCurrentIndex(0);
	chat_list_player_id.clear();
	chat_list_type.clear();
	chat_list_text.clear();
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
		ui->pushButtonTryLogin->setFocus();
        if(graphicsview!=NULL)
        {
                gameData::forceDestroyInstance();
                delete graphicsview;
                graphicsview = NULL;
        }
        if(subclient!=NULL)
        {
                delete subclient;
                subclient=NULL;
        }
}

void MainWindow::have_current_player_info()
{
	Pokecraft::Player_public_informations informations=client->get_player_informations().public_informations;
	ui->label_connecting_status->setText(tr("Loading the datapack..."));
	ui->player_informations_id->setText(tr("N°ID/%1").arg(0));
	ui->player_informations_pseudo->setText(informations.pseudo);
	ui->player_informations_cash->setText("0$");
	QPixmap playerImage(client->get_datapack_base_name()+"skin/mainCaracter/"+informations.skin+"/front.png");
	if(!playerImage.isNull())
		ui->player_informations_front->setPixmap(playerImage);
        //gameData::destroyInstanceAtTheLastCall();
        //gameData::getInstance();
        //gameData::getInstance();
        gameData* data = gameData::instance();
        data->showDebug("[new game ?]");
        if(data!=0)
        {
	    data->setClient(client);
	    connect(client,SIGNAL(haveTheDatapack()),data,SLOT(haveDatapack()));
        }
        if(graphicsview == NULL)
        {
                graphicsview = new graphicsviewkeyinput();
                ui->horizontalLayout_main_display_8->insertWidget(0,graphicsview);
        }
        if(subclient==NULL)
        {
                subclient = new craftClients(graphicsview);
		subclient->setCurrentPlayer(client->get_player_informations());
		connect(client,SIGNAL(insert_player(quint32,QString,quint16,quint16,quint8,quint16)),subclient,SLOT(insertPlayer(quint32,QString,quint16,quint16,quint8,quint16)));
		connect(client,SIGNAL(remove_player(quint32)),subclient,SLOT(removePlayer(quint32)));
		connect(client,SIGNAL(move_player(quint32,QList<QPair<quint8,quint8> >)),subclient,SLOT(movePlayer(quint32,QList<QPair<quint8,quint8> >)));
                data->setSubClient(subclient);
        }
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

void MainWindow::on_pushButtonTryLogin_pressed()
{
	ui->pushButtonTryLogin->setStyleSheet("border-radius:15px;border-color:#000;border:1px solid #000;background-color:rgb(220, 220, 220);padding:1px 10px;color:rgb(150,150,150);");
}

void MainWindow::on_pushButtonTryLogin_released()
{
	ui->pushButtonTryLogin->setStyleSheet("border-radius:15px;border-color:#000;border:1px solid #000;background-color:rgb(255, 255, 255);padding:1px 10px;color:rgb(0,0,0);");
}

void MainWindow::on_lineEditLogin_returnPressed()
{
	ui->lineEditPass->setFocus();
}

void MainWindow::on_lineEditPass_returnPressed()
{
	on_pushButtonTryLogin_clicked();
}

void MainWindow::on_pushButtonTryLogin_clicked()
{
	settings.setValue("login",ui->lineEditLogin->text());
	if(ui->checkBoxRememberPassword->isChecked())
		settings.setValue("pass",ui->lineEditPass->text());
	else
		settings.remove("pass");
	if(!ui->comboBoxServerList->currentText().contains(QRegExp("^[a-zA-Z0-9\\.\\-_]+:[0-9]{1,5}$")))
	{
		QMessageBox::warning(this,"Error","The server is not as form: [host]:[port]");
		return;
	}
	if(!server_list.contains(ui->comboBoxServerList->currentText()))
	{
		server_list.insert(0,ui->comboBoxServerList->currentText());
		settings.setValue("server_list",server_list);
	}
	settings.setValue("last_server",ui->comboBoxServerList->currentText());
	QString host=ui->comboBoxServerList->currentText();
	host.remove(QRegExp(":[0-9]{1,5}$"));
	QString port_string=ui->comboBoxServerList->currentText();
	port_string.remove(QRegExp("^.*:"));
	bool ok;
	quint16 port=port_string.toInt(&ok);
	if(!ok)
	{
		QMessageBox::warning(this,"Error","Wrong port number conversion");
		return;
	}
	client->tryConnect(host,port);
}

void MainWindow::stateChanged(QAbstractSocket::SocketState socketState)
{
	if(socketState==QAbstractSocket::UnconnectedState)
	{
		resetAll();
		ui->stackedWidget->setCurrentIndex(0);
	}
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
	QMessageBox::critical(this,tr("Error"),error);
	qDebug() << detailedError;
}

void MainWindow::protocol_is_good()
{
	client->tryLogin(ui->lineEditLogin->text(),ui->lineEditPass->text());
	ui->label_connecting_status->setText(tr("Try login..."));
}

void MainWindow::on_lineEdit_chat_text_returnPressed()
{
	QString text=ui->lineEdit_chat_text->text();
	if(text.isEmpty())
		return;
	if(text.contains(QRegExp("^ +$")))
	{
		ui->lineEdit_chat_text->clear();
		new_chat_text(0,7,"Space text not allowed");
		return;
	}
	if(text.size()>256)
	{
		ui->lineEdit_chat_text->clear();
		new_chat_text(0,7,"Message too long");
		return;
	}
	if(!text.startsWith('/'))
	{
		if(text==lastMessageSend)
		{
			ui->lineEdit_chat_text->clear();
			new_chat_text(0,7,"Send message like as previous");
			return;
		}
		if(numberForFlood>2)
		{
			ui->lineEdit_chat_text->clear();
			new_chat_text(0,7,"Stop flood");
			return;
		}
	}
	numberForFlood++;
	lastMessageSend=text;
	ui->lineEdit_chat_text->setText("");
        gameData::instance()->newMessage(text);
        gameData::destroyInstanceAtTheLastCall();
	if(!text.startsWith("/pm "))
		client->sendChatText((Pokecraft::Chat_type)(ui->comboBox_chat_type->currentIndex()+2),text);
	else if(text.contains(QRegExp("^/pm [^ ]+ .+$")))
	{
		QString pseudo=text;
		pseudo.replace(QRegExp("^/pm ([^ ]+) .+$"), "\\1");
		text.replace(QRegExp("^/pm [^ ]+ (.+)$"), "\\1");
		client->sendPM(text,pseudo);
	}
}

void MainWindow::removeNumberForFlood()
{
	if(numberForFlood<=0)
		return;
	numberForFlood--;
}

void MainWindow::new_chat_text(quint32 player_id,quint8 chat_type,QString text)
{
	chat_list_player_id << player_id;
	chat_list_type << (Pokecraft::Chat_type)chat_type;
	chat_list_text << text;
	while(chat_list_player_id.size()>64)
	{
		chat_list_player_id.removeFirst();
	}
	while(chat_list_type.size()>64)
		chat_list_type.removeFirst();
	while(chat_list_text.size()>64)
		chat_list_text.removeFirst();
	update_chat();
}

void MainWindow::update_chat()
{
/*        QList<Player_public_informations> player_informations_list=client->get_player_informations_list();
	QString nameHtml;
	int index=0;
	while(index<chat_list_player_id.size())
	{
		bool addPlayerInfo=true;
		if(chat_list_type.at(index)==Chat_type_system || chat_list_type.at(index)==Chat_type_system_important)
			addPlayerInfo=false;
		int index_player=-1;
		if(addPlayerInfo)
			index_player=client->indexOfPlayerInformations(chat_list_player_id.at(index));
		if(!addPlayerInfo)
			nameHtml+=ChatParsing::new_chat_message("",Player_type_normal,chat_list_type.at(index),chat_list_text.at(index));
		else if(index_player!=-1)
			nameHtml+=ChatParsing::new_chat_message(player_informations_list.at(index_player).pseudo,player_informations_list.at(index_player).type,chat_list_type.at(index),chat_list_text.at(index));
		index++;
	}
	ui->textBrowser_chat->setHtml(nameHtml);
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

void MainWindow::number_of_player(quint16 number,quint16 max)
{
	ui->label_interface_number_of_player->setText(QString("%1/%2").arg(number).arg(max));
}

void MainWindow::on_comboBox_chat_type_currentIndexChanged(int index)
{
	Q_UNUSED(index)
	update_chat();
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
