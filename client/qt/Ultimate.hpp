#ifndef ULTIMATE_H
#define ULTIMATE_H

#include <string>

class Ultimate
{
public:
    Ultimate();
    bool setKey(const std::string &key);
    bool isUltimate() const;

    static Ultimate ultimate;
private:
    bool m_ultimate;
};

#endif // ULTIMATE_H
