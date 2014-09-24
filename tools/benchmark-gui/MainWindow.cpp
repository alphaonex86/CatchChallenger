#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "../bot/simple/SimpleAction.h"

#include <unistd.h>

#include <QMessageBox>
#include <QFileDialog>

#define BENCHMARKTIMEASKCOUNT 5

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    inClosing(false),
    waitTheReturn(false),
    dropTime(false),
    dropStat(false),
    systemItem(-1)
{
    qRegisterMetaType<QLocalSocket::LocalSocketState>("QLocalSocket::LocalSocketState");
    qRegisterMetaType<QLocalSocket::LocalSocketError>("QLocalSocket::LocalSocketError");
    qRegisterMetaType<QProcess::ProcessState>("QProcess::ProcessState");
    qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
    ui->setupUi(this);
    serverStarted=false;
    //connect(&timer,&QTimer::timeout,this,&MainWindow::updateUserTime);

    connect(&socket,&QLocalSocket::stateChanged,this,&MainWindow::updateSocketState,Qt::QueuedConnection);
    connect(&socket,&QLocalSocket::readyRead,this,&MainWindow::readyRead,Qt::QueuedConnection);

    connect(&process,&QProcess::stateChanged,this,&MainWindow::stateChanged,Qt::QueuedConnection);
    connect(&process,static_cast<void(QProcess::*)(QProcess::ProcessError)>(&QProcess::error),this,&MainWindow::processError,Qt::QueuedConnection);
    connect(&process,&QProcess::readyReadStandardOutput,this,&MainWindow::readyReadStandardOutput,Qt::QueuedConnection);
    connect(&process,&QProcess::readyReadStandardError,this,&MainWindow::readyReadStandardError,Qt::QueuedConnection);

    connect(&multipleBotConnection,&MultipleBotConnectionImplFoprGui::emit_all_player_on_map,this,&MainWindow::all_player_on_map,Qt::QueuedConnection);

    stopBenchmarkTimer.setInterval(1000);
    stopBenchmarkTimer.setSingleShot(true);
    connect(&stopBenchmarkTimer,&QTimer::timeout,this,&MainWindow::stopBenchmark,Qt::QueuedConnection);

    //timer.setInterval(200);
    if(settings.contains("server"))
        ui->server->setText(settings.value("server").toString());
    if(settings.contains("multipleConnexion"))
        ui->connexionCount->setValue(settings.value("multipleConnexion").toUInt());
    if(settings.contains("connectBySeconds"))
        ui->connectBySeconds->setValue(settings.value("connectBySeconds").toUInt());
    if(settings.contains("maxDiffConnectedSelected"))
        ui->maxDiffConnectedSelected->setValue(settings.value("maxDiffConnectedSelected").toUInt());

    multipleBotConnection.botInterface=new SimpleAction();
    systemClockTick=sysconf(_SC_CLK_TCK);
}

MainWindow::~MainWindow()
{
    delete multipleBotConnection.botInterface;
    delete ui;
}

void MainWindow::stateChanged(QProcess::ProcessState newState)
{
    Q_UNUSED(newState);
    if(newState==QProcess::NotRunning)
    {
        if(serverStarted==false && socket.state()==QLocalSocket::UnconnectedState)
        {
            socket.connectToServer("/tmp/catchchallenger-server.sock");
            return;
        }
    }
}

void MainWindow::processError(QProcess::ProcessError error)
{
    Q_UNUSED(error);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    disconnect(&socket);
    event->ignore();
    QCoreApplication::quit();
}

void MainWindow::updateUserTime()
{
    if(systemItem!=-1)
        stopSystemMeasure();
    if(waitTheReturn)
        return;
    waitTheReturn=true;
    QByteArray data;
    data[0]=0x01;
    socket.write(data);
}

void MainWindow::readyRead()
{
    waitTheReturn=false;
    const QByteArray &data=socket.readAll();
    if(dropTime)
    {
        dropTime=false;
        return;
    }
    if(data.size()==sizeof(unsigned long long)*4)
    {
        {
            const unsigned long long timeUsed=*reinterpret_cast<const unsigned long long *>(data.constData()+sizeof(unsigned long long)*0);
            if(timeUsed>0)
            {
                double timeUsedDouble=timeUsed;
                timeUsedDouble/=1000;
                ui->userTime->setText(tr("Cpu time used: %1").arg(convertUsToString(timeUsedDouble)));
                {
                    timeList << timeUsedDouble;
                    if(timeList.size()>32)
                        timeList.removeFirst();
                    double averageTimeUsed=0;
                    double max=0,min=0;
                    int index=0;
                    while(index<timeList.size())
                    {
                        const double &tempDouble=timeList.at(index);
                        averageTimeUsed+=tempDouble;
                        if(max<tempDouble)
                            max=tempDouble;
                        if(min==0 || min>tempDouble)
                            min=tempDouble;
                        index++;
                    }
                    if(!timeList.isEmpty())
                        averageTimeUsed/=timeList.size();
                    ui->userTimeAverage->setText(tr("Average cpu time used: %1").arg(convertUsToString(averageTimeUsed)));
                    QString minString,maxString;
                    maxString=tr("Max: %1").arg(convertUsToString(max));
                    minString=tr(", min: %1").arg(convertUsToString(min));
                    ui->maxmin->setText(maxString+minString);
                }
            }
            ui->groupBox->setVisible(timeUsed!=0);
            ui->groupBox_2->setVisible(timeUsed!=0);
            ui->groupBox_3->setVisible(timeUsed!=0);
            ui->groupBox_4->setVisible(timeUsed!=0);
        }
        {
            const unsigned long long timeUsedForUser=*reinterpret_cast<const unsigned long long *>(data.constData()+sizeof(unsigned long long)*1);
            double timeUsedDouble=timeUsedForUser;
            timeUsedDouble/=1000;
            ui->userTimeForUser->setText(tr("Cpu time used: %1").arg(convertUsToString(timeUsedDouble)));
            {
                timeListForUser << timeUsedDouble;
                if(timeListForUser.size()>32)
                    timeListForUser.removeFirst();
                double averageTimeUsed=0;
                double max=0,min=0;
                int index=0;
                while(index<timeListForUser.size())
                {
                    const double &tempDouble=timeListForUser.at(index);
                    averageTimeUsed+=tempDouble;
                    if(max<tempDouble)
                        max=tempDouble;
                    if(min==0 || min>tempDouble)
                        min=tempDouble;
                    index++;
                }
                if(!timeListForUser.isEmpty())
                    averageTimeUsed/=timeListForUser.size();
                ui->userTimeAverageForUser->setText(tr("Average cpu time used: %1").arg(convertUsToString(averageTimeUsed)));
                QString minString,maxString;
                maxString=tr("Max: %1").arg(convertUsToString(max));
                minString=tr(", min: %1").arg(convertUsToString(min));
                ui->maxminForUser->setText(maxString+minString);
            }
        }
        {
            const unsigned long long timeUsedForTimer=*reinterpret_cast<const unsigned long long *>(data.constData()+sizeof(unsigned long long)*2);
            double timeUsedDouble=timeUsedForTimer;
            timeUsedDouble/=1000;
            ui->userTimeForTimer->setText(tr("Cpu time used: %1").arg(convertUsToString(timeUsedDouble)));
            {
                timeListForTimer << timeUsedDouble;
                if(timeListForTimer.size()>32)
                    timeListForTimer.removeFirst();
                double averageTimeUsed=0;
                double max=0,min=0;
                int index=0;
                while(index<timeListForTimer.size())
                {
                    const double &tempDouble=timeListForTimer.at(index);
                    averageTimeUsed+=tempDouble;
                    if(max<tempDouble)
                        max=tempDouble;
                    if(min==0 || min>tempDouble)
                        min=tempDouble;
                    index++;
                }
                if(!timeListForTimer.isEmpty())
                    averageTimeUsed/=timeListForTimer.size();
                ui->userTimeAverageForTimer->setText(tr("Average cpu time used: %1").arg(convertUsToString(averageTimeUsed)));
                QString minString,maxString;
                maxString=tr("Max: %1").arg(convertUsToString(max));
                minString=tr(", min: %1").arg(convertUsToString(min));
                ui->maxminForTimer->setText(maxString+minString);
            }
        }
        {
            const unsigned long long timeUsedForDatabase=*reinterpret_cast<const unsigned long long *>(data.constData()+sizeof(unsigned long long)*3);
            double timeUsedDouble=timeUsedForDatabase;
            timeUsedDouble/=1000;
            ui->userTimeForDatabase->setText(tr("Cpu time used: %1").arg(convertUsToString(timeUsedDouble)));
            {
                timeListForDatabase << timeUsedDouble;
                if(timeListForDatabase.size()>32)
                    timeListForDatabase.removeFirst();
                double averageTimeUsed=0;
                double max=0,min=0;
                int index=0;
                while(index<timeListForDatabase.size())
                {
                    const double &tempDouble=timeListForDatabase.at(index);
                    averageTimeUsed+=tempDouble;
                    if(max<tempDouble)
                        max=tempDouble;
                    if(min==0 || min>tempDouble)
                        min=tempDouble;
                    index++;
                }
                if(!timeListForDatabase.isEmpty())
                    averageTimeUsed/=timeListForDatabase.size();
                ui->userTimeAverageForDatabase->setText(tr("Average cpu time used: %1").arg(convertUsToString(averageTimeUsed)));
                QString minString,maxString;
                maxString=tr("Max: %1").arg(convertUsToString(max));
                minString=tr(", min: %1").arg(convertUsToString(min));
                ui->maxminForDatabase->setText(maxString+minString);
            }
        }
    }
    if(dropStat)
    {
        dropStat=false;
        timeList.clear();
        timeListForUser.clear();
        timeListForTimer.clear();
        timeListForDatabase.clear();
        ui->timeLabel->setText(tr("Time of: %1").arg(returnType));
    }
}

QString MainWindow::convertUsToString(quint64 us)
{
    if(us>5000000)
        return tr("%1s").arg(QString::number(us/1000000,'g',4));
    else if(us>5000)
        return tr("%1ms").arg(QString::number(us/1000,'g',4));
    else
        return tr("%1us").arg(QString::number(us,'g',4));
}

void MainWindow::updateSocketState(QLocalSocket::LocalSocketState socketState)
{
    if(inClosing)
        return;
    if(socketState==QLocalSocket::ConnectedState)
    {
        srand(0);
        ui->statusBar->showMessage(tr("Connected"),0);
        dropTime=true;
        updateUserTime();
        startServer();
    }
    else
    {
        if(socketState==QLocalSocket::UnconnectedState)
        {
            inClosing=true;
            if(socket.error()==QLocalSocket::UnknownSocketError)
                QMessageBox::warning(this,tr("Error"),tr("The connexion to the server have been closed"));
            else
                QMessageBox::warning(this,tr("Error"),tr("The connexion to the server have been closed: %1").arg(socket.errorString()));
            QCoreApplication::quit();
        }
        return;
    }
}

void MainWindow::on_server_editingFinished()
{
    settings.setValue("server",ui->server->text());
}

void MainWindow::on_serverBrowse_clicked()
{
    const QString &file=QFileDialog::getOpenFileName(this);
    if(file.isEmpty())
        return;
    ui->server->setText(file);
    settings.setValue("server",file);
}

void MainWindow::on_start_clicked()
{
    settings.setValue("multipleConnexion",ui->connexionCount->value());
    settings.setValue("connectBySeconds",ui->connectBySeconds->value());
    settings.setValue("maxDiffConnectedSelected",ui->maxDiffConnectedSelected->value());

    ui->groupBoxMultipleConnexion->setEnabled(false);
    ui->start->setEnabled(false);
    ui->server->setEnabled(false);
    ui->serverBrowse->setEnabled(false);
    if(QFile(ui->server->text()).exists())
    {
        process.setProgram(ui->server->text());
        process.start();
    }
    else
        QMessageBox::critical(this,tr("Error"),tr("Unable to find %1").arg(ui->server->text()));
}

void MainWindow::readyReadStandardError()
{
    if(!serverStarted)
        if(QString::fromLocal8Bit(process.readAllStandardError()).contains(QStringLiteral("Waiting connection on port ")))
        {
            srand(0);
            serverStarted=true;
            //timer.start();
            ui->start->setEnabled(false);
            if(socket.state()==QLocalSocket::UnconnectedState)
            {
                socket.connectToServer("/tmp/catchchallenger-server.sock");
                return;
            }
        }
}

void MainWindow::readyReadStandardOutput()
{
    if(!serverStarted)
        if(QString::fromLocal8Bit(process.readAllStandardOutput()).contains(QStringLiteral("Waiting connection on port ")))
        {
            serverStarted=true;
            //timer.start();
            ui->start->setEnabled(false);
            if(socket.state()==QLocalSocket::UnconnectedState)
            {
                socket.connectToServer("/tmp/catchchallenger-server.sock");
                return;
            }
        }
}

void MainWindow::startServer()
{
    multipleBotConnection.mLogin=QStringLiteral("benchmark%NUMBER%");
    multipleBotConnection.mPass=QStringLiteral("benchmarkD4tVPlylnyPA1lwF%NUMBER%");
    multipleBotConnection.mMultipleConnexion=true;
    multipleBotConnection.mAutoCreateCharacter=true;
    multipleBotConnection.mConnectBySeconds=ui->connectBySeconds->value();
    multipleBotConnection.mConnexionCount=ui->connexionCount->value();
    multipleBotConnection.mMaxDiffConnectedSelected=ui->maxDiffConnectedSelected->value();
    multipleBotConnection.mProxy=QString();
    multipleBotConnection.mProxyport=0;
    multipleBotConnection.mHost=QString("localhost");
    multipleBotConnection.mPort=42489;

    //do only the first client to download the datapack
    multipleBotConnection.createClient();
    startSystemMeasure();
}

void MainWindow::all_player_on_map()
{
    ui->timeLabel->setText(QString());
    returnType=tr("Connect all player");
    ui->groupBox_benchmark->setEnabled(true);
    updateUserTime();
    dropStat=true;
}

void MainWindow::stopBenchmark()
{
    updateUserTime();
    if(stopBenchmarkCount<BENCHMARKTIMEASKCOUNT)
        stopBenchmarkTimer.start();
    else
    {
        dropStat=true;
        ui->groupBox_benchmark->setEnabled(true);
    }
    stopBenchmarkCount++;
}

void MainWindow::resetBotConnection()
{
    multipleBotConnection.botInterface->setValue(QStringLiteral("move"),false);
    multipleBotConnection.botInterface->setValue(QStringLiteral("randomText"),false);
    multipleBotConnection.botInterface->setValue(QStringLiteral("globalChatRandomReply"),false);
}

void MainWindow::on_benchmarkIdle_clicked()
{
    if(stopBenchmarkTimer.isActive())
        return;
    stopBenchmarkCount=0;
    srand(0);
    resetBotConnection();
    dropTime=true;
    updateUserTime();
    ui->timeLabel->setText(QString());
    returnType=tr("Idle during %1ms").arg(stopBenchmarkTimer.interval()*BENCHMARKTIMEASKCOUNT);
    ui->groupBox_benchmark->setEnabled(false);
    stopBenchmarkTimer.start();
    startSystemMeasure();
}

void MainWindow::on_benchmarkMove_clicked()
{
    if(stopBenchmarkTimer.isActive())
        return;
    stopBenchmarkCount=0;
    srand(0);
    resetBotConnection();
    multipleBotConnection.botInterface->setValue(QStringLiteral("move"),true);
    dropTime=true;
    updateUserTime();
    ui->timeLabel->setText(QString());
    returnType=tr("Move during %1ms").arg(stopBenchmarkTimer.interval()*BENCHMARKTIMEASKCOUNT);
    ui->groupBox_benchmark->setEnabled(false);
    stopBenchmarkTimer.start();
    startSystemMeasure();
}

void MainWindow::on_benchmarkChat_clicked()
{
    if(stopBenchmarkTimer.isActive())
        return;
    stopBenchmarkCount=0;
    srand(0);
    resetBotConnection();
    multipleBotConnection.botInterface->setValue(QStringLiteral("randomText"),true);
    multipleBotConnection.botInterface->setValue(QStringLiteral("globalChatRandomReply"),true);
    dropTime=true;
    updateUserTime();
    ui->timeLabel->setText(QString());
    returnType=tr("Chat during %1ms").arg(stopBenchmarkTimer.interval()*BENCHMARKTIMEASKCOUNT);
    ui->groupBox_benchmark->setEnabled(false);
    stopBenchmarkTimer.start();
    startSystemMeasure();
}

void MainWindow::startSystemMeasure()
{
    ui->systemTime->setText(QString());
    systemItem=getSystemTime();
    time.restart();
}

void MainWindow::stopSystemMeasure()
{
    if(systemItem!=-1)
    {
        double newSystemItem=getSystemTime();
        if(systemClockTick!=0)
            ui->systemTime->setText(tr("System time: %1 (%2 clk), real: %3ms").arg((newSystemItem-systemItem)/systemClockTick).arg(newSystemItem-systemItem).arg(time.elapsed()));
        else
            ui->systemTime->setText(tr("System time: %1 clk, real: %2ms").arg(newSystemItem-systemItem).arg(time.elapsed()));
    }
}

double MainWindow::getSystemTime()
{
    if(process.state()==QProcess::Running)
    {
        QFile file(QStringLiteral("/proc/%1/stat").arg(process.pid()));
        if(file.open(QIODevice::ReadOnly))
        {
            QString content=QString::fromLocal8Bit(file.readAll());
            const QStringList &list=content.split(" ");
            if(list.size()>15)
            {
                QString matched = list.at(13);
                bool ok;
                double returnedValue=matched.toDouble(&ok);
                if(!ok)
                {
                    qDebug() << "File" << file.fileName() << "don't have numeric value" << content;
                    abort();
                }
                return (returnedValue);
            }
            else
                qDebug() << "File" << file.fileName() << "don't have the value" << content;
        }
        else
            qDebug() << "Missing file to do system time:" << file.fileName();
    }
    return -1;
}
