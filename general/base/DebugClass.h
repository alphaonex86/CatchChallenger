#ifndef CATCHCHALLENGER_DEBUGCLASS_H
#define CATCHCHALLENGER_DEBUGCLASS_H

#include <string>
#include <vector>
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>

namespace CatchChallenger {
class DebugClass
{
public:
    static void debugConsole(const std::basic_string<char> &errorString);
    static std::vector<std::basic_string<char> > getLog();
    static bool redirection;
    static std::vector<std::basic_string<char> > log;
};
}

#endif // DEBUGCLASS_H
