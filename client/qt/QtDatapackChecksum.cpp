#include "QtDatapackChecksum.h"

#include <regex>
#include <string>
#include <unordered_set>
#include <iostream>
#if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER)
//with Qt client
#include <QCryptographicHash>
#else
//with gateway
#include <openssl/sha.h>
#endif

#include "../../general/base/GeneralVariable.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/CommonSettingsServer.h"

using namespace CatchChallenger;

#if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER) && !defined(NOTHREADS)
QThread QtDatapackChecksum::thread;
#endif

QtDatapackChecksum::QtDatapackChecksum()
{
    #if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER) && !defined(NOTHREADS)
    if(!thread.isRunning())
        thread.start();
    moveToThread(&thread);
    thread.setObjectName("QtDatapackChecksum");
    #endif
}

QtDatapackChecksum::~QtDatapackChecksum()
{
    #if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER) && !defined(NOTHREADS)
    stopThread();
    #endif
}

#if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER) && !defined(NOTHREADS)
void QtDatapackChecksum::stopThread()
{
    thread.exit();
    thread.quit();
    thread.wait();
}
#endif

#if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER)
void QtDatapackChecksum::doDifferedChecksumBase(const std::string &datapackPath)
{
    std::cerr << "QtDatapackChecksum::doDifferedChecksumBase" << std::endl;
    const FullDatapackChecksumReturn &FullDatapackChecksumReturn=doFullSyncChecksumBase(datapackPath);
    emit datapackChecksumDoneBase(FullDatapackChecksumReturn.datapackFilesList,FullDatapackChecksumReturn.hash,FullDatapackChecksumReturn.partialHashList);
}
#endif

#if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER)
void QtDatapackChecksum::doDifferedChecksumMain(const std::string &datapackPath)
{
    const FullDatapackChecksumReturn &FullDatapackChecksumReturn=doFullSyncChecksumMain(datapackPath);
    emit datapackChecksumDoneMain(FullDatapackChecksumReturn.datapackFilesList,FullDatapackChecksumReturn.hash,FullDatapackChecksumReturn.partialHashList);
}
#endif

#if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER)
void QtDatapackChecksum::doDifferedChecksumSub(const std::string &datapackPath)
{
    const FullDatapackChecksumReturn &FullDatapackChecksumReturn=doFullSyncChecksumSub(datapackPath);
    emit datapackChecksumDoneSub(FullDatapackChecksumReturn.datapackFilesList,FullDatapackChecksumReturn.hash,FullDatapackChecksumReturn.partialHashList);
}
#endif
