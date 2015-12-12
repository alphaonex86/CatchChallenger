#ifndef TINYXMLSETTINGS_H
#define TINYXMLSETTINGS_H

#include <string>

#include "../../general/base/tinyXML/tinyxml.h"

class TinyXMLSettings
{
public:
    TinyXMLSettings();
    TinyXMLSettings(const std::string &file);
    ~TinyXMLSettings();
    void beginGroup(const std::string &group);
    void endGroup();
    std::string value(const std::string &var,const std::string &defaultValue=std::string());
    bool contains(const std::string &var);
    void setValue(const std::string &var,const std::string &value);
    void setValue(const std::string &var,const int &value);
    void setValue(const std::string &var,const double &value);
    void setValue(const std::string &var,const bool &value);
    void setValue(const std::string &var,const char * const value);
    void sync();
private:
    TiXmlDocument document;
    TiXmlElement * whereIs;
    std::string file;
};

#endif // TINYXMLSETTINGS_H
