#include "QtDatapackChecksum.hpp"

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

#include "../../general/base/GeneralVariable.hpp"
#include "../../general/base/FacilityLib.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/CommonSettingsServer.hpp"

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
    const DatapackChecksum::FullDatapackChecksumReturn &FullDatapackChecksumReturn=doFullSyncChecksumBase(datapackPath);
    emit datapackChecksumDoneBase(FullDatapackChecksumReturn.datapackFilesList,FullDatapackChecksumReturn.hash,FullDatapackChecksumReturn.partialHashList);
}
#endif

#if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER)
void QtDatapackChecksum::doDifferedChecksumMain(const std::string &datapackPath)
{
    const DatapackChecksum::FullDatapackChecksumReturn &FullDatapackChecksumReturn=doFullSyncChecksumMain(datapackPath);
    emit datapackChecksumDoneMain(FullDatapackChecksumReturn.datapackFilesList,FullDatapackChecksumReturn.hash,FullDatapackChecksumReturn.partialHashList);
}
#endif

#if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER)
void QtDatapackChecksum::doDifferedChecksumSub(const std::string &datapackPath)
{
    const DatapackChecksum::FullDatapackChecksumReturn &FullDatapackChecksumReturn=doFullSyncChecksumSub(datapackPath);
    emit datapackChecksumDoneSub(FullDatapackChecksumReturn.datapackFilesList,FullDatapackChecksumReturn.hash,FullDatapackChecksumReturn.partialHashList);
}
#endif
