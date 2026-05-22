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
    //Both the vendored copy (now v11) and the system lib (>=9) expose the
    //single ErrorStr(); the old v3 GetErrorStr1()/GetErrorStr2() pair is gone.
    const char * const tempString=doc->ErrorStr();
    if(tempString!=NULL)
    {
        baseString+=std::string(tempString);
        baseString+=", ";
    }
    baseString+="error id: "+std::to_string(doc->ErrorID());
    return baseString;
}

#endif // TINYXML2_INCLUDED
#endif // CATCHCHALLENGER_NOXML
