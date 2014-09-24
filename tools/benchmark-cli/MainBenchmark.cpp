#include "MainBenchmark.h"
#include "../bot/simple/SimpleAction.h"

#include <unistd.h>

#include <QMessageBox>
#include <QFileDialog>

MainBenchmark::MainBenchmark() :
    inClosing(false),
    systemItem(-1),
    multipleConnexion(480),
    connectBySeconds(99),
    maxDiffConnectedSelected(20),
    benchmarkStep(0)
{
    qRegisterMetaType<QLocalSocket::LocalSocketState>("QLocalSocket::LocalSocketState");
    qRegisterMetaType<QLocalSocket::LocalSocketError>("QLocalSocket::LocalSocketError");
    qRegisterMetaType<QProcess::ProcessState>("QProcess::ProcessState");
    qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");

    serverStarted=false;
    //connect(&timer,&QTimer::timeout,this,&MainWindow::updateUserTime);

    connect(&startAll,&QTimer::timeout,this,&MainBenchmark::init,Qt::QueuedConnection);
    startAll.setSingleShot(true);
    startAll.start(0);

    connect(&socket,&QLocalSocket::readyRead,this,&MainBenchmark::readyRead,Qt::QueuedConnection);

    connect(&process,static_cast<void(QProcess::*)(QProcess::ProcessError)>(&QProcess::error),this,&MainBenchmark::processError,Qt::QueuedConnection);
    connect(&process,&QProcess::readyReadStandardOutput,this,&MainBenchmark::readyReadStandardOutput,Qt::QueuedConnection);
    connect(&process,&QProcess::readyReadStandardError,this,&MainBenchmark::readyReadStandardError,Qt::QueuedConnection);

    connect(&multipleBotConnection,&MultipleBotConnectionImplFoprGui::emit_all_player_on_map,this,&MainBenchmark::all_player_on_map,Qt::QueuedConnection);

    stopBenchmarkTimer.setInterval(10000);
    stopBenchmarkTimer.setSingleShot(true);
    connect(&stopBenchmarkTimer,&QTimer::timeout,this,&MainBenchmark::stopBenchmark,Qt::QueuedConnection);

    if(settings.contains("server"))
        server=settings.value("server").toString();
    else
        settings.setValue("server","catchchallenger-server-cli-epoll");
    if(settings.contains("multipleConnexion"))
        multipleConnexion=settings.value("multipleConnexion").toUInt();
    else
        settings.setValue("multipleConnexion","480");
    if(settings.contains("connectBySeconds"))
        connectBySeconds=settings.value("connectBySeconds").toUInt();
    else
        settings.setValue("connectBySeconds","99");
    if(settings.contains("maxDiffConnectedSelected"))
        maxDiffConnectedSelected=settings.value("maxDiffConnectedSelected").toUInt();
    else
        settings.setValue("maxDiffConnectedSelected","20");

    multipleBotConnection.botInterface=new SimpleAction();
    systemClockTick=sysconf(_SC_CLK_TCK);
}

MainBenchmark::~MainBenchmark()
{
    delete multipleBotConnection.botInterface;
}

void MainBenchmark::init()
{
    if(QFile(server).exists())
    {
        process.setProgram(server);
        process.start();
    }
    else
    {
        qDebug() << tr("Unable to find %1").arg(server);
        QCoreApplication::quit();
    }
}

void MainBenchmark::processError(QProcess::ProcessError error)
{
    Q_UNUSED(error);
}

void MainBenchmark::updateUserTime()
{
    if(systemItem!=-1)
        stopSystemMeasure();
}

QString MainBenchmark::convertUsToString(quint64 us)
{
    if(us>5000000)
        return tr("%1s").arg(QString::number(us/1000000,'g',4));
    else if(us>5000)
        return tr("%1ms").arg(QString::number(us/1000,'g',4));
    else
        return tr("%1us").arg(QString::number(us,'g',4));
}

void MainBenchmark::readyReadStandardError()
{
    if(!serverStarted)
        if(QString::fromLocal8Bit(process.readAllStandardError()).contains(QStringLiteral("Waiting connection on port ")))
        {
            socket.connectToServer("/tmp/catchchallenger-server.sock");
            socket.waitForConnected(0);
            srand(0);
            serverStarted=true;
            //timer.start();
            srand(0);
            qDebug() << "The server is ready for the benchmark";
            startServerConnexion();
            return;
        }
}

void MainBenchmark::readyReadStandardOutput()
{
    if(!serverStarted)
        if(QString::fromLocal8Bit(process.readAllStandardOutput()).contains(QStringLiteral("Waiting connection on port ")))
        {
            socket.connectToServer("/tmp/catchchallenger-server.sock");
            socket.waitForConnected(0);
            serverStarted=true;
            //timer.start();
            srand(0);
            qDebug() << "The server is ready for the benchmark";
            startServerConnexion();
            return;
        }
}

void MainBenchmark::startServerConnexion()
{
    multipleBotConnection.mLogin=QStringLiteral("benchmark%NUMBER%");
    multipleBotConnection.mPass=QStringLiteral("benchmarkD4tVPlylnyPA1lwF%NUMBER%");
    multipleBotConnection.mMultipleConnexion=true;
    multipleBotConnection.mAutoCreateCharacter=true;
    multipleBotConnection.mConnectBySeconds=connectBySeconds;
    multipleBotConnection.mConnexionCount=multipleConnexion;
    multipleBotConnection.mMaxDiffConnectedSelected=maxDiffConnectedSelected;
    multipleBotConnection.mProxy=QString();
    multipleBotConnection.mProxyport=0;
    multipleBotConnection.mHost=QString("localhost");
    multipleBotConnection.mPort=42489;

    //do only the first client to download the datapack
    multipleBotConnection.createClient();
    startSystemMeasure();
}

void MainBenchmark::all_player_on_map()
{
    qDebug() << "Connect all player";
    updateUserTime();
}

void MainBenchmark::stopBenchmark()
{
    updateUserTime();
}

void MainBenchmark::resetBotConnection()
{
    multipleBotConnection.botInterface->setValue(QStringLiteral("move"),false);
    multipleBotConnection.botInterface->setValue(QStringLiteral("randomText"),false);
    multipleBotConnection.botInterface->setValue(QStringLiteral("globalChatRandomReply"),false);
}

void MainBenchmark::benchmarkIdle()
{
    if(stopBenchmarkTimer.isActive())
        return;
    srand(0);
    resetBotConnection();
    stopBenchmarkTimer.start();
    startSystemMeasure();
}

void MainBenchmark::benchmarkMove()
{
    if(stopBenchmarkTimer.isActive())
        return;
    srand(0);
    resetBotConnection();
    multipleBotConnection.botInterface->setValue(QStringLiteral("move"),true);
    stopBenchmarkTimer.start();
    startSystemMeasure();
}

void MainBenchmark::benchmarkChat()
{
    if(stopBenchmarkTimer.isActive())
        return;
    srand(0);
    resetBotConnection();
    multipleBotConnection.botInterface->setValue(QStringLiteral("randomText"),true);
    multipleBotConnection.botInterface->setValue(QStringLiteral("globalChatRandomReply"),true);
    stopBenchmarkTimer.start();
    startSystemMeasure();
}

void MainBenchmark::startSystemMeasure()
{
    systemItem=getSystemTime();
    time.restart();
}

void MainBenchmark::stopSystemMeasure()
{
    if(systemItem!=-1)
    {
        double newSystemItem=getSystemTime();
        //don't translate to allow grab it
        switch(benchmarkStep)
        {
            case 0:
                qDebug() << QStringLiteral("Result to connect all player:") << newSystemItem-systemItem;
                benchmarkIdle();
            break;
            case 1:
                qDebug() << QStringLiteral("Result to idle server:") << newSystemItem-systemItem;
                benchmarkMove();
            break;
            case 2:
                qDebug() << QStringLiteral("Result to moving on server:") << newSystemItem-systemItem;
                benchmarkChat();
            break;
            case 3:
                qDebug() << QStringLiteral("Result to chat:") << newSystemItem-systemItem;
            break;
            default:
            return;
        }
        QByteArray data;
        data[0]=0x01;
        socket.write(data);

        benchmarkStep++;
    }
}

double MainBenchmark::getSystemTime()
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

void MainBenchmark::readyRead()
{
    const QByteArray &data=socket.readAll();
    if(data.size()==sizeof(unsigned long long)*4)
    {
        unsigned long long timeUsed=*reinterpret_cast<const unsigned long long *>(data.constData()+sizeof(unsigned long long)*0);
        timeUsed/=1000;
        switch(benchmarkStep)
        {
            case 1:
                if(timeUsed!=0)
                    qDebug() << QStringLiteral("Result internal to connect all player:") << timeUsed;
            break;
            case 2:
                if(timeUsed!=0)
                    qDebug() << QStringLiteral("Result internal to idle server:") << timeUsed;
            break;
            case 3:
                if(timeUsed!=0)
                    qDebug() << QStringLiteral("Result internal to moving on server:") << timeUsed;
            break;
            case 4:
                if(timeUsed!=0)
                    qDebug() << QStringLiteral("Result internal to chat:") << timeUsed;
                process.terminate();
                process.kill();
                process.waitForFinished(0);
                QCoreApplication::quit();
            break;
            default:
            return;
        }
    }
}
