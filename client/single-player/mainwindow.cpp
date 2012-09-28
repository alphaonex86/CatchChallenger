#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "NewGame.h"

#include <QSettings>

#include "../base/render/MapVisualiserPlayer.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
	qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
	qRegisterMetaType<Pokecraft::Chat_type>("Pokecraft::Chat_type");
	qRegisterMetaType<Pokecraft::Player_type>("Pokecraft::Player_type");

    client=new Pokecraft::Api_client_real(&socket);
    baseWindow=new Pokecraft::BaseWindow(client);
    spacer=new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Expanding);
	ui->setupUi(this);
    ui->stackedWidget->addWidget(baseWindow);

    connect(client,SIGNAL(protocol_is_good()),this,SLOT(protocol_is_good()));
	connect(client,SIGNAL(disconnected(QString)),this,SLOT(disconnected(QString)));
	connect(client,SIGNAL(message(QString)),this,SLOT(message(QString)));
	connect(&socket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(error(QAbstractSocket::SocketError)),Qt::QueuedConnection);
    connect(&socket,SIGNAL(stateChanged(QAbstractSocket::SocketState)),this,SLOT(stateChanged(QAbstractSocket::SocketState)));

	stopFlood.setSingleShot(false);
	stopFlood.start(1500);
	numberForFlood=0;
	haveShowDisconnectionReason=false;
    ui->stackedWidget->addWidget(baseWindow);
    baseWindow->setMultiPlayer(true);

    stateChanged(QAbstractSocket::UnconnectedState);

    /*    ui->stackedWidget->setCurrentIndex(1);
    client->tryConnect(host,port);*/
    updateSavegameList();
}

MainWindow::~MainWindow()
{
	client->tryDisconnect();
	delete client;
    delete baseWindow;
	delete ui;
}

void MainWindow::resetAll()
{
    baseWindow->resetAll();
	ui->stackedWidget->setCurrentIndex(0);
	chat_list_player_pseudo.clear();
	chat_list_player_type.clear();
	chat_list_type.clear();
	chat_list_text.clear();
	lastMessageSend="";

    //stateChanged(QAbstractSocket::UnconnectedState);//don't call here, else infinity rescursive call
}

void MainWindow::disconnected(QString reason)
{
	QMessageBox::information(this,tr("Disconnected"),tr("Disconnected by the reason: %1").arg(reason));
	haveShowDisconnectionReason=true;
	resetAll();
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
    baseWindow->stateChanged(socketState);
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

void MainWindow::message(QString message)
{
	qDebug() << message;
}

void MainWindow::protocol_is_good()
{
    client->tryLogin("","");
}

void MainWindow::on_SaveGame_New_clicked()
{
    NewGame nameGame(this);
    if(!nameGame.haveSkin())
    {
        QMessageBox::critical(this,tr("Error"),QString("Sorry but no skin found into: %1").arg(QFileInfo(QCoreApplication::applicationDirPath()+"/datapack/skin/fighter/").absoluteFilePath()));
        return;
    }
    nameGame.exec();
    if(!nameGame.haveTheInformation())
        return;
    int index=0;
    while(QDir().exists(QCoreApplication::applicationDirPath()+"/savegames/"+QString::number(index)))
        index++;
    QString savegamesPath=QCoreApplication::applicationDirPath()+"/savegames/"+QString::number(index)+"/";
    if(!QDir().mkpath(savegamesPath))
    {
        QMessageBox::critical(this,tr("Error"),QString("Unable to write savegame into: %1").arg(savegamesPath));
        return;
    }
    if(!QFile::copy(":/pokecraft.db.sqlite",savegamesPath+"pokecraft.db.sqlite"))
    {
        QMessageBox::critical(this,tr("Error"),QString("Unable to write savegame into: %1").arg(savegamesPath));
        rmpath(savegamesPath);
        return;
    }
    bool settingOk=true;
    {
        QSettings metaData(savegamesPath+"metadata.conf",QSettings::IniFormat);
        if(!metaData.isWritable())
            settingOk=false;
        else
        {
            if(metaData.status()==QSettings::NoError)
            {
                metaData.setValue("title",nameGame.gameName());
                metaData.setValue("location","Starting city");
                metaData.setValue("time_played",0);
            }
            else
            {
                qDebug() << "Settings error: " << metaData.status();
                settingOk=false;
            }
        }
    }
    if(!settingOk)
    {
        QMessageBox::critical(this,tr("Error"),QString("Unable to write savegame into: %1").arg(savegamesPath));
        rmpath(savegamesPath);
        return;
    }
}

bool MainWindow::rmpath(const QDir &dir)
{
    if(!dir.exists())
        return true;
    bool allHaveWork=true;
    QFileInfoList list = dir.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);
    for (int i = 0; i < list.size(); ++i)
    {
        QFileInfo fileInfo(list.at(i));
        if(!fileInfo.isDir())
        {
            if(!QFile(fileInfo.absoluteFilePath()).remove())
            {
                qDebug() << "Unable the remove the file: " << fileInfo.absoluteFilePath();
                allHaveWork=false;
            }
        }
        else
        {
            //return the fonction for scan the new folder
            if(!rmpath(dir.absolutePath()+'/'+fileInfo.fileName()+'/'))
                allHaveWork=false;
        }
    }
    if(!allHaveWork)
        return allHaveWork;
    if(!dir.rmdir(dir.absolutePath()))
    {
        qDebug() << "Unable the remove the file: " << dir.absolutePath();
        return false;
    }
    return true;
}

void MainWindow::updateSavegameList()
{
    int index=0;
    while(index<savegame.size())
    {
        delete savegame.at(index);
        index++;
    }
    index=0;
    while(QDir(QCoreApplication::applicationDirPath()+"/savegames/"+QString::number(index)+"/").exists())
    {
        QString savegamesPath=QCoreApplication::applicationDirPath()+"/savegames/"+QString::number(index)+"/";
        QSettings metaData(savegamesPath+"metadata.conf",QSettings::IniFormat);
        QLabel *newEntry=new QLabel();
        newEntry->setStyleSheet("QLabel::hover{border:1px solid #bbb;background-color:rgb(180,180,180,100);border-radius:10px;}");
        QString dateString=QFileInfo(savegamesPath+"metadata.conf").lastModified().toString("dd/MM/yyyy hh:mm:ssAP");

        if(!metaData.isWritable())
            newEntry->setText(QString("<span style=\"font-size:12pt;font-weight:600;\">%1</span><br/><span style=\"color:#909090;\">%2<br/>Bug</span>").arg("Bug").arg(dateString));
        else
        {
            if(metaData.status()==QSettings::NoError)
            {
                bool ok;
                int time_played_number=metaData.value("time_played").toUInt(&ok);
                QString time_played;
                if(!ok)
                    time_played="Time player: bug";
                else if(time_played_number>=3600*24)
                    time_played=QObject::tr("%n day(s) played","",time_played_number/3600*24);
                else if(time_played_number>=3600)
                    time_played=QObject::tr("%n hour(s) played","",time_played_number/3600);
                else
                    time_played=QObject::tr("%n minute(s) played","",time_played_number/60);
                newEntry->setText(QString("<span style=\"font-size:12pt;font-weight:600;\">%1</span><br/><span style=\"color:#909090;\">%2<br/>%3 (%4)</span>")
                                  .arg(metaData.value("title").toString())
                                  .arg(dateString)
                                  .arg(metaData.value("location").toString())
                                  .arg(time_played)
                                  );
            }
            else
                newEntry->setText(QString("<span style=\"font-size:12pt;font-weight:600;\">%1</span><br/><span style=\"color:#909090;\">%2<br/>Bug</span>").arg(metaData.value("title").toString()).arg(dateString));
        }
        ui->scrollAreaWidgetContents->layout()->addWidget(newEntry);

        savegame << newEntry;
        index++;
    }
    ui->savegameEmpty->setVisible(index==0);
    if(index>0)
    {
        delete spacer;
        spacer=new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Expanding);
        ui->scrollAreaWidgetContents->layout()->addItem(spacer);
    }
}
