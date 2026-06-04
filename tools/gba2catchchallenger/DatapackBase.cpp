#include "DatapackBase.hpp"

#include <iostream>

DatapackBase::DatapackBase()
{
}

bool DatapackBase::load(const std::string &datapackPath)
{
    // The loader concatenates "plants/...", "skin/..." onto the path verbatim,
    // so it must end with a separator.
    std::string path=datapackPath;
    if(!path.empty() && path.back()!='/')
        path+="/";
    parseDatapack(path);
    std::cout << "DatapackBase: " << get_skins().size() << " skins loaded from "
              << datapackPath << std::endl;
    return !get_skins().empty();
}

void DatapackBase::emitdatapackParsed()
{
}

void DatapackBase::emitdatapackParsedMainSub()
{
}

void DatapackBase::emitdatapackChecksumError()
{
}

void DatapackBase::parseTopLib()
{
}

std::string DatapackBase::getLanguage()
{
    return std::string("en");
}
