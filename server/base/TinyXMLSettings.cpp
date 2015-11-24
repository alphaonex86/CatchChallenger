#include "TinyXMLSettings.h"

TinyXMLSettings::TinyXMLSettings()
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

TinyXMLSettings::TinyXMLSettings(const std::string &file)
{
    this->file=file;
    if(!document.LoadFile(file))
    {
        if(document.ErrorId()!=12 && errno!=2)
        {
            std::cerr << "Unable to open the file: " << file << ", Parse error at line " << document.ErrorRow() << ", column " << document.ErrorCol() << ": " << document.ErrorDesc() << ", error id: " << document.ErrorId() << std::endl;
            abort();
        }
        else
        {
            TiXmlElement * root = new TiXmlElement( "configuration" );
            document.LinkEndChild(root);
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
    TiXmlElement * item = whereIs->FirstChildElement(group);
    if(item==NULL)
    {
        TiXmlElement item(group);
        whereIs->InsertEndChild(item);
        whereIs=whereIs->FirstChildElement(group);
    }
    else
        whereIs=item;
}

void TinyXMLSettings::endGroup()
{
    TiXmlElement * item = static_cast<TiXmlElement *>(whereIs->Parent());
    if(item!=NULL)
        whereIs=item;
}

std::string TinyXMLSettings::value(const std::string &var, const std::string &defaultValue)
{
    TiXmlElement * item = whereIs->FirstChildElement(var);
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
    return whereIs->FirstChildElement(var)!=NULL;
}

void TinyXMLSettings::setValue(const std::string &var,const std::string &value)
{
    TiXmlElement * item = whereIs->FirstChildElement(var);
    if(item==NULL)
    {
        TiXmlElement item(var);
        item.SetAttribute("value",value);
        whereIs->InsertEndChild(item);
    }
    else
        item->SetAttribute("value",value);
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
    if(!document.SaveFile(file))
    {
        std::cerr << "Unable to save the file: " << file << ", Parse error at line " << document.ErrorRow() << ", column " << document.ErrorCol() << ": " << document.ErrorDesc() << std::endl;
        abort();
    }
}
