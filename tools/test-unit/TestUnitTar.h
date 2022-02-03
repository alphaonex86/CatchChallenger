#ifndef TestUnitTar_H
#define TestUnitTar_H

#include <string>

class TestUnitTar
{
public:
    TestUnitTar();
public:
    bool finalResult;

    void testUncompress();
    void testUncompressFile(const std::string &file);
};

#endif // TestUnitTar_H
