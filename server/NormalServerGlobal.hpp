#ifndef NORMALSERVERGLOBAL_H
#define NORMALSERVERGLOBAL_H

#include <string>
#include "base/TinyXMLSettings.hpp"

class NormalServerGlobal
{
public:
    NormalServerGlobal();
    static void displayInfo();
    static void checkSettingsFile(TinyXMLSettings * const settings, const std::string &datapack_basePath);
};

#endif // NORMALSERVERGLOBAL_H
