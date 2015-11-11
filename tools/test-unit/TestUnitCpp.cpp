#include "TestUnitCpp.h"
#include "../../general/base/cpp11addition.h"
#include <vector>
#include <iostream>

TestUnitCpp::TestUnitCpp()
{
}

void TestUnitCpp::testHexaToBinary()
{
    finalResult=true;
    {
        const std::vector<char> binary=hexatoBinary("01EEff");
        std::vector<char> compare;
        compare.resize(3);
        compare[0x00]=0x01;
        compare[0x01]=0xEE;
        compare[0x02]=0xFF;
        if(binary.size()!=3 || binary!=compare)
        {
            std::cerr << "hexatoBinary: Failed" << std::endl;
            finalResult=false;
            return;
        }
        else
            std::cout << "hexatoBinary: Ok" << std::endl;
    }
    {
        const std::vector<char> binary=hexatoBinary("7c81ee3bcefc71f6db2c89b334433d8a3a885568706c501260d78ebd");
        std::vector<char> compare={0x7c,0x81,0xee,0x3b,0xce,0xfc,0x71,0xf6,0xdb,0x2c,0x89,0xb3,0x34,0x43,0x3d,0x8a,0x3a,0x88,0x55,0x68,0x70,0x6c,0x50,0x12,0x60,0xd7,0x8e,0xbd};
        if(binary.size()!=compare.size() || binary!=compare)
        {
            std::cerr << "hexatoBinary 2: Failed" << std::endl;
            finalResult=false;
            return;
        }
        else
            std::cout << "hexatoBinary 2: Ok" << std::endl;
    }
}
