#ifndef TINYXMLSETTINGS_H
#define TINYXMLSETTINGS_H

#ifndef CATCHCHALLENGER_NOXML

#include <string>

#include "../../general/tinyXML2/tinyxml2.hpp"

class TinyXMLSettings
{
public:
    //No default settings file (the "to home folder" variant was never written):
    //its body was a bare abort(), a runtime landmine for a mistake the compiler
    //can catch. Deleted = the same "do not use" contract, enforced at build time.
    TinyXMLSettings() = delete;
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
    tinyxml2::XMLDocument document;
    tinyxml2::XMLElement * whereIs;
    std::string file;
    bool modified;
};

#endif // CATCHCHALLENGER_NOXML

#endif // TINYXMLSETTINGS_H
