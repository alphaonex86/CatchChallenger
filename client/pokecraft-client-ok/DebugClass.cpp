#include "DebugClass.h"

void DebugClass::debugConsole(QString errorString)
{
	qDebug() << errorString.toUtf8().constData();
}
