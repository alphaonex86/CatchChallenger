#ifndef NORMALSERVERGLOBAL_H
#define NORMALSERVERGLOBAL_H

#include <string>
#ifndef CATCHCHALLENGER_NOXML
#include "TinyXMLSettings.hpp"
#endif

class NormalServerGlobal
{
public:
    NormalServerGlobal();
    static void displayInfo();
    #ifndef CATCHCHALLENGER_NOXML
    static void checkSettingsFile(TinyXMLSettings * const settings, const std::string &datapack_basePath);
    #endif
};

#endif // NORMALSERVERGLOBAL_H
