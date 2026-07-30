#include "CliDatapack.hpp"

#include <iostream>

#include "../../../general/base/CommonDatapack.hpp"
#include "../../../general/base/CommonDatapackServerSpec.hpp"

CliDatapackClientLoader *CliDatapack::loader=NULL;
bool CliDatapack::baseLoaded=false;
bool CliDatapack::mainSubLoaded=false;
std::string CliDatapack::errorString;

CliDatapackClientLoader::CliDatapackClientLoader() :
    mChecksumError(false)
{
}

CliDatapackClientLoader::~CliDatapackClientLoader()
{
}

bool CliDatapackClientLoader::checksumError() const
{
    return mChecksumError;
}

void CliDatapackClientLoader::emitdatapackParsed()
{
}

void CliDatapackClientLoader::emitdatapackParsedMainSub()
{
}

void CliDatapackClientLoader::emitdatapackChecksumError()
{
    mChecksumError=true;
}

void CliDatapackClientLoader::parseTopLib()
{
    //the GUI client builds its tilesets here; a CLI bot renders nothing
}

std::string CliDatapackClientLoader::getLanguage()
{
    return std::string("en");
}

bool CliDatapack::loadBase(const std::string &datapackPath)
{
    if(baseLoaded)
        return true;
    if(loader==NULL)
        loader=new CliDatapackClientLoader();
    loader->parseDatapack(datapackPath);
    if(loader->checksumError())
    {
        errorString="datapack base checksum mismatch for "+datapackPath;
        return false;
    }
    if(!CatchChallenger::CommonDatapack::commonDatapack.isParsedContent())
    {
        errorString="CommonDatapack is still empty after parsing "+datapackPath;
        return false;
    }
    baseLoaded=true;
    return true;
}

bool CliDatapack::loadMainSub(const std::string &mainCode,const std::string &subCode)
{
    if(mainSubLoaded)
        return true;
    if(!baseLoaded)
    {
        errorString="CliDatapack::loadMainSub() called before loadBase()";
        return false;
    }
    //parseDatapackMainSub() loads every map and, on the way, calls
    //CommonDatapackServerSpec::parseDatapackAfterZoneAndMap() which the
    //character-load packet requires (Api_protocol_loadchar.cpp).
    loader->parseDatapackMainSub(mainCode,subCode);
    if(loader->checksumError())
    {
        errorString="datapack main/sub checksum mismatch for main="+mainCode+" sub="+subCode;
        return false;
    }
    if(!CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.isParsedContent())
    {
        errorString="CommonDatapackServerSpec is still empty after parsing main="+mainCode;
        return false;
    }
    if(loader->get_mapToId_size()==0)
    {
        errorString="no map found under map/main/"+mainCode+"/";
        return false;
    }
    mainSubLoaded=true;
    return true;
}

CATCHCHALLENGER_TYPE_MAPID CliDatapack::mapCount()
{
    if(loader==NULL)
        return 0;
    const size_t count=loader->get_mapToId_size();
    if(count>65535)
    {
        std::cerr << "CliDatapack::mapCount(): " << count << " maps, clamped to the protocol maximum 65535" << std::endl;
        return 65535;
    }
    return static_cast<CATCHCHALLENGER_TYPE_MAPID>(count);
}

const std::string &CliDatapack::lastError()
{
    return errorString;
}
