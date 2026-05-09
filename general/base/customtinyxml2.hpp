#ifndef CATCHCHALLENGER_NOXML
#ifndef customtinyxml2h
#define customtinyxml2h

#include <string>
#ifdef CC_TINYXML2_SYSTEM
#include <tinyxml2.h>
#else
#include "../tinyXML2/tinyxml2.hpp"
#endif

static inline std::string tinyxml2errordoc(const tinyxml2::XMLDocument * const doc)
{
    if(doc==NULL)
        return "Error on unknown doc";
    std::string baseString("Parse error: ");
#ifdef CC_TINYXML2_SYSTEM
    const char * const tempString=doc->ErrorStr();
    if(tempString!=NULL)
    {
        baseString+=std::string(tempString);
        baseString+=", ";
    }
#else
    const char * const tempString1=doc->GetErrorStr1();
    const char * const tempString2=doc->GetErrorStr2();
    if(tempString1!=NULL)
    {
        baseString+=std::string(tempString1);
        if(tempString2!=NULL)
            baseString+=" and "+std::string(tempString2);
        baseString+=", ";
    }
#endif
    baseString+="error id: "+std::to_string(doc->ErrorID());
    return baseString;
}

#endif // TINYXML2_INCLUDED
#endif // CATCHCHALLENGER_NOXML
