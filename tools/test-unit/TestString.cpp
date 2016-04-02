#include "TestString.h"
#include "../../general/base/cpp11addition.h"
#include "../../server/base/StringWithReplacement.h"
#include <vector>
#include <iostream>

TestString::TestString()
{
}

void TestString::testReplace1()
{
    {
        StringWithReplacement string("toto %1 tata %2 titi %3 tutu");
        std::string result=string.compose("test1","test2","test3");
        if(result!="toto test1 tata test2 titi test3 tutu")
        {
            std::cerr << "StringWithReplacement toto %1 tata %2 titi %3 tutu: Failed" << std::endl;
            finalResult=false;
            return;
        }
        else
            std::cout << "StringWithReplacement toto %1 tata %2 titi %3 tutu: Ok" << std::endl;
    }
    {
        StringWithReplacement string("toto %1 tata");
        std::string result=string.compose("test");
        if(result!="toto test tata")
        {
            std::cerr << "StringWithReplacement toto %1 tata: Failed" << std::endl;
            finalResult=false;
            return;
        }
        else
            std::cout << "StringWithReplacement toto %1 tata: Ok" << std::endl;
    }
    {
        StringWithReplacement string("toto %1 tata %2 titi");
        std::string result=string.compose("test1","test2");
        if(result!="toto test1 tata test2 titi")
        {
            std::cerr << "StringWithReplacement toto %1 tata %2 titi: Failed" << std::endl;
            finalResult=false;
            return;
        }
        else
            std::cout << "StringWithReplacement toto %1 tata %2 titi: Ok" << std::endl;
    }
    {
        StringWithReplacement string("toto %1");
        std::string result=string.compose("test");
        if(result!="toto test")
        {
            std::cerr << "StringWithReplacement toto %1: Failed" << std::endl;
            finalResult=false;
            return;
        }
        else
            std::cout << "StringWithReplacement toto %1: Ok" << std::endl;
    }
    {
        StringWithReplacement string("%1 toto");
        std::string result=string.compose("test");
        if(result!="test toto")
        {
            std::cerr << "StringWithReplacement %1 toto: Failed" << std::endl;
            finalResult=false;
            return;
        }
        else
            std::cout << "StringWithReplacement %1 toto: Ok" << std::endl;
    }
}
