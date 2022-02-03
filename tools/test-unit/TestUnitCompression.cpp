#include "TestUnitCompression.h"
#include "../../general/base/ProtocolParsing.h"
#include <string>
#include <iostream>
#include <lzma.h>
#include "../../general/base/lz4/lz4.h"

std::string text="    <monster xp_max=\"500\" type=\"fire\" pow=\"1.4\" speed=\"130\" attack=\"110\" hp=\"200\" catch_rate=\"0\" id=\"1\" weight=\"35kg\" special_attack=\"120\" defense=\"85\" special_defense=\"100\" ratio_gender=\"50%\" give_sp=\"350\" height=\"0.76m\" egg_step=\"5000\" give_xp=\"300\">"
"        <attack_list>"
"            <attack id=\"201\" level=\"0\"/>"
"            <attack id=\"202\" level=\"0\"/>"
"            <attack id=\"11\" level=\"8\"/>"
"            <attack id=\"12\" level=\"10\"/>"
"            <attack id=\"13\" level=\"14\"/>"
"            <attack id=\"41\" level=\"16\"/>"
"            <attack id=\"11\" level=\"20\" attack_level=\"2\"/>"
"            <attack id=\"12\" level=\"26\" attack_level=\"2\"/>"
"            <attack id=\"13\" level=\"36\" attack_level=\"2\"/>"
"            <attack id=\"10\" level=\"45\"/>"
"            <attack id=\"10\" level=\"55\" attack_level=\"2\"/>"
"        </attack_list>"
"        <evolutions>   "
"            <evolution type=\"level\" level=\"26\" evolveTo=\"102\"/>"
"        </evolutions>"
"        <drops>"
"            <drop item=\"2003\" luck=\"20\" quantity_min=\"1\" quantity_max=\"3\"/>"
"        </drops>"
"        <name>Panthera fira</name>"
"        <name lang=\"fr\">Panthera fira</name>"
"        <description>Panthera fira is a very rare animal, it is characterized by its very hot skin..</description>"
"        <description lang=\"fr\">Panthera fira est un animal très rare, il est caractérisé par sa peau très chaud..</description>"
"    </monster>";
char buffer[8*1024*1024];
char buffer2[8*1024*1024];

TestUnitCompression::TestUnitCompression()
{
}

void TestUnitCompression::testZlib()
{
    int32_t size=CatchChallenger::ProtocolParsing::compressZlib(text.data(),text.size(),buffer,sizeof(buffer));
    if(size<=0 || (uint32_t)size>(text.size()+text.size()/10))
    {
        std::cerr << "compress zlib: Failed" << std::endl;
        finalResult=false;
        return;
    }
    else
        std::cout << "compress zlib: Ok, ratio: " << size << "/" << text.size() << "=" << ((float)text.size()/(float)size) << std::endl;

    int32_t size2=CatchChallenger::ProtocolParsing::decompressZlib(buffer,size,buffer2,sizeof(buffer2));
    if(size2!=(int32_t)text.size() || buffer2!=text)
    {
        std::cerr << "decompress zlib: Failed" << std::endl;
        finalResult=false;
        return;
    }
    else
        std::cout << "decompress zlib: Ok" << std::endl;
}

void TestUnitCompression::testLz4()
{
    int32_t size=LZ4_compress_fast(text.data(),buffer,text.size(),sizeof(buffer),6);
    if(size<=0 || (uint32_t)size>(text.size()+text.size()/10))
    {
        std::cerr << "compress Lz4: Failed" << std::endl;
        finalResult=false;
        return;
    }
    else
        std::cout << "compress Lz4: Ok, ratio: " << size << "/" << text.size() << "=" << ((float)text.size()/(float)size) << std::endl;

    int32_t size2=LZ4_decompress_safe(buffer,buffer2,size,sizeof(buffer2));
    if(size2!=(int32_t)text.size() || buffer2!=text)
    {
        std::cerr << "decompress Lz4: Failed" << std::endl;
        finalResult=false;
        return;
    }
    else
        std::cout << "decompress Lz4: Ok" << std::endl;
}

void TestUnitCompression::testXZ()
{
    int32_t size=CatchChallenger::ProtocolParsing::compressXz(text.data(),text.size(),buffer,sizeof(buffer));
    if(size<=0 || (uint32_t)size>(text.size()+text.size()/10))
    {
        std::cerr << "compress XZ: Failed" << std::endl;
        finalResult=false;
        return;
    }
    else
        std::cout << "compress XZ: Ok, ratio: " << size << "/" << text.size() << "=" << ((float)text.size()/(float)size) << std::endl;

    int32_t size2=CatchChallenger::ProtocolParsing::decompressXz(buffer,size,buffer2,sizeof(buffer2));
    if(size2!=(int32_t)text.size() || buffer2!=text)
    {
        std::cerr << "decompress XZ: Failed" << std::endl;
        finalResult=false;
        return;
    }
    else
        std::cout << "decompress XZ: Ok" << std::endl;
}
