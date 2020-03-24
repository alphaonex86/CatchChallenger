#include "TinyXMLSettings.hpp"
#include "../../general/base/tinyXML2/customtinyxml2.hpp"
#include <iostream>

TinyXMLSettings::TinyXMLSettings() :
    modified(false)
{
    abort();//todo to home folder
    document.LoadFile("");
    whereIs=document.RootElement();
    if(whereIs==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", unable to link the base root" << std::endl;
        abort();
    }
}

TinyXMLSettings::TinyXMLSettings(const std::string &file) :
    modified(false)
{
    this->file=file;
    const auto loadOkay = document.LoadFile(file.c_str());
    if(loadOkay!=0)
    {
        modified=true;
        const auto &errorId=document.ErrorID();
        if(errorId==tinyxml2::XML_ERROR_FILE_NOT_FOUND)
        {
            tinyxml2::XMLNode * pRoot = document.NewElement("configuration");
            document.InsertFirstChild(pRoot);
        }
        else
        {
            const tinyxml2::XMLDocument * const documentBis=&document;
            std::cerr << file+", "+tinyxml2errordoc(documentBis) << std::endl;
            abort();
        }
    }
    whereIs=document.RootElement();
    if(whereIs==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", unable to link the base root" << std::endl;
        abort();
    }
}

TinyXMLSettings::~TinyXMLSettings()
{
    sync();
}

void TinyXMLSettings::beginGroup(const std::string &group)
{
    tinyxml2::XMLElement * item = whereIs->FirstChildElement(group.c_str());
    if(item==NULL)
    {
        tinyxml2::XMLNode * item = document.NewElement(group.c_str());
        whereIs->InsertFirstChild(item);
        whereIs=whereIs->FirstChildElement(group.c_str());
    }
    else
        whereIs=item;
}

void TinyXMLSettings::endGroup()
{
    if(whereIs==document.RootElement())
        return;
    tinyxml2::XMLElement * item = static_cast<tinyxml2::XMLElement *>(whereIs->Parent());
    if(item!=NULL)
        whereIs=item;
}

std::string TinyXMLSettings::value(const std::string &var, const std::string &defaultValue)
{
    tinyxml2::XMLElement * item = whereIs->FirstChildElement(var.c_str());
    if(item==NULL)
        return defaultValue;
    else
    {
        if(item->Attribute("value")==NULL)
            return defaultValue;
        else
            return item->Attribute("value");
    }
}

bool TinyXMLSettings::contains(const std::string &var)
{
    return whereIs->FirstChildElement(var.c_str())!=NULL;
}

void TinyXMLSettings::setValue(const std::string &var,const std::string &value)
{
    tinyxml2::XMLElement * item = whereIs->FirstChildElement(var.c_str());
    if(item==NULL)
    {
        modified=true;
        tinyxml2::XMLElement * item = document.NewElement(var.c_str());
        item->SetAttribute("value",value.c_str());
        whereIs->InsertFirstChild(item);
    }
    else
    {
        if(item->Attribute("value")==value)
            return;
        modified=true;
        item->SetAttribute("value",value.c_str());
    }
}

void TinyXMLSettings::setValue(const std::string &var,const int &value)
{
    setValue(var,std::to_string(value));
}

void TinyXMLSettings::setValue(const std::string &var,const double &value)
{
    setValue(var,std::to_string(value));
}

void TinyXMLSettings::setValue(const std::string &var,const bool &value)
{
    if(value)
        setValue(var,std::string("true"));
    else
        setValue(var,std::string("false"));
}

void TinyXMLSettings::setValue(const std::string &var,const char * const value)
{
    setValue(var,std::string(value));
}

void TinyXMLSettings::sync()
{
    if(!modified)
        return;
    const auto &returnVar=document.SaveFile(file.c_str());
    if(returnVar!=tinyxml2::XML_SUCCESS)
    {
        std::cerr << "Unable to save the file: " << file << ", returnVar: " << std::to_string(returnVar) << std::endl;
        abort();
    }
    modified=false;
}
