#ifndef CATCHCHALLENGER_DEBUGCLASS_H
#define CATCHCHALLENGER_DEBUGCLASS_H

#include <QString>
#include <QStringList>
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>

namespace CatchChallenger {
class DebugClass
{
public:
    static void debugConsole(const QString &errorString);
    static QStringList getLog();
    static bool redirection;
    static QStringList log;
};
}

#endif // DEBUGCLASS_H
