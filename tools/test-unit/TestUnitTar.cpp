#include "TestUnitTar.h"
#include "../../client/base/qt-tar-xz/QTarDecode.h"
#include <string>
#include <iostream>
#include <QFile>
#include <QDebug>

TestUnitTar::TestUnitTar()
{
}

void TestUnitTar::testUncompress()
{
    //done by tar cf test.tar reputation/ --owner=0 --group=0 --mtime='2010-01-01' -H ustar
    testUncompressFile(":/test.tar");
    if(!finalResult)
        return;
    //done by tar cf test.tar reputation/ --owner=0 --group=0 --mtime='2010-01-01' -H posix
    testUncompressFile(":/test-posix.tar");
    if(!finalResult)
        return;
}

void TestUnitTar::testUncompressFile(const std::string &file)
{
    QFile Qtfile(file.c_str());
    if(!Qtfile.open(QIODevice::ReadOnly))
    {
        std::cerr << file << ": " << "Unable to open internal file" << __FILE__ << ":" <<  __LINE__ << std::endl;
        finalResult=false;
        return;
    }
    QByteArray data=Qtfile.readAll();
    Qtfile.close();
    std::vector<char> dataCpp;
    dataCpp.resize(data.size());
    memcpy(dataCpp.data(),data.data(),data.size());

    QTarDecode tarDecode;
    if(!tarDecode.decodeData(dataCpp))
    {
        std::cerr << file << ": " << "tarDecode: Failed: " << tarDecode.errorString() << std::endl;
        finalResult=false;
        return;
    }
    else
        std::cout << file << ": " << "tarDecode: Ok" << std::endl;

    std::vector<std::string> fileList=tarDecode.getFileList();
    std::sort(fileList.begin(),fileList.end());

    std::vector<uint32_t> dataSize;
    {
        std::vector<std::vector<char> > dataList=tarDecode.getDataList();
        unsigned int index=0;
        while(index<dataList.size())
        {
            dataSize.push_back(dataList.at(index).size());
            index++;
        }
    }
    std::sort(dataSize.begin(),dataSize.end());

    if(fileList.size()!=6)
    {
        std::cerr << "tarDecode file count: Failed" << std::endl;
        finalResult=false;
        return;
    }
    else
        std::cout << file << ": " << "tarDecode file count: Ok" << std::endl;

    if(fileList.at(0)!="reputation/TestUnitReputation.h" || fileList.at(1)!="reputation/just-append-at-level-1.cpp" || fileList.at(2)!="reputation/just-append-at-level-max.cpp" || fileList.at(3)!="reputation/just-append.cpp" || fileList.at(4)!="reputation/level-down.cpp" || fileList.at(5)!="reputation/level-up.cpp")
    {
        std::cerr << file << ": " << "tarDecode file name: Failed" << std::endl;
        std::cerr << "file name are: "
                  << fileList.at(0) <<" "<< fileList.at(1) <<" "<< fileList.at(2) <<" "<< fileList.at(3) <<" "<< fileList.at(4) <<" "<< fileList.at(5)
                  << std::endl;
        std::cerr << "When it should be: "
                  << "reputation/TestUnitReputation.h" <<" "<< "reputation/just-append-at-level-1.cpp" <<" "<< "reputation/just-append-at-level-max.cpp" <<" "<< "reputation/just-append.cpp" <<" "<< "reputation/level-down.cpp" <<" "<< "reputation/level-up.cpp"
                  << std::endl;
        finalResult=false;
        return;
    }
    else
        std::cout << file << ": " << "tarDecode file name: Ok" << std::endl;

    if(dataSize.at(0)!=398 || dataSize.at(1)!=2732 || dataSize.at(2)!=2836 || dataSize.at(3)!=2848 || dataSize.at(4)!=5286 || dataSize.at(5)!=5322)
    {
        std::cerr << file << ": " << "tarDecode file name: Failed" << std::endl;
        finalResult=false;
        return;
    }
    else
        std::cout << file << ": " << "tarDecode file name: Ok" << std::endl;
}
