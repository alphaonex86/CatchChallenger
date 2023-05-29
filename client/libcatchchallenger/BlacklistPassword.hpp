#ifndef BlacklistPassword_H
#define BlacklistPassword_H

#include <array>
#include <string>

#include "../../general/base/lib.h"

class DLL_PUBLIC BlacklistPassword
{
public:
    static std::array<std::string, 375> list;
};

#endif // BlacklistPassword_H
