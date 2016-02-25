#ifndef NORMALSERVERGLOBAL_H
#define NORMALSERVERGLOBAL_H

#include <string>
#ifdef EPOLLCATCHCHALLENGERSERVER
#include "base/TinyXMLSettings.h"
#else
#include <QSettings>
#endif

class NormalServerGlobal
{
public:
    NormalServerGlobal();
    static void displayInfo();
    #ifdef EPOLLCATCHCHALLENGERSERVER
    static void checkSettingsFile(TinyXMLSettings * const settings, const std::string &datapack_basePath);
    #else
    static void checkSettingsFile(QSettings * const settings, const std::string &datapack_basePath);
    #endif
};

#endif // NORMALSERVERGLOBAL_H
