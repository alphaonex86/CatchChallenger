#include "DebugClass.h"

using namespace Pokecraft;

bool DebugClass::redirection = false;
QStringList DebugClass::log;

QMutex mutexForDebugClass;

void DebugClass::debugConsole(const QString &errorString)
{
	QMutexLocker lock(&mutexForDebugClass);
    if(!redirection)
        qDebug() << errorString.toUtf8().constData();
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
