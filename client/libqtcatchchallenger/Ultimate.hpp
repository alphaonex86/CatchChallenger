#ifndef ULTIMATE_H
#define ULTIMATE_H

#include <string>
#include "../../general/base/lib.h"

class DLL_PUBLIC Ultimate
{
public:
    Ultimate();
    bool setKey(const std::string &key);
    bool isUltimate() const;
    static std::string buy();

    static Ultimate ultimate;
private:
    bool m_ultimate;
};

#endif // ULTIMATE_H
