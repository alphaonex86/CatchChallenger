#include "DebugClass.h"

void DebugClass::debugConsole(const QString &errorString)
{
	qDebug() << errorString.toUtf8().constData();
}
