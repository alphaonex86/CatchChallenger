#include <QObject>
#include <QTimer>
#include <QCoreApplication>
#include <QList>
#include <QByteArray>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDir>
#include <QString>

#include "InternalServer.h"
#include "../../../server/base/GlobalServerData.h"
#include "../../../general/base/FacilityLib.h"

using namespace CatchChallenger;

/// \param settings ref is destroyed after this call
InternalServer::InternalServer(QSettings &settings) :
    QtServer()
{
    connect(this,&QtServer::need_be_started,this,&InternalServer::start_internal_server,Qt::QueuedConnection);
    const QString &currentDate=QDateTime::currentDateTime().toString("ddMMyyyy");
    if(settings.contains("gift"))
    {
        if(settings.value("gift")!=currentDate)
        {
            settings.setValue("gift",currentDate);
            timerGift.setInterval(1000);
            timerGift.setSingleShot(true);
            connect(this,&QtServer::is_started,this,&InternalServer::serverIsReady,Qt::QueuedConnection);
            connect(&timerGift,&QTimer::timeout,this,&InternalServer::timerGiftSlot,Qt::QueuedConnection);
        }
    }
    else
        settings.setValue("gift",currentDate);
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
    return;
}

/////////////////////////////////////////////////// Object removing /////////////////////////////////////

void InternalServer::removeOneClient()
{
    QtServer::removeOneClient();
    check_if_now_stopped();
}
