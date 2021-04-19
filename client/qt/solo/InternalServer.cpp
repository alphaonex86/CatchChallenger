#include <QObject>
#include <QTimer>
#include <QCoreApplication>
#include <QList>
#include <QByteArray>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDir>
#include <QString>
#include <QDateTime>

#include "InternalServer.hpp"
#include "../../../server/base/GlobalServerData.hpp"
#include "../../../general/base/FacilityLib.hpp"
#include "../Settings.hpp"

using namespace CatchChallenger;

/// \param settings ref is destroyed after this call
InternalServer::InternalServer() :
    QtServer()
{
    if(!connect(this,&QtServer::need_be_started,this,&InternalServer::start_internal_server,Qt::QueuedConnection))
        abort();
    const QString &currentDate=QDateTime::currentDateTime().toString("ddMMyyyy");
    if(Settings::settings->contains("gift"))
    {
        if(Settings::settings->value("gift")!=currentDate)
        {
            Settings::settings->setValue("gift",currentDate);
            timerGift.setInterval(1000);
            timerGift.setSingleShot(true);
            if(!connect(this,&QtServer::is_started,this,&InternalServer::serverIsReady,Qt::QueuedConnection))
                abort();
            if(!connect(&timerGift,&QTimer::timeout,this,&InternalServer::timerGiftSlot,Qt::QueuedConnection))
                abort();
        }
    }
    else
        Settings::settings->setValue("gift",currentDate);
}

void InternalServer::serverIsReady()
{
    if(timerGift.interval()==1000)
    {
        timerGift.setSingleShot(true);
        timerGift.start();
    }
}

void InternalServer::timerGiftSlot()
{
    if(isListen() && !isStopped())
        Client::timeRangeEvent(QDateTime::currentMSecsSinceEpoch()/1000);
}

/** call only when the server is down
 * \warning this function is thread safe because it quit all thread before remove */
InternalServer::~InternalServer()
{
}

//////////////////////////////////////////// server starting //////////////////////////////////////

//start with allow real player to connect
void InternalServer::start_internal_server()
{
    if(stat!=Down)
    {
        qDebug() << ("In wrong stat");
        return;
    }
    stat=InUp;
    setObjectName("InternalServer");
    #ifndef NOTHREADS
    QThread::start();
    #else
    InternalServer::run();
    #endif
}

void InternalServer::run()
{
    loadAndFixSettings();

    if(!QFakeServer::server.listen())
    {
        qDebug() << (QStringLiteral("Unable to listen the internal server"));
        stat=Down;
        emit error("Unable to listen the internal server");
        emit is_started(false);
        return;
    }

    if(!initialize_the_database())
    {
        QFakeServer::server.close();
        stat=Down;
        emit is_started(false);
        return;
    }
    preload_the_data();
    stat=Up;
}

/////////////////////////////////////////////////// Object removing /////////////////////////////////////

void InternalServer::removeOneClient()
{
    QtServer::removeOneClient();
    check_if_now_stopped();
}
