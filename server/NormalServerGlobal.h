#ifndef NORMALSERVERGLOBAL_H
#define NORMALSERVERGLOBAL_H

#include <string>
#ifdef CATCHCHALLENGER_CLASS_ALLINONESERVER
#include "base/TinyXMLSettings.h"
#else
#include <QSettings>
#endif

class NormalServerGlobal
{
public:
    NormalServerGlobal();
    static void displayInfo();
    #ifdef CATCHCHALLENGER_CLASS_ALLINONESERVER
    static void checkSettingsFile(TinyXMLSettings * const settings, const std::string &datapack_basePath);
    #else
    static void checkSettingsFile(QSettings * const settings, const std::string &datapack_basePath);
    #endif
};

#endif // NORMALSERVERGLOBAL_H
