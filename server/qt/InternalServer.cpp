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
#include <QDebug>

#include "InternalServer.hpp"
#include "../../../server/base/GlobalServerData.hpp"
#include "../../../server/qt/QFakeServer.hpp"
#include "../../../server/qt/QFakeSocket.hpp"
#include "../../../general/base/FacilityLib.hpp"

using namespace CatchChallenger;

/// \param settings ref is destroyed after this call
InternalServer::InternalServer() :
    QtServer()
{
    if(!connect(this,&QtServer::need_be_started,this,&InternalServer::start_internal_server,Qt::QueuedConnection))
        abort();
    const QString &currentDate=QDateTime::currentDateTime().toString("ddMMyyyy");
    if(settings.contains("gift"))
    {
        if(settings.value("gift")!=currentDate)
        {
            settings.setValue("gift",currentDate);
            timerGift.setInterval(1000);
            timerGift.setSingleShot(true);
            if(!connect(this,&QtServer::is_started,this,&InternalServer::serverIsReady,Qt::QueuedConnection))
                abort();
            if(!connect(&timerGift,&QTimer::timeout,this,&InternalServer::timerGiftSlot,Qt::QueuedConnection))
                abort();
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
    /*if(isListen() && !isStopped())
        Client::timeRangeEvent(QDateTime::currentMSecsSinceEpoch()/1000);
/home/user/Desktop/CatchChallenger/server/qt/InternalServer.cpp:57: error: incomplete type 'CatchChallenger::Client' named in nested name specifier
../server/qt/InternalServer.cpp:57:26: error: incomplete type 'CatchChallenger::Client' named in nested name specifier
        CatchChallenger::Client::timeRangeEvent(0);
        ~~~~~~~~~~~~~~~~~^~~~~~~~
../server/qt/../base/ServerStructures.hpp:30:7: note: forward declaration of 'CatchChallenger::Client'
class Client;
      ^
      */
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

    #if ! defined(EPOLLCATCHCHALLENGERSERVER) && ! defined (ONLYMAPRENDER) && defined(CATCHCHALLENGER_SOLO)
    if(!QFakeServer::server.listen())
    {
        qDebug() << (QStringLiteral("Unable to listen the internal server"));
        stat=Down;
        emit error("Unable to listen the internal server");
        emit is_started(false);
        return;
    }
    #endif

    if(!initialize_the_database())
    {
        #if ! defined(EPOLLCATCHCHALLENGERSERVER) && ! defined (ONLYMAPRENDER) && defined(CATCHCHALLENGER_SOLO)
        QFakeServer::server.close();
        #endif
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
