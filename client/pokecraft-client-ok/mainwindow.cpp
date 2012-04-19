#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	if(settings.contains("login"))
		ui->lineEditLogin->setText(settings.value("login").toString());
	if(settings.contains("pass"))
	{
		ui->lineEditPass->setText(settings.value("pass").toString());
		ui->checkBoxRememberPassword->setChecked(true);
	}
	connect(&client,SIGNAL(disconnected(QString)),this,SLOT(disconnected(QString)));
	connect(&client,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(error(QAbstractSocket::SocketError)));
	connect(&client,SIGNAL(haveNewError()),this,SLOT(haveNewError()),Qt::QueuedConnection);
	connect(&client,SIGNAL(notLogged(QString)),this,SLOT(notLogged(QString)));
	connect(&client,SIGNAL(logged()),this,SLOT(logged()));
	connect(&client,SIGNAL(protocol_is_good()),this,SLOT(protocol_is_good()));
	connect(&client,SIGNAL(stateChanged(QAbstractSocket::SocketState)),SLOT(stateChanged(QAbstractSocket::SocketState)));
	connect(&client,SIGNAL(have_current_player_info()),SLOT(have_current_player_info()));
	connect(&client,SIGNAL(new_chat_text(quint32,quint8,QString)),this,SLOT(new_chat_text(quint32,quint8,QString)));
	connect(&client,SIGNAL(number_of_player(quint16,quint16)),this,SLOT(number_of_player(quint16,quint16)));
	connect(&client,SIGNAL(new_player_info()),this,SLOT(update_chat()));
	connect(&stopFlood,SIGNAL(timeout()),this,SLOT(removeNumberForFlood()));
	stateChanged(QAbstractSocket::UnconnectedState);
	stopFlood.setSingleShot(false);
	stopFlood.start(1500);
	numberForFlood=0;
	resetAll();
}

MainWindow::~MainWindow()
{
	client.tryDisconnect();
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
}

void MainWindow::have_current_player_info()
{
	ui->stackedWidget->setCurrentIndex(2);
	this->setWindowTitle(tr("Pokecraft - %1").arg(client.getPseudo()));
}

void MainWindow::disconnected(QString reason)
{
	resetAll();
	QMessageBox::information(this,tr("Disconnected"),tr("Disconnected by the reason: %1").arg(reason));
}

void MainWindow::notLogged(QString reason)
{
	QMessageBox::information(this,tr("Unable to login"),tr("Unable to login: %1").arg(reason));
}

void MainWindow::logged()
{
	ui->label_connecting_status->setText(tr("Loading the starting data..."));
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

void MainWindow::on_lineEditLogin_editingFinished()
{
	settings.setValue("login",ui->lineEditLogin->text());
}

void MainWindow::on_lineEditPass_editingFinished()
{
	if(ui->checkBoxRememberPassword->isChecked())
		settings.setValue("pass",ui->lineEditPass->text());
}

void MainWindow::on_checkBoxRememberPassword_stateChanged(int arg1)
{
	Q_UNUSED(arg1)
	if(ui->checkBoxRememberPassword->isChecked())
		settings.setValue("pass",ui->lineEditPass->text());
	else
		settings.remove("pass");
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
	client.tryConnect();
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
			ui->label_connecting_status->setText(tr("Try login..."));
			client.sendProtocol();
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
		QMessageBox::information(this,tr("Connection closed"),tr("Connection closed by the server"));
	break;
	case QAbstractSocket::ConnectionRefusedError:
		QMessageBox::information(this,tr("Connection closed"),tr("Connection refused by the server"));
	break;
	case QAbstractSocket::SocketTimeoutError:
		QMessageBox::information(this,tr("Connection closed"),tr("Socket time out, server too long"));
	break;
	default:
		QMessageBox::information(this,tr("Connection error"),tr("Connection error: %1").arg(socketError));
	}
}

void MainWindow::haveNewError()
{
	QMessageBox::critical(this,tr("Error"),client.errorString());
}

void MainWindow::protocol_is_good()
{
	client.tryLogin(ui->lineEditLogin->text(),ui->lineEditPass->text());
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
	if(!text.startsWith("/pm "))
		client.sendChatText(ui->comboBox_chat_type->currentIndex()+2,text);
	else if(text.contains(QRegExp("^/pm [^ ]+ .+$")))
	{
		QString pseudo=text;
		pseudo.replace(QRegExp("^/pm ([^ ]+) .+$"), "\\1");
		text.replace(QRegExp("^/pm [^ ]+ (.+)$"), "\\1");
		client.sendPM(text,pseudo);
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
	client.add_player_watching(player_id);
	chat_list_player_id << player_id;
	chat_list_type << chat_type;
	chat_list_text << text;
	while(chat_list_player_id.size()>64)
	{
		client.remove_player_watching(chat_list_player_id.first());
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
	QList<Player_informations> player_informations_list=client.get_player_informations_list();
	QString nameHtml;
	int index=0;
	while(index<chat_list_player_id.size())
	{
		bool addPlayerInfo=true;
		if(chat_list_type.at(index)==0x07 || chat_list_type.at(index)==0x08)
			addPlayerInfo=false;
		int index_player=-1;
		if(addPlayerInfo)
			index_player=client.indexOfPlayerInformations(chat_list_player_id.at(index));
		if(!addPlayerInfo || index_player!=-1)
		{

			nameHtml+="<div id=\""+QString::number(index)+"\" style=\"";
			switch(chat_list_type.at(index))
			{
				case 0x02://all
				break;
				case 0x03://clan
					nameHtml+="color:#FFBF00;";
				break;
				/*case 0x04://aliance
					nameHtml+="color:#60BF20;";
				break;*/
				case 0x06://to player
					nameHtml+="color:#5C255F;";
				break;
				case 0x07://system
					nameHtml+="color:#707070;font-style:italic;font-size:small;";
					addPlayerInfo=false;
				break;
				case 0x08://system importance: hight
					nameHtml+="color:#60BF20;";
					addPlayerInfo=false;
				break;
				default:
				break;
			}
			if(addPlayerInfo)
			{
				switch(player_informations_list.at(index_player).status)
				{
					case 0x01://normal player
					break;
					case 0x02://premium player
					break;
					case 0x03://gm
						nameHtml+="font-weight:bold;";
					break;
					case 0x04://dev
						nameHtml+="font-weight:bold;";
					break;
					default:
					break;
				}
			}
			nameHtml+="\">";
			if(addPlayerInfo)
			{
				switch(player_informations_list.at(index_player).status)
				{
					case 0x01://normal player
					break;
					case 0x02://premium player
						nameHtml+="<img src=\":/images/chat/pokeballPremium.png\" alt\"\" />";
					break;
					case 0x03://gm
						nameHtml+="<img src=\":/images/chat/pokeballAdmin.png\" alt\"\" />";
					break;
					case 0x04://dev
						nameHtml+="<img src=\":/images/chat/pokeballDeveloper.png\" alt\"\" />";
					break;
					default:
					break;
				}
				nameHtml+=QString("%1: ").arg(player_informations_list.at(index_player).pseudo);
			}
			if(chat_list_type.at(index)==0x07 || chat_list_type.at(index)==0x06)
				nameHtml+=QString("%1").arg(chat_list_text.at(index));
			else
				nameHtml+=QString("%1").arg(toSmilies(toHtmlEntities(chat_list_text.at(index))));
			nameHtml+="</div>";
		}
		index++;
	}
	ui->textBrowser_chat->setHtml(nameHtml);
	ui->textBrowser_chat->scrollToAnchor(QString::number(index-1));
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
	client.tryDisconnect();
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
