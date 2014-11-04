#include "MainBenchmark.h"
#include "../bot/simple/SimpleAction.h"

#include <unistd.h>

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
    connect(&process,&QProcess::stateChanged,this,&MainBenchmark::processStateChanged,Qt::QueuedConnection);
    connect(&process,&QProcess::readyReadStandardOutput,this,&MainBenchmark::readyReadStandardOutput,Qt::QueuedConnection);
    connect(&process,&QProcess::readyReadStandardError,this,&MainBenchmark::readyReadStandardError,Qt::QueuedConnection);

    connect(&multipleBotConnection,&MultipleBotConnectionImplFoprGui::emit_all_player_on_map,this,&MainBenchmark::all_player_on_map,Qt::QueuedConnection);

    stopBenchmarkTimer.setInterval(10000);
    stopBenchmarkTimer.setSingleShot(true);
    connect(&stopBenchmarkTimer,&QTimer::timeout,this,&MainBenchmark::stopBenchmark,Qt::QueuedConnection);

    const QStringList &args=QCoreApplication::arguments();

    {
        const int &indexOfServer=args.indexOf(QStringLiteral("--server"));
        if(indexOfServer!=-1 && args.size()>(indexOfServer+1))
            server=args.at(indexOfServer+1);
        if(server.isEmpty())
        {
            if(settings.contains(QStringLiteral("server")))
            {
                qDebug() << "use setting to locate the server";
                server=settings.value(QStringLiteral("server")).toString();
            }
            else
            {
                qDebug() << "set setting to default";
                server=QStringLiteral("catchchallenger-server-cli-epoll");
                settings.setValue(QStringLiteral("server"),QStringLiteral("catchchallenger-server-cli-epoll"));
            }
        }
    }
    {
        const int &indexOfServer=args.indexOf(QStringLiteral("--multipleConnexion"));
        if(indexOfServer!=-1 && args.size()>(indexOfServer+1))
            multipleConnexion=args.at(indexOfServer+1).toUInt();
        if(multipleConnexion==0)
        {
            if(settings.contains(QStringLiteral("multipleConnexion")) && settings.value(QStringLiteral("multipleConnexion")).toUInt()>0)
                multipleConnexion=settings.value(QStringLiteral("multipleConnexion")).toUInt();
            else
            {
                multipleConnexion=480;
                settings.setValue(QStringLiteral("multipleConnexion"),QStringLiteral("480"));
            }
        }
    }

    if(settings.contains(QStringLiteral("connectBySeconds")))
        connectBySeconds=settings.value(QStringLiteral("connectBySeconds")).toUInt();
    else
        settings.setValue(QStringLiteral("connectBySeconds"),QStringLiteral("99"));
    if(settings.contains(QStringLiteral("maxDiffConnectedSelected")))
        maxDiffConnectedSelected=settings.value(QStringLiteral("maxDiffConnectedSelected")).toUInt();
    else
        settings.setValue(QStringLiteral("maxDiffConnectedSelected"),QStringLiteral("20"));

    multipleBotConnection.botInterface=new SimpleAction();
}

MainBenchmark::~MainBenchmark()
{
    delete multipleBotConnection.botInterface;
}

void MainBenchmark::init()
{
    qDebug() << "init done";
    if(QFile(server).exists())
    {
        const QString &settingsFile=QFileInfo(server).absolutePath()+"/server.properties";
        QSettings settingsOfServer(settingsFile,QSettings::IniFormat);
        if(settingsOfServer.status()!=QSettings::NoError)
        {
            qDebug() << tr("settingsOfServer.status()!=QSettings::NoError: %1").arg(settingsOfServer.status());
            QCoreApplication::quit();
            return;
        }
        if(!settingsOfServer.contains("max-players"))
        {
            qDebug() << tr("settings: %1 don't have max-players (%2):\n%3")
                        .arg(settingsFile);
            QCoreApplication::quit();
            return;
        }
        bool ok;
        if(multipleConnexion>settingsOfServer.value("max-players",0).toUInt(&ok))
        {
            qDebug() << tr("max-players (%1) is lower than multipleConnexion (%2) for settings: %3 (%4)")
                        .arg(settingsOfServer.value("max-players",0).toString())
                        .arg(multipleConnexion)
                        .arg(settingsFile)
                        .arg(ok);
            QCoreApplication::quit();
            return;
        }
        process.setProgram(server);
        process.setArguments(QStringList() << "--benchmark");
        process.start();
        qDebug() << "init error:" << process.errorString();
    }
    else
    {
        qDebug() << tr("Unable to find \"%1\"").arg(server);
        QCoreApplication::quit();
    }
}

void MainBenchmark::processError(QProcess::ProcessError error)
{
    Q_UNUSED(error);
    qDebug() << "QProcess::ProcessError" << error;
}

void MainBenchmark::processStateChanged(const QProcess::ProcessState &stateChanged)
{
    Q_UNUSED(stateChanged);
    qDebug() << QStringLiteral("QProcess::ProcessState") << stateChanged;
    if(stateChanged==QProcess::NotRunning)
        QCoreApplication::quit();
}

void MainBenchmark::updateUserTime()
{
    if(systemItem!=-1)
        stopSystemMeasure();
}

void MainBenchmark::readyReadStandardError()
{
    if(!serverStarted)
    {
        const QString &content=QString::fromLocal8Bit(process.readAllStandardError());
        qDebug() << content;
        if(content.contains(QStringLiteral("Waiting connection on port ")))
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
}

void MainBenchmark::readyReadStandardOutput()
{
    if(!serverStarted)
    {
        const QString &content=QString::fromLocal8Bit(process.readAllStandardOutput());
        qDebug() << content;
        if(content.contains(QStringLiteral("Waiting connection on port ")))
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
    multipleBotConnection.mHost=QStringLiteral("localhost");
    multipleBotConnection.mPort=42489;

    //do only the first client to download the datapack
    multipleBotConnection.createClient();
    startSystemMeasure();
}

void MainBenchmark::all_player_on_map()
{
    qDebug() << QStringLiteral("Connect all player");
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
