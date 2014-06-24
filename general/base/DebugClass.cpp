#include "DebugClass.h"

using namespace CatchChallenger;

bool DebugClass::redirection = false;
QStringList DebugClass::log;

QMutex mutexForDebugClass;

void DebugClass::debugConsole(const QString &errorString)
{
    QMutexLocker lock(&mutexForDebugClass);
    if(!redirection)
        qDebug() << qPrintable(errorString);
    else
        log << errorString;
}

QStringList DebugClass::getLog()
{
    QMutexLocker lock(&mutexForDebugClass);
    QStringList oldLog=log;
    log.clear();
    return oldLog;
}
