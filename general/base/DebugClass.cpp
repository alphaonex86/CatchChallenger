#include "DebugClass.h"

QMutex mutexForDebugClass;

void DebugClass::debugConsole(const QString &errorString)
{
	QMutexLocker lock(&mutexForDebugClass);
	qDebug() << errorString.toUtf8().constData();
}
