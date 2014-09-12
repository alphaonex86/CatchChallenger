#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    inClosing(false),
    waitTheReturn(false)
{
    qRegisterMetaType<QLocalSocket::LocalSocketState>("QLocalSocket::LocalSocketState");
    qRegisterMetaType<QLocalSocket::LocalSocketError>("QLocalSocket::LocalSocketError");
    ui->setupUi(this);
    connect(&timer,&QTimer::timeout,this,&MainWindow::updateUserTime);
    timer.start(200);
    connect(&socket,&QLocalSocket::stateChanged,this,&MainWindow::updateSocketState,Qt::QueuedConnection);
    connect(&socket,&QLocalSocket::readyRead,this,&MainWindow::readyRead,Qt::QueuedConnection);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    disconnect(&socket);
    event->ignore();
    QCoreApplication::quit();
}

void MainWindow::updateUserTime()
{
    if(waitTheReturn)
        return;
    //waitTheReturn=true;
    if(socket.state()==QLocalSocket::UnconnectedState)
    {
        socket.connectToServer("/tmp/catchchallenger-server.sock");
        return;
    }
    QByteArray data;
    data[0]=0x01;
    socket.write(data);
}

void MainWindow::readyRead()
{
    waitTheReturn=false;
    const QByteArray &data=socket.readAll();
    if(data.size()==sizeof(unsigned long long)*4)
    {
        {
            const unsigned long long timeUsed=*reinterpret_cast<const unsigned long long *>(data.constData()+sizeof(unsigned long long)*0);
            double timeUsedDouble=timeUsed;
            timeUsedDouble/=1000;
            if(timeUsedDouble>1000)
                ui->userTime->setText(tr("Cpu time used: %1ms").arg(QString::number(timeUsedDouble/1000,'g',4)));
            else
                ui->userTime->setText(tr("Cpu time used: %1us").arg(QString::number(timeUsedDouble,'g',4)));
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
                if(averageTimeUsed>1000)
                    ui->userTimeAverage->setText(tr("Average cpu time used: %1ms").arg(QString::number(averageTimeUsed/1000,'g',4)));
                else
                    ui->userTimeAverage->setText(tr("Average cpu time used: %1us").arg(QString::number(averageTimeUsed,'g',4)));
                QString minString,maxString;
                if(max>1000)
                    maxString=tr("Max: %1ms").arg(QString::number(max/1000,'g',4));
                else
                    maxString=tr("Max: %1us").arg(QString::number(max,'g',4));
                if(min>1000)
                    minString=tr(", min: %1ms").arg(QString::number(min/1000,'g',4));
                else
                    minString=tr(", min: %1us").arg(QString::number(min,'g',4));
                ui->maxmin->setText(maxString+minString);
            }
        }
        {
            const unsigned long long timeUsedForUser=*reinterpret_cast<const unsigned long long *>(data.constData()+sizeof(unsigned long long)*1);
            double timeUsedDouble=timeUsedForUser;
            timeUsedDouble/=1000;
            if(timeUsedDouble>1000)
                ui->userTimeForUser->setText(tr("Cpu time used: %1ms").arg(QString::number(timeUsedDouble/1000,'g',4)));
            else
                ui->userTimeForUser->setText(tr("Cpu time used: %1us").arg(QString::number(timeUsedDouble,'g',4)));
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
                if(averageTimeUsed>1000)
                    ui->userTimeAverageForUser->setText(tr("Average cpu time used: %1ms").arg(QString::number(averageTimeUsed/1000,'g',4)));
                else
                    ui->userTimeAverageForUser->setText(tr("Average cpu time used: %1us").arg(QString::number(averageTimeUsed,'g',4)));
                QString minString,maxString;
                if(max>1000)
                    maxString=tr("Max: %1ms").arg(QString::number(max/1000,'g',4));
                else
                    maxString=tr("Max: %1us").arg(QString::number(max,'g',4));
                if(min>1000)
                    minString=tr(", min: %1ms").arg(QString::number(min/1000,'g',4));
                else
                    minString=tr(", min: %1us").arg(QString::number(min,'g',4));
                ui->maxminForUser->setText(maxString+minString);
            }
        }
        {
            const unsigned long long timeUsedForTimer=*reinterpret_cast<const unsigned long long *>(data.constData()+sizeof(unsigned long long)*2);
            double timeUsedDouble=timeUsedForTimer;
            timeUsedDouble/=1000;
            if(timeUsedDouble>1000)
                ui->userTimeForTimer->setText(tr("Cpu time used: %1ms").arg(QString::number(timeUsedDouble/1000,'g',4)));
            else
                ui->userTimeForTimer->setText(tr("Cpu time used: %1us").arg(QString::number(timeUsedDouble,'g',4)));
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
                if(averageTimeUsed>1000)
                    ui->userTimeAverageForTimer->setText(tr("Average cpu time used: %1ms").arg(QString::number(averageTimeUsed/1000,'g',4)));
                else
                    ui->userTimeAverageForTimer->setText(tr("Average cpu time used: %1us").arg(QString::number(averageTimeUsed,'g',4)));
                QString minString,maxString;
                if(max>1000)
                    maxString=tr("Max: %1ms").arg(QString::number(max/1000,'g',4));
                else
                    maxString=tr("Max: %1us").arg(QString::number(max,'g',4));
                if(min>1000)
                    minString=tr(", min: %1ms").arg(QString::number(min/1000,'g',4));
                else
                    minString=tr(", min: %1us").arg(QString::number(min,'g',4));
                ui->maxminForTimer->setText(maxString+minString);
            }
        }
        {
            const unsigned long long timeUsedForDatabase=*reinterpret_cast<const unsigned long long *>(data.constData()+sizeof(unsigned long long)*3);
            double timeUsedDouble=timeUsedForDatabase;
            timeUsedDouble/=1000;
            if(timeUsedDouble>1000)
                ui->userTimeForDatabase->setText(tr("Cpu time used: %1ms").arg(QString::number(timeUsedDouble/1000,'g',4)));
            else
                ui->userTimeForDatabase->setText(tr("Cpu time used: %1us").arg(QString::number(timeUsedDouble,'g',4)));
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
                if(averageTimeUsed>1000)
                    ui->userTimeAverageForDatabase->setText(tr("Average cpu time used: %1ms").arg(QString::number(averageTimeUsed/1000,'g',4)));
                else
                    ui->userTimeAverageForDatabase->setText(tr("Average cpu time used: %1us").arg(QString::number(averageTimeUsed,'g',4)));
                QString minString,maxString;
                if(max>1000)
                    maxString=tr("Max: %1ms").arg(QString::number(max/1000,'g',4));
                else
                    maxString=tr("Max: %1us").arg(QString::number(max,'g',4));
                if(min>1000)
                    minString=tr(", min: %1ms").arg(QString::number(min/1000,'g',4));
                else
                    minString=tr(", min: %1us").arg(QString::number(min,'g',4));
                ui->maxminForDatabase->setText(maxString+minString);
            }
        }
    }
}

void MainWindow::updateSocketState(QLocalSocket::LocalSocketState socketState)
{
    if(inClosing)
        return;
    if(socketState==QLocalSocket::ConnectedState)
    {
        ui->statusBar->showMessage(tr("Connected"),0);
        updateUserTime();
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
