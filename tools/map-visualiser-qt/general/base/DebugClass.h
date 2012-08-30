#ifndef POKECRAFT_DEBUGCLASS_H
#define POKECRAFT_DEBUGCLASS_H

#include <QString>
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>

namespace Pokecraft {
class DebugClass
{
public:
	static void debugConsole(const QString &errorString);
};
}

#endif // DEBUGCLASS_H
